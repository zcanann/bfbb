// R-PSDs: as PSD but the direct-symbol side is the store
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  function ok(x, y) { return x.store && x.dir && !x.ptr && isStaticD(x) && !isLit(x) && y.ptr && isStaticD(y) && !isLit(y); }
  return (ok(a, b) || ok(b, a)) ? 1 : -1;
}
