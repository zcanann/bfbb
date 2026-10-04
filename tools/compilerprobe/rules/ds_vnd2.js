// R-DS: a direct-operand store to a named static is ordered before every later access to
//   (a) an address-taken declared frame object, or
//   (b) a different named static reached through a register (pointer-analysis alias), unless that access is a plain (non-fIsPtrOp) store
function RULE_ds(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (!(a.store && a.dir && isStaticD(a) && !isLit(a))) return -1;
  if (isFrameDecl(b) && inWC(b.obj)) return 1;
  if (!b.dir && isStaticD(b) && !isLit(b) && !b.obj.equals(a.obj) && (!b.store || b.ptr)) return 1;
  return -1;
}
// VN-Dw: a direct-symbol store to a named static kills cached values of WHOLE aliases of other named statics
function VNSTORE_a(s, ins) { if (ins.isNull()) return false; var d = D(ins); return d.dir && !d.ptr && s.h == 5 && s.st == "st"; }
function VNKILL_a(s, x) { return x.k == 0 && x.h == 5 && x.st == "st" && !x.obj.equals(s.obj); }
// VN-Df: a direct store to a named static also kills cached values of address-taken declared frame objects
function VNSTORE_b(s, ins) { if (ins.isNull()) return false; var d = D(ins); return d.dir && !d.ptr && s.h == 5 && s.st == "st"; }
function VNKILL_b(s, x) { return x.k >= 0 && x.k != 2 && x.h == 0x10005 && x.obj.add(0x18).readU32() != 0 && inWC(x.obj); }

function RULE(pa, pb, r, site) { return RULE_ds(pa, pb, r, site); }
function VNSTORE(s, ins) { return VNSTORE_a(s, ins); }
function VNKILL(s, x) { return VNKILL_a(s, x) || VNKILL_b(s, x); }
