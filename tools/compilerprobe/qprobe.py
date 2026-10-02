#!/usr/bin/env python3
"""qprobe.py: per-function may_alias query log and answer-flip search (frida).

    qprobe.py log  <unit> <fnfrag> [--mw GC/x] [--all]       queries in fn, grouped by key
    qprobe.py flip <unit> <fnfrag> <key> [<key>...]          flip those keys (only in fn), score fn
    qprobe.py auto <unit> <fnfrag> [--coarse]                flip each answer-0 key alone, score fn
    qprobe.py pairs <unit> <fnfrag>                          ordered list of distinct (a,b) pairs answered 0

Keys: fine  = "<descA>|<descB>@<site>=<ans>"  (descriptors include object names)
      coarse= sigrank-style "<sigA>|<sigB>@<site>=<ans>"
A descriptor: <opclass>[*][c][v][D]:<kind>:<storage>:<name>+<off>/<size>
  kind w (whole) s (subrange) S (set) WC (worst_case set) nil
  storage lit/st/fr/tmp/pso(pseudo)/hNNN
site: the caller of may_alias (0x511fc0) bucketed by return address.
Only the target function's queries are touched. Compiles bypass sjiswrap.
"""
import json, os, re, shlex, subprocess, sys, threading, tempfile, argparse
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import cwexec  # noqa

CLI = cwexec.objdiff_cli(ROOT)
CFG = json.load(open(os.path.join(ROOT, "objdiff.json")))
NINJA = open(os.path.join(ROOT, "build.ninja")).read()
BUILD_RE = re.compile(r"^build (?P<obj>\S+\.o):(?: \$\n\s+| )(?P<rule>mwcc_sjis|mwcc) (?P<body>(?:.*\n)*?)  basedir", re.M)
SRC_RE = re.compile(r"(?:^|\s)(src[\\/]\S+\.(?:c|cp|cpp))(?:\s|$)")
LOCK = threading.Lock()
DEV = [None]

JS = r"""
var base = Process.getModuleByName("mwcceppc.exe").base;
function va(x) { return base.add(x - 0x400000); }
function rva(p) { return (p.sub(base).toInt32() + 0x400000) >>> 0; }
var getLink = new NativeFunction(va(0x4FE710), "pointer", ["pointer"], "mscdecl");
var TARGET = %TARGET%; var MODE = "%MODE%"; var FLIP = %FLIP%; var ALL = %ALL%;
var WCP = va(0x5e9cb4);
var PSEUDO = va(0x5bd008);
var fn = "?", active = false, counts = {}, order = [], seen = {};
function flush() { }
Interceptor.attach(va(0x4333C0), { onEnter: function () {
  flush();
  try { var o = this.context.esp.add(8).readPointer(); fn = getLink(o).add(0xa).readCString(); } catch (e) { fn = "??"; }
  active = ALL || fn.indexOf(TARGET) >= 0;
} });
Interceptor.attach(Process.getModuleByName('kernel32.dll').getExportByName('ExitProcess'), { onEnter: function () { flush(); } });
function opcls(op) {
  if (op >= 21 && op <= 24) return "lb"; if (op >= 25 && op <= 33) return "lh"; if (op >= 34 && op <= 39) return "lw";
  if (op >= 40 && op <= 43) return "sb"; if (op >= 44 && op <= 48) return "sh"; if (op >= 49 && op <= 54) return "sw";
  if (op >= 142 && op <= 145) return "lfs"; if (op >= 146 && op <= 149) return "lfd";
  if (op >= 150 && op <= 153) return "stfs"; if (op >= 154 && op <= 157) return "stfd";
  return "o" + op;
}
function objdesc(o) {
  if (o.isNull()) return ["none", "", 0];
  var h = o.readU32(); var nm = "";
  try { nm = o.add(0xa).readPointer().add(0xa).readCString(); } catch (e) {}
  var st;
  if (o.add(0xe).readPointer().equals(PSEUDO)) st = "pso";
  else if (h == 5) st = (nm.charAt(0) == "@" ? "lit" : "st");
  else if (h == 0x10005) st = (o.add(0x18).readU32() == 0 || nm.charAt(0) == "@") ? "tmp" : "fr";
  else st = "h" + h.toString(16);
  return [st, nm, h];
}
function desc(p, fine) {
  var f = p.add(0x14).readU32();
  var s = opcls(p.add(0x20).readU16()) + ((f & 0x20) ? "*" : "") + ((f & 0x40) ? "c" : "") + ((f & 0x80) ? "v" : "") + (p.add(0x3c).readU8() == 3 ? "D" : "");
  var A = p.add(0x18).readPointer();
  if (A.isNull()) return s + ":nil";
  var k = A.add(0x2c).readU8();
  if (k == 2) { return s + (A.equals(WCP.readPointer()) ? ":WC" : ":S"); }
  var od = objdesc(A.add(0x10).readPointer());
  var sz = A.add(0x18).readU32();
  if (!fine) return s + ":" + od[0] + (k == 1 ? "~" : "") + (sz <= 4 ? "4" : sz <= 8 ? "8" : sz <= 16 ? "16" : "L");
  return s + ":" + (k == 1 ? "s" : "w") + ":" + od[0] + ":" + od[1] + "+" + A.add(0x14).readS32() + "/" + sz;
}
Interceptor.attach(va(0x511fc0), {
  onEnter: function () { if (!active) return; this.a = this.context.esp.add(4).readPointer(); this.b = this.context.esp.add(8).readPointer(); this.ret = rva(this.context.esp.readPointer()); },
  onLeave: function (rv) {
    if (!active) return;
    var r = rv.toInt32() & 0xff;
    var site = this.ret.toString(16);
    var kf = desc(this.a, true) + "|" + desc(this.b, true) + "@" + site;
    var kc = desc(this.a, false) + "|" + desc(this.b, false) + "@" + site;
    if (FLIP[kf + "=" + r] || FLIP[kc + "=" + r]) { rv.replace(r ? 0 : 1); r = r ? 0 : 1; kf = kf + "=F"; }
    var k = kf + "=" + r;
    send({fn: fn, k: k});
  }
});
"""


