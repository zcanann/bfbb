// R-Wn0: clause W for named statics: a store to an escaping declared frame object is ordered after an earlier WHOLE load (any opcode) of a named static of <= 4 bytes
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (a.store || !b.store) return -1;
  if (!(isStaticD(a) && !isLit(a) && a.k == 0 && a.sz <= 4)) return -1;
  if (!(isFrameDecl(b) && inWC(b.obj))) return -1;
  return 1;
}
