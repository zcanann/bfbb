#!/usr/bin/env python3
"""Classify every non-matching function in the project by *why* it differs.

"3000 functions left" is not actionable; "146 of them are byte-identical apart
from a literal pool index" is. Run this after a build to decide where the next
pass should go.

  MISSING  no symbol in our object - not written yet
  POOL     every differing instruction is identical apart from the name of an
           anonymous (@NNN) relocation target, i.e. blocked on literal or
           template pool composition rather than on codegen. objdiff pairs
           anonymous symbols by ordinal position within a section, so this
           only resolves once the whole translation unit is written
  SCHED    same instruction multiset, different order
  REGS     same mnemonics, only register numbers differ - usually responds to
           reshaping the source expression
  SIZE     different instruction count - a real source difference
  OTHER    same count, different code

Reads the objects a previous `ninja` produced; it does not build anything.
Writes classify.json next to the repo root and prints a summary.

Usage: classify.py [unit-fragment]
"""
import json
import os
import re
import subprocess
import sys
import tempfile

import cwexec
from collections import Counter
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CLI = cwexec.objdiff_cli(ROOT)
CFG = json.load(open(os.path.join(ROOT, "objdiff.json")))

REG = re.compile(r"\b[rf]\d{1,2}\b")


def norm_reloc(txt):
    # Two flavours of unstable identifier. `@1171` is an anonymous pool entry,
    # numbered by ordinal within its section, so the same literal reads as
    # @1171 on one side and @531 on the other. `name$1647` is the suffix CW
    # gives a function-local static; the number is a per-translation-unit
    # counter, so it differs for the same variable. Neither says anything
    # about codegen, and leaving $NNNN unnormalised misfiled every function
    # whose only residue was a local-static reference as OTHER.
    return re.sub(r"\$\d+", "$X", re.sub(r"@\d+", "@X", txt))


# objdiff prints a local branch's target as a section-relative address, so the
# same branch reads differently whenever the function sits at a different
# offset in .text -- which it does in most non-matching units. Comparing the
# raw text therefore reports every branch in the function as a difference.
# Rewriting the target as a delta from the branch's own address makes it
# placement-independent. `bl <symbol>` carries a relocation instead of a hex
# target and is deliberately left alone.
BRANCH_ABS = re.compile(r"^(b\S*)\s+(0x[0-9a-fA-F]+)\s*$")


def fmt(entry):
    ins = entry.get("instruction", {})
    txt = ins.get("formatted", "")
    m = BRANCH_ABS.match(txt)
    if m:
        try:
            return "%s %+d" % (m.group(1), int(m.group(2), 16) - int(ins["address"]))
        except (KeyError, TypeError, ValueError):
            pass
    return txt


def mnemonic(text):
    return text.split(" ")[0].split(",")[0]


def classify(left_sym, right_sym):
    ai = left_sym.get("instructions", [])
    bi = right_sym.get("instructions", []) if right_sym else []
    la = [fmt(e) for e in ai if e.get("instruction")]
    lb = [fmt(e) for e in bi if e.get("instruction")]
    if not lb:
        return "MISSING"

    if len(ai) == len(bi):
        pool = True
        for a, b in zip(ai, bi):
            ta, tb = fmt(a), fmt(b)
            if ta == tb:
                continue
            if norm_reloc(ta) == norm_reloc(tb) and ("@" in ta or "$" in ta):
                continue
            pool = False
            break
        if pool:
            return "POOL"

    if len(la) != len(lb):
        return "SIZE"
    if Counter(map(norm_reloc, la)) == Counter(map(norm_reloc, lb)):
        return "SCHED"
    if [mnemonic(x) for x in la] == [mnemonic(x) for x in lb] and \
            all(REG.sub("R", norm_reloc(x)) == REG.sub("R", norm_reloc(y))
                for x, y in zip(la, lb)):
        return "REGS"
    return "OTHER"


def do_unit(unit):
    fd, out = tempfile.mkstemp(suffix=".json")
    os.close(fd)
    try:
        subprocess.run([CLI, "diff",
                        "-1", os.path.join(ROOT, unit["target_path"]),
                        "-2", os.path.join(ROOT, unit["base_path"]),
                        "-o", out, "--format", "json"],
                       cwd=ROOT, capture_output=True, timeout=300)
        d = json.load(open(out))
    except Exception:
        return []
    finally:
        try:
            os.unlink(out)
        except OSError:
            pass

    if "symbols" not in d.get("left", {}) or "symbols" not in d.get("right", {}):
        return []
    right = {s["name"]: s for s in d["right"]["symbols"]
             if s.get("kind") == "SYMBOL_FUNCTION"}
    rows = []
    for ls in d["left"]["symbols"]:
        if ls.get("kind") != "SYMBOL_FUNCTION":
            continue
        pct = ls.get("match_percent", 0.0)
        if pct >= 100.0:
            continue
        rows.append((unit["name"], ls["name"], pct, int(ls.get("size", 0)),
                     classify(ls, right.get(ls["name"]))))
    return rows


def main():
    frag = sys.argv[1] if len(sys.argv) > 1 else ""
    units = [u for u in CFG["units"] if frag in u["name"]]
    rows = []
    with ThreadPoolExecutor(8) as ex:
        for r in ex.map(do_unit, units):
            rows.extend(r)

    json.dump(rows, open(os.path.join(ROOT, "classify.json"), "w"))
    print("non-matching functions: %d" % len(rows))
    for k, v in Counter(r[4] for r in rows).most_common():
        print("  %-8s %d" % (k, v))

    print("\nunits with the most POOL-blocked functions:")
    for u, n in Counter(r[0] for r in rows if r[4] == "POOL").most_common(25):
        print("  %4d  %s" % (n, u))

    print("\nunits closest to complete:")
    near = Counter(r[0] for r in rows)
    for u, n in sorted(near.items(), key=lambda x: x[1])[:25]:
        print("  %4d  %s" % (n, u))


main()
