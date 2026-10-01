#!/usr/bin/env python3
"""Build the single-part / ablated compilers used by docs/COMPILER_VARIANTS.md.

    python tools/compilerprobe/repros/build_parts.py <out-dir>

Writes <out-dir>/<name>/mwcceppc.exe (plus the dlls) for every build below.
It only READS build/compilers/GC/*; nothing there is modified. Point run.py
or sweep_units.py at them with --mw <out-dir>/<name> (run.py takes an absolute
directory; sweep_units.py/solo.py take a path relative to build/compilers).

  c_r3only    2.0p1b + R3 (LICM filter + isloopinvariant hook), veto NOT moved
  c_vetoonly  2.0p1b + large-loop veto moved to 2.5's opcodes, no R3
  s_veto      stock 2.0p1 + the veto move (provenance: == 2.5 on c_veto.c big_sfield)
  s_r4        stock 2.0p1 + R4's one byte (provenance: == 2.5 on d_r4.c)
  a_r4        2.0p1a + R4
  d_r4, d_atsched, d_atw, d_atv   2.0p1c + one 2.0p1d part
  d_gate      2.0p1c + at-sched, at-w, at-v (no R4)
  d_nov       2.0p1c + r4, at-sched, at-w (2.0p1d without at-v)
  d_never     2.0p1d with in_wc forced to 0 (E3n/A/W/V never fire on a frame object)
  d_always    2.0p1d with in_wc forced to 1 (behaves as 2.0p1c + R4)
  never_e3na / never_w / never_v   2.0p1d with only that clause's gate forced to 0
"""
import hashlib
import os
import shutil
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools"))
import patch_compiler_rw as m  # noqa: E402

C = ROOT / "build" / "compilers" / "GC"


def text(va):
    return va - m.TEXT_FILE_DELTA


def mk(out, name, base, fn):
    d = out / name
    d.mkdir(parents=True, exist_ok=True)
    for f in (C / base).iterdir():
        if f.is_file() and f.name != "mwcceppc.exe":
            shutil.copy2(f, d / f.name)
    data = fn(bytearray((C / base / "mwcceppc.exe").read_bytes()))
    (d / "mwcceppc.exe").write_bytes(data)
    print(f"{name:12s} {hashlib.sha1(data).hexdigest()}")


def r3_only(data):
    out = bytearray(m._apply_r3(data))
    o = text(m.FP_VETO_SITE)
    out[o:o + len(m.FP_VETO_OLD)] = m.FP_VETO_OLD
    return bytes(out)


def veto_only(data):
    o = text(m.FP_VETO_SITE)
    assert bytes(data[o:o + len(m.FP_VETO_OLD)]) == m.FP_VETO_OLD
    data[o:o + len(m.FP_VETO_NEW)] = m.FP_VETO_NEW
    return bytes(data)


def r4_on(data):
    o = text(m.R4_SITE)
    assert bytes(data[o:o + 2]) == m.R4_OLD
    data[o:o + 2] = m.R4_NEW
    return bytes(data)


def in_wc_const(value):
    def f(data):
        o = m.SECTION_FILE + (m.IN_WC_VA - m.SECTION_VA)
        assert data[o] == 0xE8
        data[o:o + 6] = (b"\xB8\x01\x00\x00\x00\xC3" if value else b"\x31\xC0\xC3\x90\x90\x90")
        return bytes(data)
    return f


ZERO_VA = m.SECTION_VA + 0x7F8      # free bytes after at_v_stub in 2.0p1d


def never_sites(sites):
    def f(data):
        zo = m.SECTION_FILE + (ZERO_VA - m.SECTION_VA)
        assert not any(data[zo:zo + 3])
        data[zo:zo + 3] = b"\x31\xC0\xC3"
        for va in sites:
            o = m.SECTION_FILE + (va - m.SECTION_VA)
            assert data[o] == 0xE8 and struct.unpack_from("<i", data, o + 1)[0] == m.IN_WC_VA - (va + 5)
            struct.pack_into("<i", data, o + 1, ZERO_VA - (va + 5))
        return bytes(data)
    return f


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    out = Path(sys.argv[1]).resolve()
    if out == C or C in out.parents:
        sys.exit("refusing to write under build/compilers")
    mk(out, "c_r3only", "2.0p1b", r3_only)
    mk(out, "c_vetoonly", "2.0p1b", veto_only)
    mk(out, "s_veto", "2.0p1", veto_only)
    mk(out, "s_r4", "2.0p1", r4_on)
    mk(out, "a_r4", "2.0p1a", r4_on)
    for p in m.P1D_PARTS:
        mk(out, "d_" + p.replace("-", ""), "2.0p1c", lambda d, p=p: m._apply_p1d(d, (p,)))
    mk(out, "d_gate", "2.0p1c", lambda d: m._apply_p1d(d, ("at-sched", "at-w", "at-v")))
    mk(out, "d_nov", "2.0p1c", lambda d: m._apply_p1d(d, ("r4", "at-sched", "at-w")))
    mk(out, "d_never", "2.0p1d", in_wc_const(0))
    mk(out, "d_always", "2.0p1d", in_wc_const(1))
    mk(out, "never_e3na", "2.0p1d", never_sites([m.AT_E3N_VA + 7, m.AT_A_VA + 0x17, m.AT_A_VA + 0x38]))
    mk(out, "never_w", "2.0p1d", never_sites([m.AT_W_VA + 7]))
    mk(out, "never_v", "2.0p1d", never_sites([m.AT_V_VA + 0x12]))


if __name__ == "__main__":
    main()
