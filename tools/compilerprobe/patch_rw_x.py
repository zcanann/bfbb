#!/usr/bin/env python3
"""patch_rw_x.py: EXPERIMENTAL alias parts on top of GC/2.0p1e (research only; never configure.py).

    python tools/compilerprobe/patch_rw_x.py <compilers dir> --parts psd,vnd,l8s,l8v,dsf,wg4 --out <dir>

Reads <compilers>/GC/2.0p1e/mwcceppc.exe (SHA-1 checked) and writes a patched copy (plus the
directory's other files) to <dir>. Refuses to write into GC/2.0p1..2.0p1e. Never modifies its
input. Needs keystone-engine (pip install keystone-engine).

Parts (all keyed on compiler-level properties: the PCode's direct-object operand (operand-2
kind 3, "D"), fIsPtrOp (0x20), the alias object's storage word (5 static / 0x10005 frame),
the literal-pool name ('@'), the address-taken set (worst_case membership, in_wc) and sizes):

  psd  scheduler may_alias, entries 0/1/3/4: a direct-operand access to a named static (not a
       literal, not a const-pointee pseudo-object) against a fIsPtrOp access whose alias names
       a named static, one of the two a store -> may alias.
  vnd  value numbering (update_alias_value 0x511a30, after the stock/clause work): a direct,
       non-pointer store to a named static also kills the value of every WHOLE alias of another
       named static.
  l8s  scheduler entry 0: clause C+ also admits an 8-byte literal load (the lfd int->float
       constant) against a store to a static.
  l8v  value numbering: a whole store to a static, or to an address-taken declared frame object
       (clause V's own trigger), also kills 8-byte literals.
  dsf  scheduler: a direct-operand store to a named static is ordered before a later store to
       an address-taken declared frame object.
  wg4  scheduler: clause W generalised: a store to an address-taken declared frame object is
       ordered after an earlier load of a static (named or literal) of at most 4 bytes.
"""
import hashlib, shutil, struct, sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
import patch_compiler_rw as P  # noqa
import keystone  # noqa

ALL = ("psd", "vnd", "l8s", "l8v", "dsf", "wg4", "ds", "e3nc", "we")
SEC = P.SECTION_VA
VN_FN = 0x00511A30
VN_OLD = bytes.fromhex("5356575583ec10")        # push ebx/esi/edi/ebp ; sub esp,0x10
KILL = 0x0050A2C0
LIST_HEAD = 0x005E1FD8
PSEUDO = P.PSEUDO_OBJECT_TYPE
IN_WC = P.IN_WC_VA
YES = P.MAY_ALIAS_YES
REGIONS = [(0xA70, 0xC00), (0xD54, 0xF00), (0x7F8, 0x900), (0xF20, 0x1000), (0x654, 0x700)]
KS = keystone.Ks(keystone.KS_ARCH_X86, keystone.KS_MODE_32)
SYMS = {}


def _resolver(name, value):
    n = name.decode() if isinstance(name, bytes) else name
    if n in SYMS:
        value[0] = SYMS[n]
        return True
    return False




def asm(text, at):
    enc, _ = KS.asm(text, at)
    return bytes(enc)


