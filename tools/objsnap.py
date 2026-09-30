#!/usr/bin/env python3
"""Byte-identity snapshot of every built object, for compiler-patch work.

A change to tools/patch_compiler.py changes the derived compiler's sha1, and
then every measurement either side of it is incomparable. The acceptance test
for a refactor that is supposed to change NO behaviour is therefore not the
match percentage -- it is that every object comes out byte-for-byte as it did
before, and that the DOL still hashes to retail.

report.json cannot do this job. It scores symbols by name and normalises pool
ordinals, so an object can shift, gain a symbol, or reorder its .text and still
report the same percentages. Only the bytes settle it.

Usage:
  objsnap.py save <out.json>          hash every built object and the DOL
  objsnap.py cmp  <before.json>       compare the tree against a snapshot

`cmp` exits non-zero on any difference and names the objects, so it can gate a
compiler change the way gcgate.py gates a source change.
"""

import hashlib
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "build/GQPE78/src")
DOL = os.path.join(ROOT, "build/GQPE78/main.dol")
COMPILER = os.path.join(ROOT, "build/compilers/GC/2.0p1a/mwcceppc.exe")


def sha1(path):
    h = hashlib.sha1()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def collect():
    objs = {}
    for base, _, files in os.walk(SRC):
        for f in files:
            if f.endswith(".o"):
                p = os.path.join(base, f)
                objs[os.path.relpath(p, SRC).replace("\\", "/")] = sha1(p)
    out = {"objects": objs}
    for name, path in (("dol", DOL), ("compiler", COMPILER)):
        out[name] = sha1(path) if os.path.exists(path) else None
    return out


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    mode, path = sys.argv[1], sys.argv[2]

    if mode == "save":
        snap = collect()
        with open(path, "w", newline="") as f:
            json.dump(snap, f, indent=1, sort_keys=True)
            f.write(chr(10))
        print("saved %d object(s)" % len(snap["objects"]))
        print("  dol      %s" % snap["dol"])
        print("  compiler %s" % snap["compiler"])
        return 0

    if mode != "cmp":
        sys.exit(__doc__)

    old = json.load(open(path))
    new = collect()
    a, b = old["objects"], new["objects"]

    gone = sorted(set(a) - set(b))
    added = sorted(set(b) - set(a))
    changed = sorted(k for k in set(a) & set(b) if a[k] != b[k])

    if old.get("compiler") != new.get("compiler"):
        print("compiler %s -> %s" % (old.get("compiler"), new.get("compiler")))
    else:
        print("compiler unchanged %s" % new.get("compiler"))

    for label, names in (("missing", gone), ("new", added), ("CHANGED", changed)):
        if names:
            print("\n%d %s object(s):" % (len(names), label))
            for n in names[:40]:
                print("  %s" % n)
            if len(names) > 40:
                print("  ... and %d more" % (len(names) - 40))

    dol_ok = old.get("dol") == new.get("dol")
    print("\ndol %s" % ("unchanged" if dol_ok else
                        "CHANGED %s -> %s" % (old.get("dol"), new.get("dol"))))

    if not (gone or added or changed) and dol_ok:
        print("\nPASS  %d object(s) byte-identical" % len(b))
        return 0
    print("\nFAIL")
    return 1


sys.exit(main())
