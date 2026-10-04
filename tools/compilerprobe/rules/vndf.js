// VN-Df: a direct store to a named static also kills cached values of address-taken declared frame objects
function VNSTORE(s, ins) { if (ins.isNull()) return false; var d = D(ins); return d.dir && !d.ptr && s.h == 5 && s.st == "st"; }
function VNKILL(s, x) { return x.k >= 0 && x.k != 2 && x.h == 0x10005 && x.obj.add(0x18).readU32() != 0 && inWC(x.obj); }
