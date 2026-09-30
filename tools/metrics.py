import json, sys
p = sys.argv[1] if len(sys.argv) > 1 else 'build/GQPE78/report.json'
r = json.load(open(p))
rows = [(c['name'], c['measures']) for c in r['categories']] + [('TOTAL', r['measures'])]
for name, m in rows:
    cu = m.get('complete_units', '-')
    tu = m.get('total_units', '-')
    print('%-14s exact=%7.3f fuzzy=%7.3f fn=%d/%d units=%s/%s' % (
        name, m['matched_code_percent'], m['fuzzy_match_percent'],
        m['matched_functions'], m['total_functions'], cu, tu))
