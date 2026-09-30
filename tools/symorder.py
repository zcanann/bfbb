#!/usr/bin/env python3
"""Compare the ORDER of symbols within each section against the target.

Two objects can score 100% on every symbol and still link to different bytes,
because objdiff matches symbols by name while the linker lays them out in
definition order. abort_exit.o was exactly that: `exit` and `abort` both
100%, both sections the right size, but defined in the opposite order, so
flipping it to Matching moved 396 bytes of .text around. Swapping the two
definitions in the .c fixed it.

Like solo.py, this compiles the unit fresh into a private temp directory
rather than reading whatever ninja last built -- the build/ object reports a
stale order after any source edit, which is exactly what this tool exists to
catch.

Usage: symorder.py <unit-fragment> ...
"""
import json
import os
import re
import shlex
import subprocess
import sys
import tempfile

import cwexec

ROOT = os.getcwd()
CLI = os.path.abspath(cwexec.objdiff_cli(ROOT))
CFG = json.load(open("objdiff.json"))
NINJA = open("build.ninja").read()
TMP = os.path.join(tempfile.gettempdir(), "symorder.json")

BUILD_RE = re.compile(
    r"^build (?P<obj>\S+\.o): (?P<rule>mwcc_sjis|mwcc) (?P<body>(?:.*\n)*?)  basedir",
    re.M)
SRC_RE = re.compile(r"(?:^|\s)(src[\\/]\S+\.(?:c|cp|cpp|C))(?:\s|$)")
ANON = re.compile(r"^@\d+$")

RULES = {}
for _m in BUILD_RE.finditer(NINJA):
    _b = _m.group("body")
    _mw = re.search(r"mw_version = (\S+)", _b)
    _cf = re.search(r"cflags = ((?:.*\$\n)*.*)\n", _b)
    _sr = SRC_RE.search(_b)
    if _mw and _cf and _sr:
        RULES[_m.group("obj").replace("\\", "/")] = {
            "src": _sr.group(1).replace("\\", "/"),
            "mw": _mw.group(1).replace("\\", "/"),
            "rule": _m.group("rule"),
            "flags": re.sub(r"\s+", " ", _cf.group(1).replace("$\n", " ")).strip(),
        }


def compile_fresh(unit):
    """Compile the unit into a throwaway directory, the way solo.py does."""
    info = RULES.get(unit["base_path"].replace("\\", "/"))
    if not info:
        return None
    tmp = tempfile.mkdtemp(prefix="symorder_")
    argv = shlex.split(info["flags"], posix=False)
    argv = [a.strip('"') if a.startswith('"') and a.endswith('"') else a
            for a in argv]
    cmd = cwexec.compile_prefix(NINJA, info["rule"], info["mw"])
    subprocess.run(cmd + argv + ["-c", info["src"], "-o", tmp],
                   cwd=ROOT, capture_output=True)
    objs = [f for f in os.listdir(tmp) if f.endswith(".o")]
    return os.path.join(tmp, objs[0]) if objs else None


def sides(unit):
    fresh = compile_fresh(unit)
    if not fresh:
        print("   (no build rule -- comparing the object ninja last built)")
    ours = fresh or unit["base_path"].replace("/", os.sep)
    subprocess.run([CLI, "diff",
                    "-1", unit["target_path"].replace("/", os.sep),
                    "-2", ours,
                    "-o", TMP, "--format", "json"], capture_output=True)
    return json.load(open(TMP))


def by_section(side):
    out, cur = {}, None
    for s in side.get("symbols", []):
        n = s["name"]
        if n.startswith("[") and n.endswith("]"):
            cur = n[1:-1]
            out.setdefault(cur, [])
        elif cur is not None and s.get("size"):
            # @NNN ids come from a compiler counter and never agree between
            # the two builds; only how many there are and where they sit
            # carries information. Keep the size so a pool whose *contents*
            # differ still shows up.
            if ANON.match(n):
                n = "@%s" % s.get("size")
            out[cur].append(n)
    return out


for frag in sys.argv[1:]:
    hits = [u for u in CFG["units"] if frag in u["name"]]
    if not hits:
        print("no unit matching", frag)
        continue
    u = hits[0]
    print("==", u["name"])
    d = sides(u)
    L, R = by_section(d["left"]), by_section(d["right"])
    same = True
    for sec in sorted(set(L) | set(R)):
        a, b = L.get(sec, []), R.get(sec, [])
        if a == b:
            continue
        same = False
        if sorted(a) == sorted(b):
            print("   %s: SAME SET, WRONG ORDER" % sec)
        else:
            print("   %s: different sets" % sec)
        print("      target: %s" % ", ".join(a))
        print("      ours  : %s" % ", ".join(b))
    if same:
        print("   every section matches the target's symbol order")
