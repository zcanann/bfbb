#!/usr/bin/env python3
"""Measure many units at once, then prove a shared-header change was safe.

  sweep.py snap <out.json> <unit-frag>...     measure and record
  sweep.py snap <out.json> --all-affected <header-frag>
                                              ...every TU that compiles it
  sweep.py cmp <before.json> <after.json>     diff the full distributions

The workflow for a shared header is: snap, edit the header, snap again, cmp.
`cmp` exits non-zero if anything regressed, so it can gate a commit.

This records **every symbol**, data included, not just the functions and not
just the ones at 100%. That is deliberate. A change once dropped two units'
`.sdata2` match from 65.2% to 63.8% and 92.1% to 90.6% while no function
crossed the 100% boundary, so a comparison of exact-match sets called it clean.

Three failure modes are designed out here, each of which has produced a
confident, wrong CLEAN in this project before:

  1. A unit that fails to compile must be an ERROR, never an empty result --
     an empty result compares clean against anything. `solo.py` reports
     compile failures through SystemExit, i.e. **stderr**, so a tool reading
     only stdout silently turns every failure into "no differences".
  2. Unit lists written by Windows Python arrive with a trailing `\r`, and
     substring matching then misses every lookup except the last line.
     Everything is stripped on the way in.
  3. Zero rows is a legitimate result -- a fully matching unit has no
     non-matching symbols. Success is "the measurement ran", not "rows came
     back". Treating no-rows as failure silently drops exactly the units a
     header change is most likely to break.

`snap` refuses to write a snapshot if any unit failed to measure, so a
half-populated file cannot be compared against later.
"""
import json
import os
import re
import shlex
import subprocess
import sys
import tempfile

import cwexec

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CLI = cwexec.objdiff_cli(ROOT)
CFG = json.load(open(os.path.join(ROOT, "objdiff.json")))
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


RULES = build_rules()


def find_unit(frag):
    frag = frag.strip()                      # failure mode 2
    hits = [u for u in CFG["units"] if frag in u["name"]]
    if len(hits) == 1:
        return hits[0]
    exact = [u for u in hits if u["name"].endswith(frag)]
    if len(exact) == 1:
        return exact[0]
    raise RuntimeError("ambiguous or no match: %s" % frag)


def measure(frag):
    """{symbol: percent} for every symbol, or raise. Never returns {} silently."""
    unit = find_unit(frag)
    info = RULES.get(unit["base_path"].replace("\\", "/"))
    if not info:
        raise RuntimeError("no build rule for %s" % unit["name"])
    td = tempfile.mkdtemp(prefix="sweep_")
    try:
        obj = os.path.join(td, "o.o")
        cmd = cwexec.compile_prefix(NINJA, info["rule"], info["mw"]) + \
            shlex.split(info["flags"], posix=False) + \
            ["-c", info["src"], "-o", obj]
        cmd = [c.strip('"') if c.startswith('"') and c.endswith('"') else c
               for c in cmd]
        r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True,
                           timeout=900)
        if not os.path.exists(obj):          # failure mode 1
            raise RuntimeError("COMPILE FAILED %s: %s"
                               % (unit["name"], (r.stdout + r.stderr)[-400:]))
        out = os.path.join(td, "d.json")
        subprocess.run([CLI, "diff", "-1",
                        os.path.join(ROOT, unit["target_path"]),
                        "-2", obj, "-o", out, "--format", "json"],
                       cwd=ROOT, capture_output=True)
        if not os.path.exists(out):
            raise RuntimeError("objdiff produced nothing for %s" % unit["name"])
        d = json.load(open(out))
        syms = d.get("left", {}).get("symbols", [])
        if not syms:
            raise RuntimeError("no symbols for %s" % unit["name"])
        # Every symbol, data included -- see the module docstring.
        return unit["name"], {s["name"]: round(s.get("match_percent", 0.0), 4)
                              for s in syms}
    finally:
        for f in os.listdir(td):
            try:
                os.unlink(os.path.join(td, f))
            except OSError:
                pass
        try:
            os.rmdir(td)
        except OSError:
            pass


def affected(header_frag):
    r = subprocess.run(['python', os.path.join(ROOT, 'tools', 'blast.py'),
                        header_frag], cwd=ROOT, capture_output=True, text=True)
    if r.returncode:
        raise SystemExit(r.stdout + r.stderr)
    units, on = [], False
    for line in r.stdout.splitlines():
        s = line.strip()
        if s.endswith(':') and s[:-1] in ('Matching', 'Equivalent',
                                          'NonMatching', 'not-built'):
            on = s[:-1] != 'not-built'
            continue
        if on and s.endswith(('.c', '.cp', '.cpp')):
            units.append(os.path.splitext(s)[0])
    return units


