#!/usr/bin/env python3
"""rrun.py: compile a repro with the stock compilers, with 2.0p1f+ssi, and with 2.0p1f+ssi under
one or more frida class ablations; group the functions by identical code.

    rrun.py <repro.c|.cpp> [--abl name=REGEX ...] [--mw 2.0p1,2.5,...] [--flags rw|sb] [--show fn]
           [--x name=DIR ...]       extra scratch compilers (a directory holding mwcceppc.exe)

The question each audit repro answers: does removing a clause for one class make 2.0p1f+ssi
byte-identical to stock 2.0p1 / 2.5 on that class (and leave everything else alone)?
"""
import os, re, sys, shlex, tempfile, json
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "repros"))
import run  # noqa  (repros/run.py)
import aaudit  # noqa
import qprobe  # noqa

SSI = os.path.join(run.ROOT, "xcc", "ssi")


def frida_compile(src, kind, td, abl, tag, mwdir=SSI):
    import frida, threading
    rule, flags = run.unit_flags(kind)
    rel = os.path.relpath(mwdir, os.path.join(run.ROOT, "build", "compilers"))
    prefix = run.cwexec.compile_prefix(run.NINJA, rule, rel)
    if kind == "sb" and not src.endswith(".cpp"):
        flags = flags.replace("-lang=c++", "-lang=c")
    out = os.path.join(td, tag + "_" + os.path.basename(src) + ".o")
    cmd = prefix + shlex.split(flags, posix=False) + ["-c", src, "-o", out]
    cmd = [c[1:-1] if len(c) > 1 and c[0] == c[-1] == '"' else c for c in cmd]
    if "sjiswrap" in cmd[0]:
        cmd = cmd[1:]
    cmd = [os.path.join(run.ROOT, cmd[0])] + cmd[1:]
    msgs, done = [], threading.Event()
    holder = {}

    def on_msg(m, d):
        msgs.append(m)
        if m.get("type") == "send" and "nabl" in (m.get("payload") or {}):
            holder["s"].post({"type": "ack"})
    dev = frida.get_local_device()
    pid = dev.spawn(cmd, cwd=run.ROOT)
    sess = dev.attach(pid)
    s = sess.create_script(aaudit.js(abl, True))
    holder["s"] = s
    s.on("message", on_msg)
    s.load()
    sess.on("detached", lambda *a: done.set())
    dev.resume(pid)
    done.wait(600)
    if not os.path.exists(out):
        raise SystemExit("compile failed " + tag + str([m for m in msgs if m.get("type") == "error"][:2]))
    asm = out + ".s"
    import subprocess
    subprocess.run([run.cwexec.dtk(run.ROOT), "elf", "disasm", out, asm], capture_output=True)
    pay = next((m["payload"] for m in msgs if m.get("type") == "send" and "nabl" in m["payload"]), {})
    return run.functions(open(asm).read()), pay


def main():
    args = sys.argv[1:]

    def opts(name):
        out = []
        while name in args:
            i = args.index(name); out.append(args[i + 1]); del args[i:i + 2]
        return out
    abls = [x.split("=", 1) for x in opts("--abl")]
    xs = [x.split("=", 1) for x in opts("--x")]
    mws = (opts("--mw") or ["2.0p1,2.5,2.6,2.7"])[0].split(",")
    kind = (opts("--flags") or ["rw"])[0]
    show = (opts("--show") or [None])[0]
    logkeys = "--keys" in args
    if logkeys:
        args.remove("--keys")
    td = tempfile.mkdtemp(prefix="rr_")
    for src in args:
        src = os.path.abspath(src)
        k = "sb" if src.endswith(".cpp") else kind
        per = {}
        for mw in mws:
            per[mw] = run.compile_one(src, mw, k, td)
        for name, d in xs:
            per[name] = run.compile_one(src, os.path.abspath(d), k, td)
        per["ssi"], pay = frida_compile(src, k, td, None, "ssi")
        for name, rx in abls:
            per[name], p2 = frida_compile(src, k, td, rx, name)
        order = mws + [n for n, _ in xs] + ["ssi"] + [n for n, _ in abls]
        print("== %s" % os.path.basename(src))
        for fn in per["ssi"]:
            if show and fn != show:
                continue
            groups = []
            for mw in order:
                body = tuple(per[mw].get(fn, ()))
                for g in groups:
                    if g[0] == body:
                        g[1].append(mw); break
                else:
                    groups.append((body, [mw]))
            print("  %-22s %s" % (fn[:22], "  ".join("[%s]" % " ".join(g[1]) for g in groups)))
            if logkeys:
                for kk, fns in sorted((pay.get("log") or {}).items()):
                    if fn in fns:
                        print("        %3d %s" % (fns[fn], kk))
            if show:
                for body, ms in groups:
                    print("    --- %s (%d insns)" % (" ".join(ms), len(body)))
                    for l in body:
                        print("      " + l)


if __name__ == "__main__":
    main()
