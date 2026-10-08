import subprocess, sys
base = open('udc_dwarf.c').read()
def run(s, tag):
    open('pp.c','w').write(s)
    out = subprocess.run([sys.executable,'udprobe.py','pp.c'],capture_output=True,text=True).stdout
    res = {l.split()[0]: l.split('match=')[1] for l in out.splitlines() if 'match=' in l}
    bad = [k for k,v in res.items() if v not in ('100.0','None')]
    print("%-40s ListCopy=%s ObjCopy=%s other_bad=%s" % (tag, res.get('UserDataListCopy'), res.get('UserDataObjectCopy'), [b for b in bad if b not in ('UserDataListCopy','UserDataObjectCopy')]))
    sys.stdout.flush()
if __name__ == '__main__':
    anchors = {
      'listcopy': ('    dstList->numElements = srcList->numElements;\n', 'dstList->numElements = srcList->numElements;'),
      'udcopy': ('    dstUserData->numElements = srcUserData->numElements;\n', 'dstUserData->numElements = srcUserData->numElements;'),
      'destroy': ('    list->numElements = 0;\n', 'list->numElements = 0;'),
    }
    for where,(anchor,stmt) in anchors.items():
        for k in range(1,5):
            i = base.index('static void UserDataListDestroy') if where=='destroy' else 0
            j = base.index(anchor, i)
            s = base[:j] + anchor + ("    "+stmt+"\n")*k + base[j+len(anchor):]
            run(s, "%s +%d" % (where,k))
