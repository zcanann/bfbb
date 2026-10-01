#!/usr/bin/env python3
"""Compile the compiler-variant repros with every compiler and compare.

    python tools/compilerprobe/repros/run.py                  all repros, summary
    python tools/compilerprobe/repros/run.py b_*.c            some repros
    python tools/compilerprobe/repros/run.py r3_call.c --show f_const
                                                              per-compiler asm of one function
    --mw 2.0p1,2.5,...   compilers to use (default below; a version under
                         build/compilers/GC, or an absolute directory holding
                         a mwcceppc.exe -- e.g. a scratch single-part build)
    --flags rw|sb        RenderWare (C) flags, the default, or game (C++) flags

Flags are read from build.ninja (the first rwsdk unit built with GC/2.0p1a, 2.0p1d or 2.0p1e for rw,
SB/Core/x/xAnim for sb), so they are exactly the project's. Nothing in build/
is written; objects go to a temp dir.

For every function the summary prints the compilers grouped by IDENTICAL
code (relocation targets and branch labels normalised), plus the counts of a
few instruction classes the repros are about (lfs/lfd, lwz, lwzx/lbzx, stw),
so "A != C and variant == C" is read straight off a line like

    f_const   [2.0p1 2.0p1a 2.0p1b] [2.5 2.6 2.7 2.0p1c 2.0p1d]

Every repro file states, in its header comment, which grouping is expected.
"""
import glob
import os
import re
import shlex
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import cwexec  # noqa: E402

DEFAULT_MW = "2.0p1,2.5,2.6,2.7,2.0p1a,2.0p1b,2.0p1c,2.0p1d,2.0p1e"
NINJA = open(os.path.join(ROOT, "build.ninja")).read()
BUILD_RE = re.compile(
    r"^build (?P<obj>\S+\.o):(?: \$\n\s+| )(?P<rule>mwcc_sjis|mwcc) (?P<body>(?:.*\n)*?)  basedir", re.M)


def unit_flags(kind):
    for m in BUILD_RE.finditer(NINJA):
        body = m.group("body")
        mw = re.search(r"mw_version = (\S+)", body).group(1).replace("\\", "/")
        obj = m.group("obj").replace("\\", "/")
        if kind == "rw" and not (mw in ("GC/2.0p1a", "GC/2.0p1d", "GC/2.0p1e") and "/rwsdk/" in obj):
            continue
        if kind == "sb" and not obj.endswith("SB/Core/x/xAnim.o"):
            continue
        cf = re.search(r"cflags = ((?:.*\$\n)*.*)\n", body).group(1)
        flags = re.sub(r"\s+", " ", cf.replace("$\n", " ")).strip()
        return m.group("rule"), flags
    raise SystemExit("no %s unit in build.ninja" % kind)


def compile_one(src, mw, kind, td):
    rule, flags = unit_flags(kind)
    if os.path.isabs(mw):
        rel = os.path.relpath(mw, os.path.join(ROOT, "build", "compilers"))
        prefix = cwexec.compile_prefix(NINJA, rule, rel)
    else:
        prefix = cwexec.compile_prefix(NINJA, rule, "GC/" + mw)
    out = os.path.join(td, re.sub(r"\W", "_", mw)[-40:] + "_" + os.path.basename(src) + ".o")
    if kind == "sb" and not src.endswith(".cpp"):
        flags = flags.replace("-lang=c++", "-lang=c")
    cmd = prefix + shlex.split(flags, posix=False) + ["-c", src, "-o", out]
    cmd = [c[1:-1] if len(c) > 1 and c[0] == c[-1] == '"' else c for c in cmd]
    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    if not os.path.exists(out):
        raise SystemExit("compile failed (%s, %s):\n%s%s" % (mw, src, r.stdout, r.stderr))
    asm = out + ".s"
    subprocess.run([cwexec.dtk(ROOT), "elf", "disasm", out, asm], capture_output=True)
    return functions(open(asm).read())


def functions(txt):
    fns = {}
    for m in re.finditer(r"^\.fn (\S+),.*?\n(.*?)^\.endfn \1", txt, re.M | re.S):
        lines = []
        for ln in m.group(2).splitlines():
            ln = re.sub(r"/\*.*?\*/", "", ln).strip()
            if not ln or ln.startswith("#"):
                continue
            ln = re.sub(r"@\d+", "@lit", ln)            # literal pool names
            ln = re.sub(r"\.L_[0-9A-F]+", ".L", ln)     # branch labels
            lines.append(ln)
        fns[m.group(1)] = lines
    return fns


CLASSES = (("lit", r"^lf[sd]\b.*@lit"), ("lf",r"^lf[sd]u?x?\b"), ("lwz", r"^lwzu?\b"), ("lx", r"^l[bhw]z?x\b"),
           ("stw", r"^stwu?\b"))


def counts(lines):
    return " ".join("%s=%d" % (k, sum(1 for l in lines if re.search(p, l))) for k, p in CLASSES)


def short(mw):
    return os.path.basename(mw.rstrip("/\\")) if os.path.isabs(mw) else mw


def main():
    args = sys.argv[1:]

    def opt(name, default):
        if name in args:
            i = args.index(name)
            v = args[i + 1]
            del args[i:i + 2]
            return v
        return default

    mws = opt("--mw", DEFAULT_MW).split(",")
    kind = opt("--flags", "rw")
    show = opt("--show", None)
    pats = args or ["*.c", "*.cpp"]
    srcs = []
    for p in pats:
        srcs += sorted(glob.glob(os.path.join(HERE, p)) or glob.glob(p))
    td = tempfile.mkdtemp(prefix="repro_")
    for src in srcs:
        k = "sb" if src.endswith(".cpp") else kind
        per = {mw: compile_one(src, mw, k, td) for mw in mws}
        print("== %s" % os.path.basename(src))
        names = list(per[mws[0]])
        for fn in names:
            if show and fn != show:
                continue
            groups = []
            for mw in mws:
                body = tuple(per[mw].get(fn, ()))
                for g in groups:
                    if g[0] == body:
                        g[1].append(mw)
                        break
                else:
                    groups.append((body, [mw]))
            label = "  ".join("[%s] %s" % (" ".join(map(short, g[1])), counts(g[0])) for g in groups)
            print("  %-22s %s" % (fn, label))
            if show:
                for body, ms in groups:
                    print("    --- %s (%d insns)" % (" ".join(map(short, ms)), len(body)))
                    for l in body:
                        print("      " + l)


if __name__ == "__main__":
    main()
