#!/usr/bin/env python3
"""rulesnap.py: tree-wide measurement of a candidate may_alias rule WITHOUT deriving a compiler.

    rulesnap.py snap <name> <rule.js|-> [--set sb|rw|all]   compile every unit under frida with the rule
    rulesnap.py cmp <a> <b> [--set ..]                     gains / losses / moves

A rule file is a JS snippet defining  function RULE(a, b, r, site) -> 1 (force may-alias),
0 (force no-alias) or -1 (keep r). It runs in every may_alias (0x511fc0) query of every
function. Helpers available: D(p) -> {op, cls, f, ptr, cst, vol, dir, k, wc, st, nm, off, sz, obj, h}.
VN hooks: function VNRULE(storeAlias) may be defined (optional) -- not used yet.
Results: <scratch>/snaps/<name>.json = {unit: {fn: pct}}.
"""
import json, os, sys, tempfile, threading, hashlib
from concurrent.futures import ThreadPoolExecutor
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import qprobe  # noqa

OUTDIR = os.environ.get("SNAPDIR", os.path.join(tempfile.gettempdir(), "rulesnap"))

HEAD = r"""
var base = Process.getModuleByName("mwcceppc.exe").base;
function va(x) { return base.add(x - 0x400000); }
function rva(p) { return (p.sub(base).toInt32() + 0x400000) >>> 0; }
var WCP = va(0x5e9cb4);
var PSEUDO = va(0x5bd008);
function opcls(op) {
  if (op >= 21 && op <= 24) return "lb"; if (op >= 25 && op <= 33) return "lh"; if (op >= 34 && op <= 39) return "lw";
  if (op >= 40 && op <= 43) return "sb"; if (op >= 44 && op <= 48) return "sh"; if (op >= 49 && op <= 54) return "sw";
  if (op >= 142 && op <= 145) return "lfs"; if (op >= 146 && op <= 149) return "lfd";
  if (op >= 150 && op <= 153) return "stfs"; if (op >= 154 && op <= 157) return "stfd";
  return "o" + op;
}
function D(p) {
  var f = p.add(0x14).readU32(); var op = p.add(0x20).readU16();
  var r = {op: op, cls: opcls(op), f: f, ptr: (f & 0x20) != 0, cst: (f & 0x40) != 0, vol: (f & 0x80) != 0,
           dir: p.add(0x3c).readU8() == 3, k: -1, wc: false, st: "nil", nm: "", off: 0, sz: 0, obj: null, h: 0, A: null};
  r.store = (r.cls.charAt(0) == "s");
  var A = p.add(0x18).readPointer(); r.A = A;
  if (A.isNull()) return r;
  r.k = A.add(0x2c).readU8();
  if (r.k == 2) { r.wc = A.equals(WCP.readPointer()); r.st = r.wc ? "WC" : "S"; return r; }
  var o = A.add(0x10).readPointer(); r.obj = o;
  r.off = A.add(0x14).readS32(); r.sz = A.add(0x18).readU32();
  if (o.isNull()) { r.st = "none"; return r; }
  var h = o.readU32(); r.h = h;
  try { r.nm = o.add(0xa).readPointer().add(0xa).readCString(); } catch (e) {}
  if (o.add(0xe).readPointer().equals(PSEUDO)) r.st = "pso";
  else if (h == 5) r.st = (r.nm.charAt(0) == "@" ? "lit" : "st");
  else if (h == 0x10005) r.st = (o.add(0x18).readU32() == 0 || r.nm.charAt(0) == "@") ? "tmp" : "fr";
  else r.st = "h" + h.toString(16);
  return r;
}
"""
TAIL = r"""
var NFORCE = 0;
var LOGFN = %LOGFN%; var curfn = "?";
var getLink = new NativeFunction(va(0x4FE710), "pointer", ["pointer"], "mscdecl");
if (LOGFN) Interceptor.attach(va(0x4333C0), { onEnter: function () {
  try { var o = this.context.esp.add(8).readPointer(); curfn = getLink(o).add(0xa).readCString(); } catch (e) { curfn = "??"; } } });
function fdesc(p) { var d = D(p); return d.cls + (d.ptr ? "*" : "") + (d.cst ? "c" : "") + (d.vol ? "v" : "") + (d.dir ? "D" : "") + ":" + (d.k == 2 ? d.st : ((d.k == 1 ? "s:" : "w:") + d.st + ":" + d.nm + "+" + d.off + "/" + d.sz)); }
Interceptor.attach(va(0x511fc0), {
  onEnter: function () { this.a = this.context.esp.add(4).readPointer(); this.b = this.context.esp.add(8).readPointer(); this.ret = rva(this.context.esp.readPointer());
    this.sw = [];
    if (typeof PRE === "function") {
      var L = PRE(this.a, this.b, this.ret);
      for (var i = 0; i < L.length; i++) { var p = L[i][0]; this.sw.push([p, p.add(0x18).readPointer()]); p.add(0x18).writePointer(L[i][1]); }
    }
  },
  onLeave: function (rv) {
    for (var i = this.sw.length - 1; i >= 0; i--) this.sw[i][0].add(0x18).writePointer(this.sw[i][1]);
    var r = rv.toInt32() & 0xff;
    if (this.sw.length && LOGFN && curfn.indexOf(LOGFN) >= 0) { var r0 = r; }
    if (typeof RULE !== "function") return;
    var x = RULE(this.a, this.b, r, this.ret);
    if (x >= 0 && x != r) { rv.replace(x); NFORCE++;
      if (LOGFN && curfn.indexOf(LOGFN) >= 0) send({forced: fdesc(this.a) + "|" + fdesc(this.b) + "@" + this.ret.toString(16) + "=" + r}); }
  }
});
function DA(A) {
  var r = {k: -1, wc: false, st: "nil", nm: "", off: 0, sz: 0, obj: null, h: 0, A: A, ptr: false, dir: false, f: 0, op: 0, cls: "", store: true};
  if (A.isNull()) return r;
  r.k = A.add(0x2c).readU8();
  if (r.k == 2) { r.wc = A.equals(WCP.readPointer()); r.st = r.wc ? "WC" : "S"; return r; }
  var o = A.add(0x10).readPointer(); r.obj = o;
  r.off = A.add(0x14).readS32(); r.sz = A.add(0x18).readU32();
  if (o.isNull()) { r.st = "none"; return r; }
  var h = o.readU32(); r.h = h;
  try { r.nm = o.add(0xa).readPointer().add(0xa).readCString(); } catch (e) {}
  if (o.add(0xe).readPointer().equals(PSEUDO)) r.st = "pso";
  else if (h == 5) r.st = (r.nm.charAt(0) == "@" ? "lit" : "st");
  else if (h == 0x10005) r.st = (o.add(0x18).readU32() == 0 || r.nm.charAt(0) == "@") ? "tmp" : "fr";
  else r.st = "h" + h.toString(16);
  return r;
}
if (typeof VNKILL === "function") {
  var vkill = new NativeFunction(va(0x50a2c0), "void", ["pointer", "pointer"], "mscdecl");
  var HEADP = va(0x5E1FD8);
  Interceptor.attach(va(0x511a30), {
    onEnter: function () { this.m = this.context.esp.add(4).readPointer(); this.ins = this.context.esp.add(8).readPointer(); },
    onLeave: function () {
      if (this.m.isNull()) return;
      var sd = DA(this.m);
      if (typeof VNSTORE === "function" && !VNSTORE(sd, this.ins)) return;
      var x = HEADP.readPointer(), n = 0;
      while (!x.isNull() && n < 100000) {
        if (!x.equals(this.m)) { var xd = DA(x); if (VNKILL(sd, xd)) { vkill(x, ptr(0)); NFORCE++;
            if (LOGFN && curfn.indexOf(LOGFN) >= 0) send({forced: "VN " + sd.st + ":" + sd.nm + "+" + sd.off + "/" + sd.sz + " kills " + xd.st + ":" + xd.nm + "+" + xd.off + "/" + xd.sz}); } }
        x = x.readPointer(); n++;
      }
    }
  });
}
Interceptor.attach(Process.getModuleByName('kernel32.dll').getExportByName('ExitProcess'), { onEnter: function () { send({nforce: NFORCE}); } });
"""


