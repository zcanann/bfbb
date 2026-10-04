// R-PSR2: as PSD but RAW/WAW only (direct static store first), plus non-ptr base-register loads
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (!(a.store && a.dir && !a.ptr && isStaticD(a) && !isLit(a))) return -1;
  if (!(isStaticD(b) && !isLit(b) && !b.obj.equals(a.obj))) return -1;
  if (b.ptr) return 1;
  if (!b.store && !b.dir) return 1;
  return -1;
}
