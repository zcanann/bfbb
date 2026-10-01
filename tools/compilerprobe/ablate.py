"""ablate.py <unit-stem> <sym|-> [--src file.c] [--off vn0,vn1,s0,s1,s3,s4,licm] [--show] [--asm SYM]

Runtime (in-memory) ablation of GC/2.0p1a's patch hooks: the compiler process is
created suspended, the chosen dispatch-table entries are restored to the stock
GC/2.0p1 handler addresses IN PROCESS MEMORY ONLY, then it is resumed. The
binary on disk is never touched.
  vn0  VN store-kill entry 0 (whole)    -> clauses V, F
  vn1  VN store-kill entry 1 (subrange) -> clause S (VN half, direct-store walk)
  s0,s1,s3 scheduler may_alias entries  -> A/B/C/C+/E3n/W
  s4   scheduler entry 4                -> clause S (sched half)
  licm moveinvariantsfromloop call      -> sb_licm_invariant
With --unit-file '-' and --src, compiles a standalone file with the unit's flags
and prints its disassembly (for repro snippets)."""
import ctypes, ctypes.wintypes as wt, struct, sys, os, re, subprocess, json, argparse, hashlib, shlex
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))).replace("\\", "/")
OBJDIFF = ROOT + "/build/tools/objdiff-cli.exe"
K = ctypes.WinDLL('kernel32', use_last_error=True)

STOCK = {
    'vn0': (0x5bd068, 0x511a53), 'vn1': (0x5bd06c, 0x511b0e),
    's0': (0x5bd0bc, 0x511ff2), 's1': (0x5bd0c0, 0x511fff), 's3': (0x5bd0c8, 0x511fff), 's4': (0x5bd0cc, 0x512012),
}
PATCHED = {'vn0': 0x60e5f8, 'vn1': 0x60e619, 's0': 0x60e57c, 's1': 0x60e59b, 's3': 0x60e5ba, 's4': 0x60e5d9}


class STARTUPINFO(ctypes.Structure):
    _fields_ = [('cb', wt.DWORD), ('lpReserved', wt.LPWSTR), ('lpDesktop', wt.LPWSTR), ('lpTitle', wt.LPWSTR),
                ('dwX', wt.DWORD), ('dwY', wt.DWORD), ('dwXSize', wt.DWORD), ('dwYSize', wt.DWORD),
                ('dwXCountChars', wt.DWORD), ('dwYCountChars', wt.DWORD), ('dwFillAttribute', wt.DWORD),
                ('dwFlags', wt.DWORD), ('wShowWindow', wt.WORD), ('cbReserved2', wt.WORD),
                ('lpReserved2', ctypes.c_void_p), ('hStdInput', wt.HANDLE), ('hStdOutput', wt.HANDLE),
                ('hStdError', wt.HANDLE)]


class PI(ctypes.Structure):
    _fields_ = [('hProcess', wt.HANDLE), ('hThread', wt.HANDLE), ('dwProcessId', wt.DWORD), ('dwThreadId', wt.DWORD)]


K.CreateProcessW.argtypes = [wt.LPCWSTR, wt.LPWSTR, ctypes.c_void_p, ctypes.c_void_p, wt.BOOL, wt.DWORD,
                             ctypes.c_void_p, wt.LPCWSTR, ctypes.POINTER(STARTUPINFO), ctypes.POINTER(PI)]
K.ReadProcessMemory.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
K.WriteProcessMemory.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
K.VirtualProtectEx.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_size_t, wt.DWORD, ctypes.POINTER(wt.DWORD)]
K.FlushInstructionCache.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_size_t]
K.WaitForSingleObject.argtypes = [wt.HANDLE, wt.DWORD]
K.GetExitCodeProcess.argtypes = [wt.HANDLE, ctypes.POINTER(wt.DWORD)]
K.ResumeThread.argtypes = [wt.HANDLE]


def run_ablated(cmdline, offs):
    si = STARTUPINFO(); si.cb = ctypes.sizeof(si); pi = PI()
    buf = ctypes.create_unicode_buffer(cmdline)
    if not K.CreateProcessW(None, buf, None, None, False, 0x4 | 0x08000000, None, ROOT, ctypes.byref(si), ctypes.byref(pi)):
        raise OSError(ctypes.get_last_error())
    hp = pi.hProcess

    def rd(a, n):
        b = ctypes.create_string_buffer(n); g = ctypes.c_size_t()
        if not K.ReadProcessMemory(hp, ctypes.c_void_p(a), b, n, ctypes.byref(g)):
            raise OSError('read %x err %d' % (a, ctypes.get_last_error()))
        return b.raw

    def wr(a, data):
        old = wt.DWORD(); g = ctypes.c_size_t()
        K.VirtualProtectEx(hp, ctypes.c_void_p(a), len(data), 0x40, ctypes.byref(old))
        if not K.WriteProcessMemory(hp, ctypes.c_void_p(a), data, len(data), ctypes.byref(g)):
            raise OSError('write %x' % a)
        K.VirtualProtectEx(hp, ctypes.c_void_p(a), len(data), old.value, ctypes.byref(old))
        K.FlushInstructionCache(hp, ctypes.c_void_p(a), len(data))

    for o in offs:
        if o == 'licm':
            cur = rd(0x56f472, 5)
            assert cur == bytes.fromhex('e8c4f10900'), cur.hex()
            wr(0x56f473, struct.pack('<i', 0x570f60 - (0x56f472 + 5)))
        else:
            a, v = STOCK[o]
            cur = struct.unpack('<I', rd(a, 4))[0]
            assert cur == PATCHED[o], (o, hex(cur))
            wr(a, struct.pack('<I', v))
    K.ResumeThread(pi.hThread)
    K.WaitForSingleObject(hp, 0xFFFFFFFF)
    ec = wt.DWORD(); K.GetExitCodeProcess(hp, ctypes.byref(ec))
    return ec.value


