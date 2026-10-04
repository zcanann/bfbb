// R-C40: clauses C+/C/A/S ignore flag 0x40
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (!((a.f | b.f) & 0x40)) return -1;
  return staticClauses(a, b, a.f & ~0x40, b.f & ~0x40) ? 1 : -1;
}
