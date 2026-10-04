// R-DSF1: a direct store to a named static is ordered before a later store to an escaping declared frame object (any entry)
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (!(a.store && a.dir && isStaticD(a) && !isLit(a))) return -1;
  if (!(b.store && isFrameDecl(b) && inWC(b.obj))) return -1;
  return 1;
}
