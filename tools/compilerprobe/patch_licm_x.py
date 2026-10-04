#!/usr/bin/env python3
"""patch_licm_x.py: EXPERIMENTAL IRO expression-candidate part (research only; not adopted).

    python tools/compilerprobe/patch_licm_x.py <compilers dir> --parts agg --out <dir>

Reads <compilers>/GC/2.0p1f/mwcceppc.exe (SHA-1 checked) and writes a patched copy plus the
directory's other files to <dir>. Refuses to write into any GC/2.0p1* directory. Never
modifies its input.

Parts:

  ss0 / ss  AliasPatch.c's clause C+ (0x60e06c; differing opcodes, both static objects, the
       non-store side at most 4 bytes) also answers "may alias" for two STORES, so two
       stores to different statics with different opcodes are never reordered. Stock 2.0p1
       (and 2.5..2.7) answer 0 for such a pair, and retail reorders it (zMusicNotify /
       zMusicNotifyEvent: `stwx s -> sMusicQueueData[t]` scheduled above an earlier
       `stfsx -> sMusicTimer[t]`). ss0 returns 0 for a store/store pair at entry 0's call
       of C+ (0x60e31a); ss also at clause C's call of it (0x60e12d, entries 1 and 3).
       Measured -3/+1 (ss0): direct-symbol stores (zGameLoop, FindAndInstanceAtomicCallback)
       and two stores into ONE array (zLasso_AddGuide) do need the edge.
  ssi0 / ssi  the same, only when BOTH stores are indirect (pcode flags 0x20: an X-form or
       pointer store whose memref is the whole object) and to DIFFERENT objects.
       Measured against 2.0p1f, all 224 SB + 120 RW units (sweep_units.py): ssi and ssi0 both
       +1 / -0 game (zEntPlayer_SNDInit 99.92 -> 100; zMusicNotifyEvent 86.13 -> 90.16), RW
       0 / 0. With the honest timer-first zMusic source also zMusicNotify 89.84 -> 100.
       On repros/x_ss.c q_noglob/q_ptr/q_rev the build is byte-identical to stock 2.0p1, 2.5,
       2.6 and 2.7, which all answer "no alias" for such a store pair.

  agg  IRO_IsExpressionCandidate (2.0p1 0x457540; asked by IroCSE's expression finder
       0x46b2e0, whose list IroLoop's invariant mover 0x4a6794 hoists from) refuses an
       EINDIRECT whose address is a plain OBJREF of a local with VarInfo noregister == 0:
       it treats that as a register-variable read. It does not look at the object's type,
       so a read of the member at offset 0 of a non-address-taken struct/class/array local
       (`bot.r`, where `bot.g` is indirect(add(&bot,1)) and IS an expression) is never
       hoisted or CSE'd by IRO; the backend LICM hoists it later, so it lands after the
       other members. `agg` makes such a read a candidate when the object's type is
       TYPESTRUCT (4), TYPECLASS (5) or TYPEARRAY (12) -- objects that can never live in a
       register anyway.
       Sites: the 0x20-flag path's `cmp byte [ebx+2],1 / jne` at 0x4575f6 -> jmp AGG2;
       the other path's `call 0x470b30` (IsRegable) at 0x457629 -> call AGG1.
       No released compiler (1.3.2 .. 3.0a5.2) behaves this way: they all hoist the
       offset-0 member last (tools/compilerprobe/repros/x_hoist*.cpp).
"""
import hashlib
import shutil
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
import patch_compiler_rw as P  # noqa

AGG1_VA = P.SECTION_VA + 0xF40
AGG2_VA = P.SECTION_VA + 0xF80
SS_VA = P.SECTION_VA + 0xFC0
CPLUS = 0x60E06C                      # AliasPatch.c clause C+ (a, b, ma, mb), 2.0p1a blob
SS_SITES = {"ss0": (0x60E31A,),       # entry 0's call of C+
            "ss": (0x60E31A, 0x60E12D)}   # + clause C's tail call of C+ (entries 1 and 3)
SS_SITES["ssi0"] = SS_SITES["ss0"]
SS_SITES["ssi"] = SS_SITES["ss"]
ALL = ("agg", "ss0", "ss", "ssi0", "ssi")


def ssi_stub(at):
    """C/C+ never order two INDIRECT (pointer-op, flag 0x20) stores to DIFFERENT objects."""
    c = bytearray()
    jne_go = []
    for arg in (4, 8):
        c += bytes((0x8B, 0x44, 0x24, arg))     # mov eax,[esp+arg]
        c += bytes.fromhex("8B4814")            # mov ecx,[eax+0x14]
        c += bytes.fromhex("83E124")            # and ecx,0x24
        c += bytes.fromhex("83F924")            # cmp ecx,0x24
        c += bytes.fromhex("7500"); jne_go.append(len(c) - 1)
    c += bytes.fromhex("8B44240C")              # mov eax,[esp+0xc]  ma
    c += bytes.fromhex("8B4C2410")              # mov ecx,[esp+0x10] mb
    c += bytes.fromhex("8B4010")                # mov eax,[eax+0x10] ma->object
    c += bytes.fromhex("3B4110")                # cmp eax,[ecx+0x10]
    c += bytes.fromhex("7400"); jne_go.append(len(c) - 1)
    c += bytes.fromhex("31C0C3")                # xor eax,eax ; ret
    go = len(c)
    c += bytes.fromhex("E9") + rel32(at + len(c) + 5, CPLUS)
    for f in jne_go:
        c[f] = go - (f + 1)
    return bytes(c)


