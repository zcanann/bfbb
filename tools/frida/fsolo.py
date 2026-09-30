#!/usr/bin/env python3
"""Compile ONE real unit with the tree compiler under a frida-forced clause H,
then objdiff against the target and print per-function match. Based on
tools/solo.py but the compile happens under frida instrumentation.

usage: fsolo.py <unit-fragment> [mode] [symfrag]
  mode: h1 | off | all   (default h1)
"""
import json, os, re, shlex, subprocess, sys, tempfile, threading
import frida

ROOT = r"C:\Users\joeyj\Documents\Git\bfbb_\.claude\worktrees\agent-ab229575576d6fcfb"
sys.path.insert(0, os.path.join(ROOT, "tools"))
import cwexec

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
        flags = re.sub(r"\s+", " ", cf.group(1).replace("$\n", " ")).strip()
        out[m.group("obj").replace("\\", "/")] = {
            "src": src.group(1).replace("\\", "/"),
            "mw": mw.group(1).replace("\\", "/"),
            "rule": m.group("rule"),
            "flags": flags,
        }
    return out

def find_unit(frag):
    hits = [u for u in CFG["units"] if frag in u["name"]]
    if len(hits) != 1:
        exact = [u for u in hits if u["name"].endswith(frag)]
        if len(exact) == 1:
            return exact[0]
        raise SystemExit("ambiguous or no match: " + ", ".join(u["name"] for u in hits[:12]))
    return hits[0]

SCRIPT = r"""
var MODE = "%MODE%";
var base = null;
for (const m of Process.enumerateModules()) {
    if (m.name.toLowerCase() == "mwcceppc.exe") { base = m.base; break; }
}
if (base === null) { base = Process.enumerateModules()[0].base; }
function va(x) { return base.add(x - 0x400000); }
function safeU32(p) { try { return p.readU32(); } catch (e) { return -1; } }
function baseWord(m) {
    if (m.isNull()) return -2;
    var basex = m.add(0x10).readPointer();
    return basex.isNull() ? -2 : safeU32(basex);
}
var inCb0 = false, mA = ptr(0), mB = ptr(0);
var nforced = 0, ncalls = 0;
Interceptor.attach(va(0x511cb0), {
    onEnter: function () {
        var esp = this.context.esp;
        var i1 = esp.add(4).readPointer();
        mA = i1.isNull() ? ptr(0) : i1.add(0x18).readPointer();
        mB = ptr(0);
        inCb0 = true;
        ncalls++;
    },
    onLeave: function (rv) {
        inCb0 = false;
        if (rv.toInt32() & 0xff) return;
        if (MODE == "off") return;
        if (mA.isNull() || mB.isNull()) return;
        if (MODE == "h1") {
            if (baseWord(mA) != 5) return;
            if (baseWord(mB) != 5) return;
            var sz = safeU32(mB.add(0x18));
            if (sz < 0 || sz > 4) return;
        }
        rv.replace(1);
        nforced++;
    }
});
var d2e20 = 0;
Interceptor.attach(va(0x512e20), {
    onEnter: function () { d2e20++; },
    onLeave: function (rv) {
        d2e20--;
        if (d2e20 == 0 && inCb0 && mB.isNull()) mB = rv.add(0);
    }
});
Interceptor.attach(Process.getModuleByName('kernel32.dll').getExportByName('ExitProcess'), {
    onEnter: function () { send({stats: [ncalls, nforced]}); }
});
"""

def frida_compile(cmd, mode):
    msgs = []
    dev = frida.get_local_device()
    pid = dev.spawn(cmd, cwd=ROOT)
    sess = dev.attach(pid)
    script = sess.create_script(SCRIPT.replace("%MODE%", mode))
    script.on("message", lambda m, d: msgs.append(m))
    script.load()
    done = threading.Event()
    sess.on("detached", lambda *a: done.set())
    dev.resume(pid)
    done.wait(600)
    for m in msgs:
        if m.get("type") == "send" and "stats" in m.get("payload", {}):
            print("frida stats [calls, forced]:", m["payload"]["stats"])
    for m in msgs:
        if m.get("type") == "error":
            print("FRIDA ERR:", m)

def main():
    frag = sys.argv[1]
    mode = sys.argv[2] if len(sys.argv) > 2 else "h1"
    symfrag = sys.argv[3] if len(sys.argv) > 3 else None
    unit = find_unit(frag)
    obj = unit["base_path"].replace("\\", "/")
    RULES = build_rules()
    info = RULES[obj]
    td = tempfile.mkdtemp(prefix="fsolo_")
    out = os.path.join(td, "o.o")
    cmd = cwexec.compile_prefix(NINJA, info["rule"], info["mw"]) + \
        shlex.split(info["flags"], posix=False) + ["-c", info["src"], "-o", out]
    cmd = [c.strip('"') if c.startswith('"') and c.endswith('"') else c for c in cmd]
    cmd = [os.path.join(ROOT, c) if c.startswith("build") else c for c in cmd[:2]] + cmd[2:]
    # bypass sjiswrap so frida hooks the compiler process itself
    if "sjiswrap" in cmd[0]:
        cmd = cmd[1:]
    frida_compile(cmd, mode)
    if not os.path.exists(out):
        raise SystemExit("COMPILE FAILED under frida")
    dj = os.path.join(td, "d.json")
    subprocess.run([CLI, "diff", "-1", os.path.join(ROOT, unit["target_path"]),
                    "-2", out, "-o", dj, "--format", "json",
                    "-c", "functionRelocDiffs=none"],
                   cwd=ROOT, capture_output=True)
    d = json.load(open(dj))
    right = {s["name"]: s for s in d.get("right", {}).get("symbols", [])
             if s.get("kind") == "SYMBOL_FUNCTION"}
    total = 0
    n100 = 0
    rows = []
    for name, s in right.items():
        pct = s.get("match_percent", 0.0)
        total += 1
        if pct >= 100.0: n100 += 1
        else: rows.append((pct, s.get("size", 0), name))
    rows.sort()
    print("functions:", total, " at 100:", n100)
    for pct, size, name in rows:
        if symfrag and symfrag not in name: continue
        print(f"  {pct:8.3f}  {size}  {name}")

main()
