#!/usr/bin/env python3
"""Find element strides ours emits that the target never does.

The richest bug class in this decomp is a raw byte offset applied to a typed
pointer, which then scales twice: `ptr += 0x40` on an `xSphere*` advances
0x40*16 bytes. objdiff cannot see it -- it is a valid instruction at the right
offset -- so it hides in units scoring 94-99%.

Two immediates carry stride information:

  mulli rD,rA,IMM       an element size, when indexing is not a shift
  addi  rX,rX,IMM       a loop step, when rX is its own source

Both are compared as a per-function multiset, target against ours. The `addi`
form is noisy -- a string-pool base is `lis` + `addi rN,rN,BIG` and looks
identical -- so an `addi` hit is only reported when the preceding instruction
is not a `lis` into the same register.

Usage:
  stridediff.py [unit-frag ...]
"""

import json
import os
import re
import subprocess
import sys
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OD = os.path.join(ROOT, "build", "binutils", "powerpc-eabi-objdump.exe")
FRAGS = [a for a in sys.argv[1:] if not a.startswith("-")]

FUNC = re.compile(r"^[0-9a-f]+ <([^>]+)>:")
INSN = re.compile(r"^\s*[0-9a-f]+:\s+(?:[0-9a-f]{2} ){4}\s*(\S+)\s*(.*)$")
MULLI = re.compile(r"^r\d+,r\d+,(-?\d+)$")
ADDI = re.compile(r"^r(\d+),r(\d+),(-?\d+)$")
LIS = re.compile(r"^r(\d+),")


def strides(obj):
    """{function: Counter('mulli N' | 'addi N')}"""
    out = subprocess.run([OD, "-d", "-M", "broadway", os.path.abspath(obj)],
                         capture_output=True, text=True).stdout
    res, fn, prev = {}, None, ("", "")
    for line in out.splitlines():
        m = FUNC.match(line)
        if m:
            fn = m.group(1)
            res.setdefault(fn, Counter())
            prev = ("", "")
            continue
        m = INSN.match(line)
        if not m or fn is None:
            continue
        op, args = m.group(1), m.group(2).split()[0] if m.group(2) else ""
        if op == "mulli":
            g = MULLI.match(args)
            if g:
                res[fn]["mulli %s" % g.group(1)] += 1
        elif op == "addi":
            g = ADDI.match(args)
            # self-increment only, and not the low half of a lis/addi pair
            if g and g.group(1) == g.group(2):
                lis_same = (prev[0] == "lis" and
                            LIS.match(prev[1] or "") and
                            LIS.match(prev[1]).group(1) == g.group(1))
                if not lis_same:
                    res[fn]["addi %s" % g.group(3)] += 1
        prev = (op, args)
    return res


rep = json.load(open(os.path.join(ROOT, "build/GQPE78/report.json")))
units = [u["name"][len("main/"):] for u in rep["units"]
         if u["name"].startswith("main/SB/")]
if FRAGS:
    units = [u for u in units if any(f in u for f in FRAGS)]

hits = 0
for u in units:
    t = os.path.join(ROOT, "build/GQPE78/obj", u + ".o")
    o = os.path.join(ROOT, "build/GQPE78/src", u + ".o")
    if not (os.path.exists(t) and os.path.exists(o)):
        continue
    T, O = strides(t), strides(o)
    for fn in sorted(set(T) & set(O)):
        a, b = T[fn], O[fn]
        miss, extra = a - b, b - a
        if not (miss or extra):
            continue
        hits += 1
        print("%s  [%s]" % (fn, u))
        if miss:
            print("     target only: %s" % ", ".join(
                "%s x%d" % (k, v) for k, v in sorted(miss.items())))
        if extra:
            print("     ours   only: %s" % ", ".join(
                "%s x%d" % (k, v) for k, v in sorted(extra.items())))

print("\n%d function(s) with a differing stride multiset" % hits)
