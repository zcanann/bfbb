"""tmap.py cap.json fn unit [cls] [--sym MANGLED] [--idx N] [--diff d.json] [-v]: derive TARGET
colours per vreg by aligning PCode to fdiff output.

fn: bare ('xMat3x3Mul'), qualified ('NPCBlinker::Render') or mangled name.  When several captured
functions share the name, --sym (or $TMAP_SYM) picks by mangled symbol and --idx N (or $TMAP_IDX)
by position among them.  The objdiff symbol is resolved from the capture's qualified name (exact
mangled name with --sym).  $TMAP_SRC: compile a source copy (vfdiff --src).  --diff: reuse a
vfdiff.diff_json() dump instead of compiling.
Prints per vreg: name, our colour, target colour votes. Emits 'v:col' string for solve.py.
"""
import json, sys, re, collections, os
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import ra_common
import vfdiff


def _opt(flag, env):
    if flag in sys.argv:
        i = sys.argv.index(flag)
        v = sys.argv[i + 1]
        del sys.argv[i:i + 2]
        return v
    return os.environ.get(env) or None


ops = {int(k): v for k, v in json.load(open(os.path.join(HERE, 'opmap.json'))).items()}
sym = _opt('--sym', 'TMAP_SYM')
idx = _opt('--idx', 'TMAP_IDX')
idx = int(idx) if idx not in (None, '') else None
dfile = _opt('--diff', 'TMAP_DIFF')
capf, fn, unit = sys.argv[1:4]
cls = int(sys.argv[4]) if len(sys.argv) > 4 and sys.argv[4].isdigit() else 4
if ra_common.looks_mangled(fn) and not sym:
    sym = fn
if dfile:
    data = json.load(open(dfile))
else:
    try:
        data = vfdiff.diff_json(unit, os.environ.get('TMAP_SRC'))
    except SystemExit:
        data = vfdiff.diff_json(unit.split('/')[-1], os.environ.get('TMAP_SRC'))
caps = [x for x in json.load(open(capf))]
chosen, symname = ra_common.resolve(caps, None if sym else fn, sym, idx, data)
cc = [x for x in chosen if x['cls'] == cls]
if not cc:
    raise SystemExit('no class-%d capture for %s' % (cls, symname))
c = cc[0]
N = {int(k): v for k, v in c['nodes'].items()}
pref = 'r' if cls == 4 else 'f'


def colour(v):
    n = N.get(v)
    if v < c['nreal']:
        return v
    if n is None:
        return None
    col = n['final']
    seen = 0
    while col is not None and col >= c['nreal'] and seen < 50:
        col = N[col]['final']; seen += 1
    return col


# vfdiff compiles privately (nothing in build/ is written); TMAP_SRC points it at a source COPY
out = vfdiff.rows(data, symname)
print('symbol %s' % out[0])
out = out[1:]
rows = []
for l in out:
    m = re.match(r'\s*(\d+) (.)  (.*?)\s*\|\s(.*)$', l)
    if not m:
        continue
    rows.append((m.group(3).strip(), m.group(4).strip()))
regre = re.compile(r'\b%s(\d+)\b' % pref)


def regs(s):
    return [int(x) for x in regre.findall(s)]


ours = [regs(o) for t, o in rows]
tgts = [regs(t) for t, o in rows]
mn_o = [o.split()[0] if o else '' for t, o in rows]
used = [False] * len(rows)
votes = collections.defaultdict(collections.Counter)
ptr = 0
nmatched = 0
total = 0
for b in c['blocks']:
    for op, fl, args in b:
        rv = [a[2] for a in args if a[0] == 0 and a[1] == cls]
        if not rv:
            continue
        total += 1
        phys = [colour(v) for v in rv]
        name = ops.get(op, '?')
        best = None
        for j in range(max(0, ptr - 60), min(len(rows), ptr + 120)):
            if used[j] or ours[j] != phys:
                continue
            score = abs(j - ptr) - (50 if mn_o[j].rstrip('.') == name.rstrip('.') else 0)
            if best is None or score < best[0]:
                best = (score, j)
        if best is None:
            continue
        j = best[1]
        used[j] = True
        nmatched += 1
        ptr = j
        if len(tgts[j]) == len(rv):
            for v, t in zip(rv, tgts[j]):
                if v >= c['nreal']:
                    votes[v][t] += 1
print('matched %d/%d pcode instrs' % (nmatched, total))
spec = []
for v in sorted(votes):
    oc = colour(v)
    tv = votes[v].most_common()
    mark = '' if tv[0][0] == oc else '   <== target %d' % tv[0][0]
    if mark or '-v' in sys.argv:
        print('v%-4d %-16s ours=%-3s target=%s%s' % (v, (N[v]['name'] or '-')[:16], oc, dict(tv), mark))
    spec.append('%d:%d' % (v, tv[0][0]))
open(capf + '.tgt', 'w').write(','.join(spec))
