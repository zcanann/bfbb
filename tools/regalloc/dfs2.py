import sys
from replay import load
c=load(sys.argv[1], sys.argv[2], int(sys.argv[3]))
tgt={int(a):int(b) for a,b in (x.split(':') for x in sys.argv[4].split(','))}
grp=[int(x) for x in sys.argv[5].split(',')]
N=c['nodes']
cs=[i for i in c['order'] if N[i]['final']>=13]
for v in tgt:
    if v not in cs: cs.append(v)
want={v:tgt.get(v,N[v]['final']) for v in cs}
nbs={v:set(N[v]['nb']) for v in cs}
sols=set()
def rec(order, col, claimed, nextnv):
    if len(order)==len(cs):
        sols.add(tuple(v for v in order if v in grp)); return
    for v in cs:
        if v in col: continue
        used={col[n] for n in nbs[v] if n in col}
        free=[r for r in sorted(claimed) if r not in used]
        if free: r=free[0]; nn=nextnv; cl=claimed
        else: r=nextnv; nn=nextnv-1; cl=claimed|{r}
        if r!=want[v]: continue
        col[v]=r; order.append(v)
        rec(order,col,cl,nn)
        order.pop(); del col[v]
rec([],{},frozenset(),31)
names={v:N[v]['name'] for v in grp}
print(names)
for s in sorted(sols): print(s)
