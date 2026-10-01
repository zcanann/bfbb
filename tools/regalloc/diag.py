"""diag.py <src.c> <unit-path> <fn> [--out dir] [--pairs]: one-shot register-residue diagnosis.

1. wcap capture (graph + '@' temp causes) of <fn>;
2. per register class, tmap-style target colours from tools/fdiff.py;
3. replay check, then a rank-move search: which webs must move (single move, else pair) for the
   captured graph to colour exactly like the target.
Prints web table rows for every web involved.  <unit-path> as fdiff takes it (rwsdk/src/babintex).
"""
import os, sys, json, subprocess, itertools
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import wcap
from replay import simplify, colour

src, unit, fn = sys.argv[1:4]
os.environ['TMAP_SRC'] = os.path.abspath(src)  # target colours from THIS source (may be a copy)
out = sys.argv[sys.argv.index('--out') + 1] if '--out' in sys.argv else os.environ.get('TEMP', '.')
os.makedirs(out, exist_ok=True)
caps, temps = wcap.run(src, unit.split('/')[-1], fn)
jp = os.path.join(out, fn + '.json')
json.dump(caps, open(jp, 'w'))


def kind(n, nreal):
    nm = n['name']
    if not nm:
        return 'objless'
    if nm.startswith('@'):
        return temps.get(nm, ('?',))[0]
    return 'SOURCE'


def pop_inversions(c, tgt, cls, budget=2000000):
    """Closest (current-order-first DFS) callee-saved pop order that yields the target colours."""
    N = c['nodes']
    lo = 13 if cls == 4 else 14
    cs = [i for i in c['order'] if N[i]['final'] >= lo or tgt.get(i, 0) >= lo]
    want = {v: tgt.get(v, N[v]['final']) for v in cs}
    nbs = {v: set(N[v]['nb']) for v in cs}
    steps = [0]
    found = []

    def rec(order, col, claimed, nextnv):
        steps[0] += 1
        if found or steps[0] > budget:
            return
        if len(order) == len(cs):
            found.append(list(order))
            return
        for v in cs:
            if v in col:
                continue
            used = {col[n] for n in nbs[v] if n in col}
            free = [r for r in sorted(claimed) if r not in used]
            if free:
                r, nn, cl = free[0], nextnv, claimed
            else:
                r, nn, cl = nextnv, nextnv - 1, claimed | {nextnv}
            if r != want[v]:
                continue
            col[v] = r
            order.append(v)
            rec(order, col, cl, nn)
            order.pop()
            del col[v]
    rec([], {}, frozenset(), 31 if cls == 4 else 31)
    if not found:
        print('   target colouring NOT reachable by any callee-saved pop order of this graph%s' % (
            ' (search budget hit)' if steps[0] > budget else ' (graph differs)'))
        return None
    return found[0], cs


for c in caps:
    cls = c['cls']
    K = 29 if cls == 4 else 32
    mask0 = 0x1ff9 if cls == 4 else 0x3fff
    N = {int(k): v for k, v in c['nodes'].items()}
    c['nodes'] = N
    r = subprocess.run([sys.executable, os.path.join(HERE, 'tmap.py'), jp, fn, unit, str(cls)],
                       capture_output=True, text=True, cwd=HERE)
    try:
        spec = open(jp + '.tgt').read().strip()
        open(jp + '.tgt%d' % cls, 'w').write(spec)
    except OSError:
        spec = ''
    tgt = {int(a): int(b) for a, b in (x.split(':') for x in spec.split(',') if x)}
    if not tgt:
        continue
    pops = simplify(c, K)
    col = colour(c, c['order'], mask0)
    ok = pops == c['order'] and all(col[i] == N[i]['final'] for i in c['order'])
    bad = {v: t for v, t in tgt.items() if v in col and col[v] != t}
    print('== %s cls %d: %s; replay %s; %d/%d target-mapped webs differ' % (
        fn, cls, r.stdout.splitlines()[0] if r.stdout else '?', 'exact' if ok else 'MISMATCH', len(bad), len(tgt)))
    if not bad:
        continue
    for v in sorted(bad):
        n = N[v]
        print('   v%-4d %-10s %-28s ours=%-3d target=%d' % (v, n['name'] or '-', kind(n, c['nreal']), col[v], bad[v]))

    def score(rank):
        cc = colour(c, simplify(c, K, rank), mask0)
        return sum(cc.get(v) != t for v, t in tgt.items())
    live = sorted(i for i in N if i >= c['nreal'] and not N[i]['flags'] & 4)
    slots = sorted(set([x - 0.5 for x in live] + [live[-1] + 0.5]))
    sing = [(v, s) for v in live for s in slots if score({v: s}) == 0]
    if sing:
        by = {}
        for v, s_ in sing:
            by.setdefault(v, []).append(s_)
        for v, ss in by.items():
            print('   SINGLE: move v%d %s (%s) to rank in [%s..%s] (%d slots)' % (
                v, N[v]['name'] or '-', kind(N[v], 0), min(ss), max(ss), len(ss)))
        continue
    inv = pop_inversions(c, tgt, cls)
    if inv is not None:
        sol, cs = inv
        cur = [i for i in c['order'] if i in cs]
        print('   no single rank move. current CS pop order: %s' % ' '.join('v%d' % v for v in cur))
        print('   closest target pop order:              %s' % ' '.join('v%d' % v for v in sol))
        ps = {v: k for k, v in enumerate(sol)}
        for a, b in itertools.combinations(cur, 2):
            if ps[a] > ps[b]:
                print('   INVERSION: v%d %s (%s) must pop before v%d %s (%s)' % (
                    b, N[b]['name'] or '-', kind(N[b], 0), a, N[a]['name'] or '-', kind(N[a], 0)))
    if '--pairs' not in sys.argv:
        continue
    cand = sorted(set(bad) | {i for i in c['order'] if N[i]['final'] >= (13 if cls == 4 else 14)})
    # reduced slot set: just above/below every candidate, every region boundary, and both ends
    regions = []
    prev = None
    for v in live:
        k = kind(N[v], 0).split('(')[0].split(':')[0]
        if k != prev:
            regions.append(v - 0.5)
            prev = k
    rs = sorted(set([x - 0.5 for x in cand] + [x + 0.5 for x in cand] + regions + [live[0] - 0.5, live[-1] + 0.5]))
    found = {}
    for v, w in itertools.combinations(cand, 2):
        for s_ in rs:
            for t in rs:
                if score({v: s_, w: t}) == 0:
                    found.setdefault((v, w), []).append((s_, t))
    if not found:
        print('   no pair rank move reaches the target among %s' % cand)
    for (v, w), st in found.items():
        print('   PAIR: v%d %s(%s) & v%d %s(%s): %d placements, e.g. %s' % (
            v, N[v]['name'] or '-', kind(N[v], 0), w, N[w]['name'] or '-', kind(N[w], 0), len(st), st[:3]))
