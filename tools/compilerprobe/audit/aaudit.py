#!/usr/bin/env python3
"""aaudit.py: tree-wide audit of the 2.0p1a alias clauses, one situation class at a time.

    aaudit.py log  <out.json> [--set sb|rw|all] [--mw REL]      compile every unit, log each clause firing
    aaudit.py keys <log.json> [--re RX] [--by fn|unit|n]        list class keys with counts
    aaudit.py abl  <log.json> <name> <regex> [--mw REL]         ablate every firing whose key matches regex,
                                                              recompiling only the units where it fires
    aaudit.py show <log.json> <name>                            gains / losses of an ablation
    aaudit.py batch <log.json> <spec.txt>                       many ablations: lines "name<TAB>regex"

Hooks (audit.js) sit on the clause functions of GC/2.0p1f(+ssi)'s .sbpatch blob; nothing on
disk is modified. A clause fires when it answers 1; its class key is
    <clause>@<entry>:<descA>|<descB>[=same]
    desc = <opclass>[*][c][v][D]:<storage>:<w|s><sizebucket>
    storage: lit (.sdata2 literal) st (named static) fr+ / fr- (declared frame object whose
             address escapes / does not) tmp (compiler temp) pso (const-pointee pseudo object)
             WC / S (alias set) nil
  V@<vnentry>:<store storage:kind size>|kill:<lit|st>:<w|s>     the VN literal-kill walk
  F@0:<store>      SV@1:<store>[D]      LICM:<load desc>
Ablation replaces the clause's answer by 0 (the remaining clauses and then the stock compiler
decide); for V it skips the matching kills. Only units whose baseline log contains a matching
key can change, so only those are recompiled.
--mw is a path relative to build/compilers (default ../../xcc/ssi, the 2.0p1f+ssi build
from patch_licm_x.py --parts ssi; byte-identical to GC/2.0p1g).
"""
import argparse, collections, json, os, re, sys, tempfile, hashlib
from concurrent.futures import ThreadPoolExecutor
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
import qprobe  # noqa
import rulesnap  # noqa

JS = open(os.path.join(HERE, "audit.js")).read()
DEFMW = "../../xcc/ssi"
OUT = os.environ.get("AUDITDIR", os.path.join(HERE, "..", "w", "audit"))


def js(abl, log, fn=None, vw=True):
    """vw: install the (slow) JS re-implementation of the V walk; needed to log or ablate V."""
    return JS.replace("%ABL%", json.dumps(abl)).replace("%LOG%", "true" if log else "false")         .replace("%FN%", json.dumps(fn)).replace("%VW%", "true" if (log or vw) else "false")


def needs_vw(base, rx):
    return any(k.startswith("V@") and re.search(rx, k) for r in base.values() for k in (r.get("log") or {}))


def compile_unit(u, out, js, mw):
    """qprobe.compile_unit, plus an ack handshake: the script's exit hook blocks in recv('ack')
    until the host has its final message (otherwise the process dies with it unsent)."""
    import frida, shlex, threading
    info = qprobe.RULES[u["base_path"].replace("\\", "/")]
    cmd = qprobe.cwexec.compile_prefix(qprobe.NINJA, info["rule"], mw or info["mw"]) +         shlex.split(info["flags"], posix=False) + ["-c", info["src"], "-o", out]
    cmd = [c[1:-1] if len(c) > 1 and c[0] == c[-1] == '"' else c for c in cmd]
    if "sjiswrap" in cmd[0]:
        cmd = cmd[1:]
    cmd = [os.path.join(qprobe.ROOT, cmd[0])] + cmd[1:]
    if os.path.exists(out):
        os.unlink(out)
    msgs, done = [], threading.Event()
    holder = {}

    def on_msg(m, d):
        msgs.append(m)
        if m.get("type") == "send" and isinstance(m.get("payload"), dict) and "nabl" in m["payload"]:
            holder["s"].post({"type": "ack"})
    with qprobe.LOCK:
        if qprobe.DEV[0] is None:
            qprobe.DEV[0] = frida.get_local_device()
        pid = qprobe.DEV[0].spawn(cmd, cwd=qprobe.ROOT)
        sess = qprobe.DEV[0].attach(pid)
        script = sess.create_script(js)
        holder["s"] = script
        script.on("message", on_msg)
        script.load()
        sess.on("detached", lambda *a: done.set())
        qprobe.DEV[0].resume(pid)
    done.wait(1800)
    return msgs


def compile_one(u, script, mw):
    td = tempfile.mkdtemp(prefix="aa_")
    out = os.path.join(td, "o.o")
    try:
        msgs = compile_unit(u, out, script, mw)
    except Exception as e:
        return None, str(e)
    errs = [m for m in msgs if m.get("type") == "error"]
    if errs or not os.path.exists(out):
        return None, (str(errs[:1]) if errs else "compile failed")
    pay = [m["payload"] for m in msgs if m.get("type") == "send"]
    log = next((p.get("log") for p in pay if "nabl" in p), None)
    nabl = sum(p.get("nabl", 0) for p in pay)
    sc = qprobe.score(u, out, None)
    sha = hashlib.sha1(open(out, "rb").read()).hexdigest()
    import shutil
    shutil.rmtree(td, ignore_errors=True)
    return {"f": sc, "sha": sha, "nabl": nabl, "log": log}, None


