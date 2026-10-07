#!/usr/bin/env python3
"""Compile every profiled PS2 unit in parallel and write an objdiff report.

    ps2sweep.py <out-dir> [--version SLUS-20680] [--jobs 12] [--compare old-report.json]

Uses the same compiler invocation as tools/platforms/ps2_source.py (run under WSL with
Wibo) and the cached target-only baseline from build/ps2base/<version> (see ps2solo.py).
Writes <out-dir>/report.json. With --compare, prints per-unit matched-byte / exact-count
deltas and every exact function lost relative to the old report.
"""
from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from ps2solo import DEFAULT_COMPILERS, DEFAULT_ORIG, DEFAULT_WIBO, OBJDIFF, baseline, wsl_path  # noqa: E402

WSL_SCRIPT = r'''
import sys, subprocess
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
sys.path.insert(0, "tools")
import platforms.ps2_source as ps
out, compilers, wibo, jobs = Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3]), int(sys.argv[4])
cmds = []
real_run = subprocess.run
def record(cmd, **kw):
    cmds.append((cmd, kw.get("cwd")))
    Path(cmd[cmd.index("-o") + 1]).write_bytes(b"")
ps.subprocess.run = record
ps.compile_units(out, compilers, wibo)
def go(c):
    cmd, cwd = c
    r = real_run(cmd, cwd=cwd, capture_output=True, text=True)
    return cmd[-1], r.returncode, r.stdout + r.stderr
fails = []
with ThreadPoolExecutor(jobs) as ex:
    for src, rc, log in ex.map(go, cmds):
        if rc:
            fails.append((src, log))
for o in out.rglob("*.o"):
    if o.stat().st_size == 0:
        o.unlink()
print(len(cmds), "units,", len(fails), "failed")
for src, log in fails:
    print("=====", src)
    print("\n".join(l for l in log.splitlines() if "Error" in l or "error" in l)[:2000])
'''


def load(path: Path) -> dict:
    report = json.loads(path.read_text(encoding='utf-8'))
    units = {}
    for u in report['units']:
        funcs = {f['name']: f.get('fuzzy_match_percent', 0) for f in u.get('functions', [])}
        units[u['name']] = (int(u['measures'].get('matched_code', 0)), funcs)
    return units


def compare(old: Path, new: Path) -> None:
    a, b = load(old), load(new)
    totals = [0, 0, 0, 0]
    lost, rows = [], []
    for name, (ma, fa) in a.items():
        if name not in b:
            continue
        mb, fb = b[name]
        if not any(fa.values()) and not any(fb.values()):
            continue
        ea = sum(1 for v in fa.values() if v == 100)
        eb = sum(1 for v in fb.values() if v == 100)
        totals = [totals[0] + ma, totals[1] + mb, totals[2] + ea, totals[3] + eb]
        lost += [(name, f, fb.get(f)) for f, v in fa.items() if v == 100 and fb.get(f) != 100]
        if ma != mb or ea != eb:
            rows.append((mb - ma, name, ma, mb, ea, eb))
    for row in sorted(rows):
        print('%+8d %-45s %8d -> %8d  exact %4d -> %4d' % row)
    print('TOTAL matched %d -> %d (%+d); exact %d -> %d (%+d)' % (
        totals[0], totals[1], totals[1] - totals[0], totals[2], totals[3], totals[3] - totals[2]))
    print('LOST exact: %d' % len(lost))
    for item in lost:
        print('  ', item)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('out', type=Path)
    ap.add_argument('--version', default='SLUS-20680')
    ap.add_argument('--orig-dir', type=Path, default=DEFAULT_ORIG)
    ap.add_argument('--compilers', default=DEFAULT_COMPILERS)
    ap.add_argument('--wibo', default=DEFAULT_WIBO)
    ap.add_argument('--jobs', type=int, default=12)
    ap.add_argument('--compare', type=Path, help='old report.json to compare against')
    args = ap.parse_args()

    base = baseline(args.version, args.orig_dir, False)
    out = args.out.resolve()
    shutil.rmtree(out, ignore_errors=True)
    shutil.copytree(base, out)
    env = dict(os.environ, MSYS_NO_PATHCONV='1')
    proc = subprocess.run(['wsl.exe', '-d', 'Ubuntu', '--cd', wsl_path(ROOT), 'python3', '-c', WSL_SCRIPT,
                           wsl_path(out), args.compilers, args.wibo, str(args.jobs)],
                          capture_output=True, text=True, env=env)
    print((proc.stdout + proc.stderr).strip()[-6000:])
    subprocess.run([str(OBJDIFF), 'report', 'generate', '-p', str(out), '-o', str(out / 'report.json')],
                   check=True, capture_output=True)
    if args.compare:
        compare(args.compare, out / 'report.json')
    return 0


if __name__ == '__main__':
    sys.exit(main())
