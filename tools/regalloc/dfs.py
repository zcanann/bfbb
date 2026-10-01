"""dfs.py cap fn cls 'v:col,...' : find pop orders of callee-saved webs reaching target (volatile webs ignored)."""
import sys
from replay import load
c=load(sys.argv[1], sys.argv[2], int(sys.argv[3]))
tgt={int(a):int(b) for a,b in (x.split(':') for x in sys.argv[4].split(','))}
N=c['nodes']
cs=[i for i in c['order'] if N[i]['final']>=13]  # webs that need callee-saved
for v in tgt:
    if v not in cs: cs.append(v)
want={v:tgt.get(v,N[v]['final']) for v in cs}
nbs={v:set(N[v]['nb']) for v in cs}
sols=[]
def rec(order, col, claimed, nextnv):
    if len(sols)>=200000: return
    if len(order)==len(cs):
        sols.append(list(order)); return
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
print('cs webs',[(v,N[v]['name'],want[v]) for v in cs])
for s in sols[:20]: print(s)
print(len(sols))
import itertools
pairs=[]
for a,b in itertools.permutations(cs,2):
    if all(s.index(a)<s.index(b) for s in sols): pairs.append((a,b))
print('always-before:',pairs)
print('first pops:', sorted(set(tuple(s[:3]) for s in sols)))
for pre in [(73,55,54),(73,46,47),(46,47,48)]:
    print(pre, [s for s in sols if tuple(s[:3])==pre][:3])
