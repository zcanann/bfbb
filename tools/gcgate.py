#!/usr/bin/env python3
"""The GameCube regression gate: the DOL hash AND the matched-function count.

The DOL hash alone is not sufficient, and believing it was cost 16 functions.

Units marked `NonMatching` in configure.py link from the EXTRACTED object, not
from ours. So their source can regress arbitrarily -- to 0% -- and the DOL still
comes out byte-identical, because our object never reached the link. That is
precisely where all in-progress decomp work lives, so the hash is blind to the
work most likely to break.

That is not hypothetical. The PC-port platform layer rewrote

    void cruise_bubble::init()      ->      void init()

inside `namespace cruise_bubble { namespace { ... } }`. The qualified form
defines the function in the OUTER namespace with external linkage; the bare form
defines a different function with internal linkage in the anonymous one. Sixteen
zEntCruiseBubble functions went from 100% to 0%, eight other translation units
lost the symbols they call, and the DOL hash stayed green throughout.

So both numbers, every time.

Confirm a FAIL before believing it, especially just after a branch switch. The
incremental build can hand back a stale object: switching to a branch and
rebuilding reported check_hide_entities__Fv in zCutsceneMgr down to 91.047%,
and deleting that one .o and rebuilding put it back at 100% with nothing else
changed. Neither the unit nor any header it includes differed between the two
branches. So when this fails, delete the named unit's object, rebuild, and read
it again -- the same discipline the hand-off applies to `ninja | tail`.

Usage:
  gcgate.py                 check both; exit 1 if either fails
  gcgate.py --update        record the current numbers as the new baseline
  gcgate.py --which REPORT  name the functions that regressed against REPORT,
                            a report.json saved from a known-good build
"""

import hashlib
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DOL = os.path.join(ROOT, "build", "GQPE78", "main.dol")
REPORT = os.path.join(ROOT, "build", "GQPE78", "report.json")
BASELINE = os.path.join(ROOT, "tools", "gcgate.json")

RETAIL_DOL = "306526d90b48e99894c3138f5fc8f2716d9fecf6"


def measure():
    if not os.path.exists(DOL):
        sys.exit("no DOL at %s -- run ninja first" % DOL)
    sha = hashlib.sha1(open(DOL, "rb").read()).hexdigest()
    rep = json.load(open(REPORT))
    for c in rep["categories"]:
        if c["name"] == "Game Code":
            m = c["measures"]
            return sha, m["matched_functions"], m["matched_code_percent"]
    sys.exit("no 'Game Code' category in report.json")


def load_fns(path):
    out = {}
    for u in json.load(open(path))["units"]:
        if not u["name"].startswith("main/SB/"):
            continue
        for f in (u.get("functions") or []):
            out[(u["name"], f["name"])] = f.get("fuzzy_match_percent", 0.0)
    return out


def which(old_report):
    """Name the functions that were 100% in old_report and are not now."""
    old, new = load_fns(old_report), load_fns(REPORT)
    lost = [(k, new[k]) for k in old
            if k in new and old[k] == 100.0 and new[k] < 100.0]
    if not lost:
        print("nothing regressed against %s" % old_report)
        return
    print("%d function(s) were 100%% in %s and are not now:" % (len(lost), old_report))
    print("")
    for (u, f), p in sorted(lost, key=lambda x: x[1]):
        print("  %7.3f%%  %-44s %s" % (p, f[:44], u.replace("main/SB/", "")))


if "--which" in sys.argv:
    i = sys.argv.index("--which")
    if i + 1 >= len(sys.argv):
        sys.exit("--which needs a path to a known-good report.json")
    which(sys.argv[i + 1])
    sys.exit(0)

sha, fns, pct = measure()

if "--update" in sys.argv:
    # newline="" keeps this LF on Windows too; the repo is LF-only and CI
    # would otherwise see a spurious diff every time a maintainer updates it.
    with open(BASELINE, "w", newline="") as f:
        json.dump({"matched_functions": fns, "matched_code_percent": pct},
                  f, indent=1)
        f.write(chr(10))
    print("baseline recorded: %d functions, %.5f%%" % (fns, pct))
    sys.exit(0)

ok = True

if sha == RETAIL_DOL:
    print("  PASS  DOL       %s" % sha)
else:
    print("  FAIL  DOL       %s" % sha)
    print("                  expected %s" % RETAIL_DOL)
    ok = False

if os.path.exists(BASELINE):
    b = json.load(open(BASELINE))
    want = b["matched_functions"]
    if fns >= want:
        note = "" if fns == want else "   (+%d)" % (fns - want)
        print("  PASS  functions %d / %.5f%%%s" % (fns, pct, note))
    else:
        print("  FAIL  functions %d / %.5f%%" % (fns, pct))
        print("                  expected at least %d -- %d function(s) regressed,"
              % (want, want - fns))
        print("                  which the DOL hash cannot see")
        print("")
        print("  To name them:  python tools/gcgate.py --which <known-good report.json>")
        ok = False
else:
    print("  ----  functions %d / %.5f%%   (no baseline; run --update)" % (fns, pct))

sys.exit(0 if ok else 1)
