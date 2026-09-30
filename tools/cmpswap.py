#!/usr/bin/env python3
"""Find compares whose two operands are swapped relative to the target.

The blind spot objdiff and semdiff share. Both erase register names before
comparing, so

    retail  lfs f1,0x8(r1)      ours  lfs f0,0x8(r1)
            lfs f0,0x10(r3)           lfs f1,0x10(r3)
            fcmpo cr0,f1,f0           fcmpo cr0,f1,f0
            bge  -> return            bge  -> return

is a zero difference to both -- same mnemonics, same branch, same
displacements -- and an inverted condition to the machine. `dist <
other.dist` became `other.dist < dist`. Two confirmed instances, both sitting
above 97% the whole time: sphereHitsEnv3CB (the entity-model collision
scallop) and PlayerAbsControl (Patrick's pickup lerp).

The branch is the filter. Swapping the operands and flipping the branch
(`ble` for `bge`) is the same condition written the other way round, which is
what CodeWarrior emits for `b > a` against `a < b`; semdiff already reports
that and it is not a bug. Swapping the operands and KEEPING the branch always
changes the meaning.

Each operand is resolved to the nearest preceding instruction that defines it,
reduced to a signature that survives register allocation: the load
displacement, the relocated symbol, or the defining mnemonic. Two signatures
equal as a set but opposite in order, under an unchanged branch, is the report.

Usage:
  cmpswap.py                  scan every game-code unit
  cmpswap.py <unit-frag> ...  scan only matching units
  cmpswap.py --all            include SDK/RenderWare/MSL units too
  cmpswap.py --arith          also check non-commutative arithmetic
"""

import json
import os
import re
import subprocess
import sys
from collections import Counter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cwexec

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CLI = cwexec.objdiff_cli(ROOT)
CFG = json.load(open(os.path.join(ROOT, "objdiff.json")))
GAME = "main/SB/"

CMP = re.compile(r"^(fcmpo|fcmpu|cmpw|cmplw|cmp|cmpl)$")
# Instructions whose first operand is the register they define.
DEFS = re.compile(r"^(lbz|lhz|lha|lwz|lfs|lfd|li|lis|addi|addis|add|addic|subf"
                  r"|subi|mr|fmr|fabs|fneg|frsp|fadds|fsubs|fmuls|fdivs|fadd"
                  r"|fsub|fmul|fdiv|mulli|mullw|slwi|srwi|srawi|rlwinm|clrlwi"
                  r"|or|and|xor|neg|fmadds|fmsubs|fnmadds|fnmsubs|lwzx|lfsx"
                  r"|lhzx|lbzx)\.?$")
# Non-commutative arithmetic: the same swap is just as invisible here.
ARITH = re.compile(r"^(fsubs|fsub|fdivs|fdiv|subf|subfc|divw|divwu)$")
BRANCH = re.compile(r"^b[a-z]{1,4}[+-]?$")
ORDERBR = re.compile(r"^(blt|ble|bgt|bge)$")
DISP = re.compile(r"(-?0x[0-9a-fA-F]+|-?\d+)\(")
# An anonymous pool ordinal, CodeWarrior's per-TU counter on a function-local
# static, and our decomp's spelling of the same. None of the three carries
# meaning across two builds; semdiff erases them for the same reason.
POOL = re.compile(r"@\d+")
LOCALNUM = re.compile(r"\$\d+")
DECOMPNUM = re.compile(r"_\d{3,}\b")
REGOP = re.compile(r"^[rf]\d{1,2}$")


def rows(sym, names):
    """(mnemonic, [operands], reloc-symbol-or-None) per instruction, in order."""
    out = []
    for r in sym.get("instructions") or []:
        ins = r.get("instruction")
        if not ins:
            continue
        s = (ins.get("formatted") or "").strip()
        parts = s.split(None, 1)
        mnem = parts[0]
        ops = [o.strip() for o in parts[1].split(",")] if len(parts) > 1 else []
        rel = r.get("relocation") or ins.get("relocation")
        nm = None
        if rel:
            t = rel.get("target_symbol")
            nm = names[t] if isinstance(t, int) and 0 <= t < len(names) else str(t)
        out.append((mnem, ops, nm))
    return out


def sig(stream, i, reg):
    """Signature of whatever defines `reg` just before index i.

    Register names are erased -- they are exactly what differs between two
    builds of the same source -- so what identifies a value is where it came
    from: a displacement, a relocated symbol, or the operation that made it.
    """
    if not REGOP.match(reg):
        return None
    for j in range(i - 1, max(-1, i - 40), -1):
        mnem, ops, nm = stream[j]
        if not ops or not DEFS.match(mnem):
            continue
        if ops[0] != reg:
            continue
        # Symbol AND displacement. Taking the symbol alone collapses every
        # field of a big struct onto one signature -- grabLerpMin, Max and
        # Last are all `lfs <globals>` -- which silently discarded a confirmed
        # instance, because the swap filter needs the two operands to be
        # distinguishable before it can call them swapped.
        m = DISP.search(ops[1]) if len(ops) > 1 else None
        disp = m.group(1) if m else None
        if nm:
            nm = DECOMPNUM.sub("$N", LOCALNUM.sub("$N", POOL.sub("@P", nm)))
            base = "%s <%s>" % (mnem.rstrip("."), nm)
            return base + ("+%s" % disp if disp else "")
        if disp:
            return "%s %s(?)" % (mnem.rstrip("."), disp)
        if mnem in ("li", "lis") and len(ops) > 1:
            return "%s %s" % (mnem, ops[1])
        return mnem.rstrip(".")
    return None


