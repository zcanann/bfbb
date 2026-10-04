#!/usr/bin/env python3
"""locfix.py: which of a function's residual hunks does ANOTHER compiler get right?

    locfix.py --sweep sweep.json --base 2.0p1g --alt 3.0a3[,3.0a5.2] [--set sb|rw] [--k 2]
    locfix.py --unit SB/Game/zFoo --fn sym --base 2.0p1g --alt 3.0a3     one function, print hunks

A later compiler can be far from retail overall (prologue scheduling, register numbering)
and still reproduce one local decision retail made. Per-function scores hide that, so this
works per hunk: difflib-align target vs base; for every non-equal hunk take the target window
(hunk +-k rows) and ask whether it occurs contiguously in the alt compiler's code (exact, then
with registers normalised). A window the alt reproduces and the base does not is a lead.

Run from a tree root (it compiles with that tree's build.ninja / objdiff.json; nothing in
build/ is written).
"""
import argparse, difflib, json, os, re, shlex, subprocess, sys, tempfile
from concurrent.futures import ThreadPoolExecutor

ROOT = os.getcwd()
sys.path.insert(0, os.path.join(ROOT, "tools"))
import cwexec  # noqa
NINJA = open(os.path.join(ROOT, "build.ninja")).read()
CFG = json.load(open(os.path.join(ROOT, "objdiff.json")))
BUILD_RE = re.compile(r"^build (?P<obj>\S+\.o):(?: \$\n\s+| )(?P<rule>mwcc_sjis|mwcc) (?P<body>(?:.*\n)*?)  basedir", re.M)
SRC_RE = re.compile(r"(?:^|\s)(src[\\/]\S+\.(?:c|cp|cpp))(?:\s|$)")
RULES = {}
for m in BUILD_RE.finditer(NINJA):
    body = m.group("body")
    cf = re.search(r"cflags = ((?:.*\$\n)*.*)\n", body); src = SRC_RE.search(body)
    if cf and src:
        RULES[m.group("obj").replace("\\", "/")] = (m.group("rule"), src.group(1),
                                                    re.sub(r"\s+", " ", cf.group(1).replace("$\n", " ")).strip())
TD = tempfile.mkdtemp(prefix="locfix_")


def disasm(obj):
    asm = obj + ".s"
    subprocess.run([cwexec.dtk(ROOT), "elf", "disasm", obj, asm], capture_output=True)
    fns = {}
    if not os.path.exists(asm):
        return fns
    for m in re.finditer(r"^\.fn (\S+),.*?\n(.*?)^\.endfn \1", open(asm).read(), re.M | re.S):
        lines = []
        for ln in m.group(2).splitlines():
            ln = re.sub(r"/\*.*?\*/", "", ln).strip()
            if not ln or ln.startswith("#") or ln.endswith(":"):
                continue
            ln = re.sub(r"@\d+", "@lit", ln)
            ln = re.sub(r"\.L_[0-9A-F]+", ".L", ln)
            ln = re.sub(r"\s+", " ", ln)
            lines.append(ln)
        fns[m.group(1)] = lines
    return fns


def unit_rec(unit):
    for u in CFG["units"]:
        if u["name"] == "main/" + unit:
            return u


def compile_unit(unit, mw):
    u = unit_rec(unit)
    rule, src, flags = RULES[u["base_path"].replace("\\", "/")]
    mwarg = mw if mw.startswith("GC/") or "/" in mw or "\\" in mw else "GC/" + mw
    out = os.path.join(TD, re.sub(r"\W", "_", unit + "_" + mw)[-90:] + ".o")
    cmd = cwexec.compile_prefix(NINJA, rule, mwarg) + shlex.split(flags, posix=False) + ["-c", src, "-o", out]
    cmd = [c[1:-1] if len(c) > 1 and c[0] == c[-1] == '"' else c for c in cmd]
    subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    return disasm(out) if os.path.exists(out) else None


REG = re.compile(r"\b([rf])(\d+)\b")


def normreg(lines):
    return [REG.sub(lambda m: m.group(1) + "N", l) for l in lines]


def contains(hay, needle):
    n = len(needle)
    if n == 0:
        return True
    for i in range(len(hay) - n + 1):
        if hay[i:i + n] == needle:
            return True
    return False


def hunks(tgt, base, k):
    sm = difflib.SequenceMatcher(None, tgt, base, autojunk=False)
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            continue
        a, b = max(0, i1 - k), min(len(tgt), i2 + k)
        yield (i1, i2, j1, j2, tgt[a:b])


def analyse(unit, fns, base, alts, k, verbose=False):
    u = unit_rec(unit)
    tgt = disasm_target(u)
    cb = compile_unit(unit, base)
    ca = {a: compile_unit(unit, a) for a in alts}
    rows = []
    for fn in fns:
        if fn not in tgt or not cb or fn not in cb:
            continue
        for i1, i2, j1, j2, win in hunks(tgt[fn], cb[fn], k):
            res = {}
            for a in alts:
                if not ca[a] or fn not in ca[a]:
                    res[a] = "-"
                    continue
                ex = contains(ca[a][fn], win)
                nr = contains(normreg(ca[a][fn]), normreg(win))
                res[a] = "EXACT" if ex else ("REGN" if nr else "")
            rows.append((fn, i1, i2, res, win, cb[fn][max(0, j1 - k):j2 + k]))
            if verbose:
                print("--- %s target[%d:%d]  %s" % (fn, i1, i2, res))
                print("   target: " + " | ".join(win))
                print("   base  : " + " | ".join(cb[fn][max(0, j1 - k):j2 + k]))
    return rows


_tcache = {}


def disasm_target(u):
    p = os.path.join(ROOT, u["target_path"])
    if p not in _tcache:
        import shutil
        dst = os.path.join(TD, re.sub(r"\W", "_", u["name"]) + "_tgt.o")
        shutil.copy(p, dst)
        _tcache[p] = disasm(dst)
    return _tcache[p]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sweep"); ap.add_argument("--base", default="2.0p1g")
    ap.add_argument("--alt", default="3.0a3"); ap.add_argument("--k", type=int, default=2)
    ap.add_argument("--unit"); ap.add_argument("--fn")
    ap.add_argument("--jobs", type=int, default=16)
    a = ap.parse_args()
    alts = a.alt.split(",")
    if a.unit:
        analyse(a.unit, [a.fn], a.base, alts, a.k, verbose=True)
        return
    d = json.load(open(a.sweep))[a.base]
    work = [(u, list(r["bad"])) for u, r in d.items() if r["bad"] and not r["err"]]
    with ThreadPoolExecutor(a.jobs) as ex:
        allrows = list(ex.map(lambda w: (w[0], analyse(w[0], w[1], a.base, alts, a.k)), work))
    for unit, rows in allrows:
        for fn, i1, i2, res, win, bw in rows:
            struct = not contains(normreg(bw), normreg(win))
            key = res.get(alts[0])
            if key == "EXACT" or (struct and key == "REGN"):
                print("%s %s target[%d:%d] %s%s" % (unit, fn, i1, i2, res, " STRUCT" if struct else ""))
                print("   target: " + " | ".join(win))
                print("   base  : " + " | ".join(bw))


if __name__ == "__main__":
    main()
