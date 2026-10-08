from pad import *
s = base
fn='static void UserDataListCopy('; anchor='    dstList->numElements = srcList->numElements;\n'
def ins(txt, tag):
    i = s.index(fn); j = s.index(anchor, i) + len(anchor)
    run(s[:j] + txt + s[j:], tag)
for k in range(1,7): ins("    dstList->numElements = 0;\n"*k, "const store x%d"%k)
for k in range(1,5): ins("    ((RpUserDataList*)srcList)->numElements = 0;\n"*k, "src const store x%d"%k)
for k in range(1,5): ins("    RpUserDataGetFormatSize(rpINTUSERDATA);\n"*k, "call x%d"%k)