def follow(stream, i):
    """The ordering branch after index i, if it is close enough to be this test's.

    Only an ordering branch is reported. Equality is symmetric, so `fcmpu a,b`
    with `beq` means exactly what `fcmpu b,a` with `beq` means, and which one
    CodeWarrior picks is not something the source decides -- those swaps were
    the whole of the first pass's false positives.
    """
    for j in range(i + 1, min(len(stream), i + 6)):
        if BRANCH.match(stream[j][0]):
            mnem = stream[j][0].rstrip("+-")
            return mnem if ORDERBR.match(mnem) else None
    return None


def sites(stream, want_arith):
    out = []
    for i, (mnem, ops, _) in enumerate(stream):
        if CMP.match(mnem) and len(ops) == 3:
            out.append(("cmp", mnem, sig(stream, i, ops[1]),
                        sig(stream, i, ops[2]), follow(stream, i)))
        elif want_arith and ARITH.match(mnem) and len(ops) == 3:
            out.append(("arith", mnem, sig(stream, i, ops[1]),
                        sig(stream, i, ops[2]), None))
    return out


def scan(unit, want_arith):
    tgt = os.path.join(ROOT, unit["target_path"])
    base = os.path.join(ROOT, unit["base_path"])
    if not (os.path.exists(tgt) and os.path.exists(base)):
        return []
    out = os.path.join("/tmp", "cmpswap_%d.json" % os.getpid())
    subprocess.run([CLI, "diff", "-1", tgt, "-2", base, "-o", out,
                    "--format", "json", "-c", "functionRelocDiffs=none"],
                   cwd=ROOT, capture_output=True)
    try:
        d = json.load(open(out))
    except Exception:
        return []
    finally:
        if os.path.exists(out):
            os.unlink(out)

    lnames = [x.get("name") or "" for x in d.get("left", {}).get("symbols") or []]
    rnames = [x.get("name") or "" for x in d.get("right", {}).get("symbols") or []]
    right = {s.get("name"): s for s in d.get("right", {}).get("symbols") or []
             if s.get("kind") == "SYMBOL_FUNCTION"}

    found = []
    for ls in d.get("left", {}).get("symbols") or []:
        if ls.get("kind") != "SYMBOL_FUNCTION":
            continue
        rs = right.get(ls.get("name"))
        if rs is None:
            continue
        # Multisets of ordered tests, not an index alignment. Pairing the k-th
        # compare with the k-th only works when both sides have the same
        # number of them, and PlayerAbsControl -- a confirmed instance -- does
        # not: our mwcc unrolls a loop retail leaves rolled, which adds
        # compares and made the whole function fall out of the earlier pass.
        # A swap survives as a term present on one side and absent on the
        # other, with its mirror image doing the same in reverse.
        T = Counter(t for t in sites(rows(ls, lnames), want_arith)
                    if None not in (t[2], t[3]) and t[2] != t[3]
                    and (t[0] != "cmp" or t[4] is not None))
        O = Counter(t for t in sites(rows(rs, rnames), want_arith)
                    if None not in (t[2], t[3]) and t[2] != t[3]
                    and (t[0] != "cmp" or t[4] is not None))
        only_t, only_o = T - O, O - T
        for (kind, tm, ta, tb, tbr), n in sorted(only_t.items(), key=lambda kv: tuple(str(x) for x in kv[0])):
            mirror = (kind, tm, tb, ta, tbr)
            if only_o.get(mirror, 0) < n:
                continue
            found.append({
                "unit": unit["name"], "name": ls.get("name"),
                "size": int(ls.get("size") or 0),
                "pct": ls.get("match_percent"), "kind": kind, "n": n,
                "mnem": tm, "branch": tbr,
                "target": "%s, %s" % (ta, tb), "ours": "%s, %s" % (tb, ta),
            })
    return found


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    want_arith = "--arith" in sys.argv
    allunits = "--all" in sys.argv

    units = []
    for u in CFG["units"]:
        n = u.get("name") or ""
        if not allunits and not n.startswith(GAME):
            continue
        if args and not any(a in n for a in args):
            continue
        units.append(u)

    hits = []
    for u in units:
        hits.extend(scan(u, want_arith))

    hits.sort(key=lambda h: (h["unit"], h["name"], h.get("idx", 0)))
    print("\n%d swapped-operand site(s) in %d function(s), over %d unit(s)\n"
          % (len(hits), len(set((h["unit"], h["name"]) for h in hits)), len(units)))
    for h in hits:
        pct = h["pct"]
        print("%-58s %5d  %7s  [%s]"
              % (h["name"][:58], h["size"],
                 ("%.2f%%" % pct) if pct is not None else "-", h["unit"]))
        print("      %s x%d   %s %s" % (h["kind"], h.get("n", 1), h["mnem"],
                                        h["branch"] or ""))
        print("      target : %s" % h["target"])
        print("      ours   : %s" % h["ours"])


if __name__ == "__main__":
    main()
