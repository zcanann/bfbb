#!/usr/bin/env python3
"""Rank alias-query signatures by how often they occur in non-matching functions,
and measure a candidate clause by forcing its answer in the running compiler.

  sigrank.py snap <name> [js]        compile every game unit with GC/2.0p1a
                                     (under frida if js is given), record every
                                     function's match percent
  sigrank.py cmp <a> <b>             gains / losses / partials between two snaps
  sigrank.py probe <out.json>        per-function counts of may_alias query
                                     signatures (sigprobe.js)
  sigrank.py rank <probe.json> <snap> [n]
                                     signatures ranked by non-matching count
                                     over matching count
  sigrank.py flip <name> '<json list of signatures>'
                                     snap with those answers inverted (flip.js),
                                     then cmp against the snap named in $BASE

A signature is `<a>|<b>=<answer>` for may_alias(a, b), each side
`<opclass>[*][c][v][D]:<storage>[~]<size>`: `*` indirect (flag 0x20), `c` const
(0x40), `v` volatile (0x80), `D` a direct `sym@sda21` operand; storage is `lit`
(anonymous static), `st` (named static), `fr` (declared frame local), `tmp`
(frame temporary: Object+0x18 zero or an `@` name), `set` (alias set); `~` a
subrange; size bucket 4 / 8 / 16 / L. Clause W was found as the top-ranked
`lfsD:lit4|swD:fr~4=0`.

Snapshots and objects go to build/sigrank/. Compiles bypass sjiswrap so the
hooks land in the compiler process; a null script reproduces ninja's objects
byte for byte.
"""
import hashlib, json, os, re, shlex, subprocess, sys, threading
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(ROOT, "build", "sigrank")
sys.path.insert(0, os.path.join(ROOT, "tools"))
import cwexec  # noqa: E402

CLI = cwexec.objdiff_cli(ROOT)
CFG = json.load(open(os.path.join(ROOT, "objdiff.json")))
NINJA = open(os.path.join(ROOT, "build.ninja")).read()
BUILD_RE = re.compile(
    r"^build (?P<obj>\S+\.o): (?P<rule>mwcc_sjis|mwcc) (?P<body>(?:.*\n)*?)  basedir", re.M)
SRC_RE = re.compile(r"(?:^|\s)(src[\\/]\S+\.(?:c|cp|cpp))(?:\s|$)")
JOBS = int(os.environ.get("J", "8"))


def rules():
    out = {}
    for m in BUILD_RE.finditer(NINJA):
        body = m.group("body")
        mw = re.search(r"mw_version = (\S+)", body)
        cf = re.search(r"cflags = ((?:.*\$\n)*.*)\n", body)
        src = SRC_RE.search(body)
        if mw and cf and src:
            out[m.group("obj").replace("\\", "/")] = dict(
                src=src.group(1).replace("\\", "/"), mw=mw.group(1).replace("\\", "/"),
                rule=m.group("rule"),
                flags=re.sub(r"\s+", " ", cf.group(1).replace("$\n", " ")).strip())
    return out


RULES = rules()
GAME = [u for u in CFG["units"] if u["name"].startswith("main/SB/")
        and u.get("base_path") and u.get("target_path")]
LOCK = threading.Lock()
DEV = [None]


def frida_compile(cmd, js):
    import frida
    msgs, done = [], threading.Event()
    with LOCK:
        if DEV[0] is None:
            DEV[0] = frida.get_local_device()
        pid = DEV[0].spawn(cmd, cwd=ROOT)
        try:
            sess = DEV[0].attach(pid)
            script = sess.create_script(js)
            script.on("message", lambda m, d: msgs.append(m))
            script.load()
            sess.on("detached", lambda *a: done.set())
        except Exception as e:
            DEV[0].kill(pid)
            return [{"type": "error", "description": str(e)}]
        DEV[0].resume(pid)
    done.wait(900)
    return msgs


def compile_unit(u, out, js):
    info = RULES[u["base_path"].replace("\\", "/")]
    cmd = cwexec.compile_prefix(NINJA, info["rule"], info["mw"]) + \
        shlex.split(info["flags"], posix=False) + ["-c", info["src"], "-o", out]
    cmd = [c[1:-1] if len(c) > 1 and c[0] == c[-1] == '"' else c for c in cmd]
    if os.path.exists(out):
        os.unlink(out)
    if js is None:
        subprocess.run(cmd, cwd=ROOT, capture_output=True)
        return []
    cmd = [os.path.join(ROOT, c) if c.startswith("build") else c for c in cmd[:2]] + cmd[2:]
    if "sjiswrap" in cmd[0]:
        cmd = cmd[1:]
    return frida_compile(cmd, js)


