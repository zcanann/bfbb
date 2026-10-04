// R-DSF2: a direct store to a named static is ordered before any later store to a declared frame object (escaping or not)
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (!(a.store && a.dir && isStaticD(a) && !isLit(a))) return -1;
  if (!(b.store && isFrameDecl(b))) return -1;
  return 1;
}