HELPERS = r"""
f_isstatic:
    mov eax, dword ptr [esp+4]
    test eax, eax
    jz hs_no
    cmp byte ptr [eax+0x2c], 2
    je hs_no
    mov eax, dword ptr [eax+0x10]
    test eax, eax
    jz hs_no
    cmp dword ptr [eax], 5
    jne hs_no
    call hs_pc
hs_pc:
    pop ecx
    sub ecx, hs_pc
    add ecx, PSEUDO
    cmp dword ptr [eax+0xe], ecx
    je hs_no
    mov eax, 1
    ret
hs_no:
    xor eax, eax
    ret
f_namech:
    mov eax, dword ptr [esp+4]
    mov eax, dword ptr [eax+0x10]
    mov eax, dword ptr [eax+0xa]
    test eax, eax
    jz hn_none
    movzx eax, byte ptr [eax+0xa]
    ret
hn_none:
    xor eax, eax
    ret
f_nstatic:
    push dword ptr [esp+4]
    call f_isstatic
    pop ecx
    test eax, eax
    jz hns_no
    push dword ptr [esp+4]
    call f_namech
    pop ecx
    cmp eax, 0x40
    je hns_no
    mov eax, 1
    ret
hns_no:
    xor eax, eax
    ret
f_islit:
    push dword ptr [esp+4]
    call f_isstatic
    pop ecx
    test eax, eax
    jz hl_no
    push dword ptr [esp+4]
    call f_namech
    pop ecx
    cmp eax, 0x40
    jne hl_no
    mov eax, 1
    ret
hl_no:
    xor eax, eax
    ret
f_framedecl_esc:
    mov eax, dword ptr [esp+4]
    test eax, eax
    jz hf_no
    cmp byte ptr [eax+0x2c], 2
    je hf_no
    mov eax, dword ptr [eax+0x10]
    test eax, eax
    jz hf_no
    cmp dword ptr [eax], 0x10005
    jne hf_no
    cmp dword ptr [eax+0x18], 0
    je hf_no
    push eax
    call IN_WC
    pop ecx
    ret
hf_no:
    xor eax, eax
    ret
f_isstore:
    mov eax, dword ptr [esp+4]
    movzx eax, word ptr [eax+0x20]
    lea ecx, [eax-0x28]
    cmp ecx, 0xe
    jbe hst_yes
    lea ecx, [eax-0x96]
    cmp ecx, 0x7
    jbe hst_yes
    xor eax, eax
    ret
hst_yes:
    mov eax, 1
    ret
"""

PSD = r"""
f_psd_ok:
    push esi
    push edi
    mov esi, dword ptr [esp+0xc]
    mov edi, dword ptr [esp+0x10]
    cmp byte ptr [esi+0x3c], 3
    jne po_no
    test dword ptr [esi+0x14], 0x20
    jnz po_no
    test dword ptr [edi+0x14], 0x20
    jz po_no
    push dword ptr [esi+0x18]
    call f_nstatic
    pop ecx
    test eax, eax
    jz po_no
    push dword ptr [edi+0x18]
    call f_nstatic
    pop ecx
    test eax, eax
    jz po_no
    mov eax, 1
    jmp po_out
po_no:
    xor eax, eax
po_out:
    pop edi
    pop esi
    ret
f_psd:
    push esi
    push edi
    mov esi, dword ptr [esp+0xc]
    mov edi, dword ptr [esp+0x10]
    push esi
    call f_isstore
    pop ecx
    test eax, eax
    jnz ps_st
    push edi
    call f_isstore
    pop ecx
    test eax, eax
    jz ps_out
ps_st:
    push edi
    push esi
    call f_psd_ok
    add esp, 8
    test eax, eax
    jnz ps_out
    push esi
    push edi
    call f_psd_ok
    add esp, 8
ps_out:
    pop edi
    pop esi
    ret
"""

DSF = r"""
f_dsf:
    push esi
    push edi
    mov esi, dword ptr [esp+0xc]
    mov edi, dword ptr [esp+0x10]
    cmp byte ptr [esi+0x3c], 3
    jne ds_no
    push esi
    call f_isstore
    pop ecx
    test eax, eax
    jz ds_no
    push edi
    call f_isstore
    pop ecx
    test eax, eax
    jz ds_no
    push dword ptr [esi+0x18]
    call f_nstatic
    pop ecx
    test eax, eax
    jz ds_no
    push dword ptr [edi+0x18]
    call f_framedecl_esc
    pop ecx
    jmp ds_out
ds_no:
    xor eax, eax
ds_out:
    pop edi
    pop esi
    ret
"""

WG4 = r"""
f_wg4:
    push esi
    push edi
    mov esi, dword ptr [esp+0xc]
    mov edi, dword ptr [esp+0x10]
    push esi
    call f_isstore
    pop ecx
    test eax, eax
    jnz wg_no
    push edi
    call f_isstore
    pop ecx
    test eax, eax
    jz wg_no
    push dword ptr [esi+0x18]
    call f_isstatic
    pop ecx
    test eax, eax
    jz wg_no
    mov eax, dword ptr [esi+0x18]
    cmp dword ptr [eax+0x18], 4
    ja wg_no
    push dword ptr [edi+0x18]
    call f_framedecl_esc
    pop ecx
    jmp wg_out
wg_no:
    xor eax, eax
wg_out:
    pop edi
    pop esi
    ret
"""

