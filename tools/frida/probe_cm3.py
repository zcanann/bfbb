#!/usr/bin/env python3
"""Frida probe v3: hook 0x511cb0 (arg1 instr -> memrefA) and 0x512e20
(returns memrefB for arg2) to reconstruct the dispatch pairs safely.

usage: probe_cm3.py <compiler.exe> <src.cpp> <out.o>
"""
import sys, frida, threading, json
from collections import Counter

CC, SRC, OUT = sys.argv[1], sys.argv[2], sys.argv[3]
ARGS = [CC, "-nodefaults", "-proc", "gekko", "-align", "powerpc", "-enum", "int",
        "-fp", "hardware", "-Cpp_exceptions", "off", "-W", "err", "-O4,p",
        "-inline", "auto", "-pragma", "cats off", "-pragma", "warn_notinlined off",
        "-maxerrors", "1", "-nosyspath", "-RTTI", "off", "-fp_contract", "on",
        "-str", "reuse", "-multibyte", "-DBUILD_VERSION=0", "-DVERSION_GQPE78",
        "-DNDEBUG=1", "-lang=c++", "-common", "on", "-char", "unsigned",
        "-str", "reuse,pool,readonly", "-use_lmw_stmw", "on",
        "-pragma", "cpp_extensions on", "-inline", "off", "-gccinc",
        "-DGAMECUBE", "-sym", "on", "-c", SRC, "-o", OUT]

SCRIPT = r"""
var base = Process.enumerateModules()[0].base;
var delta = base.sub(0x400000);
function va(x) { return base.add(x - 0x400000); }
function toVa(p) { return p.sub(delta).toInt32() >>> 0; }
function safeU32(p) { try { return p.readU32(); } catch (e) { return -1; } }
function memrefInfo(m) {
    if (m.isNull()) return null;
    var basex = m.add(0x10).readPointer();
    return {kind: m.add(0x2c).readU8(), size: safeU32(m.add(0x18)),
            off: safeU32(m.add(0x14)),
            bword: basex.isNull() ? 0 : safeU32(basex),
            m: toVa(m).toString(16)};
}
var inCb0 = false;
var curA = null, curRet = null;
Interceptor.attach(va(0x511cb0), {
    onEnter: function () {
        var esp = this.context.esp;
        curRet = toVa(esp.readPointer()).toString(16);
        var i1 = esp.add(4).readPointer();
        curA = i1.isNull() ? null : memrefInfo(i1.add(0x18).readPointer());
        inCb0 = true;
    },
    onLeave: function (rv) {
        inCb0 = false;
        send({t: "ans", ret: curRet, ans: rv.toInt32() & 0xff});
    }
});
Interceptor.attach(va(0x512e20), {
    onLeave: function (rv) {
        if (inCb0) {
            send({t: "pair", mA: curA, mB: memrefInfo(rv)});
            inCb0 = false;   // only the first 512e20 inside cb0
        }
    }
});
send({t: "ready"});
"""

msgs = []
def on_message(msg, data):
    if msg["type"] == "send":
        msgs.append(msg["payload"])
    else:
        print("ERR", msg)

dev = frida.get_local_device()
pid = dev.spawn(ARGS)
sess = dev.attach(pid)
script = sess.create_script(SCRIPT)
script.on("message", on_message)
script.load()
done = threading.Event()
sess.on("detached", lambda *a: done.set())
dev.resume(pid)
done.wait(180)

with open(OUT + ".cm3log.json", "w") as f:
    json.dump(msgs, f, indent=0)
pats = Counter()
lastPair = None
for m in msgs:
    if m["t"] == "pair":
        lastPair = m
    elif m["t"] == "ans":
        def d(mr):
            return (mr["kind"], mr["size"], hex(mr["bword"])) if mr else None
        if lastPair is not None:
            pats[(m["ret"], d(lastPair["mA"]), d(lastPair["mB"]), m["ans"])] += 1
        lastPair = None
for k, c in sorted(pats.items(), key=lambda x: -x[1]):
    print(c, k)
print("log:", OUT + ".cm3log.json")
