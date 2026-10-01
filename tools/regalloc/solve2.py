import sys, itertools
from replay import *
c=load(sys.argv[1], sys.argv[2], int(sys.argv[3]))
tgt={int(a):int(b) for a,b in (x.split(':') for x in sys.argv[4].split(','))}
cand=[int(x) for x in sys.argv[5].split(',')]
K=29 if c["cls"]==4 else 32; mask0=0x1ff9 if c["cls"]==4 else 0x3fff
def score(rank):
    col=colour(c, simplify(c,K,rank), mask0)
    return sum(col[v]!=t for v,t in tgt.items())
live=sorted(i for i in c['nodes'] if i>=c['nreal'] and not c['nodes'][i]['flags']&4)
slots=sorted(set([x-0.5 for x in live]+[live[-1]+0.5]))
best=99
for v,w in itertools.combinations(cand,2):
    for s in slots:
        for t in slots:
            m=score({v:s,w:t})
            if m<best: best=m; print('best',m,v,s,w,t)
            if m==0: print('SOL',v,s,w,t)
