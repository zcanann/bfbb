import subprocess, sys
base = open('udc_dwarf.c').read()
def state(src):
    open('pp.c','w').write(src)
    out = subprocess.run([sys.executable,'udprobe.py','pp.c'],capture_output=True,text=True).stdout
    return {l.split()[0]: int(l.split('size=')[1].split()[0]) for l in out.splitlines() if 'size=' in l}
def bsearch(src, pred, lo=1, hi=2000):
    # smallest n with pred(state) true
    while lo < hi:
        m = (lo+hi)//2
        if pred(state("#pragma inline_max_size(%d)\n" % m + src)): hi = m
        else: lo = m+1
    return lo
def measure(src, tag=""):
    d = state(src)
    c_list = bsearch(src, lambda s: s.get('UserDataObjectCopy',0) > 200)
    c_ud = bsearch(src, lambda s: s.get('UserDataListCopy',0) > 600 or s.get('UserDataObjectCopy',0) > 600, hi=c_list)
    print("%-30s default: ObjCopy=%s ListCopy=%s | C_list=%d C_ud=%d" % (tag, d.get('UserDataObjectCopy'), d.get('UserDataListCopy'), c_list, c_ud)); sys.stdout.flush()
if __name__ == '__main__':
    measure(base, "base")
