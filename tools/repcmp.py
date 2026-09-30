"""Compare two report.json files per function: repcmp.py <base> <new> [-v]"""
import json, sys

def load(p):
    r = json.load(open(p))
    funcs = {}
    units = {}
    for u in r['units']:
        for f in u.get('functions', []):
            funcs[(u['name'], f['name'])] = f.get('fuzzy_match_percent', 0.0)
        m = u.get('measures', {})
        units[u['name']] = m.get('matched_functions', 0)
    meas = r['measures']
    game = None
    for c in r.get('categories', []):
        if c.get('id') == 'game':
            game = c['measures']
    return funcs, units, meas, game

base_f, base_u, base_m, base_g = load(sys.argv[1])
new_f, new_u, new_m, new_g = load(sys.argv[2])
verbose = '-v' in sys.argv

print('matched_functions %d -> %d  (%+d)' % (
    base_m['matched_functions'], new_m['matched_functions'],
    new_m['matched_functions'] - base_m['matched_functions']))
print('matched_code_percent %.5f -> %.5f' % (
    base_m['matched_code_percent'], new_m['matched_code_percent']))
if base_g and new_g:
    print('GAME exact %.5f -> %.5f   fuzzy %.5f -> %.5f   funcs %d -> %d' % (
        base_g['matched_code_percent'], new_g['matched_code_percent'],
        base_g.get('fuzzy_match_percent', 0), new_g.get('fuzzy_match_percent', 0),
        base_g['matched_functions'], new_g['matched_functions']))

gained, lost, up, down = [], [], [], []
for k in set(base_f) | set(new_f):
    a = base_f.get(k)
    b = new_f.get(k)
    if a is None or b is None or a == b:
        continue
    if a < 100.0 <= b: gained.append((k, a, b))
    elif b < 100.0 <= a: lost.append((k, a, b))
    elif b > a: up.append((k, a, b))
    else: down.append((k, a, b))

uup = sorted(u for u in new_u if new_u[u] > base_u.get(u, 0))
udown = sorted(u for u in new_u if new_u[u] < base_u.get(u, 0))

print('functions: gained-100 %d, lost-100 %d, sub-100 up %d, sub-100 down %d'
      % (len(gained), len(lost), len(up), len(down)))
print('units with matched-count up: %d, down: %d' % (len(uup), len(udown)))
for name, lst in (('GAINED', gained), ('LOST', lost)):
    for (u, f), a, b in sorted(lst):
        print('  %-6s %-50s %8.3f -> %8.3f  [%s]' % (name, f, a, b, u))
if verbose:
    for name, lst in (('UP', up), ('DOWN', down)):
        for (u, f), a, b in sorted(lst):
            print('  %-6s %-50s %8.3f -> %8.3f  [%s]' % (name, f, a, b, u))
if udown:
    print('units regressed:', ', '.join(udown))
