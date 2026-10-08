from sweep import *
import sys
for m in range(1,9):
    fn = lambda k, m=m: "    p->a = " + " + ".join(["q->b"]*m) + " + %d;\n" % k
    t = threshold(fn, maxn=60)
    print(m, t, round(30.0/t,2)); sys.stdout.flush()
