"""vfdiff.py <unit> <symbol> [--src copy.c]: fdiff.py-style '--all' rows, compiled privately.

Same row format as `tools/fdiff.py <unit> <symbol> --all` (LEFT = target), but the object is
compiled by solo.py's private-tempdir path (nothing in build/ is touched) and, with --src, from a
COPY of the unit's source. tmap.py uses this when TMAP_SRC is set.
"""
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
unit_frag, symbol = sys.argv[1], sys.argv[2]

_path = os.path.join(TOOLS, 'solo.py')
_src = open(_path).read().rstrip()
if _src.endswith('main()'):
    _src = _src[:-len('main()')]
solo = types.ModuleType('solo')
solo.__file__ = _path
saved = sys.argv
sys.argv = [_path]
exec(compile(_src, _path, 'exec'), solo.__dict__)
sys.argv = saved
os.chdir(ROOT)

unit = solo.find_unit(unit_frag)
if SRC:
    obj = unit["base_path"].replace("\\", "/")
    solo.RULES[obj] = dict(solo.RULES[obj], src=SRC)
td, objf = solo.compile_unit(unit)
out = os.path.join(td, 'd.json')
subprocess.run([solo.CLI, "diff", "-1", os.path.join(ROOT, unit["target_path"]), "-2", objf, "-o", out,
                "--format", "json", "-c", "functionRelocDiffs=none"], capture_output=True, text=True)
data = json.load(open(out))
left = data["left"]["symbols"]
right = data.get("right", {}).get("symbols", [])


def fmt(ins):
    if not ins:
        return ""
    i = ins.get("instruction", {})
    text = i.get("formatted", "")
    rel = i.get("relocation")
    if rel:
        tgt = rel.get("target", {})
        text += "  <" + (tgt.get("demangled_name") or tgt.get("name", "?")) + ">"
    return text


lsym = [s for s in left if s.get("name") == symbol][0]
rsym = right[lsym["target_symbol"]] if "target_symbol" in lsym else None
print(f"{lsym.get('name')}  match={lsym.get('match_percent')}")
li = lsym.get("instructions", [])
ri = rsym.get("instructions", []) if rsym else []
for k in range(max(len(li), len(ri))):
    a = li[k] if k < len(li) else None
    b = ri[k] if k < len(ri) else None
    kind = (a or {}).get("diff_kind") or (b or {}).get("diff_kind") or ""
    mark = "  " if not kind or kind == "DIFF_NONE" else "|"
    print(f"{k:4d} {mark:2s} {fmt(a):55.55s} | {fmt(b)}")
solo.cleanup(td)