L8S = r"""
f_l8ok:
    push esi
    push edi
    mov esi, dword ptr [esp+0xc]
    mov edi, dword ptr [esp+0x10]
    push esi
    call f_isstore
    pop ecx
    test eax, eax
    jz l8_no
    push edi
    call f_isstore
    pop ecx
    test eax, eax
    jnz l8_no
    mov ax, word ptr [esi+0x20]
    cmp ax, word ptr [edi+0x20]
    je l8_no
    test dword ptr [esi+0x14], 0xffffff59
    jnz l8_no
    test dword ptr [edi+0x14], 0xffffff59
    jnz l8_no
    mov eax, dword ptr [edi+0x18]
    cmp dword ptr [eax+0x18], 8
    jne l8_no
    push eax
    call f_islit
    pop ecx
    test eax, eax
    jz l8_no
    push dword ptr [esi+0x18]
    call f_isstatic
    pop ecx
    jmp l8_out
l8_no:
    xor eax, eax
l8_out:
    pop edi
    pop esi
    ret
f_l8s:
    push esi
    push edi
    mov esi, dword ptr [esp+0xc]
    mov edi, dword ptr [esp+0x10]
    mov eax, dword ptr [esi+0x18]
    test eax, eax
    jz l8s_no
    cmp byte ptr [eax+0x2c], 0
    jne l8s_no
    mov eax, dword ptr [edi+0x18]
    test eax, eax
    jz l8s_no
    cmp byte ptr [eax+0x2c], 0
    jne l8s_no
    push edi
    push esi
    call f_l8ok
    add esp, 8
    test eax, eax
    jnz l8s_out
    push esi
    push edi
    call f_l8ok
    add esp, 8
    jmp l8s_out
l8s_no:
    xor eax, eax
l8s_out:
    pop edi
    pop esi
    ret
"""


DS = r"""
f_ds:
    push esi
    push edi
    mov esi, dword ptr [esp+0xc]
    mov edi, dword ptr [esp+0x10]
    cmp byte ptr [esi+0x3c], 3
    jne dz_no
    push esi
    call f_isstore
    pop ecx
    test eax, eax
    jz dz_no
    push dword ptr [esi+0x18]
    call f_nstatic
    pop ecx
    test eax, eax
    jz dz_no
    push dword ptr [edi+0x18]
    call f_framedecl_esc
    pop ecx
    test eax, eax
    jnz dz_yes
    cmp byte ptr [edi+0x3c], 3
    je dz_no
    push dword ptr [edi+0x18]
    call f_nstatic
    pop ecx
    test eax, eax
    jz dz_no
    mov eax, dword ptr [esi+0x18]
    mov eax, dword ptr [eax+0x10]
    mov ecx, dword ptr [edi+0x18]
    cmp eax, dword ptr [ecx+0x10]
    je dz_no
    test dword ptr [edi+0x14], 0x20
    jnz dz_yes
    push edi
    call f_isstore
    pop ecx
    test eax, eax
    jnz dz_no
dz_yes:
    mov eax, 1
    jmp dz_out
dz_no:
    xor eax, eax
dz_out:
    pop edi
    pop esi
    ret
"""

E3NC = r"""
f_e3nc:
    push esi
    push edi
    mov esi, dword ptr [esp+0xc]
    mov edi, dword ptr [esp+0x10]
    mov eax, dword ptr [edi+0x14]
    test eax, 0x40
    jz ec_no
    and eax, 0xffffffbf
    cmp eax, 2
    jne ec_no
    mov eax, dword ptr [esi+0x14]
    and eax, 0xffffffdf
    cmp eax, 4
    jne ec_no
    mov ecx, dword ptr [esi+0x18]
    test ecx, ecx
    jz ec_no
    cmp byte ptr [ecx+0x2c], 2
    je ec_no
    mov edx, dword ptr [edi+0x18]
    test edx, edx
    jz ec_no
    cmp byte ptr [edx+0x2c], 0
    jne ec_no
    test dword ptr [esi+0x14], 0x20
    jz ec_np
    movzx eax, word ptr [esi+0x20]
    cmp eax, 0x28
    jb ec_ok1
    cmp eax, 0x30
    jbe ec_no
ec_ok1:
    cmp dword ptr [ecx+0x18], 0x10
    jbe ec_no
    jmp ec_sz
ec_np:
    cmp dword ptr [ecx+0x18], 4
    ja ec_no
ec_sz:
    cmp dword ptr [edx+0x18], 8
    ja ec_no
    push edx
    call f_isstatic
    pop ecx
    test eax, eax
    jz ec_no
    push dword ptr [esi+0x18]
    call f_framedecl_esc
    pop ecx
    jmp ec_out
ec_no:
    xor eax, eax
ec_out:
    pop edi
    pop esi
    ret
"""