def measure(u, out):
    dj = out + ".json"
    subprocess.run([CLI, "diff", "-1", os.path.join(ROOT, u["target_path"]), "-2", out,
                    "-o", dj, "--format", "json", "-c", "functionRelocDiffs=none"],
                   cwd=ROOT, capture_output=True)
    d = json.load(open(dj))
    os.unlink(dj)
    right = {s["name"] for s in d.get("right", {}).get("symbols", [])
             if s.get("kind") == "SYMBOL_FUNCTION"}
    return {s["name"]: [s.get("match_percent", 0.0) if s["name"] in right else -1.0,
                        int(s.get("size", 0))]
            for s in d.get("left", {}).get("symbols", []) if s.get("kind") == "SYMBOL_FUNCTION"}


def snap(name, js=None):
    odir = os.path.join(OUT, name)
    os.makedirs(odir, exist_ok=True)

    def one(u):
        out = os.path.join(odir, u["name"].replace("main/SB/", "").replace("/", "_") + ".o")
        msgs = compile_unit(u, out, js)
        errs = [m for m in msgs if m.get("type") == "error"]
        if errs or not os.path.exists(out):
            return u["name"], None, str(errs[:1]) or "compile failed"
        return u["name"], {"funcs": measure(u, out),
                           "sha1": hashlib.sha1(open(out, "rb").read()).hexdigest()}, None

    res, bad = {}, []
    with ThreadPoolExecutor(JOBS) as ex:
        for un, r, err in ex.map(one, GAME):
            if err:
                bad.append((un, err))
            else:
                res[un] = r
    for b in bad:
        print("ERR", *b)
    json.dump(res, open(os.path.join(OUT, name + ".json"), "w"))
    exact = sum(1 for u in res.values() for p, _ in u["funcs"].values() if p >= 100.0)
    print("%s: %d units, %d exact, %d errors" % (name, len(res), exact, len(bad)))
    return not bad


def cmp(a, b):
    A = json.load(open(os.path.join(OUT, a + ".json")))
    B = json.load(open(os.path.join(OUT, b + ".json")))
    groups = {"GAINS": [], "LOSSES": [], "UP": [], "DOWN": []}
    changed = 0
    for un in sorted(set(A) & set(B)):
        changed += A[un]["sha1"] != B[un]["sha1"]
        for f, (pa, sz) in A[un]["funcs"].items():
            pb = B[un]["funcs"].get(f, [pa])[0]
            if pa == pb:
                continue
            row = (un.replace("main/SB/", ""), f, pa, pb, sz)
            key = "GAINS" if pb >= 100 > pa else "LOSSES" if pa >= 100 > pb else \
                "UP" if pb > pa else "DOWN"
            groups[key].append(row)
    for k, rows in groups.items():
        print("== %s %d (%d b)" % (k, len(rows), sum(r[4] for r in rows)))
        for r in sorted(rows, key=lambda r: r[2] - r[3]):
            print("  %-26s %8.3f -> %8.3f %6d  %s" % (r[0][:26], r[2], r[3], r[4], r[1][:90]))
    print("objects changed: %d" % changed)


def probe(outp):
    js = open(os.path.join(HERE, "sigprobe.js")).read()
    res = {}

    def one(u):
        out = os.path.join(OUT, "probe_" + u["name"].replace("/", "_") + ".o")
        d = {}
        for m in compile_unit(u, out, js):
            p = m.get("payload")
            if isinstance(p, dict) and "fn" in p:
                e = d.setdefault(p["fn"], {})
                for k, v in p["c"].items():
                    e[k] = e.get(k, 0) + v
        return u["name"], d

    os.makedirs(OUT, exist_ok=True)
    with ThreadPoolExecutor(JOBS) as ex:
        for un, d in ex.map(one, GAME):
            res[un] = d
    json.dump(res, open(outp, "w"))


def rank(probep, snapname, n=50):
    sig = json.load(open(probep))
    s = json.load(open(os.path.join(OUT, snapname + ".json")))
    nm, m, ex = {}, {}, {}
    for un, fns in sig.items():
        funcs = s.get(un, {}).get("funcs", {})
        for f, c in fns.items():
            if f not in funcs:
                continue
            tgt = m if funcs[f][0] >= 100 else nm
            for k in c:
                tgt[k] = tgt.get(k, 0) + 1
                if tgt is nm:
                    ex.setdefault(k, []).append(f[:28])
    rows = sorted(nm, key=lambda k: -nm[k] / (m.get(k, 0) + 2))
    for k in rows[:n]:
        print("%4d %5d %-44s %s" % (nm[k], m.get(k, 0), k, ex[k][:3]))


if __name__ == "__main__":
    a = sys.argv[1:]
    if a[0] == "snap":
        snap(a[1], open(a[2]).read() if len(a) > 2 else None)
    elif a[0] == "cmp":
        cmp(a[1], a[2])
    elif a[0] == "probe":
        probe(a[1])
    elif a[0] == "rank":
        rank(a[1], a[2], int(a[3]) if len(a) > 3 else 50)
    elif a[0] == "flip":
        js = open(os.path.join(HERE, "flip.js")).read().replace("%SIGS%", a[2])
        if snap(a[1], js):
            cmp(os.environ.get("BASE", "base"), a[1])
    else:
        sys.exit(__doc__)
