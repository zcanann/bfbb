var MODE = "%MODE%";
var base = null;
for (const m of Process.enumerateModules()) {
    if (m.name.toLowerCase() == "mwcceppc.exe") { base = m.base; break; }
}
if (base === null) { base = Process.enumerateModules()[0].base; }
function va(x) { return base.add(x - 0x400000); }
function safeU32(p) { try { return p.readU32(); } catch (e) { return -1; } }
function baseWord(m) {
    if (m.isNull()) return -2;
    var basex = m.add(0x10).readPointer();
    return basex.isNull() ? -2 : safeU32(basex);
}
var inCb0 = false, mA = ptr(0), mB = ptr(0);
Interceptor.attach(va(0x511cb0), {
    onEnter: function () {
        var esp = this.context.esp;
        var i1 = esp.add(4).readPointer();
        mA = i1.isNull() ? ptr(0) : i1.add(0x18).readPointer();
        mB = ptr(0);
        inCb0 = true;
    },
    onLeave: function (rv) {
        inCb0 = false;
        if (rv.toInt32() & 0xff) return;
        if (MODE == "off") return;
        if (mA.isNull() || mB.isNull()) return;
        if (MODE == "h4") {
            if (baseWord(mB) != 5) return;
            var sz4 = safeU32(mB.add(0x18));
            if (sz4 < 0 || sz4 > 4) return;
            rv.replace(1);
            return;
        }
        if (MODE == "h3") {
            if (baseWord(mA) != 5) return;
            var bw = baseWord(mB);
            if (bw != 5 && bw != 0x10005) return;
            var sz3 = safeU32(mB.add(0x18));
            if (sz3 < 0 || sz3 > 4) return;
            rv.replace(1);
            return;
        }
        if (MODE == "h1" || MODE == "h1e0" || MODE == "h2") {
            if (MODE == "h1e0") {
            if (mA.add(0x2c).readU8() != 0) return;
            if (mB.add(0x2c).readU8() != 0) return;
            }
            if (baseWord(mA) != 5) return;
            if (baseWord(mB) != 5) return;
            var sz = safeU32(mB.add(0x18));
            var cap = (MODE == "h2") ? 8 : 4;
            if (sz < 0 || sz > cap) return;
        }
        rv.replace(1);
    }
});
var d2e20 = 0;
Interceptor.attach(va(0x512e20), {
    onEnter: function () { d2e20++; },
    onLeave: function (rv) {
        d2e20--;
        if (d2e20 == 0 && inCb0 && mB.isNull()) mB = rv.add(0);
    }
});
