#!/usr/bin/env python3
"""Print a unit's .rodata object layout, target beside ours.

The target objects open .rodata with anonymous const templates that nothing
references -- literals left behind by functions the retail link deadstripped.
Reproducing them is the `__deadstripped_<unit>` idiom (zVar.cpp is the
reference); getting the sizes and the ORDER right is the whole job, and this
prints both sides so you can read them off instead of guessing.

Usage: rodatalayout.py <unit-frag> [section]
"""

import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OD = os.path.join(ROOT, "build", "binutils", "powerpc-eabi-objdump.exe")
frag = sys.argv[1]
sec = sys.argv[2] if len(sys.argv) > 2 else ".rodata"

rep = json.load(open(os.path.join(ROOT, "build/GQPE78/report.json")))
units = [u["name"][len("main/"):] for u in rep["units"]
         if u["name"].startswith("main/SB/") and frag in u["name"]]
if len(units) != 1:
    sys.exit("ambiguous or no match: %s" % ", ".join(units))
unit = units[0]


def layout(obj):
    out = subprocess.run([OD, "-t", obj], capture_output=True, text=True).stdout
    rows = []
    for line in out.splitlines():
        f = line.split()
        if len(f) >= 6 and f[-3] == sec and f[-1] != sec:
            rows.append((int(f[0], 16), int(f[-2], 16), f[-1]))
    rows.sort()
    return rows


t = layout(os.path.join(ROOT, "build/GQPE78/obj", unit + ".o"))
o = layout(os.path.join(ROOT, "build/GQPE78/src", unit + ".o"))
print("%s %s   %-34s | %s" % (unit, sec, "TARGET", "OURS"))
for i in range(max(len(t), len(o))):
    a = "%08x %5s %s" % (t[i][0], hex(t[i][1]), t[i][2]) if i < len(t) else ""
    b = "%08x %5s %s" % (o[i][0], hex(o[i][1]), o[i][2]) if i < len(o) else ""
    mark = "  " if (i < len(t) and i < len(o) and t[i][1] == o[i][1]) else "| "
    print("%s%-34s | %s" % (mark, a, b))
