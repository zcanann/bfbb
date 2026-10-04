// R-ES8: E3n via S's unit: a store to an escaping declared frame object is ordered before a later load (any opcode) of a static object of <= 8 bytes (whole or field)
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (!a.store || b.store) return -1;
  if (!small8(b)) return -1;
  if (!(isFrameDecl(a) && inWC(a.obj))) return -1;
  return 1;
}
