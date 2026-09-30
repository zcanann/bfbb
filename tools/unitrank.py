import json, sys
r = json.load(open('build/GQPE78/report.json'))
rows = []
for u in r['units']:
    if not u['name'].startswith('main/SB/'):
        continue
    nm = [f for f in u.get('functions', []) if f.get('fuzzy_match_percent', 100.0) != 100.0]
    z = [f for f in nm if f.get('fuzzy_match_percent', 100.0) == 0.0]
    rows.append((len(nm), sum(int(f['size']) for f in nm), len(z), u['name'].replace('main/SB/', '')))
rows.sort(reverse=True)
n = int(sys.argv[1]) if len(sys.argv) > 1 else 50
print('  nm    bytes  zero  unit')
for a, b, c, d in rows[:n]:
    print('%4d %8d %5d  %s' % (a, b, c, d))
print('units with >0 nm: %d of %d' % (sum(1 for x in rows if x[0]), len(rows)))
print('total nm functions: %d  bytes %d' % (sum(x[0] for x in rows), sum(x[1] for x in rows)))
