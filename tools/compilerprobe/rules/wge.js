// R-WE: clause W mirrored on E3n's object class: a store (<= 4 bytes) to an escaping declared frame object is ordered after an
//       earlier WHOLE load (any opcode) of any static (named or literal) of <= 8 bytes
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (a.store || !b.store) return -1;
  if (!(isStaticD(a) && a.k == 0 && a.sz <= 8)) return -1;
  if (b.k == 2 || b.sz > 4) return -1;
  if (!(isFrameDecl(b) && inWC(b.obj))) return -1;
  return 1;
}
