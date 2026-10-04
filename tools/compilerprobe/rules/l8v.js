// R-L8v: clause V's literal kill also kills 8-byte literals (whole-static or escaping-frame store base, as V)
function VNSTORE(s, ins) { if (s.k != 0 || !s.obj || s.obj.isNull()) return false;
  if (s.h == 5 && s.st != "pso") return true;
  if (s.h == 0x10005 && s.obj.add(0x18).readU32() != 0 && inWC(s.obj)) return true;
  return false; }
function VNKILL(s, x) { return x.k == 0 && x.h == 5 && x.st == "lit" && x.sz == 8; }
