#!/usr/bin/env python3
"""Per-function match of every SB (game) and/or rwsdk unit under several compilers.

    sweep_units.py --set sb --mw 2.0p1a,2.0p1d --out sb.json
    sweep_units.py --set rw --mw 2.0p1,2.0p1a,2.0p1b,2.0p1c,2.0p1d,2.5 --out rw.json
    sweep_units.py --diff sb.json 2.0p1a 2.0p1d        # gains / losses / moves

Runs tools/solo.py <unit> --mw GC/<v> (a private temp compile; nothing in
build/ is touched). A function absent from solo's non-matching list is 100%.
The JSON is {mw: {unit: {"n": total, "bad": {sym: pct}, "err": str|None}}}.
--mw also accepts a path-ish version (e.g. ../../../x/GC_r4) for scratch
compilers that live outside build/compilers.
"""
import argparse
import json
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
SOLO = os.path.join(ROOT, "tools", "solo.py")
HEAD = re.compile(r"^(\S+): (\d+) non-matching of (\d+)")
ROW = re.compile(r"^\s+([\d.]+)%\s+(\d+)b\s+(\S+)$")


def units(which):
    rep = json.load(open(os.path.join(ROOT, "build/GQPE78/report.json")))
    pre = {"sb": "main/SB/", "rw": "main/rwsdk/"}[which]
    return [u["name"].replace("main/", "") for u in rep["units"] if u["name"].startswith(pre)]


def one(unit, mw):
    mwarg = mw if ("/" in mw or "\\" in mw) and not mw.startswith("GC/") else "GC/" + mw
    r = subprocess.run([sys.executable, SOLO, unit, "--mw", mwarg], cwd=ROOT,
                       capture_output=True, text=True)
    res = {"n": None, "bad": {}, "err": None}
    for line in r.stdout.splitlines():
        m = HEAD.match(line)
        if m:
            res["n"] = int(m.group(3))
        m = ROW.match(line)
        if m:
            res["bad"][m.group(3)] = float(m.group(1))
    if res["n"] is None:
        res["err"] = (r.stdout + r.stderr)[-400:]
    return res


def sweep(which, mws, out, jobs):
    data = json.load(open(out)) if os.path.exists(out) else {}
    us = units(which)
    for mw in mws:
        if mw in data and len(data[mw]) == len(us):
            continue
        with ThreadPoolExecutor(jobs) as ex:
            res = list(ex.map(lambda u: one(u, mw), us))
        data[mw] = dict(zip(us, res))
        json.dump(data, open(out, "w"), indent=0)
        nfn = sum(r["n"] or 0 for r in res)
        nbad = sum(len(r["bad"]) for r in res)
        errs = sum(1 for r in res if r["err"])
        print(f"{mw}: {nfn - nbad}/{nfn} exact, {errs} unit errors", flush=True)


def pct(d, mw, unit, sym):
    return d[mw][unit]["bad"].get(sym, 100.0)


def diff(path, a, b):
    d = json.load(open(path))
    gains, losses, up, down = [], [], [], []
    for unit in d[a]:
        ra, rb = d[a][unit], d[b].get(unit)
        if not rb or ra["err"] or rb["err"]:
            print(f"!! {unit}: error under {a if ra['err'] else b}")
            continue
        for sym in set(ra["bad"]) | set(rb["bad"]):
            pa, pb = pct(d, a, unit, sym), pct(d, b, unit, sym)
            row = (unit, sym, pa, pb)
            if pa < 100 <= pb:
                gains.append(row)
            elif pb < 100 <= pa:
                losses.append(row)
            elif pb > pa + 1e-6:
                up.append(row)
            elif pb < pa - 1e-6:
                down.append(row)
    for name, rows in (("GAINS", gains), ("LOSSES", losses), ("partial up", up), ("partial down", down)):
        print(f"{name} ({len(rows)}):")
        for u, s, pa, pb in sorted(rows):
            print(f"  {u:40s} {s[:60]:60s} {pa:7.2f} -> {pb:7.2f}")
    na = sum(r["n"] - len(r["bad"]) for r in d[a].values() if r["n"])
    nb = sum(r["n"] - len(r["bad"]) for r in d[b].values() if r["n"])
    print(f"exact: {a} {na}  {b} {nb}  (+{len(gains)} / -{len(losses)})")


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--set", choices=("sb", "rw"))
    ap.add_argument("--mw", default="")
    ap.add_argument("--out")
    ap.add_argument("--jobs", type=int, default=8)
    ap.add_argument("--diff", nargs=3, metavar=("JSON", "A", "B"))
    a = ap.parse_args()
    if a.diff:
        diff(*a.diff)
    else:
        sweep(a.set, [m for m in a.mw.split(",") if m], a.out, a.jobs)
