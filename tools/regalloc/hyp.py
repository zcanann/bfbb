"""hyp.py cap.json[...]: test alternative vreg-NUMBERING hypotheses against the target colouring.

Each capture must come from diag.py (it carries the '@' temp causes and a .tgt file next to it).
For every hypothesis the webs are re-ranked (only the numbering changes; graph and costs stay),
the GC/2.0p1a simplify/select is replayed, and the number of target-mapped webs that still differ
is printed. A hypothesis that fixes residues AND keeps already-matching functions at 0 would point
at a compiler-build difference rather than at the source.
"""
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from replay import simplify, colour


def group_of(n, temps):
    nm = n['name']
    if not nm:
        return 'objless'
    if not nm.startswith('@'):
        return 'source'
    k = temps.get(nm, ['?'])[0]
    if k.startswith('INLINE'):
        return 'inline'
    if 'Second_pass' in k:
        return 'cse'
    if 'EvaluateConditiona' in k:
        return 'split'
    if 'FindLoops' in k:
        return 'loop'
    if 'LoopUnroller' in k:
        return 'unroll'
    if 'linearize' in k:
        return 'linearize'
    return 'other'


def reorder(live, N, pick, key):
    """Permute the ranks of the webs selected by pick() among their own slots, ordered by key()."""
    sel = [v for v in live if pick(v)]
    slots = sorted(sel)
    new = sorted(sel, key=key)
    return {v: s for v, s in zip(new, slots)}


def hyps(c, temps):
    N = c['nodes']
    live = sorted(i for i in N if i >= c['nreal'] and not N[i]['flags'] & 4)
    g = {v: group_of(N[v], temps) for v in live}
    num = {v: int(N[v]['name'][1:]) for v in live if N[v]['name'] and re.match(r'@\d+$', N[v]['name'])}
    H = {'base': {}}
    H['all@ forward'] = reorder(live, N, lambda v: v in num, lambda v: num[v])
    for grp in ('cse', 'split', 'loop', 'unroll', 'linearize', 'inline', 'source', 'objless'):
        H[grp + ' reversed'] = reorder(live, N, lambda v, grp=grp: g[v] == grp, lambda v: -v)
    # IRO temps (all kinds) ahead of inline objects, keeping each family's internal order
    fam = {'cse': 0, 'loop': 1, 'unroll': 2, 'split': 3, 'linearize': 4, 'other': 5, 'inline': 6}
    H['inline before IRO'] = reorder(live, N, lambda v: g[v] in fam,
                                     lambda v: (0 if g[v] == 'inline' else 1, v))
    H['source after @'] = reorder(live, N, lambda v: g[v] in fam or g[v] == 'source',
                                  lambda v: (1 if g[v] == 'source' else 0, v))
    return H


def main():
    for path in sys.argv[1:]:
        caps = json.load(open(path))
        for c in caps:
            try:
                spec = open(path + '.tgt%d' % c['cls']).read().strip()
            except OSError:
                continue
            tgt = {int(a): int(b) for a, b in (x.split(':') for x in spec.split(',') if x)}
            c['nodes'] = {int(k): v for k, v in c['nodes'].items()}
            if not tgt or not all(v in c['nodes'] for v in tgt):
                continue
            K = 29 if c['cls'] == 4 else 32
            mask0 = 0x1ff9 if c['cls'] == 4 else 0x3fff
            temps = c.get('temps', {})
            res = []
            for name, rank in hyps(c, temps).items():
                col = colour(c, simplify(c, K, rank), mask0)
                res.append('%s=%d' % (name, sum(col.get(v) != t for v, t in tgt.items())))
            print('%-40s cls%d  %s' % (c['fn'], c['cls'], '  '.join(res)))


if __name__ == '__main__':
    main()
