// R-PA (scheduler half): a pointer-op access whose alias names a named static is treated as worst_case
function PRE(pa, pb, site) {
  var L = [], wc = WCP.readPointer();
  var a = D(pa), b = D(pb);
  if (a.ptr && isStaticD(a) && !isLit(a)) L.push([pa, wc]);
  if (b.ptr && isStaticD(b) && !isLit(b)) L.push([pb, wc]);
  return L;
}
