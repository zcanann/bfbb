// R-Wg4n: as Wg4 but only named (non-literal) statics
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (a.store || !b.store) return -1;
  if (!(isStaticD(a) && !isLit(a) && a.k >= 0 && a.k != 2 && a.sz <= 4)) return -1;
  if (!(isFrameDecl(b) && inWC(b.obj))) return -1;
  return 1;
}
