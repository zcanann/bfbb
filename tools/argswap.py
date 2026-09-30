#!/usr/bin/env python3
"""Find call arguments that are swapped relative to the target.

cmpswap.py's blind spot, one level up. A compare with its operands the wrong
way round is invisible to objdiff and semdiff because both erase register
names; so is a CALL with two of its arguments the wrong way round, and an
assignment is a call:

    retail  mr r3,r29   ; &tranresult[1]     ours  mr r3,r30   ; &m1.pos
            mr r4,r30   ; &m1.pos                  mr r4,r29   ; &tranresult[1]
            bl __as__5xVec3FRC5xVec3                bl __as__5xVec3FRC5xVec3

Same mnemonics, same call, same displacements. `tranresult[1] = m1.pos`
became `m1.pos = tranresult[1]`, which is a store into a local that is never
read again -- so it cost a couple of percent of match and dropped the root
translation of every two-root cutscene model, leaving cin_hammer and
cin_tartar to play with no characters in shot. Confirmed instance:
xcsCalcAnimMatrices (xCutscene.cpp), commit 18058e5c.

Each argument register is resolved back to what defines it, through `mr`
chains and into the callee-saved register that holds it, and reduced to a
signature that survives register allocation: a frame offset, a relocated
symbol, a load through one of those. A call whose argument signatures are the
target's with exactly two positions transposed is the report.

Only ONE transposition is reported, never a general permutation: two
arguments in the wrong order is a mistake someone makes, three is almost
always this tool failing to resolve a register the same way on both sides.

Assignment operators only by default -- that is the confirmed class, and
`a = b` written backwards is silent. `--calls` widens it to every named call,
which is where an argument-order mistake in an ordinary function would show
(xVec3Sub, xMat4x3Mul).

Three filters keep it quiet, each of which cost a round of false positives to
learn: only registers the callee's mangling says are arguments (`param_slots`),
only signatures that name a value rather than a shape (`strong`), and only
transpositions that a frame relabelling does not explain (`relabelling`).
With them, both modes report NOTHING over all 543 units -- so any hit is worth
reading, and the baseline to compare a future run against is zero.

What it cannot see: a swap between two values it resolves to the same
signature, a call whose arguments are set before the previous call rather than
after it, and any transposition in a function whose two locals also changed
frame slots (the relabelling filter cannot tell those apart, and it prefers a
miss to a false alarm).

Usage:
  argswap.py                  scan every game-code unit
  argswap.py <unit-frag> ...  scan only matching units
  argswap.py --calls          every named call, not just assignment operators
  argswap.py --all            include SDK/RenderWare/MSL units too
  argswap.py --v              show the resolved signature of every argument
"""

import json
import os
import re
import subprocess
import sys
import tempfile
from collections import Counter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cwexec

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CLI = cwexec.objdiff_cli(ROOT)
CFG = json.load(open(os.path.join(ROOT, "objdiff.json")))
GAME = "main/SB/"

# The argument registers, in order. Floats come after the GPRs because a
# mangled name says which are which and this does not -- a term is a tuple, so
# all that matters is that both sides build it the same way.
ARGREGS = ["r3", "r4", "r5", "r6", "r7"]
FARGREGS = ["f1", "f2", "f3"]

CALL = re.compile(r"^bl$")
# Instructions whose first operand is the register they define.
DEFS = re.compile(r"^(lbz|lhz|lha|lwz|lfs|lfd|li|lis|addi|addis|add|addic|subf"
                  r"|subi|mr|fmr|fabs|fneg|frsp|fadds|fsubs|fmuls|fdivs|fadd"
                  r"|fsub|fmul|fdiv|mulli|mullw|slwi|srwi|srawi|rlwinm|clrlwi"
                  r"|or|and|xor|neg|fmadds|fmsubs|fnmadds|fnmsubs|lwzx|lfsx"
                  r"|lhzx|lbzx)\.?$")
