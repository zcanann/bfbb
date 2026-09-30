#!/usr/bin/env python3
"""Rebuild one unit and print a side-by-side instruction diff for a function.

Usage:
  python tools/fdiff.py <unit> [symbol-substring] [--no-build] [--all]

<unit> may be a report unit name (main/SB/Core/x/xShadow) or a source-relative
stem (SB/Core/x/xShadow). With no symbol, lists the unit's functions and scores.
"""

import argparse
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OBJDIFF = ROOT / "build" / "tools" / "objdiff-cli.exe"


def unit_name(unit: str) -> str:
    return unit if unit.startswith("main/") else "main/" + unit


def build(unit: str) -> bool:
    # Run the compiler command directly rather than ninja, so several of these
    # can run concurrently without contending for ninja's logs.
    obj = "build/GQPE78/src/" + unit[len("main/"):] + ".o"
    cmds = subprocess.run(["ninja", "-t", "commands", obj], cwd=ROOT,
                          capture_output=True, text=True).stdout.strip().splitlines()
    if not cmds:
        print("no build command for", obj)
        return False
    proc = subprocess.run(cmds[-1], cwd=ROOT, capture_output=True, text=True, shell=True)
    if proc.returncode != 0:
        print(proc.stdout[-6000:])
        print(proc.stderr[-2000:])
        return False
    return True


def run_diff(unit: str, symbol: str = "") -> dict:
    cmd = [str(OBJDIFF), "diff", "-p", str(ROOT), "-u", unit, "-o", "-", "--format", "json"]
    if symbol:
        cmd.append(symbol)
    proc = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    return json.loads(proc.stdout)


def find_symbol(syms, sub):
    exact = [s for s in syms if s.get("name") == sub]
    if exact:
        return exact[0]
    cands = [s for s in syms if s.get("kind") == "SYMBOL_FUNCTION"
             and (sub in s.get("name", "") or sub in s.get("demangled_name", ""))]
    return cands[0] if cands else None


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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("unit")
    ap.add_argument("symbol", nargs="?", default="")
    ap.add_argument("--no-build", action="store_true")
    ap.add_argument("--all", action="store_true", help="print matching lines too")
    args = ap.parse_args()
    unit = unit_name(args.unit)
    if not args.no_build and not build(unit):
        return 1
    data = run_diff(unit)
    left = data["left"]["symbols"]
    right = data.get("right", {}).get("symbols", [])
    if not args.symbol:
        funcs = [s for s in left if s.get("kind") == "SYMBOL_FUNCTION" and "size" in s]
        for s in sorted(funcs, key=lambda s: s.get("match_percent", 0)):
            print(f"{s.get('match_percent', 0):7.2f} {int(s['size']):6d} {s.get('demangled_name', s['name'])}  [{s['name']}]")
        return 0
    lsym = find_symbol(left, args.symbol)
    if lsym is None:
        print("symbol not found")
        return 1
    rsym = right[lsym["target_symbol"]] if "target_symbol" in lsym else None
    print(f"{lsym.get('demangled_name', lsym['name'])}  match={lsym.get('match_percent')}")
    li = lsym.get("instructions", [])
    ri = rsym.get("instructions", []) if rsym else []
    n = max(len(li), len(ri))
    for k in range(n):
        a = li[k] if k < len(li) else None
        b = ri[k] if k < len(ri) else None
        kind = (a or {}).get("diff_kind") or (b or {}).get("diff_kind") or ""
        mark = "  " if not kind or kind == "DIFF_NONE" else {
            "DIFF_REPLACE": "|", "DIFF_DELETE": "<", "DIFF_INSERT": ">",
            "DIFF_OP_MISMATCH": "|", "DIFF_ARG_MISMATCH": "r"}.get(kind, "?")
        if mark == "  " and not args.all:
            continue
        print(f"{k:4d} {mark:2s} {fmt(a):55.55s} | {fmt(b)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
