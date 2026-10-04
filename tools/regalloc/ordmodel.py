"""ordmodel.py: evaluate alternative simplify/select/numbering rules of the colourer offline.

    ordmodel.py --res DIR --ctl DIR [--report build/GQPE78/report.json] [--models a,b] [-v]

--res: residue captures from `corpus.py residues` (DIR/<sym>.json + .json.tgt<cls>, target colours
       from retail).  --ctl: `corpus.py controls` captures (whole units); a function whose bare name
       is not among its unit's non-matching functions in report.json is a matched control, so retail
       colours == its captured colours.

For every model and every function the replayed colouring is compared with the target:
  residues: FIXED (all target-mapped webs agree; was not before), better/worse (count moved);
  controls: BROKEN (any web's colour or any earlier round's set of failed webs changes).
A function that spilled is captured once per colouring round.  Earlier rounds end with uncoloured
webs (spill code is then inserted); a model must reproduce the same failed set there, otherwise
its graph for the next round is unknown and the function counts as "changed" (residue: unknown).

Models (simplify = 0x508a20, select = colorgraph 0x508900 in GC/2.0p1a):
  base          the compiler: scan ascending, LIFO remainder, spill pick = first min cost/degree
  desc_after    after the first spill pick, every later scan is descending
  remlist       after each spill pick, first rescan only the remainder list (LIFO = descending)
  spill_lastmin spill pick keeps the LAST minimum of the LIFO walk (ties -> lowest vreg)
  spill_asc     spill pick walks the remainder ascending (ties -> lowest vreg)
  spill_deg     spill pick = highest current degree (ties as base)
  named_last    numbering: named webs (source variables, params excluded) rank after all temps
  named_last_ps named webs scan after temps only in passes after the first spill pick
  temps_fwd     numbering: '@' objects in creation order instead of reverse
  sel_high      select: reuse the HIGHEST already-claimed callee-saved register, not the lowest
  sel_volhigh   select: highest free volatile register instead of lowest
"""
import argparse
import collections
import glob
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import ra_common

FLT_MAX = 3.4028234663852886e+38


def kind(n):
    nm = n.get('name')
    if not nm:
        return 'objless'
    return 'temp' if nm.startswith('@') else 'named'


def spill_cost(c, i, deg):
    ns = c.get('nospill')
    n = c['nodes'][i]
    if ns is not None and n.get('f10', i) >= ns:
        return FLT_MAX
    return n['cost'] / max(deg[i], 1)


def simplify(c, K, m):
    N = c['nodes']
    nreal = c['nreal']
    live = [i for i in N if i >= nreal and not (N[i]['flags'] & 4)]
    rank = {}
    if m == 'named_last':
        rank = {i: (1, i) if kind(N[i]) == 'named' else (0, i) for i in live}
    elif m == 'temps_fwd':
        tv = sorted(i for i in live if kind(N[i]) == 'temp')
        rank = {i: (0, j) for i, j in zip(tv, reversed(tv))}
    order = sorted(live, key=lambda i: rank.get(i, (0, i)))
    order_ps = sorted(live, key=lambda i: ((1 if kind(N[i]) == 'named' else 0), i)) if m == 'named_last_ps' else order
    deg = {i: len(n['nb']) for i, n in N.items()}
    removed = set()
    stack = []
    spilled = False

    def push(i):
        for nb in N[i]['nb']:
            deg[nb] -= 1
        removed.add(i)
        stack.append(i)

    while True:
        while True:
            changed = False
            remaining = []          # appended in scan order; the compiler's list is LIFO
            seq = order
            if spilled and m == 'desc_after':
                seq = order[::-1]
            elif spilled and m == 'named_last_ps':
                seq = order_ps
            for i in seq:
                if i in removed:
                    continue
                if deg[i] < K:
                    push(i)
                    changed = True
                else:
                    remaining.append(i)
            if not changed:
                break
        if not remaining:
            break
        walk = list(reversed(remaining))
        if m == 'spill_asc':
            walk = sorted(remaining)
        best = cand = None
        for i in walk:
            sc = -deg[i] if m == 'spill_deg' else spill_cost(c, i, deg)
            if best is None or sc < best or (m == 'spill_lastmin' and sc == best):
                cand, best = i, sc
        push(cand)
        spilled = True
        if m == 'remlist':
            again = True
            while again:
                again = False
                for i in reversed(remaining):
                    if i not in removed and deg[i] < K:
                        push(i)
                        again = True
    return list(reversed(stack))


def colour(c, pops, mask0, m):
    nreal = c['nreal']
    N = c['nodes']
    col = {i: i for i in range(nreal)}
    nv = list(range(31, 13, -1))
    claimed = []
    mask = mask0
    for i in pops:
        av = mask
        for nb in N[i]['nb']:
            x = col.get(nb, -1)
            if x is not None and x != -1 and x < 32:
                av &= ~(1 << x)
        if av:
            vol = av & mask0
            cs = av & ~mask0
            if m == 'sel_high' and vol == 0:
                r = cs.bit_length() - 1
            elif m == 'sel_volhigh' and vol:
                r = vol.bit_length() - 1
            else:
                r = (av & -av).bit_length() - 1
        elif nv:
            r = nv.pop(0)
            mask |= 1 << r
        else:
            r = None
        col[i] = r
    return col