def snap(out_path, frags):
    data, failed = {}, []
    for i, frag in enumerate(frags, 1):
        try:
            name, syms = measure(frag)
            data[name] = syms
            print("[%d/%d] %-46s %d symbols" % (i, len(frags), name, len(syms)))
        except Exception as e:                # noqa: BLE001 - report and continue
            failed.append((frag, str(e)))
            print("[%d/%d] %-46s FAILED" % (i, len(frags), frag))
    if failed:
        print("\n%d unit(s) failed to measure -- snapshot NOT written:"
              % len(failed))
        for frag, msg in failed:
            print("  %s: %s" % (frag, msg.splitlines()[0][:160]))
        raise SystemExit(1)
    json.dump(data, open(out_path, 'w'), indent=0, sort_keys=True)
    total = sum(len(v) for v in data.values())
    print("\nwrote %s: %d units, %d symbols" % (out_path, len(data), total))
    if not total:
        raise SystemExit("refusing a snapshot with no symbols at all")


def cmp_snaps(a_path, b_path):
    a = json.load(open(a_path))
    b = json.load(open(b_path))

    only_a = sorted(set(a) - set(b))
    only_b = sorted(set(b) - set(a))
    if only_a:
        print("!! %d unit(s) missing from the AFTER snapshot: %s"
              % (len(only_a), ", ".join(only_a[:6])))
    if only_b:
        print("   %d unit(s) only in AFTER: %s"
              % (len(only_b), ", ".join(only_b[:6])))

    regressed, improved, vanished = [], [], []
    for unit in sorted(set(a) & set(b)):
        for sym, before in a[unit].items():
            after = b[unit].get(sym)
            if after is None:
                vanished.append((unit, sym, before))
            elif after < before - 1e-6:
                regressed.append((unit, sym, before, after))
            elif after > before + 1e-6:
                improved.append((unit, sym, before, after))

    print("\n%-10s %d" % ("improved", len(improved)))
    print("%-10s %d" % ("regressed", len(regressed)))
    print("%-10s %d" % ("vanished", len(vanished)))

    if vanished:
        print("\nSYMBOLS THAT DISAPPEARED (a rename, or a body that stopped"
              " being emitted):")
        for unit, sym, before in vanished[:40]:
            print("  %-34s %7.3f  %s" % (unit.split('/')[-1], before, sym))
        if len(vanished) > 40:
            print("  ... %d more" % (len(vanished) - 40))

    if regressed:
        lost100 = [r for r in regressed if r[2] >= 100.0]
        if lost100:
            print("\n!! %d SYMBOL(S) FELL OFF 100%%:" % len(lost100))
            for unit, sym, before, after in lost100[:40]:
                print("  %-34s %7.3f -> %7.3f  %s"
                      % (unit.split('/')[-1], before, after, sym))
        print("\nALL REGRESSIONS:")
        for unit, sym, before, after in sorted(regressed,
                                               key=lambda r: r[3] - r[2])[:40]:
            print("  %-34s %7.3f -> %7.3f  %s"
                  % (unit.split('/')[-1], before, after, sym))
        if len(regressed) > 40:
            print("  ... %d more" % (len(regressed) - 40))

    if improved:
        print("\nTOP IMPROVEMENTS:")
        for unit, sym, before, after in sorted(improved,
                                               key=lambda r: r[2] - r[3])[:15]:
            print("  %-34s %7.3f -> %7.3f  %s"
                  % (unit.split('/')[-1], before, after, sym))

    bad = bool(regressed or vanished or only_a)
    print("\n%s" % ("REGRESSIONS PRESENT" if bad else "CLEAN"))
    raise SystemExit(1 if bad else 0)


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    mode = sys.argv[1]
    if mode == 'snap':
        out = sys.argv[2]
        rest = sys.argv[3:]
        if rest and rest[0] == '--all-affected':
            frags = affected(rest[1])
            print("%d TUs compile %s\n" % (len(frags), rest[1]))
        else:
            frags = [x.strip() for x in rest if x.strip()]
        if not frags:
            raise SystemExit("no units to measure")
        snap(out, frags)
    elif mode == 'cmp':
        cmp_snaps(sys.argv[2], sys.argv[3])
    else:
        raise SystemExit(__doc__)


main()
