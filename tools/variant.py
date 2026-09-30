#!/usr/bin/env python3
"""Build a named variant compiler from an edited AliasPatch.c, then put the
tree back exactly as it was.

Ablating one clause is the only way to attribute a patch cost to a clause, and
narrowing a clause is the only way to test a proposed gate. Both mean deriving
a second compiler, which means touching three files that the build verifies
against each other -- AliasPatch.c, tools/aliaspatch_blob.py and
tools/patch_compiler.py. Doing that by hand leaves a half-restored tree the one
time something raises, and a stale GC/2.0p1a is dangerous: solo.py takes the
compiler path from build.ninja without checking its sha1, so every measurement
after it would silently be against the wrong compiler.

    python tools/variant.py <name> <old-text> <new-text>

Substitutes old->new (exactly once, or it refuses) in AliasPatch.c, derives the
compiler into build/compilers/GC/<name>, restores all three files and re-derives
the real GC/2.0p1a. Measure with:

    python tools/patchcost.py --stock GC/<name>            # what the variant FIXES
    python tools/patchcost.py --stock GC/<name> --gains    # what it BREAKS
"""

import hashlib
import pathlib
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import aliaspatch_link  # noqa: E402

SRC = pathlib.Path(aliaspatch_link.SRC)
BLOB = ROOT / "tools" / "aliaspatch_blob.py"
PC = ROOT / "tools" / "patch_compiler.py"
REAL = ROOT / "build" / "compilers" / "GC" / "2.0p1a"
LIVE = [SRC, BLOB, PC]


def sha1(p):
    return hashlib.sha1(pathlib.Path(p).read_bytes()).hexdigest()


def run(*cmd):
    subprocess.run(cmd, cwd=ROOT, check=True)


def derive():
    """Re-run patch_compiler.py for GC/2.0p1a. It unlinks the destination
    first, so a failure cannot leave the stock exe sitting at the patched
    name."""
    run(sys.executable, str(PC), str(REAL / "mwcceppc.exe"))


def restore(bak):
    for live in LIVE:
        shutil.copyfile(bak / live.name, live)


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    name, old, new = sys.argv[1], sys.argv[2], sys.argv[3]
    out = ROOT / "build" / "compilers" / "GC" / name

    for live in LIVE:
        if not live.exists():
            sys.exit(f"missing {live}; refusing to start")

    if out.exists():
        shutil.rmtree(out)

    tmp = tempfile.mkdtemp(prefix="aliasvariant_")
    bak = pathlib.Path(tmp)
    for live in LIVE:
        shutil.copyfile(live, bak / live.name)

    try:
        text = SRC.read_text()
        if text.count(old) != 1:
            sys.exit(f"pattern occurs {text.count(old)} times in AliasPatch.c, "
                     f"need exactly 1:\n  {old!r}")
        SRC.write_text(text.replace(old, new))

        run(sys.executable, str(ROOT / "tools" / "aliaspatch_link.py"), "--refresh")

        # The variant's hash is unknown by definition, so drop the assertion
        # for this one derive. It goes back with restore().
        pc = PC.read_text()
        marker = 'PATCHED_SHA1 = "'
        i = pc.index(marker)
        j = pc.index('"', i + len(marker)) + 1
        PC.write_text(pc[:i] + "PATCHED_SHA1 = None" + pc[j:])

        derive()
        shutil.copytree(REAL, out)
        print(f"\nvariant GC/{name}  sha1 {sha1(out / 'mwcceppc.exe')[:12]}")
    finally:
        restore(bak)
        derive()
        shutil.rmtree(tmp, ignore_errors=True)
        real = sha1(REAL / "mwcceppc.exe")
        import patch_compiler
        want = patch_compiler.PATCHED_SHA1
        print(f"restored GC/2.0p1a sha1 {real[:12]} "
              f"({'OK' if real == want else 'MISMATCH -- ' + want[:12]})")
        if real != want:
            return 1
    return 0


sys.exit(main())
