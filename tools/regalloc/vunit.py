"""vunit.py <unit-frag> [--src copy.c] [--mw GC/x]: every symbol (code AND data) of a privately
compiled unit that is not a 100% match against the target, plus section sizes. Empty list = the
object matches symbol-for-symbol (what linking it as Matching needs)."""
import json
import os
import subprocess
import sys
import types

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.dirname(HERE)
ROOT = os.path.dirname(TOOLS)
sys.path.insert(0, TOOLS)
SRC = None
if '--src' in sys.argv:
    i = sys.argv.index('--src')
    SRC = os.path.abspath(sys.argv[i + 1])
    del sys.argv[i:i + 2]
_path = os.path.join(TOOLS, 'solo.py')
_src = open(_path).read().rstrip()
if _src.endswith('main()'):
    _src = _src[:-len('main()')]
solo = types.ModuleType('solo')
solo.__file__ = _path
exec(compile(_src, _path, 'exec'), solo.__dict__)
os.chdir(ROOT)
unit = solo.find_unit(sys.argv[1])
if SRC:
    obj = unit["base_path"].replace("\\", "/")
    solo.RULES[obj] = dict(solo.RULES[obj], src=SRC)
td, objf = solo.compile_unit(unit)
out = os.path.join(td, 'd.json')
subprocess.run([solo.CLI, "diff", "-1", os.path.join(ROOT, unit["target_path"]), "-2", objf, "-o", out,
                "--format", "json", "-c", "functionRelocDiffs=none"], capture_output=True, text=True)
data = json.load(open(out))
bad = 0
for side in ('left',):
    for s in data[side]['symbols']:
        mp = s.get('match_percent')
        if mp is not None and mp < 100.0:
            bad += 1
            print('%-8s %7.3f  %s' % (s.get('kind', '?').replace('SYMBOL_', ''), mp, s.get('name')))
for sec in data['left'].get('sections', []):
    mp = sec.get('match_percent')
    if mp is not None and mp < 100.0:
        bad += 1
        print('SECTION  %7.3f  %s' % (mp, sec.get('name')))
print('%s: %d non-matching symbols/sections' % (unit['name'], bad))
solo.cleanup(td)
