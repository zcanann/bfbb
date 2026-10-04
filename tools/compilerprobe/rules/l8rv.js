// R-L8r (RAW only): clause C+ admits an 8-byte literal load (lfd) on the load side
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (a.k != 0 || b.k != 0) return -1;
  function ok(x, y) { // x store, y literal load of 8 bytes
    if (!x.store || y.store || !isLit(y) || y.sz != 8) return false;
    if (x.op == y.op) return false;
    if ((x.f & ~0xa6) != 0 || !isStaticD(x)) return false;
    if ((y.f & ~0xa6) != 0) return false;
    return true;
  }
  return ok(a, b) ? 1 : -1;
}
// R-L8v: clause V's literal kill also kills 8-byte literals (whole-static or escaping-frame store base, as V)
function VNSTORE(s, ins) { if (s.k != 0 || !s.obj || s.obj.isNull()) return false;
  if (s.h == 5 && s.st != "pso") return true;
  if (s.h == 0x10005 && s.obj.add(0x18).readU32() != 0 && inWC(s.obj)) return true;
  return false; }
function VNKILL(s, x) { return x.k == 0 && x.h == 5 && x.st == "lit" && x.sz == 8; }
