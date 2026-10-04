"""replay.py: re-run GC/2.0p1a simplify+colour on a captured graph.

simplify(c, K, rank) -> stack (pop order)
colour(c, stack, mask0, nv_order) -> {vreg: colour}
rank: optional {vreg: float} overriding the scan position (default = vreg).
"""
import json, sys


def load(path, fn=None, cls=4):
    for c in json.load(open(path)):
        if c['cls'] == cls and (fn is None or c['fn'] == fn):
            c['nodes'] = {int(k): v for k, v in c['nodes'].items()}
            return c


def simplify(c, K, rank=None, costs=None):
    nreal = c['nreal']
    nodes = c['nodes']
    live = [i for i in nodes if i >= nreal and not (nodes[i]['flags'] & 4)]
    # nodes that are coalesced are never on the stack; captured flags after
    # simplify have bit 2 set for pushed ones.  Coalesced = bit 4.
    order = sorted(live, key=lambda i: (rank or {}).get(i, i))
    deg = {i: len(n['nb']) for i, n in nodes.items()}
    removed = set()
    stack = []
    while True:
        while True:
            changed = False
            remaining = []
            for i in order:
                if i in removed:
                    continue
                if deg[i] < K:
                    for nb in nodes[i]['nb']:
                        deg[nb] -= 1
                    removed.add(i)
                    stack.append(i)
                    changed = True
                else:
                    remaining.append(i)
            if not changed:
                break
        if not remaining:
            break
        # remaining list is built LIFO: walk from last-pushed to first
        cand = None
        best = None
        # 0x508ad2: walk the LIFO list (last scanned first), keep the first minimum of cost/degree;
        # webs numbered at or above the no-spill mark (spill temps of an earlier round) cost FLT_MAX.
        ns = c.get('nospill')
        for i in reversed(remaining):
            if ns is not None and nodes[i].get('f10', i) >= ns:
                sc = 3.4028234663852886e+38
            else:
                sc = (costs or {}).get(i, nodes[i]['cost']) / max(deg[i], 1)
            if best is None or sc < best:
                cand, best = i, sc
        for nb in nodes[cand]['nb']:
            deg[nb] -= 1
        removed.add(cand)
        stack.append(cand)
    return list(reversed(stack))


def colour(c, pops, mask0, nv=None):
    nreal = c['nreal']
    nodes = c['nodes']
    col = {i: i for i in range(nreal)}
    nv = list(nv or range(31, 13, -1))
    mask = mask0
    for i in pops:
        av = mask
        for nb in nodes[i]['nb']:
            x = col.get(nb, -1)
            if x != -1 and x < nreal:
                av &= ~(1 << x)
        if av:
            r = (av & -av).bit_length() - 1
        else:
            r = nv.pop(0)
            mask |= 1 << r
        col[i] = r
    return col


def check(c, K, mask0):
    pops = simplify(c, K)
    ok_order = pops == c['order']
    col = colour(c, c['order'], mask0)
    bad = [(i, col[i], c['nodes'][i]['final']) for i in c['order'] if col[i] != c['nodes'][i]['final']]
    return ok_order, bad, pops


if __name__ == '__main__':
    c = load(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else None, int(sys.argv[3]) if len(sys.argv) > 3 else 4)
    for K in range(20, 33):
        okk, bad, pops = check(c, K, 0x1ff9)
        if okk:
            print('K', K, 'order OK; colour mismatches', bad[:10])
