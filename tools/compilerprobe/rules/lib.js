// clause library (mirrors 2.0p1a blob, with an optional flag mask)
function isStaticD(d) { return d.k >= 0 && d.k != 2 && d.obj && !d.obj.isNull() && d.h == 5 && d.st != "pso"; }
function isFrameDecl(d) { return d.k >= 0 && d.k != 2 && d.obj && !d.obj.isNull() && d.h == 0x10005 && d.obj.add(0x18).readU32() != 0; }
function isLit(d) { return isStaticD(d) && d.nm.charAt(0) == "@"; }
function isLocalStatic(d) { return isStaticD(d) && d.nm.indexOf("$") > 0; }
function cPlus(a, b, fa, fb) {
  if (a.op == b.op) return false;
  if ((fa & ~0xa6) != 0) return false; if (!(fa & 4) && a.sz > 4) return false; if (!isStaticD(a)) return false;
  if ((fb & ~0xa6) != 0) return false; if (!(fb & 4) && b.sz > 4) return false; if (!isStaticD(b)) return false;
  return true;
}
function cBase(a, b, fa, fb) { if ((fa & 0x20) || (fb & 0x20)) return false; if (a.sz > 4 || b.sz > 4) return false; return cPlus(a, b, fa, fb); }
function inWC(obj) {
  var w = WCP.readPointer();
  if (w.isNull() || w.add(0x2c).readU8() != 2) return false;
  var n = w.add(0xc).readPointer();
  while (!n.isNull()) { if (n.add(0xc).readPointer().add(0x10).readPointer().equals(obj)) return true; n = n.readPointer(); }
  return false;
}
function frameNoEsc(d) { return d.k >= 0 && d.k != 2 && d.obj && !d.obj.isNull() && d.h == 0x10005 && !inWC(d.obj); }
function clA(fa, fb) { return (fa & ~6) == 0 && (fb & ~6) == 0; }
function clAg(a, b, fa, fb) { if (frameNoEsc(a) || frameNoEsc(b)) return false; return clA(fa, fb); }
function smallStatic(d) {
  if (d.k != 1 || !isStaticD(d)) return null;
  var m = d.A.add(8).readPointer();
  while (!m.isNull()) {
    var al = m.add(8).readPointer();
    if (!al.isNull() && al.add(0x2c).readU8() == 0 && al.add(0x10).readPointer().equals(d.obj))
      return al.add(0x18).readU32() <= 8 ? al : null;
    m = m.add(4).readPointer();
  }
  return null;
}
// entry-0/1/3/4 "static" clauses C+/C/A/S with flags fa, fb (no E3n/W/B)
function staticClauses(a, b, fa, fb) {
  if (a.k < 0 || b.k < 0 || a.k == 2 || b.k == 2) return false;
  var e = a.k * 3 + b.k;
  if (e == 0) {
    if (cPlus(a, b, fa, fb)) return true;
    if (a.sz > 4 || b.sz > 4) return false;
    if (a.op != b.op && (isStaticD(a) || isStaticD(b)) && clAg(a, b, fa, fb)) return true;
    return false;
  }
  if (e == 1 || e == 3) return cBase(a, b, fa, fb);
  if (e == 4) {
    var s1 = smallStatic(a), s2 = smallStatic(b);
    if (!s1 || !s2) return false;
    if (s1.equals(s2)) return true;
    if (a.sz > 4 || b.sz > 4) return false;
    return a.op != b.op && clA(fa, fb);
  }
  return false;
}
// object of at most 8 bytes (S's alias unit): whole alias size, or the whole alias found through the subrange's links
function small8(d) {
  if (!isStaticD(d)) return false;
  if (d.k == 0) return d.sz <= 8;
  if (d.k == 1) return smallStatic(d) != null;
  return false;
}
// clause E3n (2.0p1a) with the p1d address-taken gate, load flags given explicitly
function e3n(a, b, fa, fb) {
  if (a.k < 0 || b.k < 0 || a.k == 2 || b.k == 2) return false;
  if (b.k != 0) return false;                  // entries 0 and 3 only
  if ((fa & ~0x20) != 4) return false;
  if (fb != 2) return false;
  if (fa & 0x20) { if (a.op >= 0x28 && a.op <= 0x30) return false; if (a.sz <= 16) return false; }
  else if (a.sz > 4) return false;
  if (b.sz > 8) return false;
  if (!isFrameDecl(a)) return false;
  if (!isStaticD(b)) return false;
  return inWC(a.obj);
}
