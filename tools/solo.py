#!/usr/bin/env python3
"""Compile ONE unit into a private temp object and diff it against the target.

The point is that this touches no shared build state. `ninja` writes into
build/GQPE78/, so two people (or two agents) working in the same checkout
clobber each other's objects and report.json. This compiles a single source
file into a throwaway directory using the exact compiler and flags that
build.ninja would use, then diffs that object against the original with
objdiff-cli. About two seconds, and any number of them can run at once.

Usage:
  solo.py <unit-fragment>                 list the unit's non-matching functions
  solo.py <unit-fragment> <symbol-frag>   side-by-side diff for one function
  solo.py <unit-fragment> --missing       list target functions absent from ours
  solo.py <unit-fragment> --bands         histogram of match quality, by count
                                          and by bytes. Six lines instead of a
                                          hundred-row listing, and it tells you
                                          whether a unit's remaining work is a
                                          few broken bodies or a pool ceiling.

Output-size flags (the full listings are large; these keep them out of an
agent's context when it is iterating):
  -q, --quiet        print only the summary header line, no per-function rows
  --top N            print only the N worst-matching rows
  -C N, --context N  in a symbol diff, print only differing rows plus N rows
                     of context (default 3). A 99.8% function prints two rows
                     instead of five hundred.
  --full             in a symbol diff, print every row (the old behaviour)
  --relocs           count @NNN relocation-name rows as differences;
                     off by default because report.json does not count them

Truncation is always reported, never silent.

A `|` in the left margin of a side-by-side diff marks a differing pair.

The LEFT column is the TARGET (retail) and the RIGHT column is our build --
`left` comes from `-1 <target_path>`. Getting this backwards inverts every
conclusion you draw from a diff, so it is worth re-checking if something is
not making sense.

Requires a configured tree: run `configure.py` first so build.ninja exists, and
have build/tools/objdiff-cli downloaded (any prior `ninja` does that).
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

# build.ninja puts the source on the same line as the `build` statement when it
# fits and on the next line when it does not, so match the whole rule body and
# search it rather than anchoring to a fixed position.
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
        flags = re.sub(r"\s+", " ", cf.group(1).replace("$\n", " ")).strip()
        out[m.group("obj").replace("\\", "/")] = {
            "src": src.group(1).replace("\\", "/"),
            "mw": mw.group(1).replace("\\", "/"),
            "rule": m.group("rule"),
            "flags": flags,
        }
    return out


RULES = build_rules()


def find_unit(frag):
    hits = [u for u in CFG["units"] if frag in u["name"]]
    if len(hits) != 1:
        exact = [u for u in hits if u["name"].endswith(frag)]
        if len(exact) == 1:
            return exact[0]
        raise SystemExit("ambiguous or no match: " +
                         ", ".join(u["name"] for u in hits[:12]))
    return hits[0]


def compile_unit(unit):
    obj = unit["base_path"].replace("\\", "/")
    info = RULES.get(obj)
    if not info:
        raise SystemExit("no build rule for %s - is the source file missing?" % obj)
    td = tempfile.mkdtemp(prefix="solo_")
    out = os.path.join(td, "o.o")
    # --shadow <dir> prepends an include directory, so a shared-header change
    # can be measured from a private copy of the tree. Editing the real header
    # while another agent is compiling silently hands that agent numbers from
    # the wrong tree, and those numbers look perfectly ordinary.
    shadow = opt_arg("--shadow")
    # --mw <version> compiles with a different CodeWarrior under build/compilers,
    # which is how you tell a patch cost from a source problem. `--mw GC/2.0p1`
    # is the stock compiler this branch's GC/2.0p1a is derived from.
    mw = opt_arg("--mw") or info["mw"]
    cmd = cwexec.compile_prefix(NINJA, info["rule"], mw) + \
        (["-i", shadow] if shadow else []) + \
        shlex.split(info["flags"], posix=False) + \
        ["-c", info["src"], "-o", out]
    cmd = [c.strip('"') if c.startswith('"') and c.endswith('"') else c for c in cmd]
    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, timeout=600)
    if not os.path.exists(out):
        raise SystemExit("COMPILE FAILED:\n" + (r.stdout + r.stderr)[-3000:])
    return td, out


def diff(unit, obj, symbol=None):
    out = os.path.join(os.path.dirname(obj), "d.json")
    args = [CLI, "diff", "-1", os.path.join(ROOT, unit["target_path"]),
            "-2", obj, "-o", out, "--format", "json"]
    # `objdiff-cli diff` defaults to functionRelocDiffs=name_address, but
    # `report generate` -- which writes the report.json the project is scored
    # on -- uses `none`. Under the default, every anonymous @NNN relocation
    # whose ordinal differs counts as a differing row, and solo compiles into
    # a private temp dir where mwcc numbers those differently from the real
    # build, so the rows are an artifact of the measurement rather than of the
    # source. On zNPCTypeBossSB2 that was the difference between 27
    # non-matching and 9. Score what the project scores; --relocs restores the
    # old behaviour for the rare case where you are chasing a real relocation.
    if "--relocs" not in sys.argv:
        args += ["-c", "functionRelocDiffs=none"]
    if symbol:
        args.append(symbol)
    subprocess.run(args, cwd=ROOT, capture_output=True)
    return json.load(open(out))


def text(entry):
    ins = entry.get("instruction")
    if not ins:
        return ""
    s = ins.get("formatted", "")
    rel = ins.get("relocation")
    if rel:
        s += "  <%s>" % rel.get("target_symbol")
    return s


def cleanup(td):
    for f in os.listdir(td):
        try:
            os.unlink(os.path.join(td, f))
        except OSError:
            pass
    try:
        os.rmdir(td)
    except OSError:
        pass


def opt_arg(flag):
    """Value following `flag` on the command line, or None."""
    if flag in sys.argv:
        i = sys.argv.index(flag)
        if i + 1 < len(sys.argv):
            return sys.argv[i + 1]
    return None


def emit_rows(rows, limit, noun):
    """Print `rows`, honouring `limit`, and say what was withheld."""
    shown = rows if not limit else rows[:limit]
    for r in shown:
        print(r)
    if len(shown) < len(rows):
        print("  ... %d more %s (re-run without --top to see them)"
              % (len(rows) - len(shown), noun))


def main():
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    frag = sys.argv[1]
    sym = sys.argv[2] if len(sys.argv) > 2 and not sys.argv[2].startswith("-") else None
    missing = "--missing" in sys.argv
    bands = "--bands" in sys.argv
    quiet = "-q" in sys.argv or "--quiet" in sys.argv
    full = "--full" in sys.argv
    top = int(opt_arg("--top") or 0)
    ctx = int(opt_arg("-C") or opt_arg("--context") or 3)
    unit = find_unit(frag)
    td, obj = compile_unit(unit)
    try:
        d = diff(unit, obj, sym)
        # An object with no symbols at all - a stub unit - has no "symbols" key.
        left = {s["name"]: s for s in d.get("left", {}).get("symbols", [])
                if s.get("kind") == "SYMBOL_FUNCTION"}
        right = {s["name"]: s for s in d.get("right", {}).get("symbols", [])
                 if s.get("kind") == "SYMBOL_FUNCTION"}

        if bands:
            # Absent functions are reported separately rather than folded into
            # the 0% band: "not written" and "written wrong" are different work,
            # and conflating them has sent agents at the wrong target before.
            absent = [s for n, s in left.items() if n not in right]
            present = [s for n, s in left.items() if n in right]
            edges = [(100.0, 100.1, "exact"), (99.0, 100.0, "99-100"),
                     (90.0, 99.0, "90-99"), (50.0, 90.0, "50-90"),
                     (0.0, 50.0, "<50")]
            print("%s: %d target functions" % (unit["name"], len(left)))
            for lo, hi, label in edges:
                sel = [s for s in present
                       if lo <= s.get("match_percent", 0.0) < hi]
                if sel:
                    print("  %-7s %4d  %8d b" % (
                        label, len(sel), sum(int(s.get("size", 0)) for s in sel)))
            if absent:
                print("  %-7s %4d  %8d b  (no symbol in our object)" % (
                    "absent", len(absent),
                    sum(int(s.get("size", 0)) for s in absent)))
            return

        if missing:
            gone = [(int(s.get("size", 0)), n) for n, s in left.items()
                    if n not in right]
            print("%s: %d target functions not in our object"
                  % (unit["name"], len(gone)))
            if not quiet:
                emit_rows(["  %6db  %s" % (size, name)
                           for size, name in sorted(gone)], top, "absent")
            return

        if not sym:
            bad = [(n, s.get("match_percent", 0.0), int(s.get("size", 0)))
                   for n, s in left.items() if s.get("match_percent", 0.0) < 100.0]
            print("%s: %d non-matching of %d" % (unit["name"], len(bad), len(left)))
            if not quiet:
                emit_rows(["  %7.3f%%  %6db  %s" % (pct, size, name)
                           for name, pct, size in sorted(bad, key=lambda x: -x[1])],
                          top, "non-matching")
            return

        for name in [n for n in left if sym in n]:
            a, b = left[name], right.get(name)
            print("\n=== %s  %.3f%% ===" % (name, a.get("match_percent", 0.0)))
            ai = a.get("instructions", [])
            bi = (b or {}).get("instructions", [])
            rows = []
            for k in range(max(len(ai), len(bi))):
                ea = ai[k] if k < len(ai) else {}
                eb = bi[k] if k < len(bi) else {}
                kind = ea.get("diff_kind") or eb.get("diff_kind") or ""
                differs = bool(kind) and kind != "DIFF_NONE"
                rows.append((differs, " %s %-46s %s"
                             % ("|" if differs else " ", text(ea), text(eb))))

            if full:
                keep = set(range(len(rows)))
            else:
                keep = set()
                for k, (differs, _) in enumerate(rows):
                    if differs:
                        keep.update(range(max(0, k - ctx),
                                          min(len(rows), k + ctx + 1)))

            prev = None
            for k in sorted(keep):
                if prev is not None and k > prev + 1:
                    print("      ... %d identical" % (k - prev - 1))
                print(rows[k][1])
                prev = k
            hidden = len(rows) - len(keep)
            if hidden and not full:
                print("      (%d of %d rows identical and withheld; --full shows all)"
                      % (hidden, len(rows)))
    finally:
        cleanup(td)


main()
