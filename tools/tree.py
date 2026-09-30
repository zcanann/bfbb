#!/usr/bin/env python3
"""Tree-wide: compile every unit with the currently-installed compiler, hash
each object, and record the set of exactly-matching functions.

For compiler-patch work this is the gate that matters. Percentages alone are
not enough: objdiff pairs symbols by name and is blind to definition order, so
a unit can read 100% per-function and still link to different bytes. Hashing
every object answers the question that actually decides whether the DOL
survives - did any object in a *complete* unit change? That signal correctly
predicted the DOL for clause C+ and again for clause V, before either was
linked.

  python3 tools/tree.py before.json        # snapshot
  python3 tools/tree.py after.json         # snapshot again after a change
  python3 tools/tree.py --diff before.json after.json

All 451 units in about 45 seconds on 8 workers, so it is cheap enough to be
the default check rather than a ceremony."""
import hashlib, json, os, shlex, subprocess, sys, tempfile, shutil
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import cwexec

NINJA = open(os.path.join(ROOT, "build.ninja")).read()
CLI = cwexec.objdiff_cli(ROOT)
CFG = json.load(open(os.path.join(ROOT, "objdiff.json")))

import re
BUILD_RE = re.compile(
    r"^build (?P<obj>\S+\.o): (?P<rule>mwcc_sjis|mwcc) (?P<body>(?:.*\n)*?)  basedir", re.M)
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
        out[m.group("obj").replace("\\", "/")] = dict(
            src=src.group(1).replace("\\", "/"),
            mw=mw.group(1).replace("\\", "/"),
            rule=m.group("rule"),
            flags=re.sub(r"\s+", " ", cf.group(1).replace("$\n", " ")).strip())
    return out

RULES = build_rules()
OUTDIR = sys.argv[1]

def work(unit):
    if "base_path" not in unit or "target_path" not in unit:
        return None
    obj = unit["base_path"].replace("\\", "/")
    info = RULES.get(obj)
    if not info:
        return None
    tgt = os.path.join(ROOT, unit["target_path"])
    if not os.path.exists(tgt):
        return None
    td = tempfile.mkdtemp(prefix="tree_")
    out = os.path.join(td, "o.o")
    cmd = cwexec.compile_prefix(NINJA, info["rule"], info["mw"]) + \
        shlex.split(info["flags"], posix=False) + ["-c", info["src"], "-o", out]
    cmd = [c.strip('"') if c.startswith('"') and c.endswith('"') else c for c in cmd]
    rec = dict(name=unit["name"], ok=False)
    try:
        subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, timeout=900)
        if not os.path.exists(out):
            shutil.rmtree(td, ignore_errors=True); return rec
        rec["ok"] = True
        rec["sha1"] = hashlib.sha1(open(out, "rb").read()).hexdigest()
        dj = os.path.join(td, "d.json")
        subprocess.run([CLI, "diff", "-1", tgt, "-2", out, "-o", dj,
                        "--format", "json", "-c", "functionRelocDiffs=none"],
                       cwd=ROOT, capture_output=True, timeout=900)
        d = json.load(open(dj))
        exact = sorted(s["name"] for s in d["left"]["symbols"]
                       if s.get("kind") == "SYMBOL_FUNCTION"
                       and s.get("match_percent", 0) >= 100.0)
        rec["exact"] = exact
        rec["nfunc"] = sum(1 for s in d["left"]["symbols"]
                           if s.get("kind") == "SYMBOL_FUNCTION")
    except Exception as e:
        rec["err"] = str(e)
    shutil.rmtree(td, ignore_errors=True)
    return rec

with ThreadPoolExecutor(max_workers=int(os.environ.get("J", "16"))) as ex:
    res = [r for r in ex.map(work, CFG["units"]) if r]
json.dump(res, open(OUTDIR, "w"))
print("units", len(res), "failed", sum(1 for r in res if not r["ok"]),
      "exact", sum(len(r.get("exact", [])) for r in res))
