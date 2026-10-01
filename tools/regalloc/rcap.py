"""rcap.py: capture GC/2.0p1a graph-colouring state with a tiny Win32 debugger.

usage: python rcap.py <src.c> <unit-substring-for-flags> [--fn NAME] [--json out.json]

Breakpoints (GC/2.0p1a mwcceppc.exe, image base 0x400000):
  0x508680 colorinstructions(Object *proc)   -> current function name
  0x508900 colorgraph(IGNode *stack)         -> graph + pop order (pre-colour)
  0x508766 (return from colorgraph)          -> chosen colours
IGNode: +0 next, +4 object, +0xc cost, +0x10 short, +0x12 degree, +0x14 color,
        +0x16 flags, +0x18 arraySize, +0x1a short neighbors[]
"""
import os as _os
_REPO = _os.path.dirname(_os.path.dirname(_os.path.dirname(_os.path.abspath(__file__)))).replace("\\", "/")
import ctypes, ctypes.wintypes as wt, struct, sys, os, re, shlex, json, subprocess

ROOT = _REPO
K = ctypes.WinDLL('kernel32', use_last_error=True)

BP_COLORINSTR = 0x508680
BP_COLORGRAPH = 0x508900
BP_COLORRET = 0x508766
G_IGRAPH = 0x5e9858
G_CLASS = 0x5ea299
G_NREAL = 0x5e9800
G_USEDVR = 0x5e9b04
G_AVAIL_FN = None

DEBUG_ONLY_THIS_PROCESS = 2
CREATE_NO_WINDOW = 0x08000000
DBG_CONTINUE = 0x00010002
DBG_EXCEPTION_NOT_HANDLED = 0x80010001


class STARTUPINFO(ctypes.Structure):
    _fields_ = [('cb', wt.DWORD), ('lpReserved', wt.LPWSTR), ('lpDesktop', wt.LPWSTR), ('lpTitle', wt.LPWSTR),
                ('dwX', wt.DWORD), ('dwY', wt.DWORD), ('dwXSize', wt.DWORD), ('dwYSize', wt.DWORD),
                ('dwXCountChars', wt.DWORD), ('dwYCountChars', wt.DWORD), ('dwFillAttribute', wt.DWORD),
                ('dwFlags', wt.DWORD), ('wShowWindow', wt.WORD), ('cbReserved2', wt.WORD),
                ('lpReserved2', ctypes.c_void_p), ('hStdInput', wt.HANDLE), ('hStdOutput', wt.HANDLE),
                ('hStdError', wt.HANDLE)]


class PROCESS_INFORMATION(ctypes.Structure):
    _fields_ = [('hProcess', wt.HANDLE), ('hThread', wt.HANDLE), ('dwProcessId', wt.DWORD), ('dwThreadId', wt.DWORD)]


K.CreateProcessW.argtypes = [wt.LPCWSTR, wt.LPWSTR, ctypes.c_void_p, ctypes.c_void_p, wt.BOOL, wt.DWORD,
                             ctypes.c_void_p, wt.LPCWSTR, ctypes.POINTER(STARTUPINFO), ctypes.POINTER(PROCESS_INFORMATION)]
K.WaitForDebugEvent.argtypes = [ctypes.c_void_p, wt.DWORD]
K.ContinueDebugEvent.argtypes = [wt.DWORD, wt.DWORD, wt.DWORD]
K.ReadProcessMemory.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
K.WriteProcessMemory.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
K.OpenThread.argtypes = [wt.DWORD, wt.BOOL, wt.DWORD]
K.OpenThread.restype = wt.HANDLE
K.Wow64GetThreadContext.argtypes = [wt.HANDLE, ctypes.c_void_p]
K.Wow64SetThreadContext.argtypes = [wt.HANDLE, ctypes.c_void_p]
K.FlushInstructionCache.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_size_t]

CTX_SIZE = 716
OFF_EIP, OFF_EFL, OFF_ESP, OFF_EAX = 184, 192, 196, 176