def classinfo(cls):
    return (29, 0x1ff9) if cls == 4 else (32, 0x3fff)


def run_rounds(rounds, m):
    """rounds: captures of one function+class in round order. Returns (final colours or None,
    rounds_ok) where rounds_ok says every non-final round reproduces its failed set."""
    ok = True
    for r in rounds[:-1]:
        K, m0 = classinfo(r['cls'])
        col = colour(r, simplify(r, K, m), m0, m)
        fail = {i for i in r['order'] if col.get(i) is None}
        want = {i for i in r['order'] if r['nodes'][i]['final'] in (-1, None) or r['nodes'][i].get('fflags', 0) & 1}
        if fail != want:
            ok = False
    last = rounds[-1]
    K, m0 = classinfo(last['cls'])
    return colour(last, simplify(last, K, m), m0, m), ok


def load(path):
    caps = json.load(open(path))
    for c in caps:
        c['nodes'] = {int(k): v for k, v in c['nodes'].items()}
    return caps


def group(caps):
    g = collections.OrderedDict()
    for c in caps:
        g.setdefault((c.get('fnidx', c['fn']), c['fn'], c['cls']), []).append(c)
    return g


MODELS = ['base', 'desc_after', 'remlist', 'spill_lastmin', 'spill_asc', 'spill_deg', 'named_last',
          'named_last_ps', 'temps_fwd', 'sel_high', 'sel_volhigh']


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--res')
    ap.add_argument('--ctl')
    ap.add_argument('--report', default=os.path.join(ra_common.ROOT, 'build/GQPE78/report.json'))
    ap.add_argument('--models', default=','.join(MODELS))
    ap.add_argument('-v', action='store_true')
    a = ap.parse_args()
    models = a.models.split(',')
    stat = {m: collections.Counter() for m in models}
    detail = collections.defaultdict(list)

    if a.res:
        for p in sorted(glob.glob(os.path.join(a.res, '*.json'))):
            if p.endswith('.diff.json'):
                continue
            caps = load(p)
            for (fi, fn, cls), rounds in group(caps).items():
                try:
                    spec = open(p + '.tgt%d' % cls).read().strip()
                except OSError:
                    continue
                tgt = {int(x): int(y) for x, y in (s.split(':') for s in spec.split(',') if s)}
                if not tgt or not all(v in rounds[-1]['nodes'] for v in tgt):
                    continue
                base = None
                for m in models:
                    col, ok = run_rounds(rounds, m)
                    d = sum(col.get(v) != t for v, t in tgt.items())
                    if m == 'base':
                        base = d
                        stat[m]['res_fns'] += 1
                        stat[m]['res_bad'] += d > 0
                        continue
                    if base is None:
                        continue
                    if not ok:
                        stat[m]['res_unknown'] += 1
                    elif d == 0 and base > 0:
                        stat[m]['FIXED'] += 1
                        detail[m].append('FIX %s cls%d (%d->0)' % (fn, cls, base))
                    elif d < base:
                        stat[m]['better'] += 1
                        detail[m].append('better %s cls%d (%d->%d)' % (fn, cls, base, d))
                    elif d > base:
                        stat[m]['worse'] += 1
    if a.ctl:
        rep = json.load(open(a.report))
        bad = collections.defaultdict(set)
        for u in rep['units']:
            stem = u['name'].split('/')[-1]
            for f in u.get('functions', []):
                if f.get('fuzzy_match_percent', 0) < 100:
                    nm = f['name']
                    pm = ra_common.parse_mangled(nm)
                    bad[stem].add(pm[0] if pm else nm)
        for p in sorted(glob.glob(os.path.join(a.ctl, '*.ctl.json'))):
            stem = os.path.basename(p)[:-len('.ctl.json')]
            caps = load(p)
            for (fi, fn, cls), rounds in group(caps).items():
                if fn in bad.get(stem, ()):
                    continue
                last = rounds[-1]
                if any(last['nodes'][i]['final'] in (-1, None) for i in last['order']):
                    continue    # capture glitch: last round should colour everything
                spill = len(rounds) > 1
                for m in models:
                    col, ok = run_rounds(rounds, m)
                    same = ok and all(col.get(i) == last['nodes'][i]['final'] for i in last['order'])
                    if m == 'base':
                        stat[m]['ctl_fns'] += 1
                        stat[m]['ctl_spill_fns'] += spill
                        if not same:
                            stat[m]['BASE_REPLAY_DIFF'] += 1
                            detail[m].append('base replay differs: %s %s cls%d' % (stem, fn, cls))
                        continue
                    if not same:
                        stat[m]['BROKEN'] += 1
                        stat[m]['broken_spill'] += spill
                        detail[m].append('BREAK %s %s cls%d%s' % (stem, fn, cls, ' (spills)' if spill else ''))
    for m in models:
        print('%-14s %s' % (m, ' '.join('%s=%d' % kv for kv in sorted(stat[m].items()))))
        if a.v:
            for l in detail[m][:40]:
                print('    ' + l)


if __name__ == '__main__':
    main()
