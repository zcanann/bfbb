// R-DSWC: a direct-operand store to a named static aliases every later access whose object is address-taken (worst_case member)
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (!(a.store && a.dir && isStaticD(a) && !isLit(a))) return -1;
  if (b.k < 0 || b.k == 2 || !b.obj || b.obj.isNull()) return -1;
  if (b.obj.equals(a.obj)) return -1;
  return inWC(b.obj) ? 1 : -1;
}
