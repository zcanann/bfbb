#!/usr/bin/env python3
"""Derive GC/2.0p1b, the RenderWare SDK's compiler, from GC/2.0p1a.

GC/2.0p1b is GC/2.0p1a (tools/patch_compiler.py) plus ONE change:

    Clause V's literal-kill walk does not fire on a store to a compiler
    temporary.

update_alias_value's whole-object entry (VN dispatch table 0x5bd068, entry 0)
is the hook that 2.0p1a points at its VN stub (clauses V and F, 0x60e5f8).
2.0p1b points that entry at one more stub, which looks at the stored alias's
base expression (alias+0x10) first:

    base != NULL && base->word0 == 0x10005      (a frame object)
                 && *(base+0x18) == 0           (no declared Object: a temp)
        -> jmp 0x511a53   the compiler's own stock whole-object kill
                          (no V walk, so no F either)
    otherwise
        -> jmp 0x60e5f8   2.0p1a's VN stub, unchanged

This is exactly what tools/compilerprobe/cablate.py --mode vtmp does under a
debugger; docs/RW_RESIDUE.md ("What would close the rest", item 1) has the
measurement (+4 RW functions, 0 regressions, game code 0/0). Neither branch
target reads eax or the flags on entry (both start by pushing esi/ebx, or by
`call $+5`), so the stub may use eax freely.

The stub goes into unused space at the end of 2.0p1a's .sbpatch section, so no
section, header, size or relocation changes: the only other edit is the one
dispatch word, which already carries a HIGHLOW base relocation, and the stub's
two branches are rel32 to targets inside the same image, which survive the
sjiswrap rebase. Inputs and outputs are SHA-1-checked; GC/2.0p1 and GC/2.0p1a
are only read, never modified.
"""

import hashlib
import os
import shutil
import struct
import sys
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import patch_compiler

BASE_VERSION = patch_compiler.PATCHED_VERSION      # GC/2.0p1a
RW_VERSION = "GC/2.0p1b"

BASE_SHA1 = patch_compiler.PATCHED_SHA1
RW_SHA1 = "c8e72ea91fd2cf7f992f90de4f2c6a0f5b897d73"

# 2.0p1a layout this patch builds on (all verified before writing).
VN_DISPATCH_OFFSET = patch_compiler.VN_DISPATCH_OFFSET   # table entry 0, file offset
P1A_VN_STUB = 0x0060E5F8           # 2.0p1a's entry-0 target (clauses V, F)
STOCK_KILL = 0x00511A53            # stock AliasType0 whole-object kill

SECTION_VA = patch_compiler.BLOB_VA                # .sbpatch
SECTION_FILE = patch_compiler.SECTION_FILE
SECTION_SIZE = patch_compiler.SECTION_SIZE
VTMP_STUB_VA = SECTION_VA + 0xF00                  # well clear of blob + stubs

FRAME_OBJECT_WORD = 0x00010005


def _rel32(frm_end, to):
    return struct.pack("<i", to - frm_end)


def vtmp_stub(at):
    """ebx = the alias being stored through (update_alias_value's arg 1)."""
    b = bytearray()
    b += b"\x8B\x43\x10"                       # mov eax,[ebx+0x10]   base
    b += b"\x85\xC0"                           # test eax,eax
    b += b"\x74\x12"                          # jz   p1a
    b += b"\x81\x38" + struct.pack("<I", FRAME_OBJECT_WORD)  # cmp dword [eax],0x10005
    b += b"\x75\x0A"                           # jne  p1a
    b += b"\x83\x78\x18\x00"                   # cmp dword [eax+0x18],0
    b += b"\x0F\x84"; b += _rel32(at + len(b) + 4, STOCK_KILL)   # je stock kill
    assert len(b) == 0x19                      # p1a: label
    b += b"\xE9"; b += _rel32(at + len(b) + 4, P1A_VN_STUB)      # p1a: jmp V/F stub
    return bytes(b)


def _apply(data: bytearray) -> bytes:
    stub = vtmp_stub(VTMP_STUB_VA)
    so = SECTION_FILE + (VTMP_STUB_VA - SECTION_VA)
    if so + len(stub) > SECTION_FILE + SECTION_SIZE or len(data) != SECTION_FILE + SECTION_SIZE:
        sys.exit(f"unexpected 2.0p1a layout (file size {len(data):#x})")
    # The stub region must be unused section padding ...
    if any(data[so:so + len(stub)]):
        sys.exit(f"stub region at {so:#x} is not free in {BASE_VERSION}")
    # ... and nothing in the section may live past it (the region is padding,
    # not a hole between pieces of 2.0p1a's code).
    if any(data[so:SECTION_FILE + SECTION_SIZE]):
        sys.exit(f"{BASE_VERSION} uses .sbpatch bytes past {VTMP_STUB_VA:#x}")
    data[so:so + len(stub)] = stub

    found = struct.unpack_from("<I", data, VN_DISPATCH_OFFSET)[0]
    if found != P1A_VN_STUB:
        sys.exit(f"vn dispatch entry 0 is {found:#x}, expected {P1A_VN_STUB:#x}")
    struct.pack_into("<I", data, VN_DISPATCH_OFFSET, VTMP_STUB_VA)
    return bytes(data)


def patch_compiler_rw(compilers: Path) -> bool:
    """Create GC/2.0p1b next to GC/2.0p1a. Returns True if it is present."""
    src_dir = compilers / BASE_VERSION
    dst_dir = compilers / RW_VERSION
    src = src_dir / "mwcceppc.exe"
    dst = dst_dir / "mwcceppc.exe"

    if not src.exists():
        return False

    actual = patch_compiler.sha1(src)
    if actual != BASE_SHA1:
        sys.exit(f"{src} has unexpected SHA-1 {actual}\n"
                 f"  expected {BASE_SHA1}; refusing to patch an unknown build")

    if dst.exists() and patch_compiler.sha1(dst) == RW_SHA1:
        return True

    dst_dir.mkdir(parents=True, exist_ok=True)
    # mwcceppc.exe needs lmgr326b.dll beside it; copy the whole directory, then
    # drop the copied (wrong-version) exe before anything can fail.
    for f in src_dir.iterdir():
        if f.is_file():
            shutil.copy2(f, dst_dir / f.name)
    if dst.exists():
        dst.unlink()

    out = _apply(bytearray(src.read_bytes()))
    result = hashlib.sha1(out).hexdigest()
    if result != RW_SHA1:
        sys.exit(f"derived {RW_VERSION} has SHA-1 {result}, expected {RW_SHA1}")

    tmp = dst.with_suffix(".exe.tmp")
    tmp.write_bytes(out)
    os.replace(tmp, dst)
    print(f"Patched compiler written to {dst}  (sha1 {result})")
    return True


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit("usage: patch_compiler_rw.py <compilers dir | GC/2.0p1b mwcceppc.exe>")
    arg = Path(sys.argv[1])
    # Invoked from ninja with $out, i.e. <compilers>/GC/2.0p1b/mwcceppc.exe
    root = arg.parents[2] if arg.name.endswith(".exe") else arg
    if not patch_compiler_rw(root):
        sys.exit(f"{root / BASE_VERSION} not found")
