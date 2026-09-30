#!/usr/bin/env python3
"""Compare our .text definition order against the target's.

CodeWarrior emits a top-level function and then its pending inlines, so a
target object's `.text` order is a direct read of retail's *source* order.
Where ours diverges, the literal pool and `@stringBase0` diverge with it --
and those are compared by offset, so one misplaced function can leave a dozen
neighbours permanently non-matching for no reason of their own.

  order.py <unit-frag>            where the orders diverge, and by how much
  order.py <unit-frag> --runs     the target order as runs contiguous in ours
  order.py <unit-frag> --moves    functions that must move, in target order
  order.py <unit-frag> --top      hide header inlines, show only the unit's
                                  own functions (the ones you can actually
                                  reorder by editing the .cpp)

`--top` matters: most of the tail of a modern unit is template and header
inlines, and their position is decided by first-use order inside the function
that pulls them in, not by anything you can write in the .cpp. Reordering the
top-level functions correctly drags the inlines into place for free, so those
rows are noise when you are planning the edit.

Reads the objects from the last `ninja`, so build first if the source moved.
"""
import os
import re
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INLINE_HINT = re.compile(r"tier_queue|static_queue|fixed_queue|8iterator|"
                         r"allocator|__as__|__rf__|__eq__|__ne__|__pl__|"
                         r"__mi__|__ml__|__mm__|__vc__|__apl__|__ami__")


def text_order(path):
    """Function symbols in .text address order."""
    if not os.path.exists(path):
        raise SystemExit("no object at %s -- run ninja first" % path)
    f = open(path, 'rb').read()
    if f[:4] != b'\x7fELF':
        raise SystemExit("not an ELF: %s" % path)
    shoff = struct.unpack('>I', f[0x20:0x24])[0]
    shent = struct.unpack('>H', f[0x2e:0x30])[0]
    shnum = struct.unpack('>H', f[0x30:0x32])[0]
    secs = []
    for i in range(shnum):
        o = shoff + i * shent
        v = struct.unpack('>IIIIIIIIII', f[o:o + 40])
        secs.append(dict(name=v[0], typ=v[1], off=v[4], link=v[6], idx=i))
    base = secs[struct.unpack('>H', f[0x32:0x34])[0]]['off']
    for s in secs:
        e = f.index(b'\0', base + s['name'])
        s['nm'] = f[base + s['name']:e].decode()
    st = [s for s in secs if s['typ'] == 2]
    if not st:
        return []
    st = st[0]
    strt = secs[st['link']]
    text_idx = {s['idx'] for s in secs if s['nm'].startswith('.text')}
    rows = []
    for i in range(st['size'] // 16 if 'size' in st else 0):
        pass
    # size field was not captured above; re-read it directly.
    o = shoff + st['idx'] * shent
    st_size = struct.unpack('>IIIIIIIIII', f[o:o + 40])[5]
    for i in range(st_size // 16):
        o = st['off'] + i * 16
        nameoff, value, size, info, other, shndx = \
            struct.unpack('>IIIBBH', f[o:o + 16])
        if (info & 0xf) != 2 or shndx not in text_idx or not size:
            continue
        e = f.index(b'\0', strt['off'] + nameoff)
        rows.append((shndx, value, f[strt['off'] + nameoff:e].decode()))
    rows.sort()
    return [n for _, _, n in rows]


def resolve(frag):
    import json
    cfg = json.load(open(os.path.join(ROOT, "objdiff.json")))
    hits = [u for u in cfg["units"] if frag in u["name"]]
    if len(hits) != 1:
        exact = [u for u in hits if u["name"].endswith(frag)]
        if len(exact) != 1:
            raise SystemExit("ambiguous or no match: " +
                             ", ".join(u["name"] for u in hits[:12]))
        hits = exact
    name = hits[0]["name"]
    rel = name.split('/', 1)[1] if '/' in name else name
    return rel


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('-')]
    if not args:
        raise SystemExit(__doc__)
    rel = resolve(args[0])
    tgt = text_order(os.path.join(ROOT, 'build/GQPE78/obj/%s.o' % rel))
    our = text_order(os.path.join(ROOT, 'build/GQPE78/src/%s.o' % rel))
    top_only = '--top' in sys.argv
    if top_only:
        tgt = [n for n in tgt if not INLINE_HINT.search(n)]
        our = [n for n in our if not INLINE_HINT.search(n)]

    pos = {n: i for i, n in enumerate(our)}
    common = [n for n in tgt if n in pos]
    print("%s: %d target functions, %d ours, %d in both%s"
          % (rel, len(tgt), len(our), len(common),
             "  (header inlines hidden)" if top_only else ""))

    ourc = [n for n in our if n in set(common)]
    first = next((i for i, (a, b) in enumerate(zip(common, ourc)) if a != b),
                 None)
    if first is None:
        print("ORDER MATCHES for every shared symbol")
        return
    print("order agrees for 0..%d, diverges at %d: target has %s, we have %s"
          % (first - 1, first, common[first], ourc[first]))

    n, m = len(common), len(ourc)
    dp = [[0] * (m + 1) for _ in range(n + 1)]
    for i in range(n - 1, -1, -1):
        for j in range(m - 1, -1, -1):
            dp[i][j] = (dp[i + 1][j + 1] + 1) if common[i] == ourc[j] \
                else max(dp[i + 1][j], dp[i][j + 1])
    keep, i, j = set(), 0, 0
    while i < n and j < m:
        if common[i] == ourc[j]:
            keep.add(common[i])
            i += 1
            j += 1
        elif dp[i + 1][j] >= dp[i][j + 1]:
            i += 1
        else:
            j += 1
    moves = [x for x in common if x not in keep]
    print("longest common subsequence %d of %d -> %d must move"
          % (len(keep), len(common), len(moves)))

    runs = []
    cur = [common[0]]
    for prev, nx in zip(common, common[1:]):
        (cur.append(nx) if pos[nx] == pos[prev] + 1
         else (runs.append(cur), cur.clear(), cur.append(nx)))
    runs.append(list(cur))
    print("%d contiguous-in-ours runs (avg %.1f)"
          % (len(runs), len(common) / max(1, len(runs))))

    if '--runs' in sys.argv:
        print()
        print("%5s %5s %5s  %s" % ("tgt@", "our@", "len", "first in run"))
        start = 0
        for r in runs:
            print("%5d %5d %5d  %s" % (start, pos[r[0]], len(r), r[0]))
            start += len(r)
    if '--moves' in sys.argv:
        print()
        for x in moves:
            print("  tgt@%-4d ours@%-4d  %s" % (common.index(x), pos[x], x))


main()
