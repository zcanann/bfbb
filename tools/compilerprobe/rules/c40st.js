// R-C40st: static clauses ignore 0x40 when both sides are statics (frames keep the const exemption)
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (!((a.f | b.f) & 0x40)) return -1;
  if (!(isStaticD(a) && isStaticD(b))) return -1;
  return staticClauses(a, b, a.f & ~0x40, b.f & ~0x40) ? 1 : -1;
}
