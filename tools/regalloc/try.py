"""try.py <srcfile> <symbol> <unitstem> <variantfile> [--keep]
variantfile: python file defining list VARIANTS = [(name, [(old,new),...]), ...]"""
import os as _os
_REPO = _os.path.dirname(_os.path.dirname(_os.path.dirname(_os.path.abspath(__file__)))).replace("\\", "/")
import sys, subprocess, re, runpy
src, sym, unit, vf = sys.argv[1:5]
keep = '--keep' in sys.argv
orig = open(src, newline='').read()
V = runpy.run_path(vf)['VARIANTS']
def measure():
    r = subprocess.run(['python','tools/fdiff.py',unit,sym], cwd=_REPO, capture_output=True, text=True)
    m = re.search(r'match=([\d.]+)', r.stdout)
    return (float(m.group(1)) if m else None), r.stdout
base,_ = measure()
print('base', base)
try:
    for name, edits in V:
        s = orig
        ok = True
        for o,n in edits:
            if o not in s: print(name, 'MISSING', repr(o[:60])); ok=False; break
            s = s.replace(o,n,1)
        if not ok: continue
        open(src,'w',newline='').write(s)
        v,out = measure()
        nd = sum(1 for l in out.splitlines()[1:] if re.match(r'\s*\d+ ', l))
        print(f'{name:30s} {v} rows={nd}')
        if '-v' in sys.argv: print(out[:3000])
finally:
    if not keep: open(src,'w',newline='').write(orig)
