"""solve.py cap.json fn cls 'v:col,v:col' : single/double rank moves reaching target colours"""
import sys, itertools
from replay import *
c=load(sys.argv[1], sys.argv[2], int(sys.argv[3]))
tgt={int(a):int(b) for a,b in (x.split(':') for x in sys.argv[4].split(','))}
K=int(sys.argv[5]) if len(sys.argv)>5 else (29 if c['cls']==4 else 32)
mask0=0x1ff9 if c['cls']==4 else 0x3fff
def score(rank):
    col=colour(c, simplify(c,K,rank), mask0, None if c['cls']==4 else range(31,13,-1))
    return sum(col[v]!=t for v,t in tgt.items()), col
base,_=score(None)
print('baseline mismatches',base)
live=sorted(i for i in c['nodes'] if i>=c['nreal'] and not c['nodes'][i]['flags']&4)
slots=[x-0.5 for x in live]+[live[-1]+0.5]
res=[]
for v in live:
    for s in slots:
        r={v:s}
        m,_=score(r)
        if m==0: res.append((v,s))
print('single moves:',res[:40]); import collections; print('by vreg:', {k:(min(s for v,s in res if v==k), max(s for v,s in res if v==k)) for k in set(v for v,s in res)})
if not res and '--pairs' in sys.argv:
    cand=[v for v in live]
    for v,w in itertools.combinations(cand,2):
        for s in slots:
            for t in slots:
                m,_=score({v:s,w:t})
                if m==0: print('pair',v,s,w,t)
