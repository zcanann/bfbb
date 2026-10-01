"""trace.py <unit> [--src f] [--mode vdecl]: log entry-0 scheduler clause answers (E3n/W/C/A/B) that return 1, with object names."""
import sys, os, struct, ctypes, argparse, collections
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "regalloc")); sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rcap, ablate
from rcap import K, Dbg, OFF_EIP, OFF_EFL, OFF_ESP, OFF_EAX, DBG_CONTINUE, DBG_EXCEPTION_NOT_HANDLED
SITES = {0x60e2f3: ('E3n@0', 4), 0x60e309: ('W@0', 4), 0x60e31f: ('C+@0', 4), 0x60e376: ('A@0', 2), 0x60e385: ('B@0', 4), 0x60e3aa: ('W@1', 4), 0x60e3c0: ('C@1', 4), 0x60e3d6: ('B@1', 4), 0x60e3f8: ('E3n@3', 4), 0x60e40a: ('C@3', 4), 0x60e41c: ('B@3', 4)}
ap = argparse.ArgumentParser(); ap.add_argument("unit"); ap.add_argument("--src"); a = ap.parse_args()
flags = ablate.unit_cmd(a.unit)
src = os.path.abspath(a.src) if a.src else next(ablate.ROOT + "/src/" + a.unit + e for e in (".c", ".cpp") if os.path.exists(ablate.ROOT + "/src/" + a.unit + e))
inc = " -I" + os.path.dirname(ablate.ROOT + "/src/" + a.unit + ".c").replace("/", "\\")
out = os.path.dirname(os.path.abspath(__file__)) + "/w/trace.o"
cmd = '"%s" %s%s -c "%s" -o "%s"' % ((ablate.ROOT + "/build/compilers/GC/2.0p1a/mwcceppc.exe").replace("/", "\\"), flags, inc, src.replace("/", "\\"), out.replace("/", "\\"))
d = Dbg(cmd, ablate.ROOT); ev = ctypes.create_string_buffer(256); rearm = {}; log = collections.Counter()
OPN = {}
def memname(m):
    if not m: return None
    base = d.u32(m + 0x10)
    return rcap.objname(d, base) if base else None
while True:
    K.WaitForDebugEvent(ev, 0xFFFFFFFF)
    code, pid, tid = struct.unpack_from('<III', ev, 0); cont = DBG_CONTINUE
    if code == 3:
        d.threads[tid] = struct.unpack_from('<Q', ev, 32)[0]
        for s in SITES: d.arm(s)
    elif code == 2: d.threads[tid] = struct.unpack_from('<Q', ev, 16)[0]
    elif code == 5: K.ContinueDebugEvent(pid, tid, DBG_CONTINUE); break
    elif code == 1:
        exc = struct.unpack_from('<I', ev, 16)[0]; addr = struct.unpack_from('<Q', ev, 32)[0]
        if exc in (0x80000003, 0x4000001F) and addr in SITES:
            h, c = d.ctx(tid); esp = struct.unpack_from('<I', c, OFF_ESP)[0]; eax = struct.unpack_from('<I', c, OFF_EAX)[0]
            if eax:
                pa, pb = d.u32(esp), d.u32(esp + 4)
                ma, mb = d.u32(pa + 0x18), d.u32(pb + 0x18)
                opa, opb = struct.unpack('<H', d.rd(pa + 0x20, 2))[0], struct.unpack('<H', d.rd(pb + 0x20, 2))[0]
                log[(SITES[addr][0], hex(opa), memname(ma), hex(opb), memname(mb))] += 1
            d.wr(addr, d.bps[addr]); struct.pack_into('<I', c, OFF_EIP, addr)
            struct.pack_into('<I', c, OFF_EFL, struct.unpack_from('<I', c, OFF_EFL)[0] | 0x100); K.Wow64SetThreadContext(h, c); rearm[tid] = addr
        elif exc in (0x80000004, 0x4000001E) and tid in rearm: d.arm(rearm.pop(tid))
        elif exc not in (0x80000003, 0x4000001F): cont = DBG_EXCEPTION_NOT_HANDLED
    K.ContinueDebugEvent(pid, tid, cont)
for k, v in sorted(log.items(), key=lambda kv: -kv[1]): print(v, k)