def rules():
    out = {}
    for m in BUILD_RE.finditer(NINJA):
        body = m.group("body")
        mw = re.search(r"mw_version = (\S+)", body)
        cf = re.search(r"cflags = ((?:.*\$\n)*.*)\n", body)
        src = SRC_RE.search(body)
        if mw and cf and src:
            out[m.group("obj").replace("\\", "/")] = dict(
                src=src.group(1).replace("\\", "/"), mw=mw.group(1).replace("\\", "/"), rule=m.group("rule"),
                flags=re.sub(r"\s+", " ", cf.group(1).replace("$\n", " ")).strip())
    return out


RULES = rules()


def find_unit(frag):
    hits = [u for u in CFG["units"] if u["name"].endswith(frag) or frag in u["name"]]
    ex = [u for u in hits if u["name"].endswith(frag)]
    return (ex or hits)[0]


def compile_unit(u, out, js, mw=None, src=None):
    info = RULES[u["base_path"].replace("\\", "/")]
    cmd = cwexec.compile_prefix(NINJA, info["rule"], mw or info["mw"]) + \
        shlex.split(info["flags"], posix=False) + ["-c", src or info["src"], "-o", out]
    cmd = [c[1:-1] if len(c) > 1 and c[0] == c[-1] == '"' else c for c in cmd]
    if "sjiswrap" in cmd[0]:
        cmd = cmd[1:]
    cmd = [os.path.join(ROOT, cmd[0])] + cmd[1:]
    if os.path.exists(out):
        os.unlink(out)
    import frida
    msgs, done = [], threading.Event()
    with LOCK:
        if DEV[0] is None:
            DEV[0] = frida.get_local_device()
        pid = DEV[0].spawn(cmd, cwd=ROOT)
        sess = DEV[0].attach(pid)
        script = sess.create_script(js)
        script.on("message", lambda m, d: msgs.append(m))
        script.load()
        sess.on("detached", lambda *a: done.set())
        DEV[0].resume(pid)
    done.wait(900)
    return msgs