def run_units(us, script, mw, jobs):
    res, bad = {}, []
    with ThreadPoolExecutor(jobs) as ex:
        for u, (r, err) in zip(us, ex.map(lambda u: compile_one(u, script, mw), us)):
            if err:
                bad.append((u["name"], err))
            else:
                res[u["name"]] = r
    for b in bad:
        print("ERR", b[0], b[1][:300], flush=True)
    return res, bad


def cmd_log(a):
    us = rulesnap.units(a.set)
    res, bad = run_units(us, js(None, True), a.mw, a.jobs)
    json.dump(res, open(a.out, "w"))
    ex = sum(1 for r in res.values() for p in r["f"].values() if p >= 100.0)
    print("%d units, %d exact fns, %d errors" % (len(res), ex, len(bad)))


def keystats(base, rx=None):
    K = collections.defaultdict(lambda: {"n": 0, "fns": set(), "units": set()})
    for un, r in base.items():
        for k, fns in (r.get("log") or {}).items():
            if rx and not re.search(rx, k):
                continue
            e = K[k]
            for fn, c in fns.items():
                e["n"] += c
                e["fns"].add((un, fn))
            e["units"].add(un)
    return K


def cmd_keys(a):
    base = json.load(open(a.log))
    K = keystats(base, a.re)
    rows = sorted(K.items(), key=lambda kv: -len(kv[1]["fns"]))
    for k, e in rows[: a.top]:
        print("%7d %5d %4d  %s" % (e["n"], len(e["fns"]), len(e["units"]), k))
    print(len(K), "keys")


def ablate(base, name, rx, mw, jobs, quiet=False):
    us_all = {u["name"]: u for u in rulesnap.units("all")}
    hit = sorted({un for un, r in base.items() for k in (r.get("log") or {}) if re.search(rx, k)})
    us = [us_all[un] for un in hit if un in us_all]
    res, bad = run_units(us, js(rx, False, None, needs_vw(base, rx)), mw, jobs)
    G = diff(base, res)
    G["units"] = len(us)
    G["nabl"] = sum(r["nabl"] for r in res.values())
    G["rx"] = rx
    os.makedirs(OUT, exist_ok=True)
    json.dump({"rx": rx, "res": res, "G": G}, open(os.path.join(OUT, name + ".json"), "w"))
    if not quiet:
        report(name, G)
    return G


def diff(base, res):
    G = {"GAINS": [], "LOSSES": [], "UP": [], "DOWN": [], "changed": 0}
    for un, r in res.items():
        b = base[un]
        G["changed"] += b["sha"] != r["sha"]
        for f, pb in r["f"].items():
            pa = b["f"].get(f, pb)
            if abs(pa - pb) < 1e-9:
                continue
            row = (un.replace("main/", ""), f, pa, pb)
            k = "GAINS" if pb >= 100 > pa else "LOSSES" if pa >= 100 > pb else "UP" if pb > pa else "DOWN"
            G[k].append(row)
    return G


def report(name, G, full=True):
    print("== %s  +%d/-%d  (up %d, down %d; %d units, %d objs changed, %d answers ablated)  %s" % (
        name, len(G["GAINS"]), len(G["LOSSES"]), len(G["UP"]), len(G["DOWN"]), G["units"], G["changed"],
        G["nabl"], G["rx"]), flush=True)
    if full:
        for k in ("GAINS", "LOSSES"):
            for r in sorted(G[k], key=lambda r: r[1]):
                print("   %s %-28s %-60s %7.2f -> %7.2f" % (k[0], r[0][-28:], r[1][:60], r[2], r[3]))


def cmd_abl(a):
    base = json.load(open(a.log))
    ablate(base, a.name, a.rx, a.mw, a.jobs)


def cmd_show(a):
    d = json.load(open(os.path.join(OUT, a.name + ".json")))
    report(a.name, d["G"])
    if a.moves:
        for k in ("UP", "DOWN"):
            for r in sorted(d["G"][k], key=lambda r: r[1]):
                print("   %s %-28s %-60s %7.2f -> %7.2f" % (k[0], r[0][-28:], r[1][:60], r[2], r[3]))


def cmd_batch(a):
    base = json.load(open(a.log))
    for line in open(a.spec):
        line = line.rstrip("\n")
        if not line.strip() or line.startswith("#"):
            continue
        name, rx = line.split("\t", 1)
        if os.path.exists(os.path.join(OUT, name + ".json")) and not a.redo:
            d = json.load(open(os.path.join(OUT, name + ".json")))
            if d["rx"] == rx:
                report(name, d["G"])
                continue
        ablate(base, name, rx, a.mw, a.jobs)


