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

GC/2.0p1c (EXPERIMENTAL; not used by configure.py yet)
------------------------------------------------------
GC/2.0p1b plus GC/2.5's loop-invariant code motion of loads through a
pointer-to-const ("R3" in docs/RW_RESIDUE.md). Built only when asked for by
path (`patch_compiler_rw.py <compilers>/GC/2.0p1c/mwcceppc.exe`) or with
`patch_compiler_rw.py <compilers dir> --r3`.

What 2.5 does differently (measured under a debugger on both binaries):

  * Both compilers' CodeGen give the `mr vN, var` that reads a register
    variable whose type is pointer-to-const (or restrict) an alias for a fresh
    pseudo-object (2.0p1: 0x513330, 2.5: 0x513620), and both compilers' pointer
    analysis hands that alias on to loads whose base register comes from it.
  * 2.0p1 makes it a one-member alias SET {pseudo-object}, so the load keeps
    fIsPtrOp (0x20) and alias kind 2, and moveinvariantsfromloop rejects it
    twice: its flag filter (`flags & 0x1a8`) and isloopinvariant's
    `alias->kind == 2 -> not invariant`.
  * 2.5 makes it a plain alias (kind 1) on the pseudo-object, clears 0x20,
    and only propagates it through plain register copies (its def-alias helper
    0x512ec0 returns an alias only for `mr` or an object-address operand; 2.0p1
    also lets it flow through `add`). isloopinvariant then walks the
    pseudo-object's defs, and there are none -- calls and stores through other
    pointers never define it. So the load is invariant whatever else the loop
    does (calls, stores, early returns).
  * 2.5 also moved moveinvariantsfromloop's large-loop veto (a loop of more
    than 25 instructions) from LFS..LFDUX (opcodes 0x8e-0x95) to
    VSPLTIS{B,H,W} (0x161-0x163), so FP loads are hoisted out of big loops too.
    Without that, the const-matrix loads of VectorMultPoint/VectorMultVector
    stay in the loop.