def score(u, out, fnfrag):
    dj = out + ".json"
    subprocess.run([CLI, "diff", "-1", os.path.join(ROOT, u["target_path"]), "-2", out, "-o", dj,
                    "--format", "json", "-c", "functionRelocDiffs=none"], cwd=ROOT, capture_output=True)
    d = json.load(open(dj))
    res = {}
    for s in d.get("left", {}).get("symbols", []):
        if s.get("kind") == "SYMBOL_FUNCTION":
            res[s["name"]] = s.get("match_percent", 0.0)
    return {k: v for k, v in res.items() if fnfrag in k} if fnfrag else res


def make_js(target, mode, flip, all_=False):
    return JS.replace("%TARGET%", json.dumps(target)).replace("%MODE%", mode) \
        .replace("%FLIP%", json.dumps({k: 1 for k in flip})).replace("%ALL%", "true" if all_ else "false")


def run(unit, fnfrag, flip=(), mw=None, src=None, all_=False):
    u = find_unit(unit)
    td = tempfile.mkdtemp(prefix="qp_")
    out = os.path.join(td, "o.o")
    msgs = compile_unit(u, out, make_js(fnfrag, "x", flip, all_), mw, src)
    errs = [m for m in msgs if m.get("type") == "error"]
    if errs:
        print("JSERR", errs[:2])
    agg = {}
    for m in msgs:
        if m.get("type") != "send":
            continue
        p = m["payload"]
        L = agg.setdefault(p["fn"], {"fn": p["fn"], "c": {}, "order": []})
        if p["k"] not in L["c"]:
            L["order"].append(p["k"])
        L["c"][p["k"]] = L["c"].get(p["k"], 0) + 1
    logs = list(agg.values())
    if not os.path.exists(out):
        return logs, None
    return logs, score(u, out, fnfrag)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("cmd"); ap.add_argument("unit"); ap.add_argument("fn"); ap.add_argument("keys", nargs="*")
    ap.add_argument("--mw"); ap.add_argument("--src"); ap.add_argument("--coarse", action="store_true")
    ap.add_argument("--jobs", type=int, default=10); ap.add_argument("--min", type=int, default=0)
    a = ap.parse_args()
    if a.cmd == "log" or a.cmd == "pairs":
        logs, sc = run(a.unit, a.fn, (), a.mw, a.src)
        print("score", sc)
        for L in logs:
            print("== fn", L["fn"])
            keys = L["order"] if a.cmd == "pairs" else sorted(L["c"], key=lambda k: -L["c"][k])
            for k in keys:
                if a.cmd == "pairs" and not k.endswith("=0"):
                    continue
                print("%5d %s" % (L["c"][k], k))
    elif a.cmd == "flip":
        logs, sc = run(a.unit, a.fn, a.keys, a.mw, a.src)
        nf = sum(v for L in logs for k, v in L["c"].items() if "=F=" in k)
        print("score", sc, "flipped", nf)
    elif a.cmd == "auto":
        logs, base = run(a.unit, a.fn, (), a.mw, a.src)
        print("base", base)
        want = "=1" if os.environ.get("ONES") else "=0"
        keys = sorted({k for L in logs for k in L["c"] if k.endswith(want)})
        if a.coarse:
            keys = sorted({coarse_of(k) for k in keys})
        print(len(keys), "candidate keys")

        def one(k):
            lg, sc = run(a.unit, a.fn, [k], a.mw, a.src)
            return k, sc
        rows = []
        with ThreadPoolExecutor(a.jobs) as ex:
            for k, sc in ex.map(one, keys):
                rows.append((k, sc))
        for k, sc in sorted(rows, key=lambda r: -sum((r[1] or {}).values())):
            delta = {f: round(sc[f] - base.get(f, 0), 2) for f in (sc or {}) if abs(sc[f] - base.get(f, 0)) > 1e-6}
            if delta:
                print(delta, k)


def coarse_of(k):
    raise SystemExit("--coarse: use log --coarse keys instead")


if __name__ == "__main__":
    main()
