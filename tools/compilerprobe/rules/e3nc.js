// R-E3nc: clause E3n ignores the load's 0x40 flag (the store-side const exemption stays)
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (!(b.f & 0x40)) return -1;
  return e3n(a, b, a.f, b.f & ~0x40) ? 1 : -1;
}
