"""vsolo.py --src <copy.c> <solo.py args...>: run tools/solo.py on a private COPY of a unit's source.

Lets you measure an experiment without touching the real file (other agents may be editing it).
Example: python tools/regalloc/vsolo.py --src /tmp/palquant.c palquant ExtractNodes -C 0
"""
import os
import sys
import types

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.dirname(HERE)
sys.path.insert(0, TOOLS)
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
_orig = solo.compile_unit


def compile_unit(unit):
    obj = unit["base_path"].replace("\\", "/")
    solo.RULES[obj] = dict(solo.RULES[obj], src=SRC)
    return _orig(unit)


solo.compile_unit = compile_unit
os.chdir(os.path.dirname(TOOLS))
solo.main()
