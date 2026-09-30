#!/usr/bin/env python3
"""Recompile units with an extra compiler flag and count how many more match.

Some libraries in configure.py were configured by guesswork. Appending a
candidate flag and counting 100%-matching functions finds the real setting:
this is how MSL_C's `-opt level=4 -inline on` and rwsdk's `-fp_contract on`
were found. CodeWarrior lets a later flag override an earlier one, so every
candidate is just an append.

Usage: flagsweep.py <object-path-fragment> [flag[,flag...]]

  flagsweep.py MSL_C                    sweep the default candidate list
  flagsweep.py rwsdk "-fp_contract on"  test one flag

CAVEAT: the reconstructed command matches ninja's exactly for `.c` units, but
for some C++ units it does not, and this has reported gains a real build did
not reproduce. Treat a hit as a hypothesis and confirm it with a full build
before committing the configure.py change.
"""
import json
import os
import re
import shlex
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

import cwexec

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CLI = cwexec.objdiff_cli(ROOT)
CFG = json.load(open(os.path.join(ROOT, "objdiff.json")))
NINJA = open(os.path.join(ROOT, "build.ninja")).read()

DEFAULT_FLAGS = [
    "-inline auto", "-inline off", "-inline on", "-inline all", "-inline deferred",
    "-O4,p", "-O4,s", "-O3", "-O2", "-O1", "-O0",
    "-opt level=1", "-opt level=2", "-opt level=3", "-opt level=4",
    "-opt nopeephole", "-opt noschedule", "-opt space", "-opt speed",
    "-schedule off", "-use_lmw_stmw off", "-use_lmw_stmw on",
    "-fp_contract off", "-fp_contract on", "-fp fmadd", "-fp softway",
    "-str reuse", "-str pool", "-str reuse,pool,readonly", "-rostr",
    "-common on", "-common off", "-enum min", "-char signed",
    "-sym off", "-sym on", "-RTTI on", "-align mac68k",
    "-func_align 4", "-func_align 16", "-sdatathreshold 0", "-sdata2threshold 0",
]

BUILD_RE = re.compile(
    r"^build (?P<obj>\S+\.o): (?P<rule>mwcc_sjis|mwcc) (?P<body>(?:.*\n)*?)  basedir",
    re.M)
SRC_RE = re.compile(r"(?:^|\s)(src[\\/]\S+\.(?:c|cp|cpp))(?:\s|$)")


def build_rules():
    out = {}
    for m in BUILD_RE.finditer(NINJA):
        body = m.group("body")
        mw = re.search(r"mw_version = (\S+)", body)
        cf = re.search(r"cflags = ((?:.*\$\n)*.*)\n", body)
        src = SRC_RE.search(body)
        if not (mw and cf and src):
            continue
        out[m.group("obj").replace("\\", "/")] = {
            "src": src.group(1).replace("\\", "/"),
            "mw": mw.group(1).replace("\\", "/"),
            "flags": re.sub(r"\s+", " ", cf.group(1).replace("$\n", " ")).strip(),
        }
    return out


RULES = build_rules()
BY_BASE = {u["base_path"].replace("\\", "/"): u for u in CFG["units"]
           if "base_path" in u and "target_path" in u}


def count_matching(target, obj):
    fd, out = tempfile.mkstemp(suffix=".json")
    os.close(fd)
    try:
        subprocess.run([CLI, "diff", "-1", target, "-2", obj, "-o", out,
                        "--format", "json"],
                       cwd=ROOT, capture_output=True, timeout=300)
        d = json.load(open(out))
    except Exception:
        return None
    finally:
        try:
            os.unlink(out)
        except OSError:
            pass
    if "symbols" not in d.get("left", {}):
        return None
    fns = [s for s in d["left"]["symbols"] if s.get("kind") == "SYMBOL_FUNCTION"]
    return sum(1 for s in fns if s.get("match_percent", 0.0) >= 100.0), len(fns)


def score(objpath, info, extra):
    unit = BY_BASE.get(objpath)
    if not unit:
        return None
    flags = info["flags"] + (" " + extra if extra else "")
    with tempfile.TemporaryDirectory() as td:
        out = os.path.join(td, "o.o")
        cmd = [os.path.join(ROOT, "build", "compilers", info["mw"], "mwcceppc.exe")] \
            + shlex.split(flags, posix=False) + ["-c", info["src"], "-o", out]
        cmd = [c.strip('"') if c.startswith('"') and c.endswith('"') else c
               for c in cmd]
        try:
            r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True,
                               timeout=300)
        except subprocess.TimeoutExpired:
            return None
        if r.returncode != 0 or not os.path.exists(out):
            return None
        return count_matching(os.path.join(ROOT, unit["target_path"]), out)


def main():
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    frag = sys.argv[1]
    flags = sys.argv[2].split(",") if len(sys.argv) > 2 else DEFAULT_FLAGS
    todo = [(o, i) for o, i in RULES.items() if frag in o and o in BY_BASE]
    print("%d units matching %r, %d flags" % (len(todo), frag, len(flags)))

    def work(item):
        obj, info = item
        base = score(obj, info, None)
        if not base:
            return obj, None, []
        wins = []
        for f in flags:
            s = score(obj, info, f)
            if s and s[0] > base[0]:
                wins.append((s[0], f))
        return obj, base, sorted(wins, reverse=True)

    total = 0
    with ThreadPoolExecutor(6) as ex:
        for obj, base, wins in ex.map(work, todo):
            if not base or not wins:
                continue
            total += wins[0][0] - base[0]
            best = ", ".join("%s -> %d" % (f, n) for n, f in wins[:5])
            print("  %-52s base %d/%d   %s" % (obj[17:], base[0], base[1], best))
    print("upper bound if every unit takes its best single flag: +%d" % total)


main()