LOADS = re.compile(r"^(lbz|lhz|lha|lwz|lfs|lfd)$")
DISP = re.compile(r"^(-?0x[0-9a-fA-F]+|-?\d+)\((r\d{1,2})\)$")
# Volatile registers cannot survive a call, so their definition is always
# after the previous one. Everything else can, and usually does.
VOLATILE = set(["r0"] + ["r%d" % n for n in range(3, 13)])
REGOP = re.compile(r"^[rf]\d{1,2}$")
# An anonymous pool ordinal, CodeWarrior's per-TU counter on a function-local
# static, and our decomp's spelling of the same. None carries meaning across
# two builds; semdiff erases them for the same reason.
POOL = re.compile(r"@\d+")
LOCALNUM = re.compile(r"\$\d+")
DECOMPNUM = re.compile(r"_\d{3,}\b")

ASSIGN = re.compile(r"^__as__")

# C library functions carry no mangling, so their arity has to be written down.
# Only the ones that actually turned up: memcpy is called with a live float in
# f1 and f2 in iSndPlaySound, and the two of them changing places was the last
# false positive on the tree.
CARITY = {
    "memcpy": (3, 0), "memmove": (3, 0), "memset": (3, 0), "memcmp": (3, 0),
    "strcpy": (2, 0), "strncpy": (3, 0), "strcmp": (2, 0), "strncmp": (3, 0),
    "strlen": (1, 0), "strcat": (2, 0), "strchr": (2, 0), "strstr": (2, 0),
}


def param_slots(name):
    """(GPR slots, FPR slots) the callee actually takes, from its mangled name.

    The setup window still holds registers the call does not read -- a float
    computed for the NEXT statement, a pointer left in r5 by the last one --
    and those differ freely between two builds. Six of the first ten hits on
    the tree were a pair of them changing places. CodeWarrior's mangling says
    how many arguments there really are, so ask it.

    None when the name does not parse: a C symbol with no mangling at all
    (memcpy), a template, a nested qualified name. Then every slot is kept and
    the reader gets the noise, which is the old behaviour rather than a
    silently narrowed sweep.
    """
    # Deliberately not one regex. A class name is length-prefixed and can hold
    # anything -- `19@unnamed@xFont_cpp@` -- so the count has to be honoured
    # rather than pattern-matched: a regex stops at the F in `xFont` and calls
    # the name unparseable.
    if name in CARITY:
        return CARITY[name]

    split = None
    for m in re.finditer("__", name):
        i, method = m.end(), False
        j = i
        while j < len(name) and name[j].isdigit():
            j += 1
        if j > i:
            k = j + int(name[i:j])
            if k > len(name):
                continue
            rest, method = name[k:], True
        else:
            rest = name[i:]
        q = 0
        while q < len(rest) and rest[q] in "CV":
            q += 1
        if q < len(rest) and rest[q] == "F":
            split = (method, rest[q + 1:])
            break
    if split is None:
        return None

    method, p = split
    gpr = 1 if method else 0  # this
    fpr = 0
    i, n = 0, len(p)
    while i < n:
        indirect = False
        while i < n and p[i] in "PRUSCV":
            if p[i] in "PR":
                indirect = True
            i += 1
        if i >= n:
            return None
        c = p[i]
        if c.isdigit():
            j = i
            while j < n and p[j].isdigit():
                j += 1
            ln = int(p[i:j])
            if j + ln > n:
                return None
            i = j + ln
            gpr += 1
        elif c in "fd" and not indirect:
            fpr += 1
            i += 1
        elif c in "ilscbwxfd":
            gpr += 1
            i += 1
        elif c == "v":
            i += 1
        else:
            return None
    return (gpr, fpr)


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


def clean(nm):
    return DECOMPNUM.sub("$N", LOCALNUM.sub("$N", POOL.sub("@P", nm)))


