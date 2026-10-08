from cx import *
s = base
fn='static void UserDataListCopy('; anchor='    dstList->numElements = srcList->numElements;\n'
def ins(txt):
    i = s.index(fn); j = s.index(anchor, i) + len(anchor)
    return s[:j] + txt + s[j:]
for k in [1,2,3]: measure(ins(("    "+anchor.strip()+"\n")*k), "assign x%d"%k)
for k in [3,4]: measure(ins("    dstList->numElements = 0;\n"*k), "const store x%d"%k)
measure(ins("    (void)0;\n"*5), "void0 x5")
measure(ins("    RpUserDataGetFormatSize(rpINTUSERDATA);\n"*4), "call x4")
