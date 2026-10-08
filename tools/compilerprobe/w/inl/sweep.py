import subprocess, sys, os
D = os.path.dirname(os.path.abspath(__file__))
CC = "C:/Projects/bfbb/build/compilers/GC/2.0p1e/mwcceppc.exe"
DTK = "C:/Projects/bfbb/build/tools/dtk.exe"
FLAGS = "-nodefaults -proc gekko -align powerpc -enum int -fp hardware -O4,p -inline auto -lang=c -Cpp_exceptions off".split()

HDR = """typedef struct S { int a, b, c, d; int *p; } S;
extern int ext(int);
extern int ext2(int, int);
"""


def inlined(src, callee="f", caller="g"):
    open(D + "/s.c", "w").write(src)
    if os.path.exists(D + "/s.o"):
        os.remove(D + "/s.o")
    r = subprocess.run([CC] + FLAGS + ["-c", "s.c", "-o", "s.o"], cwd=D, capture_output=True, text=True)
    if not os.path.exists(D + "/s.o"):
        print(r.stdout, r.stderr)
        raise SystemExit
    subprocess.run([DTK, "elf", "disasm", "s.o", "s.s"], cwd=D, capture_output=True)
    s = open(D + "/s.s").read()
    body = s.split(".fn " + caller + ",")[1].split(".endfn")[0]
    return ("bl " + callee + "\n") not in body and ("bl " + callee + " ") not in body and not body.rstrip().endswith("bl " + callee)


def threshold(stmt_fn, maxn=80, prefix="", suffix=""):
    last = None
    for n in range(0, maxn):
        body = "".join(stmt_fn(k) for k in range(n))
        src = HDR + "static void f(S* p, S* q)\n{\n" + prefix + body + suffix + "}\nvoid g(S* p, S* q) { f(p, q); }\n"
        if not inlined(src):
            return n
    return None


if __name__ == "__main__":
    kinds = {
        "assign": lambda k: "    p->a = q->b + %d;\n" % k,
        "call": lambda k: "    ext(%d);\n" % k,
        "if": lambda k: "    if (q->a == %d) p->b = 1;\n" % k,
        "bigexpr": lambda k: "    p->a = q->a * q->b + q->c * q->d + %d;\n" % k,
    }
    for name, fn in kinds.items():
        print(name, threshold(fn))