def resolve(stream, i, reg, depth=0):
    """What defines `reg` just before index i, as a build-independent string.

    Register names are erased -- they are exactly what differs between two
    builds of the same source -- so a value is identified by where it came
    from. `mr` is followed rather than reported, because the whole point of
    the bug is that the two values reach the call through different registers
    on the two sides.
    """
    if depth > 4 or not REGOP.match(reg):
        return None

    cross = reg not in VOLATILE
    for j in range(i - 1, -1, -1):
        mnem, ops, nm = stream[j]
        if CALL.match(mnem) and not cross:
            # A volatile register set before this call would not have survived
            # it, so there is nothing further back to find.
            return None
        if not ops or not DEFS.match(mnem) or ops[0] != reg:
            continue

        if mnem == "mr" and len(ops) > 1:
            return resolve(stream, j, ops[1], depth + 1)
        if mnem == "fmr" and len(ops) > 1:
            return resolve(stream, j, ops[1], depth + 1)
        if nm:
            base = "<%s>" % clean(nm)
            m = DISP.match(ops[1]) if len(ops) > 1 else None
            return base + ("+%s" % m.group(1) if m else "")
        if mnem == "addi" and len(ops) > 2:
            if ops[1] == "r1":
                return "sp+%s" % ops[2]
            inner = resolve(stream, j, ops[1], depth + 1)
            return "%s+%s" % (inner or "?", ops[2])
        if LOADS.match(mnem) and len(ops) > 1:
            m = DISP.match(ops[1])
            if m:
                inner = resolve(stream, j, m.group(2), depth + 1)
                if m.group(2) == "r1":
                    inner = "sp"
                return "[%s+%s]" % (inner or "?", m.group(1))
            return mnem
        if mnem in ("li", "lis") and len(ops) > 1:
            return "%s %s" % (mnem, ops[1])
        return mnem.rstrip(".")
    return None


def sites(stream, want_calls):
    """One term per named call: the callee, its argument slots, their values.

    Which registers are arguments is decided by the SETUP WINDOW -- the
    instructions between the previous call and this one. CodeWarrior fills the
    argument registers there, and everything else still sitting in r5 or f2 is
    a leftover from earlier code that differs freely between two builds.
    Taking r3..r7 unconditionally drowns the signal: the confirmed instance has
    live junk in r5 and r6, which made the two sides differ in four positions
    rather than two and hid the transposition.
    """
    out = []
    prev = -1
    for i, (mnem, ops, nm) in enumerate(stream):
        if not CALL.match(mnem):
            continue
        name = clean(nm) if nm else None
        if name and (want_calls or ASSIGN.match(name)):
            lim = param_slots(name)
            regs = (ARGREGS[:lim[0]] + FARGREGS[:lim[1]]) if lim \
                else (ARGREGS + FARGREGS)
            slots, vals = [], []
            for reg in regs:
                if not any(stream[j][1] and DEFS.match(stream[j][0])
                           and stream[j][1][0] == reg
                           for j in range(prev + 1, i)):
                    continue
                v = resolve(stream, i, reg)
                if v is not None:
                    slots.append(reg)
                    vals.append(v)
            if len(slots) >= 2:
                out.append((name, tuple(slots), tuple(vals)))
        prev = i
    return out


def strong(sig):
    """Does this signature identify a particular value, or merely a shape?

    `sp+0x94` names one local, `<globals>+0x4` names one field, `[sp+0x8]` names
    one load. `fadds` names every add in the function, and two of those trading
    places is what the scheduler does for a living -- three of the first four
    hits on the tree were exactly that. A transposition is only evidence when
    both sides of it are things rather than shapes.
    """
    return "sp+" in sig or "<" in sig or "[" in sig


def transposition(a, b):
    """The one pair of positions that differ, if swapping them makes a == b."""
    if len(a) != len(b):
        return None
    diff = [k for k in range(len(a)) if a[k] != b[k]]
    if len(diff) != 2:
        return None
    x, y = diff
    if a[x] != b[y] or a[y] != b[x]:
        return None
    if not (strong(a[x]) and strong(a[y])):
        return None
    return (x, y)


