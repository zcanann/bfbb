"""cablate.py <unit-stem> <sym|-> --mode M [--src f.c] [--asm] [--off ...]
Conditional runtime ablation of clause V's walk (VN entry 0 stub 0x60e5f8) under a debugger:
when the stored object's base expression word is 0x10005 (frame object) and the mode's
condition holds, jump straight to the stock handler (0x511a53), i.e. no V walk, no F.
modes: vfr (all frame stores), vtmp (frame, Object+0x18==0: compiler temps),
       vdecl (frame, Object+0x18!=0: declared locals), log (no change; log hits)."""
import sys, os, struct, ctypes, argparse, re, subprocess, json, hashlib
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "regalloc"))
import rcap
from rcap import K, Dbg, CTX_SIZE, OFF_EIP, OFF_EFL, DBG_CONTINUE, DBG_EXCEPTION_NOT_HANDLED
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ablate
STUB = 0x60e5f8; STOCK = 0x511a53; OFF_EBX = 164

def run(cmd, mode, offs, log, kills=()):
    d = Dbg(cmd, ablate.ROOT)
    ev = ctypes.create_string_buffer(256); rearm = {}
    while True:
        K.WaitForDebugEvent(ev, 0xFFFFFFFF)
        code, pid, tid = struct.unpack_from('<III', ev, 0)
        cont = DBG_CONTINUE
        if code == 3:
            d.threads[tid] = struct.unpack_from('<Q', ev, 32)[0]
            for o in offs:
                a, v = ablate.STOCK[o]; d.wr(a, struct.pack('<I', v))
            d.arm(STUB)
            for k in kills: d.arm(k)
        elif code == 2:
            d.threads[tid] = struct.unpack_from('<Q', ev, 16)[0]
        elif code == 5:
            K.ContinueDebugEvent(pid, tid, DBG_CONTINUE); return
        elif code == 1:
            exc = struct.unpack_from('<I', ev, 16)[0]; addr = struct.unpack_from('<Q', ev, 32)[0]
            if exc in (0x80000003, 0x4000001F) and addr in kills:
                h, c = d.ctx(tid)
                esp = struct.unpack_from('<I', c, rcap.OFF_ESP)[0]
                ret = d.u32(esp)
                struct.pack_into('<I', c, rcap.OFF_EAX, 0)
                struct.pack_into('<I', c, OFF_EIP, ret)
                struct.pack_into('<I', c, rcap.OFF_ESP, esp + 4)
                K.Wow64SetThreadContext(h, c)
                if log is not None: log.append(('kill', hex(addr)))
            elif exc in (0x80000003, 0x4000001F) and addr == STUB:
                h, c = d.ctx(tid)
                ebx = struct.unpack_from('<I', c, OFF_EBX)[0]
                base = d.u32(ebx + 0x10)
                word = d.u32(base) if base else 0
                o18 = d.u32(base + 0x18) if base else 0
                skip = word == 0x10005 and (mode == 'vfr' or (mode == 'vtmp' and o18 == 0) or (mode == 'vdecl' and o18 != 0))
                if log is not None:
                    log.append((hex(word), o18 != 0, rcap.objname(d, base) if base else None, skip))
                if skip:
                    struct.pack_into('<I', c, OFF_EIP, STOCK); K.Wow64SetThreadContext(h, c)
                else:
                    d.wr(addr, d.bps[addr]); struct.pack_into('<I', c, OFF_EIP, addr)
                    struct.pack_into('<I', c, OFF_EFL, struct.unpack_from('<I', c, OFF_EFL)[0] | 0x100)
                    K.Wow64SetThreadContext(h, c); rearm[tid] = addr
            elif exc in (0x80000004, 0x4000001E) and tid in rearm:
                d.arm(rearm.pop(tid))
            elif exc not in (0x80000003, 0x4000001F):
                cont = DBG_EXCEPTION_NOT_HANDLED
        K.ContinueDebugEvent(pid, tid, cont)

ap = argparse.ArgumentParser()
ap.add_argument("unit"); ap.add_argument("sym"); ap.add_argument("--mode", default="log")
ap.add_argument("--src"); ap.add_argument("--asm", action="store_true"); ap.add_argument("--off", default="")
ap.add_argument("--kill", default="")
ap.add_argument("--show", action="store_true"); ap.add_argument("--log", action="store_true")
a = ap.parse_args()
flags = ablate.unit_cmd(a.unit)
src = os.path.abspath(a.src) if a.src else next(ablate.ROOT + "/src/" + a.unit + e for e in (".c", ".cpp", ".cp") if os.path.exists(ablate.ROOT + "/src/" + a.unit + e))
inc = " -I" + os.path.dirname(ablate.ROOT + "/src/" + a.unit + ".c").replace("/", "\\")
out = os.path.dirname(os.path.abspath(__file__)) + "/w/cab_" + hashlib.md5((a.unit + src + a.mode + a.off + a.kill).encode()).hexdigest()[:10] + ".o"
if os.path.exists(out): os.remove(out)
exe = ablate.ROOT + "/build/compilers/GC/2.0p1a/mwcceppc.exe"
cmd = '"%s" %s%s -c "%s" -o "%s"' % (exe.replace("/", "\\"), flags, inc, src.replace("/", "\\"), out.replace("/", "\\"))
log = [] if a.log else None
run(cmd, a.mode, [x for x in a.off.split(",") if x], log, [int(x,16) for x in a.kill.split(",") if x])
if log is not None:
    import collections
    for k, v in collections.Counter(log).most_common(40): print(v, k)
if a.asm:
    d = out + ".s"
    subprocess.run([ablate.ROOT + "/build/tools/dtk.exe", "elf", "disasm", out, d], capture_output=True)
    txt = re.sub(r"/\* [0-9A-F]+ [0-9A-F]+ [0-9A-F ]+\*/\t", "", open(d).read())
    i = txt.find(".fn " + a.sym); j = txt.find(".endfn " + a.sym, i)
    print("\n".join(l.strip() for l in txt[i:j].splitlines() if l.strip() and not l.strip().startswith((".", "#"))))
    sys.exit()
cfg = json.load(open(ablate.ROOT + "/objdiff.json"))
u = [x for x in cfg["units"] if x["name"] == "main/" + a.unit][0]
dd = json.loads(subprocess.run([ablate.OBJDIFF, "diff", "-1", ablate.ROOT + "/" + u["target_path"], "-2", out, "-o", "-", "--format", "json"], capture_output=True, text=True).stdout)
for s in dd["left"]["symbols"]:
    if s.get("kind") == "SYMBOL_FUNCTION" and (a.sym == "-" or s["name"] == a.sym):
        print(f"{s.get('match_percent', 0):7.2f} {s['name']}  [mode={a.mode} off={a.off}]")
