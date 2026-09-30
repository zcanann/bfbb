#!/usr/bin/env python3
"""Find which units can be marked Matching in configure.py without moving the DOL.

`complete_units` counts `Object(Matching, ...)` entries, not units that reach
100% in report.json. Flipping a marker makes dtk link our object instead of the
extracted original, so it is only safe when ours would link to the same bytes --
and report.json scores things 100% that would not. It called zDispatcher 23/23
with 100% matched data while the built object's .data was 92 bytes against the
target's 96.

The only reliable test is a real link. It is cheap: the objects are already
built, so flipping one marker and running ninja costs about nine seconds. This
flips each candidate on its own -- do not bisect, the failures are independent.

Usage:
  fliptest.py                list candidates (units at 100% not marked Matching)
  fliptest.py --test         flip each candidate alone and report PASS/FAIL
  fliptest.py --test <frag>  test only candidates matching these fragments
  fliptest.py --apply <frag> flip these for real, leaving configure.py edited

--test and --apply rewrite configure.py; --test restores it with `git checkout`
when it is done, so commit or stash your own configure.py edits first.

When a unit fails, tools/symorder.py usually explains it.
"""
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CONFIG = ROOT / "configure.py"
REPORT = ROOT / "build/GQPE78/report.json"
MARKER = re.compile(r'Object\((NonMatching|Equivalent), "([^"]+)"')


def candidates():
    """Units at 100% in report.json that configure.py does not link."""
    report = json.load(open(REPORT))
    done = set()
    for u in report["units"]:
        m, md = u["measures"], (u.get("metadata") or {})
        if md.get("complete") is not False:
            continue
        if m.get("fuzzy_match_percent") != 100.0:
            continue
        if md.get("source_path"):
            done.add(md["source_path"].replace("\\", "/"))
    out = []
    for _, name in MARKER.findall(CONFIG.read_text(encoding="utf-8")):
        if any(sp.endswith("/" + name) or sp == name for sp in done):
            out.append(name)
    return out


def flip(names):
    src = CONFIG.read_text(encoding="utf-8")
    hit = []

    def sub(mo):
        if mo.group(2) in names:
            hit.append(mo.group(2))
            return 'Object(Matching, "%s"' % mo.group(2)
        return mo.group(0)

    with open(CONFIG, "w", encoding="utf-8", newline="") as f:
        f.write(MARKER.sub(sub, src))
    return hit


def links():
    return subprocess.run(["ninja"], cwd=ROOT, capture_output=True).returncode == 0


def restore():
    subprocess.run(["git", "checkout", "--", "configure.py"], cwd=ROOT,
                   capture_output=True)


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    names = candidates()
    if args:
        names = [n for n in names if any(a in n for a in args)]

    if "--apply" in sys.argv:
        print("flipped %d:" % len(flip(names)))
        print("\n".join("  " + n for n in names))
        sys.exit(0)

    if "--test" not in sys.argv:
        print("%d candidates:" % len(names))
        print("\n".join("  " + n for n in names))
        sys.exit(0)

    passed = []
    for name in names:
        restore()
        flip([name])
        ok = links()
        passed.append(name) if ok else None
        print("%s  %s" % ("PASS" if ok else "FAIL", name), flush=True)
    restore()
    if not links():
        sys.exit("could not restore a matching DOL -- check configure.py")
    print("\n%d of %d pass individually. Flip them together with:" %
          (len(passed), len(names)))
    print("  python tools/fliptest.py --apply " + " ".join(passed))
