#!/usr/bin/env python3
"""Compile every unit into a throwaway object and report only the failures.

A stand-in for `ninja` when you cannot afford to write shared build state -
someone else is mid-edit, or you want to know whether a header change breaks
any translation unit without waiting for a link. Each unit goes to its own
temp directory, so nothing in build/GQPE78 is touched.

This proves the tree still *compiles*. It says nothing about whether the DOL
still matches; run a real build for that.

Usage:
  smoke.py                          every unit
  smoke.py <frag> [<frag>...]       only units whose object path contains a frag
  smoke.py --skip <frag>,<frag>     everything except sources matching a frag
"""
import os
import re
import shlex
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

import cwexec

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NINJA = open(os.path.join(ROOT, "build.ninja")).read()

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
            "rule": m.group("rule"),
            "flags": re.sub(r"\s+", " ", cf.group(1).replace("$\n", " ")).strip(),
        }
    return out


def work(item):
    obj, info = item
    td = tempfile.mkdtemp(prefix="smoke_")
    out = os.path.join(td, "o.o")
    cmd = cwexec.compile_prefix(NINJA, info["rule"], info["mw"]) + \
        shlex.split(info["flags"], posix=False) + \
        ["-c", info["src"], "-o", out]
    cmd = [c.strip('"') if c.startswith('"') and c.endswith('"') else c for c in cmd]
    try:
        r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, timeout=600)
        ok = os.path.exists(out)
        err = "" if ok else (r.stdout + r.stderr)[-1500:]
    except Exception as e:
        ok, err = False, str(e)
    for f in os.listdir(td):
        try:
            os.unlink(os.path.join(td, f))
        except OSError:
            pass
    try:
        os.rmdir(td)
    except OSError:
        pass
    return info["src"], ok, err


def main():
    rules = build_rules()
    argv = sys.argv[1:]
    skip = []
    if "--skip" in argv:
        i = argv.index("--skip")
        skip = argv[i + 1].split(",")
        del argv[i:i + 2]

    todo = [(o, i) for o, i in rules.items()
            if not any(s and s in i["src"] for s in skip)
            and (not argv or any(a in o for a in argv))]
    print("%d units" % len(todo))

    bad = 0
    with ThreadPoolExecutor(8) as ex:
        for src, ok, err in ex.map(work, todo):
            if not ok:
                bad += 1
                print("FAIL %s\n%s" % (src, err))
    print("failures: %d of %d" % (bad, len(todo)))
    sys.exit(1 if bad else 0)


main()
