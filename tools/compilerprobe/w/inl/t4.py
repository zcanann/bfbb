from sweep import *
import sys
A = lambda k: "    p->a = q->b + %d;\n" % k
cases = {
 "none": "",
 "return": None,  # suffix
 "assign_simple": "    p->a = q->b;\n",
 "assign_cast_ptr": "    p->p = (int*)q->p;\n",
 "assign_ptr": "    p->p = q->p;\n",
 "if_ptr": "    if (q->p) p->a = 1;\n",
 "if_ptr_ne_null": "    if (q->p != (int*)0) p->a = 1;\n",
 "if_null_ne_ptr": "    if ((int*)0 != q->p) p->a = 1;\n",
 "dowhile0": "    do { p->a = 1; } while (0);\n",
 "block": "    { p->a = 1; }\n",
 "call": "    ext(1);\n",
 "call_void_cast": "    (void)ext(1);\n",
 "local_init": "    { int t = q->a; p->a = t; }\n",
 "assign_mul_sizeof": "    p->a = q->b * sizeof(int);\n",
 "assign_mul4": "    p->a = q->b * 4;\n",
 "ptr_index": "    p->a = q->p[3];\n",
 "ptr_deref_inc": "    { int *t = q->p; p->a = *t; t++; }\n",
}
for name, pre in cases.items():
    if pre is None:
        t = threshold(A, prefix="", suffix="    return;\n")
    else:
        t = threshold(A, prefix=pre)
    print("%-20s threshold=%s cost=%s" % (name, t, 30 - t)); sys.stdout.flush()