WE = r"""
f_we:
    push esi
    push edi
    mov esi, dword ptr [esp+0xc]
    mov edi, dword ptr [esp+0x10]
    push esi
    call f_isstore
    pop ecx
    test eax, eax
    jnz we_no
    push edi
    call f_isstore
    pop ecx
    test eax, eax
    jz we_no
    mov eax, dword ptr [esi+0x18]
    test eax, eax
    jz we_no
    cmp byte ptr [eax+0x2c], 0
    jne we_no
    cmp dword ptr [eax+0x18], 8
    ja we_no
    mov ecx, dword ptr [edi+0x18]
    test ecx, ecx
    jz we_no
    cmp byte ptr [ecx+0x2c], 2
    je we_no
    cmp dword ptr [ecx+0x18], 4
    ja we_no
    push eax
    call f_isstatic
    pop ecx
    test eax, eax
    jz we_no
    push dword ptr [edi+0x18]
    call f_framedecl_esc
    pop ecx
    jmp we_out
we_no:
    xor eax, eax
we_out:
    pop edi
    pop esi
    ret
"""


def pred_x(parts):
    calls = "".join("""
    push dword ptr [esp+8]
    push dword ptr [esp+8]
    call f_%s
    add esp, 8
    test eax, eax
    jnz px_yes
""" % p for p in ("psd", "dsf", "ds", "e3nc", "we", "l8s", "wg4") if p in parts)
    return "f_pred_x:" + calls + """
    xor eax, eax
    ret
px_yes:
    mov eax, 1
    ret
"""


def vn_extra(parts):
    s = r"""
f_vn_extra:
    push ebx
    push esi
    push edi
    push ebp
    mov esi, dword ptr [esp+0x14]
    mov ebp, dword ptr [esp+0x18]
    xor edi, edi
    test esi, esi
    jz vx_done
"""
    if "vnd" in parts:
        s += r"""
    test ebp, ebp
    jz vx_s1
    cmp byte ptr [ebp+0x3c], 3
    jne vx_s1
    test dword ptr [ebp+0x14], 0x20
    jnz vx_s1
    push esi
    call f_nstatic
    pop ecx
    test eax, eax
    jz vx_s1
    or edi, 1
vx_s1:
"""
    if "l8v" in parts:
        s += r"""
    cmp byte ptr [esi+0x2c], 0
    jne vx_s2
    mov eax, dword ptr [esi+0x10]
    test eax, eax
    jz vx_s2
    cmp dword ptr [eax], 5
    jne vx_fr
    push esi
    call f_isstatic
    pop ecx
    test eax, eax
    jz vx_s2
    jmp vx_on
vx_fr:
    cmp dword ptr [eax], 0x10005
    jne vx_s2
    cmp dword ptr [eax+0x18], 0
    je vx_s2
    push eax
    call IN_WC
    pop ecx
    test eax, eax
    jz vx_s2
vx_on:
    or edi, 2
vx_s2:
"""
    s += r"""
    test edi, edi
    jz vx_done
    call vx_pc
vx_pc:
    pop ebx
    sub ebx, vx_pc
    add ebx, LIST_HEAD
    mov ebx, dword ptr [ebx]
vx_loop:
    test ebx, ebx
    jz vx_done
    cmp ebx, esi
    je vx_next
    cmp byte ptr [ebx+0x2c], 0
    jne vx_next
    test edi, 1
    jz vx_t2
    push ebx
    call f_nstatic
    pop ecx
    test eax, eax
    jz vx_t2
    mov eax, dword ptr [ebx+0x10]
    cmp eax, dword ptr [esi+0x10]
    je vx_t2
    jmp vx_kill
vx_t2:
    test edi, 2
    jz vx_next
    cmp dword ptr [ebx+0x18], 8
    jne vx_next
    push ebx
    call f_islit
    pop ecx
    test eax, eax
    jz vx_next
vx_kill:
    push 0
    push ebx
    call KILL
    add esp, 8
vx_next:
    mov ebx, dword ptr [ebx]
    jmp vx_loop
vx_done:
    pop ebp
    pop edi
    pop esi
    pop ebx
    ret
f_vn_stub:
    push dword ptr [esp+8]
    push dword ptr [esp+8]
    call vn_body
    add esp, 8
    push eax
    push dword ptr [esp+0xc]
    push dword ptr [esp+0xc]
    call f_vn_extra
    add esp, 8
    pop eax
    ret
vn_body:
    push ebx
    push esi
    push edi
    push ebp
    sub esp, 0x10
    jmp VN_BODY_RESUME
"""
    return s


