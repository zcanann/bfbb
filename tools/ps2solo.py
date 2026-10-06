#!/usr/bin/env python3
"""Compile one shared-source PS2 unit and diff it against the original, like solo.py.

    ps2solo.py <unit-fragment>                 non-matching functions and percents
    ps2solo.py <unit-fragment> <symbol-frag>   side-by-side instruction diff (LEFT = target)
    ps2solo.py <unit-fragment> --all           every function, matching included

The unit must already have a profile in config/platforms/ps2-toolchain.json.
The compiler runs under WSL (Ubuntu) with Wibo, exactly as tools/platforms/ps2_source.py
invokes it; the target object comes from a cached target-only baseline in
build/ps2base/<version> (regenerated with --rebase). Each run writes to a private
temporary directory, so any number can run concurrently.

Toolchain locations default to the copies already downloaded on this machine and can
be overridden with --compilers/--wibo (WSL paths) and --orig-dir.
"""
from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from fdiff import fmt, find_symbol  # noqa: E402

OBJDIFF = Path(os.environ.get('BFBB_OBJDIFF', r'C:\Projects\bfbb\build\tools\objdiff-cli.exe'))
DEFAULT_COMPILERS = os.environ.get('BFBB_PS2_COMPILERS',
                                   '/mnt/c/Projects/bfbb-agent-plankton/build/ps2compiler142')
DEFAULT_WIBO = os.environ.get('BFBB_PS2_WIBO', DEFAULT_COMPILERS + '/wibo-i686')
DEFAULT_ORIG = Path(os.environ.get('BFBB_PS2_ORIG', r'C:\Projects\bfbb-bink\orig'))


def wsl_path(path: Path) -> str:
    p = str(Path(path).resolve()).replace('\\', '/')
    if len(p) > 1 and p[1] == ':':
        p = '/mnt/' + p[0].lower() + p[2:]
    return p


def baseline(version: str, orig: Path, rebase: bool) -> Path:
    base = ROOT / 'build' / 'ps2base' / version
    if rebase or not (base / 'objdiff.json').is_file():
        subprocess.run([sys.executable, str(ROOT / 'tools/platform_progress.py'), 'report',
                        '--version', version, '--orig-dir', str(orig),
                        '--build-dir', str(ROOT / 'build' / 'ps2base'), '--objdiff', str(OBJDIFF)],
                       check=True, capture_output=True)
    return base


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('unit')
    ap.add_argument('symbol', nargs='?', default='')
    ap.add_argument('--version', default='SLUS-20680')
    ap.add_argument('--orig-dir', type=Path, default=DEFAULT_ORIG)
    ap.add_argument('--compilers', default=DEFAULT_COMPILERS)
    ap.add_argument('--wibo', default=DEFAULT_WIBO)
    ap.add_argument('--rebase', action='store_true', help='regenerate the cached target baseline')
    ap.add_argument('--all', action='store_true', help='list matching functions / lines too')
    ap.add_argument('--keep', action='store_true', help='keep the temporary directory')
    args = ap.parse_args()

    base = baseline(args.version, args.orig_dir, args.rebase)
    config = json.loads((base / 'objdiff.json').read_text(encoding='utf-8'))
    profile = json.loads((ROOT / 'config/platforms/ps2-toolchain.json').read_text(encoding='utf-8'))
    profiled = {u['target_unit'] for u in profile['units']}
    units = [u for u in config['units'] if args.unit in u['name'] and u['name'] in profiled]
    exact = [u for u in units if u['name'].endswith('/' + args.unit) or u['name'] == args.unit
             or u['name'].endswith('/' + args.unit + '.cpp')]
    units = exact or units
    if len(units) != 1:
        print('unit fragment matches %d profiled units: %s' % (len(units), [u['name'] for u in units][:10]))
        return 1
    unit = dict(units[0])
    unit['target_path'] = str((base / unit['target_path']).resolve())

    tmp = Path(tempfile.mkdtemp(prefix='ps2solo-', dir=ROOT / 'build'))
    try:
        shutil.copy(base / 'coverage.json', tmp / 'coverage.json')
        cfg = {k: v for k, v in config.items() if k != 'units'}
        cfg['units'] = [unit]
        (tmp / 'objdiff.json').write_text(json.dumps(cfg, indent=2) + '\n', encoding='utf-8')
        script = ('import sys; from pathlib import Path; sys.path.insert(0, "tools"); '
                  'from platforms.ps2_source import compile_units; '
                  'r = compile_units(Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3])); '
                  'sys.exit(0 if r else 3)')
        env = dict(os.environ, MSYS_NO_PATHCONV='1')
        proc = subprocess.run(['wsl.exe', '-d', 'Ubuntu', '--cd', wsl_path(ROOT), 'python3', '-c', script,
                               wsl_path(tmp), args.compilers, args.wibo],
                              capture_output=True, text=True, env=env)
        if proc.returncode != 0:
            print('COMPILE FAILED (%d)' % proc.returncode)
            print((proc.stdout + proc.stderr)[-4000:])
            return 1
        cfg = json.loads((tmp / 'objdiff.json').read_text(encoding='utf-8'))
        name = cfg['units'][0]['name']
        if not args.symbol:
            subprocess.run([str(OBJDIFF), 'report', 'generate', '-p', str(tmp), '-o', str(tmp / 'report.json')],
                           check=True, capture_output=True)
            report = json.loads((tmp / 'report.json').read_text(encoding='utf-8'))
            funcs = report['units'][0].get('functions', [])
            bad = [f for f in funcs if f.get('fuzzy_match_percent', 0) != 100.0]
            m = report['units'][0]['measures']
            print('%s [%s]: %d non-matching of %d; matched code %s / %s bytes' % (
                name, args.version, len(bad), len(funcs), m.get('matched_code', 0), m.get('total_code', 0)))
            for f in sorted(funcs if args.all else bad, key=lambda f: f.get('fuzzy_match_percent', 0)):
                print('  %8.3f%% %7sb  %s' % (f.get('fuzzy_match_percent', 0), f.get('size', '?'), f['name']))
            return 0
        proc = subprocess.run([str(OBJDIFF), 'diff', '-p', str(tmp), '-u', name, '-o', '-', '--format', 'json',
                               args.symbol], capture_output=True, text=True)
        data = json.loads(proc.stdout)
        left = data['left']['symbols']
        right = data.get('right', {}).get('symbols', [])
        lsym = find_symbol(left, args.symbol)
        if lsym is None:
            print('symbol not found')
            return 1
        rsym = right[lsym['target_symbol']] if 'target_symbol' in lsym else None
        print('%s  match=%s' % (lsym.get('demangled_name', lsym['name']), lsym.get('match_percent')))
        li = lsym.get('instructions', [])
        ri = rsym.get('instructions', []) if rsym else []
        for k in range(max(len(li), len(ri))):
            a = li[k] if k < len(li) else None
            b = ri[k] if k < len(ri) else None
            kind = (a or {}).get('diff_kind') or (b or {}).get('diff_kind') or ''
            mark = '  ' if not kind or kind == 'DIFF_NONE' else {
                'DIFF_REPLACE': '|', 'DIFF_DELETE': '<', 'DIFF_INSERT': '>',
                'DIFF_OP_MISMATCH': '|', 'DIFF_ARG_MISMATCH': 'r'}.get(kind, '?')
            if mark == '  ' and not args.all:
                continue
            print('%4d %-2s %-55.55s | %s' % (k, mark, fmt(a), fmt(b)))
        return 0
    finally:
        if not args.keep:
            shutil.rmtree(tmp, ignore_errors=True)


if __name__ == '__main__':
    sys.exit(main())
