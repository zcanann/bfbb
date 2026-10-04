// R-PSDb: a direct (symbolic-operand) access to a named static vs a base-register (pointer-analysis) access to a different named static; one a store
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (!(a.store || b.store)) return -1;
  function ok(x, y) { return x.dir && isStaticD(x) && !isLit(x) && !y.dir && isStaticD(y) && !isLit(y) && !x.obj.equals(y.obj); }
  return (ok(a, b) || ok(b, a)) ? 1 : -1;
}
