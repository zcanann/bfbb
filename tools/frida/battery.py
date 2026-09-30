#!/usr/bin/env python3
"""Run fsolo-style off/h1 A-B across a battery of units; print per-function deltas.

usage: battery.py <mode> <unit> [unit...]
"""
import json, os, re, shlex, subprocess, sys, tempfile, threading
import importlib.util
spec = importlib.util.spec_from_file_location("fsolo_mod", os.path.join(os.path.dirname(__file__), "fsolo.py"))

# reuse fsolo internals by importing pieces manually
ROOT = r"C:\Users\joeyj\Documents\Git\bfbb_\.claude\worktrees\agent-ab229575576d6fcfb"
sys.path.insert(0, os.path.join(ROOT, "tools"))
import cwexec
import frida

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
            "rule": m.group("rule"), "flags": flags}
    return out
RULES = build_rules()

SCRIPT_TMPL = open(os.path.join(os.path.dirname(__file__), "hookscript.js")).read()

def frida_compile(cmd, mode):
    dev = frida.get_local_device()
    pid = dev.spawn(cmd, cwd=ROOT)
    sess = dev.attach(pid)
    script = sess.create_script(SCRIPT_TMPL.replace("%MODE%", mode))
    errs = []
    script.on("message", lambda m, d: errs.append(m) if m.get("type") == "error" else None)
    script.load()
    done = threading.Event()
    sess.on("detached", lambda *a: done.set())
    dev.resume(pid)
    done.wait(600)
    if errs:
        print("FRIDA ERRS:", errs[:3])

def measure(unitname, mode):
    unit = next(u for u in CFG["units"] if u["name"].endswith(unitname))
    obj = unit["base_path"].replace("\\", "/")
    info = RULES[obj]
    td = tempfile.mkdtemp(prefix="bat_")
    out = os.path.join(td, "o.o")
    cmd = cwexec.compile_prefix(NINJA, info["rule"], info["mw"]) + \
        shlex.split(info["flags"], posix=False) + ["-c", info["src"], "-o", out]
    cmd = [c.strip('"') if c.startswith('"') and c.endswith('"') else c for c in cmd]
    cmd = [os.path.join(ROOT, c) if c.startswith("build") else c for c in cmd[:2]] + cmd[2:]
    if "sjiswrap" in cmd[0]:
        cmd = cmd[1:]
    frida_compile(cmd, mode)
    if not os.path.exists(out):
        return None
    dj = os.path.join(td, "d.json")
    subprocess.run([CLI, "diff", "-1", os.path.join(ROOT, unit["target_path"]),
                    "-2", out, "-o", dj, "--format", "json",
                    "-c", "functionRelocDiffs=none"], cwd=ROOT, capture_output=True)
    d = json.load(open(dj))
    return {s["name"]: s.get("match_percent", 0.0)
            for s in d.get("right", {}).get("symbols", [])
            if s.get("kind") == "SYMBOL_FUNCTION"}

def main():
    mode = sys.argv[1]
    for unitname in sys.argv[2:]:
        off = measure(unitname, "off")
        on = measure(unitname, mode)
        if off is None or on is None:
            print(unitname, "COMPILE FAILED")
            continue
        ups = downs = crossed_up = crossed_down = 0
        details = []
        for name in off:
            a, b = off[name], on.get(name, 0.0)
            if abs(a - b) < 1e-9: continue
            if b > a: ups += 1
            else: downs += 1
            if a >= 100.0 > b: crossed_down += 1; details.append(f"  LOST {name} {a:.3f}->{b:.3f}")
            elif b >= 100.0 > a: crossed_up += 1; details.append(f"  GAIN {name} {a:.3f}->{b:.3f}")
            else: details.append(f"  {'+' if b>a else '-'} {name} {a:.3f}->{b:.3f}")
        print(f"{unitname}: up {ups} down {downs} gained100 {crossed_up} lost100 {crossed_down}")
        for d in details[:25]:
            print(d)

main()