def cmd_table(a):
    """One row per ablation: firing stats from the log, then +gains/-losses, up/down."""
    base = json.load(open(a.log))
    K = keystats(base)
    rows = []
    for fn in sorted(os.listdir(OUT)):
        if not fn.endswith(".json"):
            continue
        d = json.load(open(os.path.join(OUT, fn)))
        G, rx = d["G"], d["rx"]
        n = f = u = 0
        for k, e in K.items():
            if re.search(rx, k):
                n += e["n"]; f += len(e["fns"]); u += len(e["units"])
        rows.append((fn[:-5], n, f, u, len(G["GAINS"]), len(G["LOSSES"]), len(G["UP"]), len(G["DOWN"]), rx))
    if a.nonzero:
        rows = [r for r in rows if r[4] or r[5] or r[6] or r[7]]
    for r in sorted(rows, key=lambda r: (-(r[4] - r[5]), r[0])):
        print("%-10s %7d %5d %4d  +%-3d -%-3d up%-3d dn%-3d  %s" % r)


def cmd_fnabl(a):
    """For every NON-matching function and every class key that fires in it, ablate that key
    inside that function only and score the function: which single answers does a holdout want
    flipped back to stock?"""
    base = json.load(open(a.log))
    us_all = {u["name"]: u for u in rulesnap.units("all")}
    jobs = []
    for un, r in base.items():
        bad = {f for f, p in r["f"].items() if p < 100}
        for k, fm in (r.get("log") or {}).items():
            for f in fm:
                if f in bad and (not a.re or re.search(a.re, k)):
                    jobs.append((un, f, k))

    def one(j):
        un, f, k = j
        r, err = compile_one(us_all[un], js("^" + re.escape(k) + "$", False, f, k.startswith("V@")), a.mw)
        return j, (r["f"] if r else None), (r["nabl"] if r else 0), err
    out = []
    with ThreadPoolExecutor(a.jobs) as ex:
        for (un, f, k), sc, nabl, err in ex.map(one, jobs):
            if err:
                print("ERR", un, f, k, err[:200]); continue
            b = base[un]["f"]
            ch = {g: (b.get(g, 0), p) for g, p in sc.items() if abs(b.get(g, 0) - p) > 1e-9}
            out.append({"unit": un, "fn": f, "key": k, "nabl": nabl, "self": [b.get(f), sc.get(f)], "other": ch})
    json.dump(out, open(a.out, "w"), indent=0)
    for o in sorted(out, key=lambda o: -((o["self"][1] or 0) - (o["self"][0] or 0))):
        d = (o["self"][1] or 0) - (o["self"][0] or 0)
        if d > 1e-9:
            oth = [g for g, (x, y) in o["other"].items() if g != o["fn"] and x >= 100 > y]
            print("%+7.2f %7.2f->%7.2f %-30s %-40s %s%s" % (d, o["self"][0], o["self"][1], o["unit"][-30:], o["fn"][:40],
                  o["key"], ("  BREAKS " + ",".join(oth)) if oth else ""))


def cmd_diff(a):
    """Compile one unit with an ablation (optionally only inside --fn) and diff fn against the target."""
    u = qprobe.find_unit(a.unit)
    td = tempfile.mkdtemp(prefix="aad_")
    out = os.path.join(td, "o.o")
    msgs = compile_unit(u, out, js(a.rx, False, a.fn if a.only else None), a.mw)
    print("score", qprobe.score(u, out, a.fn))
    rulesnap.show_diff(u, out, a.fn)


def main():
    ap = argparse.ArgumentParser()
    sp = ap.add_subparsers(dest="cmd")
    p = sp.add_parser("log"); p.add_argument("out"); p.add_argument("--set", default="all")
    p = sp.add_parser("keys"); p.add_argument("log"); p.add_argument("--re"); p.add_argument("--top", type=int, default=400)
    p = sp.add_parser("abl"); p.add_argument("log"); p.add_argument("name"); p.add_argument("rx")
    p = sp.add_parser("show"); p.add_argument("name"); p.add_argument("--moves", action="store_true")
    p = sp.add_parser("table"); p.add_argument("log"); p.add_argument("--nonzero", action="store_true")
    p = sp.add_parser("fnabl"); p.add_argument("log"); p.add_argument("out"); p.add_argument("--re")
    p = sp.add_parser("diff"); p.add_argument("unit"); p.add_argument("fn"); p.add_argument("rx", nargs="?")
    p.add_argument("--only", action="store_true")
    p = sp.add_parser("batch"); p.add_argument("log"); p.add_argument("spec"); p.add_argument("--redo", action="store_true")
    for p in sp.choices.values():
        p.add_argument("--mw", default=DEFMW); p.add_argument("--jobs", type=int, default=int(os.environ.get("J", "16")))
    a = ap.parse_args()
    {"log": cmd_log, "keys": cmd_keys, "abl": cmd_abl, "show": cmd_show, "batch": cmd_batch, "table": cmd_table, "fnabl": cmd_fnabl, "diff": cmd_diff}[a.cmd](a)


if __name__ == "__main__":
    main()
