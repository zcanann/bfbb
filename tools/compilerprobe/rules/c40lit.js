// R-C40lit: clauses C+/C/A ignore flag 0x40 when the store is to a static and the load is a literal
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (!((a.f | b.f) & 0x40)) return -1;
  if (!(a.store && isStaticD(a) && !b.store && isLit(b))) return -1;
  return staticClauses(a, b, a.f & ~0x40, b.f & ~0x40) ? 1 : -1;
}
