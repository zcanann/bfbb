import sys, itertools
from replay import *
c=load(sys.argv[1], sys.argv[2], int(sys.argv[3]))
tgt={int(a):int(b) for a,b in (x.split(':') for x in sys.argv[4].split(','))}
grp=[int(x) for x in sys.argv[5].split(',')]
K=29 if c['cls']==4 else 32; mask0=0x1ff9 if c['cls']==4 else 0x3fff
slots=sorted(grp)
sols=[]
for p in itertools.permutations(slots):
    rank=dict(zip(grp,p))
    col=colour(c, simplify(c,K,rank), mask0)
    if all(col[v]==t for v,t in tgt.items()):
        sols.append(rank)
print(len(sols))
for s in sols[:20]: print(sorted(s.items(), key=lambda x:x[1]))
