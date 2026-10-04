// R-PSR: a direct store to a named static is ordered before a later non-direct access to a different named static
//        that is a load (any base register) or a pointer-op store
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (!(a.store && a.dir && isStaticD(a) && !isLit(a))) return -1;
  if (!(!b.dir && isStaticD(b) && !isLit(b) && !b.obj.equals(a.obj))) return -1;
  if (b.store && !b.ptr) return -1;
  return 1;
}
// VN-Dw: a direct-symbol store to a named static kills cached values of WHOLE aliases of other named statics
function VNSTORE(s, ins) { if (ins.isNull()) return false; var d = D(ins); return d.dir && !d.ptr && s.h == 5 && s.st == "st"; }
function VNKILL(s, x) { return x.k == 0 && x.h == 5 && x.st == "st" && !x.obj.equals(s.obj); }
