// R-PSD: a direct-symbol (sda21) access to a named static vs a pointer-op access whose alias names a different named static; at least one is a store
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (!(a.store || b.store)) return -1;
  function ok(x, y) { return x.dir && !x.ptr && isStaticD(x) && !isLit(x) && y.ptr && isStaticD(y) && !isLit(y); }
  return (ok(a, b) || ok(b, a)) ? 1 : -1;
}
