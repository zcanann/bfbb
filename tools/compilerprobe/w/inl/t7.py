from sweep import *
import sys
for hn in [10,20,28]:
    h = "static void h(S* p, S* q)\n{\n" + "".join("    p->c = q->d + %d;\n"%k for k in range(hn)) + "}\n"
    res=[]
    for n in [0,10,20,28,40,60]:
        body="".join("    p->a = q->b + %d;\n"%k for k in range(n))
        src=HDR+h+"void f(S* p, S* q)\n{\n    h(p,q);\n"+body+"}\n"
        res.append((n, inlined(src,"h","f")))
    print(hn,res); sys.stdout.flush()
