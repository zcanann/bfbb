#!/usr/bin/env python3
"""hprobe.py: log (and optionally flip) IRO expression-candidate decisions (frida, GC/2.0p1x).

    hprobe.py <src> <unit-for-flags> <fnfrag> [--mw GC/2.0p1f] [--flip] [--score fn|all] [--plain]

--plain compiles without frida (any compiler; just --score / ASM).

Hooks IRO_IsExpressionCandidate (2.0p1 0x457540; the predicate IroCSE's
expression finder 0x46b2e0 asks before an IROLinear becomes a CSE/LICM
expression) and prints, for every EINDIRECT node of functions whose name
contains <fnfrag>: the node flags, the object read (when the address is a
plain OBJREF), its VarInfo noregister byte (+0x22), the object type's
TypeType and size, the access type's size, and the answer.
--flip makes the predicate answer 1 for an indirect of a non-scalar
(struct/class/array) local that it would otherwise refuse -- the
"aggregate member read is an expression" rule -- to test it in-process.
Nothing on disk is modified.
"""
import json, os, re, shlex, subprocess, sys, tempfile, threading
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import cwexec  # noqa

NINJA = open(os.path.join(ROOT, "build.ninja")).read()

JS = r"""
var base = Process.getModuleByName("mwcceppc.exe").base;
function va(x) { return base.add(x - 0x400000); }
var getLink = new NativeFunction(va(0x4FE710), "pointer", ["pointer"], "mscdecl");
var TARGET = %TARGET%; var FLIP = %FLIP%;
var fn = "?", active = false;
Interceptor.attach(va(0x4333C0), { onEnter: function () {
  try { var o = this.context.esp.add(8).readPointer(); fn = getLink(o).add(0xa).readCString(); } catch (e) { fn = "??"; }
  active = fn.indexOf(TARGET) >= 0;
} });
function hex(p, n) { var b = new Uint8Array(p.readByteArray(n)); var s = ''; for (var i = 0; i < n; i++) s += ('0' + b[i].toString(16)).slice(-2); return s; }
function nm(o) { try { return o.add(0xa).readPointer().add(0xa).readCString(); } catch (e) { return "?"; } }
Interceptor.attach(va(0x457540), {
  onEnter: function () {
    this.info = null;
    if (!active) return;
    var n = this.context.esp.add(4).readPointer();
    var ty = n.readU8(), nt = n.add(1).readU8();
    if (ty != 2 || nt != 4) return;
    var fl = n.add(2).readU32();
    var op = n.add(0x20).readPointer();
    var d = {fn: fn, flags: fl.toString(16), opty: op.readU8()};
    var rt = n.add(0x10).readPointer();
    if (op.readU8() == 1) {
      var en = op.add(0x20).readPointer();
      d.enode = en.readU8();
      if (en.readU8() == 0x38) {
        var obj = en.add(0xe).readPointer();
        d.obj = nm(obj); d.dt = obj.add(2).readU8();
        d.raw = hex(n, 0x30);
        try { var t = obj.add(0xe).readPointer(); d.otype = t.readU8(); d.osize = t.add(2).readS32(); var lt = n.add(0xe).readPointer(); d.ltype = lt.readU8(); d.lsize = lt.add(2).readS32(); } catch (e) { d.terr = 1; }
        try { var vi = obj.add(0x2a).readPointer(); d.noreg = vi.isNull() ? -1 : vi.add(0x22).readU8(); } catch (e) { d.verr = 1; }
      }
    } else if (op.readU8() == 3) {
      d.binop = op.add(1).readU8();
    }
    this.info = d;
  },
  onLeave: function (r) {
    if (!this.info) return;
    var d = this.info; d.ret = r.toInt32() & 0xff;
    if (FLIP && d.ret == 0 && !(parseInt(d.flags, 16) & 4) && d.otype !== undefined && (d.otype == 4 || d.otype == 5 || d.otype == 12 || d.otype == 13)) { r.replace(ptr(1)); d.flipped = 1; }
    send(d);
  }
});
"""


