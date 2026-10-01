"""show.py <cap.json> [cls] [--all] : print pop order with names/colours."""
import json, sys
caps = json.load(open(sys.argv[1]))
want_cls = int(sys.argv[2]) if len(sys.argv) > 2 and sys.argv[2].isdigit() else None
for c in caps:
    if want_cls is not None and c['cls'] != want_cls:
        continue
    nodes = {int(k): v for k, v in c['nodes'].items()}
    print('== %s class %d nreal %d used %d ok %s' % (c['fn'], c['cls'], c['nreal'], c['used'], c.get('ok')))
    pos = {v: i for i, v in enumerate(c['order'])}
    for i in c['order']:
        n = nodes[i]
        nb = [x for x in n['nb'] if x >= c['nreal']]
        phys = [x for x in n['nb'] if x < c['nreal']]
        print('  pop%3d v%-4d %-18s fin=%-3d deg=%-3d fl=%x cost=%d nbv=%s phys=%s' % (
            pos[i], i, (n['name'] or '-')[:18], n['final'], n['deg'], n['flags'], n['cost'], nb,
            ''.join('%d,' % x for x in phys) if '--all' in sys.argv else len(phys)))
    co = [(i, n) for i, n in nodes.items() if i >= c['nreal'] and i not in pos]
    for i, n in co:
        print('  ---  v%-4d %-18s fin=%-3d fl=%x (not in stack)' % (i, (n['name'] or '-')[:18], n['final'], n['flags']))
