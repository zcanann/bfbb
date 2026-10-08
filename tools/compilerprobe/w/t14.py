from pad import *
import subprocess, sys
def state(src):
    open('pp.c','w').write(src)
    out = subprocess.run([sys.executable,'udprobe.py','pp.c'],capture_output=True,text=True).stdout
    sizes = {l.split()[0]: int(l.split('size=')[1].split()[0]) for l in out.splitlines() if 'size=' in l}
    return sizes
for n in [64,128,200,256,300,400,512,1000]:
    sz = state("#pragma inline_max_size(%d)\n" % n + base)
    print(n, "ObjCopy", sz.get('UserDataObjectCopy'), "ListCopy", sz.get('UserDataListCopy'), "ObjDestruct", sz.get('UserDataObjectDestruct'), "StreamRead-objs", sz.get('UserDataObjectStreamRead')); sys.stdout.flush()
