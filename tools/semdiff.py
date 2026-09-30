#!/usr/bin/env python3
"""Find functions whose asm differs from the target SEMANTICALLY, not cosmetically.

Most non-matching functions differ only in register allocation and instruction
scheduling. Those are compiler-track problems: the source is already correct and
the game behaves right. A much smaller set differ in a way no scheduler or
allocator can produce -- a different immediate, a different memory offset, a
different mnemonic, an instruction present on one side and not the other. Those
are the ones where our C says something different from what retail's C said, so
those are the ones that are gameplay bugs.

The filter is a multiset comparison over normalised instructions. Register names
are erased, branch targets are erased, and anonymous @NNN pool ids are erased,
because none of those carry meaning. What survives is the mnemonic plus every
literal constant and memory offset. Reordering the instructions cannot change a
multiset and renaming registers cannot change a normalised one, so a multiset
difference is evidence of a real difference in what the code does.

This reads objects the ordinary `ninja` build already produced; it does not
compile anything, so it is safe to run while other work is going on.

Usage:
  semdiff.py                  scan every game-code unit
  semdiff.py <unit-frag> ...  scan only matching units
  semdiff.py --all            include SDK/MSL/RenderWare/Bink units too
  semdiff.py -v               also print each function's differing terms
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

# The game's own code. Everything else in the link -- Dolphin SDK, MSL,
# RenderWare, Bink, the CW runtime -- is a vendor library, and a mismatch
# there is not a gameplay bug.
GAME = "main/SB/"

REG = re.compile(r"\b[rf](?:3[01]|[12]\d|\d)\b")
CR = re.compile(r"\bcr\d\b")
QR = re.compile(r"\bqr\d\b")
POOL = re.compile(r"@\d+")
# A `$NNNN` suffix on a local static's name (tb$731) is the compiler's own
# numbering. It differs between any two builds of the same source, so leaving
# it in floods every function that touches a static with false differences --
# it buried a real fnmsubs/fnmadds sign error in xTRC under 50 phantom terms.
LOCALNUM = re.compile(r"\$\d+")
# Our decomp spells a recovered local static `sStripVert_2188` where CW emits
# `sStripVert$2188`. Same object, different spelling -- but a symbol name is
# attached to every instruction that references it, so the mismatch is counted
# once per reference. It reported 172 differing terms for xFXShineRender, which
# has 8, and put it top of srcprogress --list. Three digits or more, so an
# ordinary identifier ending in a small number is left alone.
DECOMPNUM = re.compile(r"_\d{3,}\b")
# A bare hex/decimal word that is a branch destination rather than a value.
BRANCH = re.compile(r"^(b|b[a-z]{1,4}|b[a-z]{1,4}\+|b[a-z]{1,4}-)$")


def norm(formatted):
    """Erase register identity, branch destinations and pool ordinals."""
    s = formatted.strip()
    parts = s.split(None, 1)
    mnem = parts[0]
    rest = parts[1] if len(parts) > 1 else ""
    if BRANCH.match(mnem):
        # Keep the condition (which branch) but drop where it goes: an address
        # differs between two objects for reasons that are never semantic.
        rest = re.sub(r"0x[0-9a-fA-F]+", "@@", rest)
    rest = REG.sub("R", rest)
    rest = CR.sub("CR", rest)
    rest = QR.sub("QR", rest)
    rest = POOL.sub("@P", rest)
    rest = LOCALNUM.sub("$N", rest)
    rest = DECOMPNUM.sub("$N", rest)
    return mnem + " " + rest.strip()


def instrs(sym, names):
    out = []
    for r in sym.get("instructions") or []:
        ins = r.get("instruction")
        if not ins:
            continue
        s = ins.get("formatted", "")
        rel = r.get("relocation") or ins.get("relocation")
        if rel:
            # A relocation names a symbol; the ordinal of an anonymous pool
            # entry does not matter but the symbol it points at does.
            t = rel.get("target_symbol")
            # target_symbol is an index into this side's symbol table. The two
            # objects number their symbols differently, so the index itself is
            # meaningless -- resolve it to the name, which is not.
            nm = names[t] if isinstance(t, int) and 0 <= t < len(names) else str(t)
            nm = DECOMPNUM.sub("$N", LOCALNUM.sub("$N", POOL.sub("@P", nm)))
            out.append(norm(s) + " <%s>" % nm)
        else:
            out.append(norm(s))
    return out


def stored(counter):
    """The operand of every store in a stream, e.g. `R, 0x1c8(R)`.

    Used to recognise the store-then-reload class. Retail stores a value and
    loads it straight back where our mwcc forwards the stored value, so the
    store cancels between the two streams and only the load shows as a
    difference. An operand that is stored on BOTH sides and loaded on only one
    is that pattern rather than a missing read.
    """
    out = set()
    for term in counter:
        parts = term.split(None, 1)
        if len(parts) == 2 and parts[0].startswith("st"):
            out.add(parts[1])
    return out


def scan(unit):
    tgt = os.path.join(ROOT, unit["target_path"])
    base = os.path.join(ROOT, unit["base_path"])
    if not (os.path.exists(tgt) and os.path.exists(base)):
        return []
    out = os.path.join("/tmp", "semdiff_%d.json" % os.getpid())
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

    right = {}
    for s in d.get("right", {}).get("symbols") or []:
        if s.get("kind") == "SYMBOL_FUNCTION":
            right[s.get("name")] = s

    found = []
    for ls in d.get("left", {}).get("symbols") or []:
        if ls.get("kind") != "SYMBOL_FUNCTION":
            continue
        pct = ls.get("match_percent")
        if pct is None or pct >= 100.0:
            continue
        rs = right.get(ls.get("name"))
        if rs is None:
            continue
        L = Counter(instrs(ls, lnames))
        R = Counter(instrs(rs, rnames))
        only_t, only_o = L - R, R - L
        if only_t or only_o:
            found.append({
                "unit": unit["name"],
                "name": ls.get("name"),
                "size": int(ls.get("size") or 0),
                "pct": pct,
                "target_only": sorted(only_t.elements()),
                "ours_only": sorted(only_o.elements()),
                "stored_both": stored(L) & stored(R),
                "ours_terms": set(R),
            })
    return found


# Address arithmetic, branches and register moves are what loop unrolling and
# strength reduction rewrite. Two builds of the same C differ in these all the
# time. A genuinely different computation shows up as an arithmetic, memory or
# call mnemonic that one side executes and the other never does.
SHAPE = re.compile(r"^(addi|addic|subi|add|subf|mulli|slwi|srwi|srawi|sraw"
                   r"|li|lis|mr|neg|rlwinm|clrlwi|clrrwi|extrwi|insrwi"
                   r"|cmp\w*|b\w*|mtctr|mfctr|andi|ori|nop)\.?$")


# `stwx` and `stw` store the same word; only the addressing differs, and which
# one you get is decided by strength reduction, not by the C. Fold the indexed
# and update forms onto the base so that a memory op only counts as a difference
# when the WIDTH changes -- lhz against lwz is a type error, lwzx against lwz is
# not.
MEMFORM = re.compile(r"^(lbz|lhz|lha|lwz|lfs|lfd|stb|sth|stw|stfs|stfd)(u|x|ux)$")

# Saving and restoring callee-saved registers. lmw/stmw and the paired-single
# psq_l/psq_st only ever appear in a prologue or epilogue, and lfd/stfd join
# them there to save the halves of an FPR. How many registers a function saves
# is decided by the allocator, so a difference made up ENTIRELY of these says
# nothing about the source -- it was the whole of xScrFXGlareRender's apparent
# `retail uses paired-singles` lead, which was really a colour expression being
# recomputed sixteen times.
FRAME = {"lmw", "stmw", "psq_l", "psq_st", "lfd", "stfd"}


def exclusive_kinds(hit):
    """Mnemonics one side executes and the other never does, shape ignored.

    This is the triage that separates a real difference from an unrolled loop.
    Tridiag_Solve shows 110 differing terms and nothing here but address
    arithmetic -- it is our loop unrolled twice against retail's, and the source
    is right. xScrFXGlareRender shows retail using psq_l/psq_st/lmw where we use
    fctiwz/fmuls, which is a different computation and worth reading.
    """
    def mnem(t):
        m = t.split(None, 1)[0]
        f = MEMFORM.match(m)
        return f.group(1) if f else m
    t = set(mnem(x) for x in hit["target_only"])
    o = set(mnem(x) for x in hit["ours_only"])
    et = sorted(m for m in t - o if not SHAPE.match(m))
    eo = sorted(m for m in o - t if not SHAPE.match(m))
    if set(et) | set(eo) <= FRAME:
        return [], []          # register save/restore only; the allocator's business
    return et, eo


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    verbose = "-v" in sys.argv
    kinds = "--kinds" in sys.argv
    allunits = "--all" in sys.argv

    units = []
    for u in CFG["units"]:
        n = u["name"]
        if not allunits and not n.startswith(GAME):
            continue
        if args and not any(a.lower() in n.lower() for a in args):
            continue
        units.append(u)

    hits = []
    for u in units:
        hits.extend(scan(u))

    hits.sort(key=lambda h: -(h["size"] or 0))
    print("%d function(s) with a semantic difference, over %d unit(s)\n"
          % (len(hits), len(units)))
    for h in hits:
        n = len(h["target_only"]) + len(h["ours_only"])
        print("%-64s %-7s %6.2f%%  %2d term(s)  [%s]"
              % (h["name"][:64], h["size"], h["pct"], n, h["unit"]))
        if kinds:
            et, eo = exclusive_kinds(h)
            if et:
                print("      retail-only mnemonics: %s" % ", ".join(et))
            if eo:
                print("      ours-only   mnemonics: %s" % ", ".join(eo))
            if not et and not eo:
                print("      (no operation either side lacks -- shape only)")
        if verbose:
            for t in h["target_only"]:
                print("      target only : %s" % t)
            for t in h["ours_only"]:
                print("      ours   only : %s" % t)
    return 0


if __name__ == "__main__":
    sys.exit(main())
