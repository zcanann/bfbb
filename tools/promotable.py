#!/usr/bin/env python3
"""Units whose functions and data all match but which configure.py still links
from the extracted object.

A unit only reaches `complete_units`, and only links from OUR object, when
configure.py marks it Matching. objdiff reaching 100% is necessary and not
sufficient -- it pairs symbols by name and is blind to definition order -- so a
promotion is a hypothesis that the DOL sha1 has to confirm.

Measured 2026-08-31: of the 7 units this listed, exactly ONE (zAnimList)
promoted with the DOL still at retail's 306526d9. The other six -- iCamera,
xDebug, xHudMeter, xParEmitterType, zShrapnel, zSurface -- each moved the DOL
on their own. Treat a row here as a lead worth one build, not as free progress,
and promote ONE unit per build: once a unit shifts addresses the link moves
everything after it, so a combined attempt cannot tell you which unit is at
fault.

Usage: promotable.py
"""

import json
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
rep = json.load(open(os.path.join(ROOT, "build/GQPE78/report.json")))
cfg = open(os.path.join(ROOT, "configure.py")).read()

out = []
for u in rep["units"]:
    n = u["name"]
    if not n.startswith("main/SB/"):
        continue
    m = u["measures"]
    if m.get("matched_functions") != m.get("total_functions"):
        continue
    if int(m.get("matched_data", 0)) != int(m.get("total_data", 0)):
        continue
    st = "?"
    for ext in (".cpp", ".c"):
        path = n.replace("main/", "") + ext
        mm = re.search(r'Object\((\w+), "%s"[,)]' % re.escape(path), cfg)
        if mm:
            st = mm.group(1)
            break
    if st != "Matching":
        out.append((st, path, m.get("total_functions", 0)))

for st, path, nf in sorted(out):
    print("%-14s %-44s %d functions" % (st, path, nf))
print("%d unit(s) fully matching but not marked Matching" % len(out))
