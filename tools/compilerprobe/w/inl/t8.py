from sweep import *
import sys
def fthr(h_extra_pre="", h_extra_post="", hn=20, fpre=""):
    h = "static void h(S* p, S* q)\n{\n" + h_extra_pre + "".join("    p->c = q->d + %d;\n"%k for k in range(hn)) + h_extra_post + "}\n"
    for n in range(0,40):
        body="".join("    p->a = q->b + %d;\n"%k for k in range(n))
        src=HDR+h+"static void f(S* p, S* q)\n{\n"+fpre+"    h(p,q);\n"+body+"}\nvoid g(S* p, S* q) { f(p, q); }\n"
        if not inlined(src): return n
print("base", fthr())
print("h return", fthr(h_extra_post="    return;\n"))
print("h empty stmt", fthr(h_extra_pre="    ;\n"))
print("h (void)0", fthr(h_extra_pre="    (void)0;\n"))
print("h 0", fthr(h_extra_pre="    0;\n"))
print("h dead local", fthr(h_extra_pre="    int t;\n    t = 0;\n"))
print("h dead local init", fthr(h_extra_pre="    int t = 0;\n"))
print("h do-while0 empty", fthr(h_extra_pre="    do { } while (0);\n"))
print("h if(0)", fthr(h_extra_pre="    if (0) { ext(1); }\n"))
print("f empty stmt", fthr(fpre="    ;\n"))
print("f (void)0", fthr(fpre="    (void)0;\n"))
print("f dead local", fthr(fpre="    int t;\n    t = 0;\n"))
print("f if(0)", fthr(fpre="    if (0) { ext(1); }\n"))