def ss_stub(at):
    """C/C+ never order two stores: both pcodes carry the store flag (pcode+0x14 & 4) -> 0."""
    c = bytearray()
    c += bytes.fromhex("8B442404")      # mov eax,[esp+4]   a
    c += bytes.fromhex("F6401404")      # test byte [eax+0x14],4
    c += bytes.fromhex("740D")            # je go
    c += bytes.fromhex("8B442408")      # mov eax,[esp+8]   b
    c += bytes.fromhex("F6401404")      # test byte [eax+0x14],4
    c += bytes.fromhex("7403")            # je go
    c += bytes.fromhex("31C0C3")        # xor eax,eax ; ret
    c += bytes.fromhex("E9") + rel32(at + len(c) + 5, CPLUS)   # go: jmp C+
    return bytes(c)


def rel32(frm_end, to):
    return struct.pack("<i", to - frm_end)


def agg1(at):
    # called with [esp+4] = Object*; returns IsRegable(obj) unless obj is an aggregate
    c = bytearray()
    c += bytes.fromhex("8B4C2404")      # mov ecx,[esp+4]
    c += bytes.fromhex("8B510E")        # mov edx,[ecx+0xe]   obj->type
    c += bytes.fromhex("0FB612")        # movzx edx,byte [edx]
    fix = []
    for t in (4, 5, 12):
        c += bytes((0x83, 0xFA, t))     # cmp edx,t
        c += b"\x74\x00"                # je ret0
        fix.append(len(c) - 1)
    c += b"\xE9" + rel32(at + len(c) + 5, 0x470B30)   # jmp IsRegable
    r0 = len(c)
    c += bytes.fromhex("31C0C3")        # xor eax,eax ; ret
    for f in fix:
        c[f] = r0 - (f + 1)
    return bytes(c)


def agg2(at):
    T, BACK = 0x457610, 0x4575FC
    c = bytearray()
    c += bytes.fromhex("807B0201")      # cmp byte [ebx+2],1
    c += b"\x0F\x85" + rel32(at + len(c) + 6, T)
    c += bytes.fromhex("8B4B0E")        # mov ecx,[ebx+0xe]
    c += bytes.fromhex("0FB609")        # movzx ecx,byte [ecx]
    for t in (4, 5, 12):
        c += bytes((0x83, 0xF9, t))
        c += b"\x0F\x84" + rel32(at + len(c) + 6, T)
    c += b"\xE9" + rel32(at + len(c) + 5, BACK)
    return bytes(c)


def build(data, parts):
    def text(va):
        return va - P.TEXT_FILE_DELTA

    def expect(o, want, what):
        if bytes(data[o:o + len(want)]) != want:
            sys.exit("%s at %#x is %s, expected %s" % (what, o, data[o:o + len(want)].hex(), want.hex()))

    def put(va, code):
        so = P.SECTION_FILE + (va - P.SECTION_VA)
        if any(data[so:so + len(code)]):
            sys.exit("region %#x not free" % va)
        data[so:so + len(code)] = code

    if "agg" in parts:
        o = text(0x4575F6)
        expect(o, bytes.fromhex("807B0201 7514".replace(" ", "")), "agg2 site")
        put(AGG2_VA, agg2(AGG2_VA))
        data[o:o + 6] = b"\xE9" + rel32(0x4575F6 + 5, AGG2_VA) + b"\x90"
        o = text(0x457629)
        expect(o, b"\xE8" + rel32(0x457629 + 5, 0x470B30), "agg1 site")
        put(AGG1_VA, agg1(AGG1_VA))
        data[o:o + 5] = b"\xE8" + rel32(0x457629 + 5, AGG1_VA)
    ss = [p for p in parts if p in SS_SITES]
    if len(ss) > 1:
        sys.exit("ss0, ss, ssi0 and ssi are alternatives")
    if ss:
        put(SS_VA, (ssi_stub if ss[0].startswith("ssi") else ss_stub)(SS_VA))
        for site in SS_SITES[ss[0]]:
            o = P.SECTION_FILE + (site - P.SECTION_VA)
            expect(o, bytes.fromhex("E8") + rel32(site + 5, CPLUS), "C+ call at %#x" % site)
            data[o:o + 5] = bytes.fromhex("E8") + rel32(site + 5, SS_VA)
    return bytes(data)


def main():
    root = Path(sys.argv[1])
    parts = set(sys.argv[sys.argv.index("--parts") + 1].split(","))
    out = Path(sys.argv[sys.argv.index("--out") + 1]).resolve()
    if parts - set(ALL):
        sys.exit("parts must be from " + ",".join(ALL))
    if out.name.startswith("2.0p1") and out.parent.name == "GC":
        sys.exit("refusing to write to %s" % out)
    src = root / "GC" / "2.0p1f"
    raw = (src / "mwcceppc.exe").read_bytes()
    if hashlib.sha1(raw).hexdigest() != P.P1F_SHA1:
        sys.exit("input is not GC/2.0p1f")
    outb = build(bytearray(raw), parts)
    out.mkdir(parents=True, exist_ok=True)
    for f in src.iterdir():
        if f.is_file() and f.name != "mwcceppc.exe":
            shutil.copy2(f, out / f.name)
    (out / "mwcceppc.exe").write_bytes(outb)
    print("%s  sha1 %s" % (out / "mwcceppc.exe", hashlib.sha1(outb).hexdigest()))


if __name__ == "__main__":
    main()
