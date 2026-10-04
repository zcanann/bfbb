// R-WS8: a store to an escaping declared frame object is ordered after an earlier load (any opcode) of a static object of <= 8 bytes (whole or field)
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (a.store || !b.store) return -1;
  if (!small8(a)) return -1;
  if (!(isFrameDecl(b) && inWC(b.obj))) return -1;
  return 1;
}
