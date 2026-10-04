

var isVar = new NativeFunction(va(0x46a330), "pointer", ["pointer"], "mscdecl");
var NPROP = 0;
Interceptor.attach(va(0x4709f0), { onEnter: function () { this.l = this.context.esp.add(4).readPointer(); },
  onLeave: function (rv) { if ((rv.toInt32() & 0xff) == 0) return; var l = this.l;
    var L = isVar(l.add(0x20).readPointer()), R = isVar(l.add(0x24).readPointer());
    if (R.isNull() || L.isNull()) return;
    if (L.add(2).readU8() != 1) return; var vi = L.add(0x2a).readPointer(); if (vi.isNull()) return;
    if (vi.add(0x22).readU8() == 0) return;
    rv.replace(0); NPROP++; } });
