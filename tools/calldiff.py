#!/usr/bin/env python3
"""Compare the SET of functions each function calls, target vs ours.

A `bl` to the wrong function is the same instruction, the same size, at the same
offset, so objdiff scores it 100%. iParMgrRenderParSys_QuadStreak called
iRenderPushFlat instead of iRenderPushQuadStreak and sat at 99.878% while every
quad-streak particle system in the game rendered as a flat ground quad.

This resolves each R_PPC_REL24 relocation to its callee and compares the multiset
per function.

The attribution has to be by ADDRESS RANGE, not by the labels objdump prints.
dtk's reconstructed symbols overlap in the target objects -- zNPCSupplement has a
weak NextAvail at 0x14c, 0x78 bytes inside NPCC_MakeLightningInfo -- so objdump
emits a label mid-body and one function's calls get split across two names. A
label-keyed version of this sweep reported hundreds of adjacent pairs holding
each other's callees, all of it noise. Symbols that overlap another are dropped
rather than guessed at.

Self-check: comparing an object against itself must report nothing. Run with
--self to verify that before believing any output.

Usage:
  calldiff.py [unit-frag ...] [--self] [--min-pct N]
"""

import json
import os
import re
import subprocess
import sys
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OD = os.path.join(ROOT, "build", "binutils", "powerpc-eabi-objdump.exe")

SYM = re.compile(r"^([0-9a-f]{8})\s.{7}\s+(\S+)\s+([0-9a-f]{8})\s+(\S+)$")
INSN = re.compile(r"^\s*([0-9a-f]+):\t")
SECT = re.compile(r"^Disassembly of section (\S+):$")
REL24 = re.compile(r"R_PPC_REL24\s+(\S+)")

SELF = "--self" in sys.argv
MIN_PCT = 0.0
if "--min-pct" in sys.argv:
    MIN_PCT = float(sys.argv[sys.argv.index("--min-pct") + 1])
FRAGS = [a for a in sys.argv[1:]
         if not a.startswith("-") and not re.match(r"^[\d.]+$", a)]


def franges(obj):
    """[(section, start, end, name)] for .text symbols, overlaps dropped."""
    out = subprocess.run([OD, "-t", obj], capture_output=True, text=True).stdout
    fns = []
    for l in out.splitlines():
        m = SYM.match(l)
        if m and ".text" in m.group(2):
            start, size = int(m.group(1), 16), int(m.group(3), 16)
            if size:
                fns.append((m.group(2), start, start + size, m.group(4)))
    return [f for f in fns
            if not any(g is not f and g[0] == f[0] and g[1] < f[2] and f[1] < g[2]
                       for g in fns)]


def callees(obj):
    ranges = franges(obj)
    by_sec = {}
    for sec, st, en, nm in ranges:
        by_sec.setdefault(sec, []).append((st, en, nm))
    res = {nm: Counter() for _, _, _, nm in ranges}

    def owner(sec, a):
        for st, en, nm in by_sec.get(sec, ()):
            if st <= a < en:
                return nm
        return None

    out = subprocess.run([OD, "-d", "-r", "-M", "broadway", obj],
                         capture_output=True, text=True).stdout
    sec, cur = None, None
    for l in out.splitlines():
        m = SECT.match(l)
        if m:
            sec, cur = m.group(1), None
            continue
        m = INSN.match(l)
        if m:
            cur = owner(sec, int(m.group(1), 16))
            continue
        m = REL24.search(l)
        if m and cur:
            res[cur][m.group(1)] += 1
    return res


rep = json.load(open(os.path.join(ROOT, "build/GQPE78/report.json")))
units = [u["name"][len("main/"):] for u in rep["units"]
         if u["name"].startswith("main/SB/")]
if FRAGS:
    units = [u for u in units if any(f in u for f in FRAGS)]

score = {}
for u in rep["units"]:
    if u["name"].startswith("main/SB/"):
        for f in u.get("functions", []):
            score[(u["name"][len("main/"):], f["name"])] = f.get("fuzzy_match_percent", 0.0)

hits = 0
for u in units:
    t = os.path.join(ROOT, "build/GQPE78/obj", u + ".o")
    o = os.path.join(ROOT, "build/GQPE78/src", u + ".o")
    if not (os.path.exists(t) and os.path.exists(o)):
        continue
    T = callees(t)
    O = callees(t if SELF else o)
    for fn, tc in sorted(T.items()):
        oc = O.get(fn)
        if oc is None or tc == oc:
            continue
        if score.get((u, fn), 0.0) < MIN_PCT:
            continue
        miss, extra = tc - oc, oc - tc
        hits += 1
        print("%s  [%s]  %.2f%%" % (fn, u, score.get((u, fn), 0.0)))
        for k, v in sorted(miss.items()):
            print("     target only: %s x%d" % (k, v))
        for k, v in sorted(extra.items()):
            print("     ours   only: %s x%d" % (k, v))

print("\n%d function(s) with a differing callee set%s"
      % (hits, "  [SELF-CHECK]" if SELF else ""))
