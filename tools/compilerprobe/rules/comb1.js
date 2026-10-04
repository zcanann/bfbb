// R-PSD: a direct-symbol (sda21) access to a named static vs a pointer-op access whose alias names a different named static; at least one is a store
function RULE_psd(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (!(a.store || b.store)) return -1;
  function ok(x, y) { return x.dir && !x.ptr && isStaticD(x) && !isLit(x) && y.ptr && isStaticD(y) && !isLit(y); }
  return (ok(a, b) || ok(b, a)) ? 1 : -1;
}
// VN-Dw: a direct-symbol store to a named static kills cached values of WHOLE aliases of other named statics
function VNSTORE_vndw(s, ins) { if (ins.isNull()) return false; var d = D(ins); return d.dir && !d.ptr && s.h == 5 && s.st == "st"; }
function VNKILL_vndw(s, x) { return x.k == 0 && x.h == 5 && x.st == "st" && !x.obj.equals(s.obj); }
// R-L8s: clause C+ admits an 8-byte literal load (lfd) on the load side
function RULE_l8s(pa, pb, r, site) {
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
  return (ok(a, b) || ok(b, a)) ? 1 : -1;
}
// R-L8v: clause V's literal kill also kills 8-byte literals (whole-static or escaping-frame store base, as V)
function VNSTORE_l8v(s, ins) { if (s.k != 0 || !s.obj || s.obj.isNull()) return false;
  if (s.h == 5 && s.st != "pso") return true;
  if (s.h == 0x10005 && s.obj.add(0x18).readU32() != 0 && inWC(s.obj)) return true;
  return false; }
function VNKILL_l8v(s, x) { return x.k == 0 && x.h == 5 && x.st == "lit" && x.sz == 8; }

function RULE(pa, pb, r, site) { var x = RULE_psd(pa, pb, r, site); if (x >= 0) return x; return RULE_l8s(pa, pb, r, site); }
function VNSTORE(s, ins) { s.m1 = VNSTORE_vndw(s, ins); s.m2 = VNSTORE_l8v(s, ins); return s.m1 || s.m2; }
function VNKILL(s, x) { return (s.m1 && VNKILL_vndw(s, x)) || (s.m2 && VNKILL_l8v(s, x)); }
