// R-DS: a direct-operand store to a named static is ordered before every later access to
//   (a) an address-taken declared frame object, or
//   (b) a different named static reached through a register (pointer-analysis alias), unless that access is a plain (non-fIsPtrOp) store
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (!(a.store && a.dir && isStaticD(a) && !isLit(a))) return -1;
  if (isFrameDecl(b) && inWC(b.obj)) return 1;
  if (!b.dir && isStaticD(b) && !isLit(b) && !b.obj.equals(a.obj) && (!b.store || b.ptr)) return 1;
  return -1;
}
