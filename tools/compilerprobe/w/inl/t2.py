from sweep import *
for hn in [0,5,10,20]:
    h = "static void h(S* p, S* q)\n{\n" + "".join("    p->c = q->d + %d;\n"%k for k in range(hn)) + "}\n"
    for n in range(0,40):
        body="".join("    p->a = q->b + %d;\n"%k for k in range(n))
        src=HDR+h+"static void f(S* p, S* q)\n{\n    h(p,q);\n"+body+"}\nvoid g(S* p, S* q) { f(p, q); }\n"
        if not inlined(src):
            print("h",hn,"f threshold",n); break
