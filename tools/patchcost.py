#!/usr/bin/env python3
"""What the compiler patch buys and what it costs, function by function.

Default lists the COST: exact under stock GC/2.0p1, not exact under GC/2.0p1a.
`--gains` lists the other direction -- exact under the patch and not under
stock, which is the clauses' actual yield by name rather than by a remembered
total.

The branch's GC/2.0p1a is a net win by a wide margin, but every clause has
collateral, and a function the patch broke looks exactly like a function whose
source is wrong. Compiling the same unit twice -- once with each compiler --
separates them for free, and the answer is worth more than another source
variant: a function that is 100.000% under stock has nothing left to recover
from source.

Usage:
  patchcost.py [unit-frag ...] [--gains] [--stock GC/2.0p1]
"""

import json
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOLO = os.path.join(ROOT, "tools", "solo.py")
STOCK = "GC/2.0p1"
argv = sys.argv[1:]
if "--stock" in argv:
    i = argv.index("--stock")
    STOCK = argv[i + 1]
    # Drop the VALUE as well as the flag. It does not start with "-", so
    # leaving it in makes "--stock GC/2.0p1a-sz8" also mean "only units whose
    # name contains GC/2.0p1a-sz8" -- no units, and a silent 0-function sweep
    # that looks exactly like a clean result.
    del argv[i:i + 2]
FRAGS = [a for a in argv if not a.startswith("-")]

rep = json.load(open(os.path.join(ROOT, "build/GQPE78/report.json")))
units = [u["name"] for u in rep["units"] if u["name"].startswith("main/SB/")]
if FRAGS:
    units = [u for u in units if any(f in u for f in FRAGS)]

ROW = re.compile(r"^\s+([\d.]+)%\s+(\d+)b\s+(\S+)$")


def bad(unit, mw=None):
    """{symbol: percent} for every non-matching function in one unit."""
    cmd = [sys.executable, SOLO, unit.replace("main/", "")]
    if mw:
        cmd += ["--mw", mw]
    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    out = {}
    for line in r.stdout.splitlines():
        m = ROW.match(line)
        if m:
            out[m.group(3)] = (float(m.group(1)), int(m.group(2)))
    return out


GAINS = "--gains" in sys.argv


def do(unit):
    ours, stock = bad(unit), bad(unit, STOCK)
    if GAINS:
        # exact under the patch = absent from OUR non-matching list
        return [(unit, sym, pct, size) for sym, (pct, size) in stock.items()
                if sym not in ours]
    # exact under stock = absent from stock's non-matching list
    return [(unit, sym, pct, size) for sym, (pct, size) in ours.items()
            if sym not in stock]


rows = []
with ThreadPoolExecutor(6) as ex:
    for r in ex.map(do, units):
        rows.extend(r)

rows.sort(key=lambda r: -r[3])
col = "stock" if GAINS else "ours"
print("%-30s %-56s %8s %7s" % ("unit", "function", col, "bytes"))
for unit, sym, pct, size in rows:
    print("%-30s %-56s %7.3f%% %6d" % (unit.replace("main/SB/", ""), sym[:56], pct, size))
if GAINS:
    print("\n%d function(s), %d bytes, exact under the patch and not under %s"
          % (len(rows), sum(r[3] for r in rows), STOCK))
else:
    print("\n%d function(s), %d bytes, exact under %s and not under the patch"
          % (len(rows), sum(r[3] for r in rows), STOCK))
