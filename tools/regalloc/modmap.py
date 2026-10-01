"""modmap.py addr...: guess the source module (Xxx.c assert-string xrefs) of GC/2.0p1a code addresses."""
import os, re, struct, sys, bisect
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pe import PE
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
p = PE(os.path.join(ROOT, 'build/compilers/GC/2.0p1a/mwcceppc.exe'))
b = p.b
tb, t = p.text()
refs = []
for m in re.finditer(rb'([A-Za-z][A-Za-z0-9_]{2,40}\.[ch])\x00', b):
    va = p.off2va(m.start())
    if not va:
        continue
    pat = struct.pack('<I', va)
    i = t.find(pat)
    while i >= 0:
        refs.append((tb + i, m.group(1).decode()))
        i = t.find(pat, i + 1)
refs.sort()
keys = [a for a, _ in refs]


def guess(a):
    i = bisect.bisect_right(keys, a)
    lo = refs[i - 1] if i else None
    hi = refs[i] if i < len(refs) else None
    out = []
    if lo:
        out.append('%s-%x' % (lo[1], a - lo[0]))
    if hi:
        out.append('%s+%x' % (hi[1], hi[0] - a))
    return ' '.join(out)


if __name__ == '__main__':
    for x in sys.argv[1:]:
        print(x, guess(int(x, 16)))