def units(which):
    pre = {"sb": ("main/SB/",), "rw": ("main/rwsdk/",), "all": ("main/SB/", "main/rwsdk/")}[which]
    out = []
    for u in qprobe.CFG["units"]:
        if u["name"].startswith(pre) and u.get("base_path") and u.get("target_path"):
            if u["base_path"].replace("\\", "/") in qprobe.RULES:
                f = os.environ.get("UNITS")
                if not f or any(x in u["name"] for x in f.split(",")):
                    out.append(u)
    return out


def snap(name, rulefile, which, mw=None):
    lib = os.path.join(os.path.dirname(os.path.abspath(rulefile)), "lib.js") if rulefile != "-" else ""
    js = HEAD + (open(lib).read() if lib and os.path.exists(lib) else "") + \
        (open(rulefile).read() if rulefile != "-" else "function RULE(a,b,r,s){return -1;}") +         TAIL.replace("%LOGFN%", json.dumps(os.environ.get("LOGFN", "")))
    os.makedirs(OUTDIR, exist_ok=True)
    us = units(which)

    def one(u):
        td = tempfile.mkdtemp(prefix="rs_")
        out = os.path.join(td, "o.o")
        try:
            msgs = qprobe.compile_unit(u, out, js, mw)
        except Exception as e:
            return u["name"], None, str(e)
        errs = [m for m in msgs if m.get("type") == "error"]
        if errs or not os.path.exists(out):
            return u["name"], None, str(errs[:1]) or "compile failed"
        nf = sum(m["payload"].get("nforce", 0) for m in msgs if m.get("type") == "send")
        sc = qprobe.score(u, out, None)
        sha = hashlib.sha1(open(out, "rb").read()).hexdigest()
        return u["name"], {"f": sc, "sha": sha, "nf": nf}, None

    res, bad = {}, []
    with ThreadPoolExecutor(int(os.environ.get("J", "14"))) as ex:
        for un, r, err in ex.map(one, us):
            if err:
                bad.append((un, err))
            else:
                res[un] = r
    for b in bad:
        print("ERR", b[0], b[1][:200])
    path = os.path.join(OUTDIR, name + ".json")
    old = json.load(open(path)) if os.path.exists(path) else {}
    old.update(res)
    json.dump(old, open(path, "w"))
    ex_ = sum(1 for u in res.values() for p in u["f"].values() if p >= 100.0)
    print("%s [%s]: %d units, %d exact fns, %d forced answers, %d errors" % (
        name, which, len(res), ex_, sum(u["nf"] for u in res.values()), len(bad)))


