from pad import *
s = base
anchors = {
 'ListCopy': ('static void UserDataListCopy(', '    RwInt32 i;\n'),
 'Destroy': ('static void UserDataListDestroy(', '    RwInt32 i;\n'),
 'Destruct': ('static void UserDataDestruct(', '    RwChar** charData;\n'),
 'UDCopy': ('static void UserDataCopy(', '    RwChar** dstCharData;\n'),
}
for name,(fn,anchor) in anchors.items():
    for k in range(1,6):
        i = s.index(fn); j = s.index(anchor, i) + len(anchor)
        run(s[:j] + "\n" + "    (void)0;\n"*k + s[j:], "%s (void)0 x%d" % (name,k))
