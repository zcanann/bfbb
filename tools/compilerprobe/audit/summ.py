"""summ.py <log.json>: firing counts per clause@entry (firings, distinct functions, units, keys)."""
import json, re, sys, collections
b = json.load(open(sys.argv[1]))
A = collections.defaultdict(lambda: [0, set(), set(), set()])
for un, r in b.items():
    for k, fm in (r.get("log") or {}).items():
        m = re.match(r"(LICM|[A-Za-z0-9+=]+@\d)", k)
        g = m.group(1)
        for f, c in fm.items():
            for gg in (g, g.split("@")[0] + "@*"):
                A[gg][0] += c; A[gg][1].add((un, f)); A[gg][2].add(un); A[gg][3].add(k)
for g in sorted(A):
    n, f, u, k = A[g]
    print("%-8s %7d firings %5d fns %4d units %4d keys" % (g, n, len(f), len(u), len(k)))