def compile_cmd(src, unit, out, mw):
    m = None
    for m0 in re.finditer(r"^build (?P<obj>\S+\.o):(?: \$\n\s+| )(?P<rule>mwcc_sjis|mwcc) (?P<body>(?:.*\n)*?)  basedir", NINJA, re.M):
        if unit in m0.group("obj").replace("\\", "/"):
            m = m0
            break
    body = m.group("body")
    flags = re.sub(r"\s+", " ", re.search(r"cflags = ((?:.*\$\n)*.*)\n", body).group(1).replace("$\n", " ")).strip()
    mwv = mw or re.search(r"mw_version = (\S+)", body).group(1).replace("\\", "/")
    cmd = cwexec.compile_prefix(NINJA, m.group("rule"), mwv) + shlex.split(flags, posix=False) + ["-c", src, "-o", out]
    cmd = [c[1:-1] if len(c) > 1 and c[0] == c[-1] == '"' else c for c in cmd]
    if "sjiswrap" in cmd[0]:
        cmd = cmd[1:]
    if src.endswith(".c"):
        cmd = [c.replace("-lang=c++", "-lang=c") for c in cmd]
    return [os.path.join(ROOT, cmd[0])] + cmd[1:]


def main():
    a = sys.argv[1:]
    mw = None
    if "--mw" in a:
        i = a.index("--mw"); mw = a[i + 1]; del a[i:i + 2]
    flip = "--flip" in a
    plain = "--plain" in a
    a = [x for x in a if x != "--plain"]
    score = None
    if "--score" in a:
        i = a.index("--score"); score = a[i + 1]; del a[i:i + 2]
    a = [x for x in a if x != "--flip"]
    src, unit, frag = os.path.abspath(a[0]), a[1], a[2]
    out = os.path.join(tempfile.mkdtemp(prefix="hp_"), "o.o")
    cmd = compile_cmd(src, unit, out, mw)
    if plain:
        r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
        if not os.path.exists(out):
            print(r.stdout[-2000:], r.stderr[-2000:])
        msgs = []
    else:
        msgs = run_frida(cmd, frag, flip)
    for m in msgs:
        print(m["payload"] if m.get("type") == "send" else m)
    report(out, unit, frag, score)


def run_frida(cmd, frag, flip):
    import frida
    js = JS.replace("%TARGET%", json.dumps(frag)).replace("%FLIP%", "true" if flip else "false")
    msgs, done = [], threading.Event()
    dev = frida.get_local_device()
    pid = dev.spawn(cmd, cwd=ROOT)
    sess = dev.attach(pid)
    sc = sess.create_script(js)
    sc.on("message", lambda m, d: msgs.append(m))
    sc.load()
    sess.on("detached", lambda *x: done.set())
    dev.resume(pid)
    done.wait(600)
    return msgs


def report(out, unit, frag, score):
    if os.path.exists(out):
        asm = out + ".s"
        subprocess.run([cwexec.dtk(ROOT), "elf", "disasm", out, asm], capture_output=True)
        print("ASM", asm)
        if score:
            cfg = json.load(open(os.path.join(ROOT, "objdiff.json")))
            u = [x for x in cfg["units"] if x["name"].endswith(unit)][0]
            r = subprocess.run([cwexec.objdiff_cli(ROOT), "diff", "-1", os.path.join(ROOT, u["target_path"]), "-2", out,
                                "-o", "-", "--format", "json", "-c", "functionRelocDiffs=none"], capture_output=True, text=True)
            d = json.loads(r.stdout)
            for sy in d.get("left", {}).get("symbols", []):
                if sy.get("kind") == "SYMBOL_FUNCTION" and (score == "all" or frag in sy["name"]):
                    print("SCORE %8.3f %s" % (sy.get("match_percent", 0.0), sy["name"]))


if __name__ == "__main__":
    main()
