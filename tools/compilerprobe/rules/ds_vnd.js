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
// VN-Dw: a direct-symbol store to a named static kills cached values of WHOLE aliases of other named statics
function VNSTORE(s, ins) { if (ins.isNull()) return false; var d = D(ins); return d.dir && !d.ptr && s.h == 5 && s.st == "st"; }
function VNKILL(s, x) { return x.k == 0 && x.h == 5 && x.st == "st" && !x.obj.equals(s.obj); }
