// E3t: clause E3n (gated on address-taken frame object) also for SUBRANGE loads of a small (<=8 byte) static
// whose whole object is a literal-pool template ('@' name) [MODE lit] or any static [MODE st].
var MODE = "lit";
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  function ok(s, l) {
    if (s.k < 0 || l.k < 0 || s.k == 2 || l.k == 2) return false;
    if (l.k != 1) return false;                  // the new case only: subrange load
    if (!s.store || l.store) return false;
    if ((s.f & ~0x20) != 4) return false;
    if (l.f != 2) return false;
    if (s.f & 0x20) { if (s.op >= 0x28 && s.op <= 0x30) return false; if (s.sz <= 16) return false; }
    else if (s.sz > 4) return false;
    if (!isFrameDecl(s) || !inWC(s.obj)) return false;
    if (!isStaticD(l)) return false;
    if (MODE == "lit" && !isLit(l)) return false;
    return smallStatic(l) != null;
  }
  return ok(a, b) ? 1 : -1;
}
