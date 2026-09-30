import json, sys
rep = json.load(open('build/GQPE78/report.json'))
cls = {(r[0], r[1]): r[4] for r in json.load(open('classify.json'))}
frags = sys.argv[1:]
for u in rep['units']:
    n = u['name']
    if not n.startswith('main/SB/'):
        continue
    if frags and not any(f in n for f in frags):
        continue
    nm = [f for f in u.get('functions', []) if f.get('fuzzy_match_percent', 100.0) != 100.0]
    if not nm:
        continue
    print('== %s  (%d non-matching)' % (n.replace('main/SB/', ''), len(nm)))
    for f in sorted(nm, key=lambda x: -x['fuzzy_match_percent']):
        print('   %7.3f %6s  %-7s %s' % (f['fuzzy_match_percent'], f['size'],
                                         cls.get((n, f['name']), '?'), f['name']))
