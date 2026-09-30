"""Per-function A/B of a variant compiler vs shipped 2.0p1a via solo.py.

usage: abtest2.py <variant_exe> <unit> [<unit> ...]
Prints only functions whose match %% differs between the two compilers.
"""
import re, shutil, subprocess, sys, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, 'build', 'compilers', 'GC', '2.0p1a', 'mwcceppc.exe')
ORIG = os.environ.get('ORIG_MWCC',
    r'C:\Users\joeyj\AppData\Local\Temp\claude\C--Users-joeyj-Documents-Git-bfbb-'
    r'\458c7547-ca83-49e5-8c1d-e42d1a0ce65d\scratchpad\mwcc_2.0p1a_orig.exe')
ROW = re.compile(r'^\s+([\d.]+)%\s+(\d+)b\s+(\S+)')

def solo(unit):
    p = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'solo.py'),
                        unit], capture_output=True, text=True)
    d = {}
    for line in p.stdout.splitlines():
        m = ROW.match(line)
        if m:
            d[m.group(3)] = (float(m.group(1)), int(m.group(2)))
    return d

variant = sys.argv[1]
try:
    for u in sys.argv[2:]:
        shutil.copy2(ORIG, EXE)
        a = solo(u)
        shutil.copy2(variant, EXE)
        b = solo(u)
        keys = sorted(set(a) | set(b))
        changed = [k for k in keys if a.get(k, (100.0,))[0] != b.get(k, (100.0,))[0]]
        print(f'{u}: {len(changed)} function(s) changed')
        for k in changed:
            pa = a.get(k, (100.0, 0))[0]
            pb = b.get(k, (100.0, 0))[0]
            size = a.get(k, b.get(k))[1]
            tag = 'GAIN' if pb > pa else 'LOSS'
            if pb == 100.0: tag = 'MATCHED'
            if pa == 100.0: tag = 'UNMATCHED'
            print(f'   {tag:10s} {k:55s} {pa:8.3f} -> {pb:8.3f}  ({size}b)')
finally:
    shutil.copy2(ORIG, EXE)
