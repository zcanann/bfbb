// E3t: clause E3n (gated on address-taken frame object) also for SUBRANGE loads of a small (<=8 byte) static
// whose whole object is a literal-pool template ('@' name) [MODE lit] or any static [MODE st].
var MODE = "lit";
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  function ok(s, l) {
    if (s.k < 0 || l.k < 0 || s.k == 2 || l.k == 2) return false;
    if (l.k != 1) return false;                  // the new case only: subrange load
    if (!s.store || l.store) return false;
    if ((s.f & ~0x20) != 4) return false;
    if (l.f != 2) return false;
    if (s.f & 0x20) { if (s.op >= 0x28 && s.op <= 0x30) return false; if (s.sz <= 16) return false; }
    else if (s.sz > 4) return false;
    if (!isFrameDecl(s) || !inWC(s.obj)) return false;
    if (!isStaticD(l)) return false;
    if (MODE == "lit" && !isLit(l)) return false;
    if (smallStatic(l) == null) return false;
    var ws = wholeSize(s);
    return ws > 8;
  }
  return ok(a, b) ? 1 : -1;
}

function wholeSize(d) {
  if (d.k == 0) return d.sz;
  var m = d.A.add(8).readPointer();
  while (!m.isNull()) {
    var al = m.add(8).readPointer();
    if (!al.isNull() && al.add(0x2c).readU8() == 0 && al.add(0x10).readPointer().equals(d.obj)) return al.add(0x18).readU32();
    m = m.add(4).readPointer();
  }
  return -1;
}