def relabelling(T, O, a, b):
    """Is the whole function explained by these two values trading places?

    A frame slot names a variable only as long as both compilers allocated the
    frame the same way, and at 89% they often did not. `_xAnimTableAddTransition`
    declares `stateCount` then `allocCount`; retail puts them at sp+0x8 and
    sp+0xc, ours the other way round, and EVERY use follows -- so the two
    helper calls look transposed while the source passes them in the same
    order. That is a frame layout to fix for the match, not a bug to fix for
    the game.

    The test: rename a to b and b to a throughout OUR terms, and count how many
    terms agree with the target before and after. More agreement after means
    the two values are the same two variables under different names.

    The count has to run over ALL terms, not the differing ones. A real swap
    leaves the function's OTHER uses of the pair in the right order -- in
    xcsCalcAnimMatrices the same two locals are assigned the other way a few
    lines earlier, correctly -- and those terms match, so a difference-only
    test discards the very evidence that says the frame is not relabelled, and
    suppresses the one confirmed bug this tool exists to find.
    """
    swap = {a: b, b: a}
    O2 = Counter()
    for (callee, slots, vals), n in O.items():
        O2[(callee, slots, tuple(swap.get(v, v) for v in vals))] += n
    before = sum((T & O).values())
    after = sum((T & O2).values())
    return after > before


def scan(unit, want_calls):
    # A unit with no source of its own -- dtk lists plenty under --all -- has
    # no base object to compare against, and asking for one is a KeyError.
    if not unit.get("target_path") or not unit.get("base_path"):
        return []
    tgt = os.path.join(ROOT, unit["target_path"])
    base = os.path.join(ROOT, unit["base_path"])
    if not (os.path.exists(tgt) and os.path.exists(base)):
        return []
    out = os.path.join(tempfile.gettempdir(), "argswap_%d.json" % os.getpid())
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

    found, seen = [], set()
    for ls in d.get("left", {}).get("symbols") or []:
        if ls.get("kind") != "SYMBOL_FUNCTION":
            continue
        rs = right.get(ls.get("name"))
        if rs is None:
            continue
        # Multisets, not an index alignment: our compiler can emit a different
        # NUMBER of calls than retail (an unrolled loop, an inlined body), and
        # a swap still survives as a term on one side whose transposition is
        # on the other. Same reasoning as cmpswap.py.
        T = Counter(sites(rows(ls, lnames), want_calls))
        O = Counter(sites(rows(rs, rnames), want_calls))
        only_t, only_o = T - O, O - T
        for (callee, tslots, targs), n in sorted(only_t.items(), key=lambda kv: str(kv[0])):
            for (ocallee, oslots, oargs), m in only_o.items():
                if ocallee != callee or oslots != tslots or m < n:
                    continue
                pair = transposition(targs, oargs)
                if pair is None:
                    continue
                if relabelling(T, O, targs[pair[0]], targs[pair[1]]):
                    continue
                hit = {
                    "unit": unit["name"], "name": ls.get("name"),
                    "size": int(ls.get("size") or 0),
                    "pct": ls.get("match_percent"), "n": n,
                    "callee": callee, "pair": pair, "slots": tslots,
                    "target": targs, "ours": oargs,
                }
                key = (hit["unit"], hit["name"], callee, pair, targs, oargs)
                if key not in seen:
                    seen.add(key)
                    found.append(hit)
                break
    return found


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    want_calls = "--calls" in sys.argv
    allunits = "--all" in sys.argv
    verbose = "--v" in sys.argv

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
        hits.extend(scan(u, want_calls))

    hits.sort(key=lambda h: (h["unit"], h["name"]))
    print("\n%d transposed-argument site(s) in %d function(s), over %d unit(s)\n"
          % (len(hits), len(set((h["unit"], h["name"]) for h in hits)), len(units)))
    for h in hits:
        pct = h["pct"]
        x, y = h["pair"]
        print("%-58s %5d  %7s  [%s]"
              % (h["name"][:58], h["size"],
                 ("%.2f%%" % pct) if pct is not None else "-", h["unit"]))
        print("      %s  %s <-> %s  x%d"
              % (h["callee"], h["slots"][x], h["slots"][y], h["n"]))
        if verbose:
            print("      target : %s" % ", ".join(str(a) for a in h["target"]))
            print("      ours   : %s" % ", ".join(str(a) for a in h["ours"]))
        else:
            print("      target : %s=%s  %s=%s"
                  % (h["slots"][x], h["target"][x], h["slots"][y], h["target"][y]))
            print("      ours   : %s=%s  %s=%s"
                  % (h["slots"][x], h["ours"][x], h["slots"][y], h["ours"][y]))


if __name__ == "__main__":
    main()
