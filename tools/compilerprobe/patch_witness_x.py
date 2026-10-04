#!/usr/bin/env python3
"""patch_witness_x.py: EXPERIMENTAL parts motivated by source-hack witnesses (research only).

    python tools/compilerprobe/patch_witness_x.py <compilers dir> --parts nap --out <dir>

Reads <compilers>/GC/2.0p1e/mwcceppc.exe (SHA-1 checked), writes a patched copy plus the
directory's other files to <dir>. Refuses to write into GC/2.0p1..2.0p1e. Never modifies its input.

Parts:

  nap  IRO copy propagation (IroPropagate.c IsPropagatable, 0x4709f0) refuses a variable-to-
       variable copy `y = x` when the destination y is a local whose VarInfo marks it as not
       register-allocatable (vi+0x22 != 0, i.e. its address is taken). Stock 2.0p1 rejects only
       the opposite direction (regable y <- non-regable x). The read of y then stays a load of
       y's home, and the backend's store->load forwarding supplies it (with `frsp` for F32,
       since stfs rounds). GC/3.0a3 and 3.0a5.2 behave this way on every repro tried
       (fw_yaw.cpp / fw2.c); 1.3.2 .. 2.7 do not.
       Patch: one byte, the `jne` at 0x470aec (`75 05` -> `75 28`) goes to the `return 0` tail
       at 0x470b16 instead of the "destination not regable -> propagate" exit.
"""
import hashlib
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
import patch_compiler_rw as P  # noqa

ALL = ("nap",)
# (va, old bytes, new bytes)
BYTES = {
    "nap": [(0x00470AEC, bytes.fromhex("7505"), bytes.fromhex("7528"))],
}


def build(data, parts):
    for part in sorted(parts):
        for va, old, new in BYTES[part]:
            o = va - P.TEXT_FILE_DELTA
            if bytes(data[o:o + len(old)]) != old:
                sys.exit("%s: bytes at %#x are %s, expected %s" % (part, va, data[o:o + len(old)].hex(), old.hex()))
            data[o:o + len(new)] = new
    return bytes(data)


def main():
    root = Path(sys.argv[1])
    parts = set(sys.argv[sys.argv.index("--parts") + 1].split(","))
    out = Path(sys.argv[sys.argv.index("--out") + 1]).resolve()
    if parts - set(ALL):
        sys.exit("parts must be from " + ",".join(ALL))
    real = Path(__file__).resolve().parents[2] / "build" / "compilers"
    for v in ("2.0p1", "2.0p1a", "2.0p1b", "2.0p1c", "2.0p1d", "2.0p1e"):
        if out == (root / "GC" / v).resolve() or out == (real / "GC" / v).resolve():
            sys.exit("refusing to write to %s" % out)
    src = root / "GC" / "2.0p1e"
    raw = (src / "mwcceppc.exe").read_bytes()
    if hashlib.sha1(raw).hexdigest() != P.P1E_SHA1:
        sys.exit("input is not GC/2.0p1e")
    res = build(bytearray(raw), parts)
    out.mkdir(parents=True, exist_ok=True)
    for f in src.iterdir():
        if f.is_file() and f.name != "mwcceppc.exe":
            shutil.copy2(f, out / f.name)
    (out / "mwcceppc.exe").write_bytes(res)
    print("wrote %s (%s) sha1 %s" % (out / "mwcceppc.exe", ",".join(sorted(parts)), hashlib.sha1(res).hexdigest()))


if __name__ == "__main__":
    main()
