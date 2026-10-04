// VN-D: a direct-symbol store to a named static kills cached values of every other named (non-literal) static
function VNSTORE(s, ins) { if (ins.isNull()) return false; var d = D(ins); s.dir = d.dir; return d.dir && !d.ptr && s.h == 5 && s.st == "st"; }
function VNKILL(s, x) { return x.k >= 0 && x.k != 2 && x.h == 5 && x.st == "st" && !x.obj.equals(s.obj); }
