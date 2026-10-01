"""webs.py cap.json fn [cls]: per vreg: name, colour, pop pos, first defs/uses (compact)."""
import json, sys, os
HERE = os.path.dirname(os.path.abspath(__file__))
ops = {int(k): v for k, v in json.load(open(os.path.join(HERE, 'opmap.json'))).items()}
caps = json.load(open(sys.argv[1]))
fn = sys.argv[2]
cls = int(sys.argv[3]) if len(sys.argv) > 3 else 4
c = [x for x in caps if x['fn'] == fn and x['cls'] == cls][0]
N = {int(k): v for k, v in c['nodes'].items()}
pos = {v: i for i, v in enumerate(c['order'])}
occ = {}
for bi, b in enumerate(c['blocks']):
    for ii, (op, fl, args) in enumerate(b):
        rv = [a[2] for a in args if a[0] == 0 and a[1] == cls]
        txt = '%s %s' % (ops.get(op, op), ','.join(('v%d' % a[2]) if a[0] == 0 else str(a[3]) for a in args))
        for v in set(rv):
            occ.setdefault(v, []).append('b%d:%s' % (bi, txt))
lim = int(sys.argv[sys.argv.index('--n') + 1]) if '--n' in sys.argv else 3
for v in sorted(N):
    if v < c['nreal']:
        continue
    n = N[v]
    if '--named' in sys.argv and not n['name']:
        continue
    if '--cs' in sys.argv and not (n['final'] >= 13 and v in pos):
        continue
    print('v%-4d %-8s col=%-3s pop=%-4s deg=%-3d fl=%-3x %s' % (v, n['name'] or '-', n['final'], pos.get(v, '--'),
          len(n['nb']), n['flags'], ' | '.join(occ.get(v, [])[:lim])))
