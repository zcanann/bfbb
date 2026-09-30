"""A/B a variant compiler against the shipped 2.0p1a using solo.py.

usage: abtest.py <variant_exe> <unit> [<unit> ...]
Swaps build/compilers/GC/2.0p1a/mwcceppc.exe, runs solo.py -q per unit
under each compiler, prints the two summary lines, restores the original.
"""
import shutil, subprocess, sys, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, 'build', 'compilers', 'GC', '2.0p1a', 'mwcceppc.exe')
ORIG = os.environ.get('ORIG_MWCC',
    r'C:\Users\joeyj\AppData\Local\Temp\claude\C--Users-joeyj-Documents-Git-bfbb-'
    r'\458c7547-ca83-49e5-8c1d-e42d1a0ce65d\scratchpad\mwcc_2.0p1a_orig.exe')

def solo(unit):
    p = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'solo.py'),
                        unit, '-q'], capture_output=True, text=True)
    out = (p.stdout + p.stderr).strip().splitlines()
    return out[-1] if out else '(no output)'

variant = sys.argv[1]
units = sys.argv[2:]
try:
    for u in units:
        shutil.copy2(ORIG, EXE)
        a = solo(u)
        shutil.copy2(variant, EXE)
        b = solo(u)
        flag = '' if a.split(':')[-1] == b.split(':')[-1] else '   <== CHANGED'
        print(f'{u}\n  base: {a}\n  var : {b}{flag}')
finally:
    shutil.copy2(ORIG, EXE)
