// R-Wg4: clause W generalised: a store to an escaping declared frame object may not pass an earlier load of a named static or literal of <= 4 bytes (any opcode)
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (a.store || !b.store) return -1;
  if (!(isStaticD(a) && a.k >= 0 && a.k != 2 && a.sz <= 4)) return -1;
  if (!(isFrameDecl(b) && inWC(b.obj))) return -1;
  return 1;
}
