#!/usr/bin/env python3
"""Patch a built main.dol into a copy of the retail disc image.

The playtest build (`configure.py --non-matching && ninja`) produces a DOL with
every src/SB unit built from our source. It still needs the disc's assets, so to
run it you boot the retail ISO with our DOL swapped in.

Our DOL is smaller than retail's, so it is written over the original in place and
the slack up to the FST is zeroed. Nothing points into that slack -- the apploader
reads only what the DOL header describes -- so the disc layout is untouched and no
filesystem rebuild is needed.

    python tools/playtest_iso.py <retail.iso> <output.iso>

The copy is made once; re-run after every build to refresh the DOL, which rewrites
about 3 MB rather than the 1.4 GB image.
"""

import argparse
import hashlib
import os
import shutil
import struct
import sys

RETAIL_DOL_SHA1 = "306526d90b48e99894c3138f5fc8f2716d9fecf6"
EXPECTED_GAME_ID = "GQPE78"


def read_header(path):
    with open(path, "rb") as f:
        hdr = f.read(0x450)
    if len(hdr) < 0x450:
        sys.exit(f"{path}: too small to be a GameCube disc image")
    return {
        "game_id": hdr[0x00:0x06].decode("ascii", "replace"),
        "title": hdr[0x20:0x60].split(b"\0")[0].decode("ascii", "replace"),
        "dol_off": struct.unpack(">I", hdr[0x420:0x424])[0],
        "fst_off": struct.unpack(">I", hdr[0x424:0x428])[0],
    }


def dol_len(blob):
    """Length of a DOL from its own section table."""
    offs = struct.unpack(">18I", blob[0x00:0x48])
    sizes = struct.unpack(">18I", blob[0x90:0xD8])
    return max(o + s for o, s in zip(offs, sizes) if s)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("source", help="retail GQPE78 disc image")
    ap.add_argument("output", help="patched image to create or refresh")
    ap.add_argument("--dol", default=os.path.join("build", "GQPE78", "main.dol"),
                    help="DOL to inject (default: %(default)s)")
    ap.add_argument("--force", action="store_true",
                    help="patch even if the source is not the expected build")
    args = ap.parse_args()

    if not os.path.exists(args.dol):
        sys.exit(f"{args.dol}: not found -- run `python configure.py --non-matching && ninja` first")
    new_dol = open(args.dol, "rb").read()

    src = read_header(args.source)
    if src["game_id"] != EXPECTED_GAME_ID and not args.force:
        sys.exit(f"{args.source}: game id is {src['game_id']}, expected {EXPECTED_GAME_ID}"
                 " (use --force to override)")

    # Verify the source really is the build this decomp targets, by hashing the
    # DOL it ships. Only meaningful on an unpatched image.
    with open(args.source, "rb") as f:
        f.seek(src["dol_off"])
        head = f.read(0x100)
        n = dol_len(head)
        f.seek(src["dol_off"])
        sha = hashlib.sha1(f.read(n)).hexdigest()
    if sha == RETAIL_DOL_SHA1:
        print(f"source: {src['game_id']}  {src['title']}  (retail DOL verified)")
    elif os.path.abspath(args.source) == os.path.abspath(args.output):
        print(f"source: {src['game_id']}  (already patched, refreshing in place)")
    elif not args.force:
        sys.exit(f"{args.source}: embedded DOL sha1 is {sha},\n"
                 f"  expected {RETAIL_DOL_SHA1}.\n"
                 "  This is a different build of the game (use --force to override).")

    room = src["fst_off"] - src["dol_off"]
    if len(new_dol) > room:
        sys.exit(f"{args.dol} is {len(new_dol)} bytes but only {room} are available "
                 "before the FST; in-place patching is not possible.")

    if os.path.abspath(args.source) != os.path.abspath(args.output):
        if not os.path.exists(args.output):
            print(f"copying {os.path.getsize(args.source) / 1024**3:.2f} GB -> {args.output}")
            shutil.copyfile(args.source, args.output)
        else:
            print(f"reusing existing {args.output}")

    out = read_header(args.output)
    if out["dol_off"] != src["dol_off"] or out["fst_off"] != src["fst_off"]:
        sys.exit(f"{args.output}: disc layout differs from the source; delete it and re-run")

    with open(args.output, "r+b") as f:
        f.seek(out["dol_off"])
        f.write(new_dol)
        f.write(b"\0" * (room - len(new_dol)))

    print(f"patched {len(new_dol)} bytes at 0x{out['dol_off']:08x}"
          f", zeroed {room - len(new_dol)} bytes of slack")
    print(f"  DOL sha1 {hashlib.sha1(new_dol).hexdigest()}")
    print(f"\nready: {args.output}")


if __name__ == "__main__":
    main()