class Dbg:
    def __init__(self, cmdline, cwd):
        si = STARTUPINFO(); si.cb = ctypes.sizeof(si)
        pi = PROCESS_INFORMATION()
        buf = ctypes.create_unicode_buffer(cmdline)
        if not K.CreateProcessW(None, buf, None, None, False, DEBUG_ONLY_THIS_PROCESS | CREATE_NO_WINDOW,
                                None, cwd, ctypes.byref(si), ctypes.byref(pi)):
            raise OSError(ctypes.get_last_error())
        self.hp = pi.hProcess
        self.pid = pi.dwProcessId
        self.threads = {pi.dwThreadId: pi.hThread}
        self.bps = {}
        self.base = None
        self.extra = []  # additional breakpoint addresses armed at process start

    def rd(self, a, n):
        b = ctypes.create_string_buffer(n); got = ctypes.c_size_t()
        if not K.ReadProcessMemory(self.hp, ctypes.c_void_p(a), b, n, ctypes.byref(got)):
            raise OSError('read %x' % a)
        return b.raw

    def wr(self, a, data):
        got = ctypes.c_size_t()
        K.WriteProcessMemory(self.hp, ctypes.c_void_p(a), data, len(data), ctypes.byref(got))
        K.FlushInstructionCache(self.hp, ctypes.c_void_p(a), len(data))

    def u32(self, a): return struct.unpack('<I', self.rd(a, 4))[0]
    def s16(self, a): return struct.unpack('<h', self.rd(a, 2))[0]

    def arm(self, a):
        if a not in self.bps:
            self.bps[a] = self.rd(a, 1)
        self.wr(a, b'\xcc')

    def ctx(self, tid):
        h = self.threads.get(tid) or K.OpenThread(0x1FFFFF, False, tid)
        self.threads[tid] = h
        c = ctypes.create_string_buffer(CTX_SIZE)
        struct.pack_into('<I', c, 0, 0x10007)
        if not K.Wow64GetThreadContext(h, c):
            raise OSError('getctx %d' % ctypes.get_last_error())
        return h, c

    def run(self, on_bp):
        ev = ctypes.create_string_buffer(256)
        rearm = {}
        while True:
            if not K.WaitForDebugEvent(ev, 0xFFFFFFFF):
                raise OSError('wait')
            code, pid, tid = struct.unpack_from('<III', ev, 0)
            cont = DBG_CONTINUE
            if code == 3:  # CREATE_PROCESS
                self.threads[tid] = struct.unpack_from('<Q', ev, 32)[0]
                self.base = struct.unpack_from('<Q', ev, 40)[0]
                for a in (BP_COLORINSTR, BP_COLORGRAPH, BP_COLORRET) + tuple(self.extra):
                    self.arm(a)
            elif code == 2:  # CREATE_THREAD
                self.threads[tid] = struct.unpack_from('<Q', ev, 16)[0]
            elif code == 5:  # EXIT_PROCESS
                K.ContinueDebugEvent(pid, tid, DBG_CONTINUE)
                return struct.unpack_from('<I', ev, 16)[0]
            elif code == 1:  # EXCEPTION
                exc = struct.unpack_from('<I', ev, 16)[0]
                addr = struct.unpack_from('<Q', ev, 32)[0]
                if exc in (0x80000003, 0x4000001F):
                    if addr in self.bps:
                        h, c = self.ctx(tid)
                        regs = {'eip': addr, 'esp': struct.unpack_from('<I', c, OFF_ESP)[0],
                                'eax': struct.unpack_from('<I', c, OFF_EAX)[0]}
                        on_bp(self, addr, regs)
                        self.wr(addr, self.bps[addr])
                        struct.pack_into('<I', c, OFF_EIP, addr)
                        efl = struct.unpack_from('<I', c, OFF_EFL)[0] | 0x100
                        struct.pack_into('<I', c, OFF_EFL, efl)
                        K.Wow64SetThreadContext(h, c)
                        rearm[tid] = addr
                elif exc in (0x80000004, 0x4000001E):
                    if tid in rearm:
                        self.arm(rearm.pop(tid))
                    else:
                        cont = DBG_EXCEPTION_NOT_HANDLED
                else:
                    cont = DBG_EXCEPTION_NOT_HANDLED
            K.ContinueDebugEvent(pid, tid, cont)


def objname(d, obj):
    if not obj:
        return None
    try:
        hn = d.u32(obj + 0xa)
        raw = d.rd(hn + 0xa, 64)
        return raw.split(b'\0')[0].decode('latin1')
    except OSError:
        return '?'


