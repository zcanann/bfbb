var base = Process.getModuleByName("mwcceppc.exe").base;
function va(x) { return base.add(x - 0x400000); }
var fn = "?";
var getLink = new NativeFunction(va(0x4FE710), "pointer", ["pointer"], "mscdecl");
var counts = {};
function flush() {
  if (fn !== "?") send({fn: fn, c: counts});
  counts = {};
}
Interceptor.attach(va(0x4333C0), { onEnter: function () {
  flush();
  try { var o = this.context.esp.add(8).readPointer(); fn = getLink(o).add(0xa).readCString(); } catch (e) { fn = "??"; }
} });
Interceptor.attach(Process.getModuleByName('kernel32.dll').getExportByName('ExitProcess'), { onEnter: function () { flush(); } });
function opcls(op) {
  if (op >= 21 && op <= 24) return "lb"; if (op >= 25 && op <= 33) return "lh"; if (op >= 34 && op <= 39) return "lw";
  if (op >= 40 && op <= 43) return "sb"; if (op >= 44 && op <= 48) return "sh"; if (op >= 49 && op <= 54) return "sw";
  if (op >= 142 && op <= 145) return "lfs"; if (op >= 146 && op <= 149) return "lfd";
  if (op >= 150 && op <= 153) return "stfs"; if (op >= 154 && op <= 157) return "stfd";
  return "o" + op;
}
function sig(p) {
  var f = p.add(0x14).readU32();
  var s = opcls(p.add(0x20).readU16()) + ((f & 0x20) ? "*" : "") + ((f & 0x40) ? "c" : "") + ((f & 0x80) ? "v" : "") + (p.add(0x3c).readU8() == 3 ? "D" : "");
  var A = p.add(0x18).readPointer();
  if (A.isNull()) return s + ":nil";
  var k = A.add(0x2c).readU8();
  if (k == 2) return s + ":set";
  var o = A.add(0x10).readPointer();
  var st = "none";
  if (!o.isNull()) {
    var h = o.readU32();
    var nm = "";
    try { nm = o.add(0xa).readPointer().add(0xa).readCString(); } catch (e) {}
    if (h == 5) st = (nm.charAt(0) == "@" ? "lit" : "st");
    else if (h == 0x10005) st = (o.add(0x18).readU32() == 0 || nm.charAt(0) == "@") ? "tmp" : "fr";
    else st = "h" + h.toString(16);
  }
  var sz = A.add(0x18).readU32();
  return s + ":" + st + (k == 1 ? "~" : "") + (sz <= 4 ? "4" : sz <= 8 ? "8" : sz <= 16 ? "16" : "L");
}
Interceptor.attach(va(0x511fc0), {
  onEnter: function () { this.a = this.context.esp.add(4).readPointer(); this.b = this.context.esp.add(8).readPointer(); },
  onLeave: function (rv) {
    var k = sig(this.a) + "|" + sig(this.b) + "=" + (rv.toInt32() & 0xff);
    counts[k] = (counts[k] || 0) + 1;
  }
});
