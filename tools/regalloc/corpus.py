"""corpus.py: capture a corpus of colouring graphs for testing allocator-model hypotheses offline.

  corpus.py residues res.json [res2.json ...] --out DIR [--shape REG] [--jobs 6] [--also SYM:UNIT ...]
      every function of a tools/residue.py --json dump with the given shape (default REG): runs
      `diag.py <unit src> <unit> <symbol> --capture-only`, writing DIR/<sym>.json(.tgt3/.tgt4)
      plus DIR/<sym>.log (target colours come from tmap, i.e. from retail's registers).
  corpus.py controls UNIT [UNIT ...] --out DIR [--jobs 6]
      captures EVERY function of each unit (rcap, no --fn) into DIR/<unit stem>.ctl.json. A
      function that matches retail at 100% has retail colours == its captured 'final' colours,
      so an alternative model that changes any of them would break it (see ordmodel.py).

Sources are the units' own files under the repo root ($BFBB_ROOT), i.e. what report.json scored.
"""
import argparse
import json
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import ra_common


def do_residue(sym, unit, out):
    try:
        rule = ra_common.find_rule(unit)
    except SystemExit as e:
        return sym, 'SKIPPED %s' % e
    src = os.path.join(ra_common.ROOT, rule['src'])
    base = os.path.join(out, ra_common.safe_name(sym))
    if os.path.exists(base + '.log'):
        return sym, 'cached'
    r = subprocess.run([sys.executable, os.path.join(HERE, 'diag.py'), src, unit, sym, '--capture-only',
                        '--out', out], capture_output=True, text=True, cwd=HERE)
    open(base + '.log', 'w').write(r.stdout + r.stderr)
    return sym, (r.stdout.strip().splitlines() or ['?'])[-1][:120]


def do_control(unit, out):
    try:
        rule = ra_common.find_rule(unit)
    except SystemExit as e:
        return unit, 'SKIPPED %s' % e
    src = os.path.join(ra_common.ROOT, rule['src'])
    path = os.path.join(out, ra_common.safe_name(unit.split('/')[-1]) + '.ctl.json')
    if os.path.exists(path):
        return unit, 'cached'
    r = subprocess.run([sys.executable, os.path.join(HERE, 'rcap.py'), src, unit, '--json', path + '.tmp'],
                       capture_output=True, text=True, cwd=HERE)
    try:
        caps = json.load(open(path + '.tmp'))
    except (OSError, ValueError):
        return unit, 'FAILED ' + (r.stdout + r.stderr)[-200:]
    for c in caps:
        c.pop('blocks', None)   # keep the corpus small: graphs, order, colours only
    json.dump(caps, open(path, 'w'))
    os.remove(path + '.tmp')
    return unit, '%d captures' % len(caps)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=['residues', 'controls'])
    ap.add_argument('items', nargs='+')
    ap.add_argument('--out', required=True)
    ap.add_argument('--shape', default='REG')
    ap.add_argument('--jobs', type=int, default=6)
    ap.add_argument('--also', nargs='*', default=[])
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    if a.mode == 'residues':
        work = []
        for p in a.items:
            for sym, v in json.load(open(p)).items():
                if v['shape'] in a.shape.split(','):
                    work.append((sym, v['unit']))
        for s in a.also:
            sym, unit = s.split(':', 1)
            work.append((sym, unit))
        with ThreadPoolExecutor(a.jobs) as ex:
            for sym, msg in ex.map(lambda w: do_residue(w[0], w[1], a.out), work):
                print('%-60s %s' % (sym[:60], msg), flush=True)
    else:
        with ThreadPoolExecutor(a.jobs) as ex:
            for unit, msg in ex.map(lambda u: do_control(u, a.out), a.items):
                print('%-40s %s' % (unit, msg), flush=True)


if __name__ == '__main__':
    main()
