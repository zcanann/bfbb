"""vfdiff.py <unit> <symbol> [--src copy.c]: fdiff.py-style '--all' rows, compiled privately.

Same row format as `tools/fdiff.py <unit> <symbol> --all` (LEFT = target), but the object is
compiled by solo.py's private-tempdir path (nothing in build/ is touched) and, with --src, from a
COPY of the unit's source. tmap.py uses this when TMAP_SRC is set.

<symbol> may be the mangled name (Render__10NPCBlinkerFPC5xVec3fPC8RwRaster), a mangled prefix
(Render__10NPCBlinker), a qualified name (NPCBlinker::Render) or a bare name; anything that is
not unique is an error listing the candidates.  The compiler/flags follow rcap.py: $RCAP_MW
overrides the unit's mw_version, $RCAP_EXTRA_FLAGS is appended, and a --src copy outside the
unit's directory gets '-i <unit source dir>'.  Importable: diff_json(), find_symbol(), rows().
"""
import json
import os
import subprocess
import sys
import types

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import ra_common  # noqa: E402

ROOT = ra_common.ROOT
TOOLS = os.path.join(ROOT, 'tools')
sys.path.insert(0, TOOLS)
_solo = None


def solo():
    """tools/solo.py loaded as a module (its trailing main() call stripped)."""
    global _solo
    if _solo is None:
        path = os.path.join(TOOLS, 'solo.py')
        src = open(path).read().rstrip()
        if src.endswith('main()'):
            src = src[:-len('main()')]
        m = types.ModuleType('solo')
        m.__file__ = path
        saved, cwd = sys.argv, os.getcwd()
        sys.argv = [path]
        os.chdir(ROOT)
        try:
            exec(compile(src, path, 'exec'), m.__dict__)
        finally:
            sys.argv = saved
            os.chdir(cwd)
        _solo = m
    return _solo


def diff_json(unit_frag, src=None):
    """objdiff JSON of the unit compiled privately (optionally from a source copy)."""
    s = solo()
    cwd = os.getcwd()
    os.chdir(ROOT)
    saved = sys.argv
    sys.argv = [saved[0]]   # solo.compile_unit reads --mw/--shadow from argv; we set them below
    try:
        unit = s.find_unit(unit_frag)
        obj = unit["base_path"].replace("\\", "/")
        rule = dict(s.RULES[obj])
        nrule = dict(rule)
        if src:
            nrule['src'] = os.path.abspath(src).replace('\\', '/')
        nrule['mw'] = ra_common.compiler_version(rule)
        ex = ra_common.extra_flags(rule, nrule['src'])
        if ex:
            nrule['flags'] = rule['flags'] + ' ' + ex
        s.RULES[obj] = nrule
        try:
            td, objf = s.compile_unit(unit)
        finally:
            s.RULES[obj] = rule
        out = os.path.join(td, 'd.json')
        subprocess.run([s.CLI, "diff", "-1", os.path.join(ROOT, unit["target_path"]), "-2", objf, "-o", out,
                        "--format", "json", "-c", "functionRelocDiffs=none"], capture_output=True, text=True)
        data = json.load(open(out))
        s.cleanup(td)
        return data
    finally:
        sys.argv = saved
        os.chdir(cwd)


def find_symbol(data, symbol):
    """Target-side (left) symbol for `symbol`; falls back to our side if the target lacks it."""
    left = data["left"]["symbols"]
    try:
        return ra_common.find_symbol(left, symbol)
    except SystemExit as e:
        if 'not found' not in str(e):
            raise
        return ra_common.find_symbol(data.get("right", {}).get("symbols", []), symbol)


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


def rows(data, symbol):
    """[header, row, row, ...] exactly as the CLI prints them."""
    left = data["left"]["symbols"]
    right = data.get("right", {}).get("symbols", [])
    lsym = find_symbol(data, symbol)
    if lsym in left:
        rsym = right[lsym["target_symbol"]] if "target_symbol" in lsym else None
    else:   # only in our object: show it on the right
        lsym, rsym = None, lsym
    head = lsym or rsym
    out = [f"{head.get('name')}  match={head.get('match_percent')}"]
    li = (lsym or {}).get("instructions", [])
    ri = rsym.get("instructions", []) if rsym else []
    for k in range(max(len(li), len(ri))):
        a = li[k] if k < len(li) else None
        b = ri[k] if k < len(ri) else None
        kind = (a or {}).get("diff_kind") or (b or {}).get("diff_kind") or ""
        mark = "  " if not kind or kind == "DIFF_NONE" else "|"
        out.append(f"{k:4d} {mark:2s} {fmt(a):55.55s} | {fmt(b)}")
    return out


def main():
    argv = list(sys.argv)
    src = None
    if '--src' in argv:
        i = argv.index('--src')
        src = os.path.abspath(argv[i + 1])
        del argv[i:i + 2]
    if '--mw' in argv:   # same as RCAP_MW
        i = argv.index('--mw')
        os.environ['RCAP_MW'] = argv[i + 1]
        del argv[i:i + 2]
    unit_frag, symbol = argv[1], argv[2]
    for line in rows(diff_json(unit_frag, src), symbol):
        print(line)


if __name__ == '__main__':
    main()
