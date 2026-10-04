// R-PS: a pointer-op access whose alias names a static may alias any other static access (one side a store)
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if (!(a.store || b.store)) return -1;
  if (a.ptr && isStaticD(a) && !isLit(a) && isStaticD(b) && !isLit(b)) return 1;
  if (b.ptr && isStaticD(b) && !isLit(b) && isStaticD(a) && !isLit(a)) return 1;
  return -1;
}