class Capture:
    def __init__(self, want):
        self.want = want
        self.fn = None
        self.cur = None
        self.out = []

    def __call__(self, d, addr, regs):
        if addr == BP_COLORINSTR:
            self.fn = objname(d, d.u32(regs['esp'] + 4))
            return
        if self.want and self.fn != self.want:
            return
        cls = d.rd(G_CLASS, 1)[0]
        g = d.u32(G_IGRAPH)
        nreal = d.u32(G_NREAL + 4 * cls)
        used = d.u32(G_USEDVR + 4 * cls)
        if addr == BP_COLORGRAPH:
            head = d.u32(regs['esp'] + 4)
            nodes = {}
            for i in range(used):
                p = d.u32(g + 4 * i)
                if not p:
                    continue
                hdr = d.rd(p, 0x1a)
                nxt, obj, cost = struct.unpack_from('<III', hdr, 0)
                f10, deg, col, flg, n = struct.unpack_from('<hhhHh', hdr, 0x10)
                nb = list(struct.unpack('<%dh' % n, d.rd(p + 0x1a, 2 * n))) if n > 0 else []
                nodes[i] = dict(ptr=p, obj=obj, name=objname(d, obj), cost=cost, f10=f10, deg=deg,
                                color=col, flags=flg, nb=nb)
            order = []
            p = head
            ptr2i = {v['ptr']: k for k, v in nodes.items()}
            while p and len(order) < 100000:
                order.append(ptr2i.get(p, -1))
                p = d.u32(p)
            blocks = []
            b = d.u32(0x5e9838)
            while b and len(blocks) < 100000:
                ins = []
                pc = d.u32(b + 0x14)
                while pc and len(ins) < 100000:
                    hdr = d.rd(pc, 0x24)
                    op, n = struct.unpack_from('<hh', hdr, 0x20)
                    raw = d.rd(pc + 0x24, 12 * n) if n > 0 else b''
                    args = []
                    for k in range(max(n, 0)):
                        a = raw[12 * k:12 * k + 12]
                        args.append([a[0], a[1], struct.unpack_from('<h', a, 4)[0], struct.unpack_from('<i', a, 4)[0], a.hex()])
                    ins.append([op, struct.unpack_from('<I', hdr, 0x14)[0], args])
                    pc = struct.unpack_from('<I', hdr, 0)[0]
                blocks.append(ins)
                b = d.u32(b)
            self.cur = dict(fn=self.fn, cls=cls, nreal=nreal, used=used, nodes=nodes, order=order, blocks=blocks)
        elif addr == BP_COLORRET and self.cur is not None:
            for i, nd in self.cur['nodes'].items():
                nd['final'] = d.s16(nd['ptr'] + 0x14)
                nd['fflags'] = struct.unpack('<H', d.rd(nd['ptr'] + 0x16, 2))[0]
            self.cur['ok'] = regs['eax']
            self.out.append(self.cur)
            self.cur = None


def compile_cmd(src, unit, outobj):
    N = open(os.path.join(ROOT, 'build.ninja')).read()
    for m in re.finditer(r"^build (\S+\.o):(?: \$\n\s+| )(mwcc_sjis|mwcc) ((?:.*\n)*?)  basedir", N, re.M):
        if unit in m.group(1).replace('\\', '/'):
            body = m.group(3)
            mw = re.search(r"mw_version = (\S+)", body).group(1).replace('\\', '/')
            cf = re.search(r"cflags = ((?:.*\$\n)*.*)\n", body).group(1)
            flags = re.sub(r"\s+", " ", cf.replace("$\n", " ")).strip()
            exe = os.path.join(ROOT, 'build/compilers', mw, 'mwcceppc.exe').replace('/', '\\')
            flags += ' ' + os.environ.get('RCAP_EXTRA_FLAGS', '')
            if os.environ.get('RCAP_MW'):
                exe = os.path.join(ROOT, 'build/compilers', os.environ['RCAP_MW'], 'mwcceppc.exe').replace('/', '\\')
            return '"%s" %s -c "%s" -o "%s"' % (exe, flags, src, outobj)
    raise SystemExit('unit not found')


def main():
    src = os.path.abspath(sys.argv[1]); unit = sys.argv[2]
    want = sys.argv[sys.argv.index('--fn') + 1] if '--fn' in sys.argv else None
    outj = sys.argv[sys.argv.index('--json') + 1] if '--json' in sys.argv else None
    import tempfile
    td = tempfile.mkdtemp(prefix='rcap_')
    cmd = compile_cmd(src, unit, os.path.join(td, 'o.o'))
    d = Dbg(cmd, ROOT)
    cap = Capture(want)
    rc = d.run(cap)
    if rc != 0:
        print('compiler exit', rc)
    if outj:
        json.dump(cap.out, open(outj, 'w'))
    return cap.out


if __name__ == '__main__':
    out = main()
    for c in out:
        print(c['fn'], 'class', c['cls'], 'nodes', len(c['nodes']), 'ok', c.get('ok'))
