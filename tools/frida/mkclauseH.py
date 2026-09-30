#!/usr/bin/env python3
"""Build a clause-H variant compiler from the tree's patched 2.0p1a:
  1. grow .text SizeOfRawData 0x17dc00 -> 0x17e000 (insert 0x400 zero bytes
     at file 0x17e000, bump PointerToRawData of all later sections)
  2. write the clause-H handler at VA 0x57ec00 (file 0x17e000)
  3. replace the 7-byte `jmp [ebx*4+0x5bd074]` at VA 0x511ce5 with
     `jmp 0x57ec00; nop; nop` and neutralize the HIGHLOW reloc for 0x511ce8
usage: mkclauseH.py <out.exe> [--null]   (--null: steps 1 only, no clause)
"""
import hashlib, shutil, struct, sys
from pathlib import Path

SRC = Path(r"C:\Users\joeyj\Documents\Git\bfbb_\.claude\worktrees\agent-ab229575576d6fcfb\build\compilers\GC\2.0p1a\mwcceppc.exe")
out = Path(sys.argv[1])
null_only = "--null" in sys.argv

data = bytearray(SRC.read_bytes())
assert hashlib.sha1(data).hexdigest() == "5965be762f2584ca4f6eac3d100abeae2251d214"

pe = struct.unpack_from("<I", data, 0x3c)[0]
nsec = struct.unpack_from("<H", data, pe + 6)[0]
optsz = struct.unpack_from("<H", data, pe + 20)[0]
sec0 = pe + 24 + optsz

INSERT_AT = 0x17E000          # end of current .text raw data
GROW = 0x400

# 1a. section table: .text raw size += GROW; later sections raw ptr += GROW
for i in range(nsec):
    o = sec0 + 40 * i
    name = bytes(data[o:o+8]).rstrip(b"\0").decode()
    rsz, rptr = struct.unpack_from("<II", data, o + 16)
    if name == ".text":
        assert rsz == 0x17DC00, hex(rsz)
        struct.pack_into("<I", data, o + 16, rsz + GROW)
    elif rptr >= INSERT_AT and rptr != 0:
        struct.pack_into("<I", data, o + 20, rptr + GROW)

# 1b. insert the zero page
data[INSERT_AT:INSERT_AT] = bytes(GROW)

if not null_only:
    CAVE2_VA = 0x57EC00
    tail_jmp = 0x511E05           # answer tail of 0x511cb0 (mov al,bl ...)
    table_va = 0x5BD074

    code = bytearray()
    code += bytes.fromhex("8b4a10")        # mov ecx,[edx+0x10]
    code += bytes.fromhex("85c9")          # test ecx,ecx
    code += bytes.fromhex("7421")          # je disp
    code += bytes.fromhex("833905")        # cmp dword [ecx],5
    code += bytes.fromhex("751c")          # jne disp
    code += bytes.fromhex("8b4810")        # mov ecx,[eax+0x10]
    code += bytes.fromhex("85c9")          # test ecx,ecx
    code += bytes.fromhex("7415")          # je disp
    code += bytes.fromhex("833905")        # cmp dword [ecx],5
    code += bytes.fromhex("7510")          # jne disp
    code += bytes.fromhex("83781804")      # cmp dword [eax+0x18],4
    code += bytes.fromhex("770a")          # ja disp
    code += bytes.fromhex("bb01000000")    # mov ebx,1
    rel = tail_jmp - (CAVE2_VA + len(code) + 5)
    code += b"\xe9" + struct.pack("<i", rel)          # jmp 0x511e05
    disp_off = len(code)
    code += bytes.fromhex("e800000000")    # call $+5
    next_va = CAVE2_VA + len(code)
    code += b"\x59"                        # pop ecx
    code += b"\x81\xc1" + struct.pack("<i", table_va - next_va)  # add ecx,imm
    code += bytes.fromhex("ff2499")        # jmp [ecx+ebx*4]
    # sanity: the short-jump targets must equal disp_off
    assert disp_off == 40, disp_off
    assert len(code) == 55, len(code)

    fo = CAVE2_VA - 0x400C00 + GROW  # after insertion .text file mapping unchanged (insert at end)
    # careful: cave2 VA 0x57ec00 -> file offset = 0x17e000 (the inserted page start)
    fo = INSERT_AT
    assert all(b == 0 for b in data[fo:fo+len(code)])
    data[fo:fo+len(code)] = code

    # 3a. patch the dispatch jmp at VA 0x511ce5 (file 0x1110e5)
    fo = 0x511CE5 - 0x400C00
    assert data[fo:fo+7] == bytes.fromhex("ff249d74d05b00"), data[fo:fo+7].hex()
    rel = CAVE2_VA - (0x511CE5 + 5)
    data[fo:fo+7] = b"\xe9" + struct.pack("<i", rel) + b"\x90\x90"

    # 3b. neutralize the HIGHLOW reloc for RVA 0x111ce8
    #     (file offset 0x1e6a98 pre-insert; .reloc raw ptr was 0x1d5a00 > insert point)
    ro = 0x1E6A98 + GROW
    w = struct.unpack_from("<H", data, ro)[0]
    assert w == 0x3CE8, hex(w)
    struct.pack_into("<H", data, ro, 0x0CE8)  # type ABSOLUTE (padding no-op)

out.parent.mkdir(parents=True, exist_ok=True)
out.write_bytes(bytes(data))
for f in SRC.parent.iterdir():
    if f.name != "mwcceppc.exe" and f.is_file():
        t = out.parent / f.name
        if not t.exists():
            shutil.copy2(f, t)
print("wrote", out, len(data), hashlib.sha1(data).hexdigest())
