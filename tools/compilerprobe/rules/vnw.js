// VN-W: a store to a WHOLE named static kills cached values of every other named (non-literal) static
function VNSTORE(s, ins) { return s.k == 0 && s.h == 5 && s.st == "st"; }
function VNKILL(s, x) { return x.k >= 0 && x.k != 2 && x.h == 5 && x.st == "st" && !x.obj.equals(s.obj); }