def unit_cmd(unit):
    N = open(ROOT + "/build.ninja").read()
    obj = "build/GQPE78/src/" + unit + ".o"
    for m in re.finditer(r"^build (\S+\.o):(?: \$\n\s+| )(mwcc_sjis|mwcc) ((?:.*\n)*?)  basedir", N, re.M):
        if m.group(1).replace('\\', '/') == obj:
            body = m.group(3)
            cf = re.search(r"cflags = ((?:.*\$\n)*.*)\n", body).group(1)
            return re.sub(r"\s+", " ", cf.replace("$\n", " ")).strip()
    raise SystemExit("unit not found " + unit)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("unit"); ap.add_argument("sym")
    ap.add_argument("--src"); ap.add_argument("--off", default="")
    ap.add_argument("--mw", default="2.0p1a")
    ap.add_argument("--show", action="store_true"); ap.add_argument("--all", action="store_true")
    ap.add_argument("--asm", action="store_true", help="print disassembly of sym (standalone snippet)")
    a = ap.parse_args()
    offs = [x for x in a.off.split(",") if x]
    flags = unit_cmd(a.unit)
    src = os.path.abspath(a.src) if a.src else next(ROOT + "/src/" + a.unit + e for e in (".c", ".cpp", ".cp") if os.path.exists(ROOT + "/src/" + a.unit + e))
    inc = " -I" + os.path.dirname(ROOT + "/src/" + a.unit + ".c").replace("/", "\\")
    scr = os.path.dirname(os.path.abspath(__file__)) + "/w"
    tag = hashlib.md5((a.unit + a.off + src + a.mw).encode()).hexdigest()[:10]
    out = scr + "/ab_" + tag + ".o"
    if os.path.exists(out): os.remove(out)
    exe = ROOT + "/build/compilers/GC/" + a.mw + "/mwcceppc.exe"
    cmd = '"%s" %s%s -c "%s" -o "%s"' % (exe.replace("/", "\\"), flags, inc, src.replace("/", "\\"), out.replace("/", "\\"))
    if a.mw == "2.0p1a":
        rc = run_ablated(cmd, offs)
    else:
        assert not offs
        rc = subprocess.run(cmd, cwd=ROOT, shell=True).returncode
    if not os.path.exists(out):
        print("compile failed rc", rc, cmd); sys.exit(1)
    if a.asm:
        d = out + ".s"
        subprocess.run([ROOT + "/build/tools/dtk.exe", "elf", "disasm", out, d], capture_output=True)
        txt = open(d).read()
        txt = re.sub(r"/\* [0-9A-F]+ [0-9A-F]+ [0-9A-F ]+\*/\t", "", txt)
        i = txt.find(".fn " + a.sym); j = txt.find(".endfn " + a.sym, i)
        print("\n".join(l.strip() for l in txt[i:j].splitlines() if l.strip() and not l.strip().startswith((".", "#"))))
        return
    cfg = json.load(open(ROOT + "/objdiff.json"))
    u = [x for x in cfg["units"] if x["name"] == "main/" + a.unit][0]
    d = json.loads(subprocess.run([OBJDIFF, "diff", "-1", ROOT + "/" + u["target_path"], "-2", out, "-o", "-", "--format", "json"],
                                  capture_output=True, text=True).stdout)
    L = d["left"]["symbols"]; R = d.get("right", {}).get("symbols", [])

    def fmt(ins):
        if not ins: return ""
        i = ins.get("instruction", {}); t = i.get("formatted", "")
        r = i.get("relocation")
        if r: t += "  <" + r.get("target", {}).get("name", "?") + ">"
        return t
    for s in L:
        if s.get("kind") != "SYMBOL_FUNCTION": continue
        if a.sym != "-" and s["name"] != a.sym: continue
        print(f"{s.get('match_percent', 0):7.2f} {s['name']}  [off={a.off or 'none'}]")
        if a.show:
            r = R[s["target_symbol"]] if "target_symbol" in s else {}
            li = s.get("instructions", []); ri = r.get("instructions", [])
            for k in range(max(len(li), len(ri))):
                x = li[k] if k < len(li) else None; y = ri[k] if k < len(ri) else None
                kind = (x or {}).get("diff_kind") or (y or {}).get("diff_kind") or ""
                mark = "  " if not kind or kind == "DIFF_NONE" else {"DIFF_REPLACE": "|", "DIFF_DELETE": "<", "DIFF_INSERT": ">", "DIFF_OP_MISMATCH": "|", "DIFF_ARG_MISMATCH": "r"}.get(kind, "?")
                if mark == "  " and not a.all: continue
                print(f"{k:4d} {mark:2s} {fmt(x):55.55s} | {fmt(y)}")


if __name__ == "__main__":
    main()