def subst(text, syms):
    for k, v in sorted(syms.items(), key=lambda kv: -len(kv[0])):
        text = text.replace(k, hex(v))
    return text


def split_funcs(text):
    out, cur, name = [], [], None
    for ln in text.strip("\n").splitlines():
        s = ln.strip()
        if s.endswith(":") and s.startswith("f_") and " " not in s:
            if name:
                out.append((name, "\n".join(cur)))
            name, cur = s[:-1], []
        else:
            cur.append(ln)
    if name:
        out.append((name, "\n".join(cur)))
    return out


def place(regions, size):
    for r in regions:
        at = (SEC + r[0] + 3) & ~3
        if at + size <= SEC + r[1]:
            r[0] = at - SEC + size
            return at
    sys.exit("no room (%d bytes)" % size)


def build(data: bytearray, parts):
    consts = {"PSEUDO": PSEUDO, "IN_WC": IN_WC, "LIST_HEAD": LIST_HEAD, "KILL": KILL,
              "VN_BODY_RESUME": VN_FN + 7}
    text = HELPERS
    if "psd" in parts: text += PSD
    if "dsf" in parts: text += DSF
    if "wg4" in parts: text += WG4
    if "l8s" in parts: text += L8S
    if "ds" in parts: text += DS
    if "e3nc" in parts: text += E3NC
    if "we" in parts: text += WE
    sched = any(p in parts for p in ("psd", "dsf", "l8s", "wg4", "ds", "e3nc", "we"))
    if sched: text += pred_x(parts)
    vn = any(p in parts for p in ("vnd", "l8v"))
    if vn: text += vn_extra(parts)
    funcs = [(n, subst(t, consts)) for n, t in split_funcs(text)]
    for n, _ in funcs:
        SYMS.setdefault(n, SEC + 0x800)
    for passno in range(3):
        regions = [list(r) for r in REGIONS]
        placed = []
        sized = [(len(asm(subst(t, SYMS), SYMS[n])), n, t) for n, t in funcs]
        for size, n, t in sorted(sized, key=lambda x: -x[0]):
            at = place(regions, size + 8)
            placed.append((n, at, t))
        for n, at, t in placed:
            SYMS[n] = at
    total = 0
    for n, at, t in placed:
        code = asm(subst(t, SYMS), at)
        off = P.SECTION_FILE + (at - SEC)
        if any(data[off:off + len(code)]):
            sys.exit("region for %s at %#x not free" % (n, at))
        data[off:off + len(code)] = code
        total += len(code)
    print("  %d functions, %d bytes" % (len(placed), total))
    if sched:
        pred = SYMS["f_pred_x"]
        for e in (0, 1, 3, 4):
            word, old = P.SCHED_DISPATCH[e]
            woff = P._p1e_data_offset(data, word)
            cur = struct.unpack_from("<I", data, woff)[0]
            if cur != old:
                sys.exit("sched entry %d is %#x, expected %#x" % (e, cur, old))
            t = "push ebp\npush esi\ncall %#x\nadd esp, 8\ntest eax, eax\njnz %#x\njmp %#x\n" % (pred, YES, old)
            at = place(regions, len(asm(t, SEC + 0x800)))
            code = asm(t, at)
            off = P.SECTION_FILE + (at - SEC)
            assert not any(data[off:off + len(code)])
            data[off:off + len(code)] = code
            struct.pack_into("<I", data, woff, at)
    if vn:
        o = VN_FN - P.TEXT_FILE_DELTA
        if bytes(data[o:o + 7]) != VN_OLD:
            sys.exit("0x511a30 prologue differs")
        data[o:o + 7] = asm("jmp %#x" % SYMS["f_vn_stub"], VN_FN) + b"\x90\x90"
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
