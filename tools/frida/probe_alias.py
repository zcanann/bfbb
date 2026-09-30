#!/usr/bin/env python3
"""Frida probe: log calls to the may-alias predicate 0x511fc0 (with caller),
the VN store-kill 0x511a30, and 0x50a2c0 records/kills during a compile.

usage: probe_alias.py <compiler.exe> <src.cpp> <out.o>
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
            basex: toVa(basex).toString(16),
            m: toVa(m).toString(16)};
}
Interceptor.attach(va(0x511fc0), {
    onEnter: function (args) {
        var esp = this.context.esp;
        this.ret = toVa(esp.readPointer());
        var ia = esp.add(4).readPointer();
        var ib = esp.add(8).readPointer();
        this.opa = safeU32(ia.add(0x20)) & 0xffff;
        this.opb = safeU32(ib.add(0x20)) & 0xffff;
        this.fa = safeU32(ia.add(0x14));
        this.fb = safeU32(ib.add(0x14));
        this.ma = memrefInfo(ia.add(0x18).readPointer());
        this.mb = memrefInfo(ib.add(0x18).readPointer());
    },
    onLeave: function (rv) {
        send({t: "alias", ret: this.ret.toString(16), opa: this.opa, opb: this.opb,
              fa: this.fa, fb: this.fb, ma: this.ma, mb: this.mb,
              ans: rv.toInt32() & 0xff});
    }
});
Interceptor.attach(va(0x511a30), {
    onEnter: function (args) {
        var esp = this.context.esp;
        var ret = toVa(esp.readPointer());
        var m = esp.add(4).readPointer();
        send({t: "kill", ret: ret.toString(16), m: memrefInfo(m),
              value: esp.add(8).readPointer().toString()});
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

with open(OUT + ".aliaslog.json", "w") as f:
    json.dump(msgs, f, indent=0)
al = [m for m in msgs if m["t"] == "alias"]
kl = [m for m in msgs if m["t"] == "kill"]
print("alias queries:", len(al), " kills:", len(kl))
c = Counter(m["ret"] for m in al)
print("alias callers:", dict(c))
ck = Counter(m["ret"] for m in kl)
print("kill callers:", dict(ck))
print("log:", OUT + ".aliaslog.json")
