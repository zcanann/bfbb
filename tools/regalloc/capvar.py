"""capvar.py src unit fn variantfile name out.json : apply named variant to a COPY of src and capture"""
import sys, runpy, os, subprocess
src, unit, fn, vf, name, outj = sys.argv[1:7]
V = dict(runpy.run_path(vf)['VARIANTS'])
s = open(src, newline='').read()
for o, n in V[name]:
    assert o in s, o[:80]
    s = s.replace(o, n, 1)
tmp = os.path.join(os.path.dirname(os.path.abspath(__file__)), '_rcap_' + os.path.basename(src))
open(tmp, 'w', newline='').write(s)
try:
    here = os.path.dirname(os.path.abspath(__file__))
    subprocess.run([sys.executable, os.path.join(here, 'rcap.py'), tmp, unit, '--fn', fn, '--json', outj], check=True)
finally:
    os.remove(tmp)
