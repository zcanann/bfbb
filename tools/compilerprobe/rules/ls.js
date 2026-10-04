// R-LS: a function-local static (name has $) is a member of worst_case
function RULE(pa, pb, r, site) {
  if (r) return -1;
  var a = D(pa), b = D(pb);
  if ((a.wc && isLocalStatic(b)) || (b.wc && isLocalStatic(a))) return 1;
  return -1;
}