What 2.0p1c changes (all of it is new code at .sbpatch+0xC00, plus three
rel32/immediate edits, none of which overlaps a base relocation):

  1. moveinvariantsfromloop's flag filter (`and eax,0x1a8 / jne` at 0x56f461)
     -> R3_FILTER: still skip 0x188 (volatile / side effects / 0x8), but let
     fIsPtrOp through when the load is a "cps" load (below).
  2. Its `call isloopinvariant` (0x56f472, 2.0p1a's LICM stub target) ->
     R3_INV: a cps load has its alias temporarily swapped for the set's one
     member and is handed to the stock isloopinvariant (0x570f60), which then
     walks the pseudo-object's defs exactly as 2.5 does; if the pseudo-object
     has no use-def node at all it enters isloopinvariant after the memory
     checks (0x5710d0). Every other pcode goes to 2.0p1a's stub as before.
  3. The FP-load veto immediates at 0x570985 (`sub eax,5 / cmp eax,7`) ->
     (`sub eax,0xd8 / cmp eax,2`): 2.5's opcode range.

A "cps" load: a read, not a write; alias is a one-member set, not worst_case,
whose member is a whole-object alias of an object typed 0x5bd008 (the
pseudo-object type); and every register it uses is defined only by `mr`
instructions that carry such a set (or not at all -- an incoming register).
The last condition is 2.5's "propagate through mr only" rule.

Measured (scratch derivation, solo.py on every unit; RW baseline 2.0p1b, game
baseline 2.0p1a/2.0p1b, which are identical on the game):
  RW:   +6 / -0 among RW_COMPILER units: _rpMaterialListFindMaterialIndex,
        _rpMaterialListStreamGetSize, _rpMaterialListStreamWrite, RwImageCopy,
        RpGeometryStreamGetSize, and multiTexGcnData's
        GameCubeMTEffectStreamWrite (that unit is 7/7, so its GC/2.5 override
        can go). Up without crossing: RpGeometryStreamWrite 94.41 -> 99.59,
        ImageConvertDepth 92.31 -> 99.82, RwImageApplyMask 96.49 -> 99.09,
        UserDataListCopy 96.51 -> 98.52 (the GC/2.5 numbers). The only
        function that drops is in stdkey, which is built with GC/2.0p1;
        RpHAnimKeyFrameStreamWrite goes 100 -> 94.26, as it does under
        GC/2.5.
  Game: +2 / -0 (xfont::irender, zFX validate_popper);
        xParCmdAnimalMagentism_Update 81.04 -> 85.85.
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


# ---- GC/2.0p1c: R3, 2.5-style LICM through pointer-to-const -----------------
# See the module docstring. Everything below is checked against 2.0p1b's bytes
# before it is written.

R3_VERSION = "GC/2.0p1c"
R3_SHA1 = "0a0662b29a66e4e42310ec45b9b9a9a0269f1350"

TEXT_FILE_DELTA = 0x00400C00       # .text: file offset = VA - this
R3_BLOB_VA = SECTION_VA + 0xC00    # free .sbpatch padding (2.0p1a ends ~+0x652, vtmp is +0xF00)

# 2.0p1 addresses the R3 code reaches (CodeMotion.c / Alias.c / UseDefChains.c)
R3_FILTER_SITE = 0x0056F461        # moveinvariantsfromloop: and eax,0x1a8 ; jne skip
R3_FILTER_OLD = bytes.fromhex("25a80100007526")
R3_FILTER_CONT = 0x0056F468        # -> push 0 ... call isloopinvariant
R3_FILTER_SKIP = 0x0056F48E        # -> not a candidate
R3_CALL_SITE = 0x0056F472          # call isloopinvariant (2.0p1a: -> its LICM stub)
P1A_LICM_STUB = 0x0060E63B         # 2.0p1a's sb_licm_invariant stub
ISLOOPINVARIANT = 0x00570F60
ISLOOPINVARIANT_MEMDONE = 0x005710D0   # isloopinvariant past its memory-operand checks
FINDOBJECTUSEDEF = 0x0056F1D0
WORST_CASE_ALIAS = 0x005E9CB4      # Alias.c: worst_case (global holding the Alias*)
PSEUDO_OBJECT_TYPE = 0x005BD008    # type of the const/restrict pointee pseudo-objects
PSEUDO_MAKER_TYPE_IMM = 0x0051334D # `mov dword [ebx+0xe], PSEUDO_OBJECT_TYPE` in 0x513330
REG_DEFLISTS = 0x005E9A60          # per register class: reg -> list of def ids
DEFS = 0x005E966C                  # Defs[] (10-byte entries, PCode* first)
FP_VETO_SITE = 0x00570985          # large-loop veto: op-0x89 ; sub eax,5 ; cmp eax,7 ; jbe ; jmp
FP_VETO_OLD = bytes.fromhex("2d0500000083f8077635eb3f")
FP_VETO_NEW = bytes.fromhex("2dd800000083f8027635eb3f")   # 2.5: 0x161-0x163 instead

# Assembled from this source (Intel syntax). E8/E9 rel32 to compiler code and
# the four PC-relative displacements (0x11111111 worst_case, 0x22222222 pseudo
# type, 0x33333333 REG_DEFLISTS, 0x44444444 DEFS) are filled in by r3_blob().
#
#   is_cps_alias:            ; (Alias *a) -> the member Alias* if `a` is {pseudo}, else 0
#     mov eax,[esp+4] ; test eax,eax ; jz no
#     call $+5 ; pop edx                         ; edx = VA of the pop
#     cmp eax,[edx+worst_case] ; je no
#     cmp byte [eax+0x2c],2 ; jne no             ; a set
#     mov eax,[eax+0xc] ; test eax,eax ; jz no   ; first member node
#     cmp dword [eax],0 ; jne no                 ; exactly one member
#     mov eax,[eax+0xc] ; cmp byte [eax+0x2c],0 ; jne no   ; whole-object alias
#     mov ecx,[eax+0x10] ; lea edx,[edx+PSEUDO_OBJECT_TYPE] ; cmp [ecx+0xe],edx ; jne no
#     ret
#   no: xor eax,eax ; ret
#   is_cps:                  ; (PCode *p) -> member Alias* if p is a cps load, else 0
#     push ebx/esi/edi/ebp ; mov ebx,[esp+0x14]
#     test byte [ebx+0x14],4 ; jnz fail          ; not a write
#     push [ebx+0x18] ; call is_cps_alias ; pop ecx ; test eax,eax ; jz fail
#     mov edi,eax ; call $+5 ; pop ebp
#     movsx esi,word [ebx+0x22] ; add ebx,0x24   ; operands
#   oploop: sub esi,1 ; jb done
#     cmp byte [ebx],0 ; jne next ; test byte [ebx+2],1 ; jz next   ; a used register
#     movzx eax,byte [ebx+1] ; mov eax,[ebp+eax*4+REG_DEFLISTS]
#     movsx ecx,word [ebx+4] ; mov ecx,[eax+ecx*4]
#   dloop: test ecx,ecx ; jz next
#     mov eax,[ecx+4] ; lea eax,[eax+eax*4] ; mov edx,[ebp+DEFS] ; mov eax,[edx+eax*2]
#     cmp word [eax+0x20],0x8b ; jne fail        ; every def is an `mr` ...
#     push ecx ; push [eax+0x18] ; call is_cps_alias ; pop ecx ; pop ecx
#     test eax,eax ; jz fail                     ; ... carrying a pseudo set
#     mov ecx,[ecx] ; jmp dloop
#   next: add ebx,12 ; jmp oploop
#   done: mov eax,edi ; jmp out
#   fail: xor eax,eax
#   out: pop ebp/edi/esi/ebx ; ret
#   filter:                  ; eax = flags, ebx = pcode (replaces and eax,0x1a8 ; jne)
#     test eax,0x188 ; jnz skip ; test al,0x20 ; jz cont
#     push ebx ; call is_cps ; pop ecx ; test eax,eax ; jz skip
#   cont: jmp R3_FILTER_CONT
#   skip: jmp R3_FILTER_SKIP
#   inv:                     ; isloopinvariant(pcode, loop, vec, f1, f2), cdecl
#     push [esp+4] ; call is_cps ; pop ecx ; test eax,eax ; jz p1a
#     push eax ; push [eax+0x10] ; call FINDOBJECTUSEDEF ; pop ecx
#     test eax,eax ; pop eax ; jz nodefs
#     mov ecx,[esp+4] ; push [ecx+0x18] ; mov [ecx+0x18],eax   ; swap in the member
#     push [esp+0x18] x5 ; call ISLOOPINVARIANT ; add esp,0x14
#     pop edx ; mov ecx,[esp+4] ; mov [ecx+0x18],edx ; ret     ; restore
#   nodefs:                  ; no use-def node: re-enter past the memory checks
#     push ebx/esi/edi/ebp ; sub esp,0x10 ; mov ebp,[esp+0x28] ; mov esi,[esp+0x2c]
#     jmp ISLOOPINVARIANT_MEMDONE
#   p1a: jmp P1A_LICM_STUB
R3_BLOB = bytes.fromhex(
    "8b44240485c07438e8000000005a3b8211111111742a80782c0275248b400c85c0741d83380075188b400c80782c00750f"
    "8b48108d922222222239510e7501c331c0c3535657558b5c2414f64314047575ff7318e8a7ffffff5985c0746889c7e8"
    "000000005d0fbf732283c32483ee017250803b007546f643020174400fb643018b8485333333330fbf4b048b0c8885c974"
    "2a8b41048d04808b95444444448b0442668178208b00751c51ff7018e84dffffff595985c0740d8b09ebd283c30cebab89"
    "f8eb0231c05d5f5e5bc3a9880100007514a820740b53e865ffffff5985c07405e900000000e900000000ff742404e84dff"
    "ffff5985c0745250ff7010e8000000005985c058742f8b4c2404ff7118894118ff742418ff742418ff742418ff742418ff"
    "742418e80000000083c4145a8b4c2404895118c35356575583ec108b6c24288b74242ce900000000e900000000")
R3_FILTER_ENTRY = 0xCD
R3_INV_ENTRY = 0xED


def r3_blob(at):
    """The R3 code placed at VA `at`; returns (bytes, filter_va, inv_va)."""
    b = bytearray(R3_BLOB)
    if len(b) != 0x152:
        sys.exit("R3 blob has the wrong length")

    def disp(off, pc, target, placeholder):
        if struct.unpack_from("<I", b, off)[0] != placeholder:
            sys.exit(f"R3 blob: no placeholder at {off:#x}")
        struct.pack_into("<i", b, off, target - (at + pc))

    def branch(off, op, target):
        if b[off] != op or any(b[off + 1:off + 5]):
            sys.exit(f"R3 blob: no rel32 slot at {off:#x}")
        b[off + 1:off + 5] = _rel32(at + off + 5, target)

    disp(0x10, 0x0D, WORST_CASE_ALIAS, 0x11111111)
    disp(0x36, 0x0D, PSEUDO_OBJECT_TYPE, 0x22222222)
    disp(0x84, 0x65, REG_DEFLISTS, 0x33333333)
    disp(0x9B, 0x65, DEFS, 0x44444444)
    branch(0xE3, 0xE9, R3_FILTER_CONT)
    branch(0xE8, 0xE9, R3_FILTER_SKIP)
    branch(0xFF, 0xE8, FINDOBJECTUSEDEF)
    branch(0x128, 0xE8, ISLOOPINVARIANT)
    branch(0x148, 0xE9, ISLOOPINVARIANT_MEMDONE)
    branch(0x14D, 0xE9, P1A_LICM_STUB)
    return bytes(b), at + R3_FILTER_ENTRY, at + R3_INV_ENTRY


def _apply_r3(data: bytearray) -> bytes:
    def text(va):
        return va - TEXT_FILE_DELTA

    def expect(off, want, what):
        if bytes(data[off:off + len(want)]) != want:
            sys.exit(f"{what} at {off:#x} is {bytes(data[off:off + len(want)]).hex()}, "
                     f"expected {want.hex()}")

    if len(data) != SECTION_FILE + SECTION_SIZE:
        sys.exit(f"unexpected {RW_VERSION} layout (file size {len(data):#x})")
    # Sanity: the addresses this patch assumes are 2.0p1's.
    expect(text(PSEUDO_MAKER_TYPE_IMM), b"\xC7\x43\x0E" + struct.pack("<I", PSEUDO_OBJECT_TYPE),
           "pseudo-object type store")

    blob, filter_va, inv_va = r3_blob(R3_BLOB_VA)
    so = SECTION_FILE + (R3_BLOB_VA - SECTION_VA)
    if any(data[so:so + len(blob)]) or R3_BLOB_VA + len(blob) > SECTION_VA + 0xF00:
        sys.exit(f"R3 region at {so:#x} is not free in {RW_VERSION}")
    data[so:so + len(blob)] = blob

    # 1. the flag filter -> R3 filter (5-byte jmp + 2 nops over and/jne)
    o = text(R3_FILTER_SITE)
    expect(o, R3_FILTER_OLD, "LICM flag filter")
    data[o:o + 7] = b"\xE9" + _rel32(R3_FILTER_SITE + 5, filter_va) + b"\x90\x90"

    # 2. call isloopinvariant (via 2.0p1a's stub) -> R3 inv
    o = text(R3_CALL_SITE)
    expect(o, b"\xE8" + _rel32(R3_CALL_SITE + 5, P1A_LICM_STUB), "LICM call")
    data[o + 1:o + 5] = _rel32(R3_CALL_SITE + 5, inv_va)

    # 3. 2.5's large-loop veto opcode range
    o = text(FP_VETO_SITE)
    expect(o, FP_VETO_OLD, "large-loop veto")
    data[o:o + len(FP_VETO_NEW)] = FP_VETO_NEW
    return bytes(data)


def patch_compiler_r3(compilers: Path) -> bool:
    """Create GC/2.0p1c next to GC/2.0p1b (deriving 2.0p1b first if needed)."""
    if not patch_compiler_rw(compilers):
        return False
    src_dir = compilers / RW_VERSION
    dst_dir = compilers / R3_VERSION
    src = src_dir / "mwcceppc.exe"
    dst = dst_dir / "mwcceppc.exe"

    actual = patch_compiler.sha1(src)
    if actual != RW_SHA1:
        sys.exit(f"{src} has unexpected SHA-1 {actual}\n"
                 f"  expected {RW_SHA1}; refusing to patch an unknown build")
    if dst.exists() and patch_compiler.sha1(dst) == R3_SHA1:
        return True

    dst_dir.mkdir(parents=True, exist_ok=True)
    for f in src_dir.iterdir():
        if f.is_file():
            shutil.copy2(f, dst_dir / f.name)
    if dst.exists():
        dst.unlink()

    out = _apply_r3(bytearray(src.read_bytes()))
    result = hashlib.sha1(out).hexdigest()
    if result != R3_SHA1:
        sys.exit(f"derived {R3_VERSION} has SHA-1 {result}, expected {R3_SHA1}")

    tmp = dst.with_suffix(".exe.tmp")
    tmp.write_bytes(out)
    os.replace(tmp, dst)
    print(f"Patched compiler written to {dst}  (sha1 {result})")
    return True


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit("usage: patch_compiler_rw.py <compilers dir | GC/2.0p1b or GC/2.0p1c "
                 "mwcceppc.exe> [--r3]")
    arg = Path(sys.argv[1])
    # Invoked from ninja with $out, i.e. <compilers>/GC/2.0p1b/mwcceppc.exe
    # (or .../GC/2.0p1c/mwcceppc.exe for the experimental R3 compiler)
    root = arg.parents[2] if arg.name.endswith(".exe") else arg
    want_r3 = "--r3" in sys.argv[2:] or (arg.name.endswith(".exe")
                                          and arg.parent.name == R3_VERSION.split("/")[1])
    ok = patch_compiler_r3(root) if want_r3 else patch_compiler_rw(root)
    if not ok:
        sys.exit(f"{root / BASE_VERSION} not found")
