// R-C40ld: every clause ignores the 0x40 flag of the LOAD side (store side unchanged)
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  var fa = a.f, fb = b.f;
  if (!a.store && (fa & 0x40)) fa &= ~0x40;
  if (!b.store && (fb & 0x40)) fb &= ~0x40;
  if (fa == a.f && fb == b.f) return -1;
  if (staticClauses(a, b, fa, fb)) return 1;
  if (e3n(a, b, fa, fb)) return 1;
  return -1;
}