def disasm_fn(obj, fn):
    import re, subprocess
    asm = obj + ".s"
    subprocess.run([qprobe.cwexec.dtk(qprobe.ROOT), "elf", "disasm", obj, asm], capture_output=True)
    txt = open(asm).read()
    m = re.search(r"^\.fn (\S*%s\S*),.*?\n(.*?)^\.endfn" % re.escape(fn), txt, re.M | re.S)
    if not m:
        return []
    out = []
    for ln in m.group(2).splitlines():
        ln = re.sub(r"/\*.*?\*/", "", ln).strip()
        if not ln or ln.startswith("#") or ln.startswith(".L"):
            continue
        ln = re.sub(r"@\d+", "@lit", ln); ln = re.sub(r"\.L_[0-9A-F]+", ".L", ln); ln = re.sub(r"\$\d+", "$N", ln)
        out.append(ln)
    return out


def show_diff(u, out, fn):
    import difflib
    a = disasm_fn(os.path.join(qprobe.ROOT, u["target_path"]), fn)
    b = disasm_fn(out, fn)
    for l in difflib.unified_diff(a, b, "target", "ours", n=2, lineterm=""):
        print(l)


def rulelog(rulefile, unit, fn):
    lib = os.path.join(os.path.dirname(os.path.abspath(rulefile)), "lib.js")
    js = HEAD + (open(lib).read() if os.path.exists(lib) else "") + open(rulefile).read() +         TAIL.replace("%LOGFN%", json.dumps(fn))
    u = qprobe.find_unit(unit)
    td = tempfile.mkdtemp(prefix="rl_")
    out = os.path.join(td, "o.o")
    msgs = qprobe.compile_unit(u, out, js)
    import collections
    c = collections.Counter(m["payload"]["forced"] for m in msgs if m.get("type") == "send" and "forced" in m["payload"])
    for m in msgs:
        if m.get("type") == "error":
            print("JSERR", m)
    print("score", qprobe.score(u, out, fn))
    if os.environ.get("SHOW"):
        show_diff(u, out, fn)
    for k, v in c.most_common():
        print("%4d %s" % (v, k))


def cmp(a, b, which="all", quiet=False):
    A = json.load(open(os.path.join(OUTDIR, a + ".json")))
    B = json.load(open(os.path.join(OUTDIR, b + ".json")))
    pre = {"sb": "main/SB/", "rw": "main/rwsdk/", "all": "main/"}[which]
    G = {"GAINS": [], "LOSSES": [], "UP": [], "DOWN": []}
    changed = 0
    for un in sorted(set(A) & set(B)):
        if not un.startswith(pre):
            continue
        changed += A[un]["sha"] != B[un]["sha"]
        for f, pa in A[un]["f"].items():
            pb = B[un]["f"].get(f, pa)
            if abs(pa - pb) < 1e-9:
                continue
            row = (un.replace("main/", ""), f, pa, pb)
            k = "GAINS" if pb >= 100 > pa else "LOSSES" if pa >= 100 > pb else "UP" if pb > pa else "DOWN"
            G[k].append(row)
    for k, rows in G.items():
        print("== %s %d" % (k, len(rows)))
        if quiet and k in ("UP", "DOWN"):
            continue
        for r in sorted(rows, key=lambda r: r[2] - r[3]):
            print("  %-34s %8.3f -> %8.3f  %s" % (r[0][-34:], r[2], r[3], r[1][:80]))
    print("objects changed: %d" % changed)
    return G


if __name__ == "__main__":
    a = sys.argv[1:]
    which = "all"
    if "--set" in a:
        i = a.index("--set"); which = a[i + 1]; del a[i:i + 2]
    q = "-q" in a
    if q:
        a.remove("-q")
    if a[0] == "snap":
        snap(a[1], a[2], which)
    elif a[0] == "log":
        os.environ["LOGFN"] = a[3]
        rulelog(a[1], a[2], a[3])
    elif a[0] == "cmp":
        cmp(a[1], a[2], which, q)
