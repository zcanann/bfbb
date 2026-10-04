// R-PSD: a direct-symbol (sda21) access to a named static vs a pointer-op access whose alias names a different named static; at least one is a store
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (!(a.store || b.store)) return -1;
  function ok(x, y) { return x.dir && !x.ptr && isStaticD(x) && !isLit(x) && y.ptr && isStaticD(y) && !isLit(y); }
  return (ok(a, b) || ok(b, a)) ? 1 : -1;
}
// VN-Dw: a direct-symbol store to a named static kills cached values of WHOLE aliases of other named statics
function VNSTORE(s, ins) { if (ins.isNull()) return false; var d = D(ins); return d.dir && !d.ptr && s.h == 5 && s.st == "st"; }
function VNKILL(s, x) { return x.k == 0 && x.h == 5 && x.st == "st" && !x.obj.equals(s.obj); }
