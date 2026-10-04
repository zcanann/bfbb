"""cc.py <file.c> [symbol] : compile a scratch C file with the babintex RW flags, print disassembly."""
import os as _os
import sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
import ra_common
_REPO = ra_common.ROOT
import sys, os, subprocess, shlex, tempfile, re
ROOT = _REPO
sys.path.insert(0, ROOT + "/tools")
os.chdir(ROOT)
import cwexec
NINJA = open(os.path.join(ROOT, "build.ninja")).read()
BUILD_RE = re.compile(
    r"^build (?P<obj>\S+\.o):(?: \$\n\s+| )(?P<rule>mwcc_sjis|mwcc) (?P<body>(?:.*\n)*?)  basedir", re.M)
info = None
for m in BUILD_RE.finditer(NINJA):
    body = m.group("body")
    if "babintex" not in m.group("obj"):
        continue
    mw = re.search(r"mw_version = (\S+)", body).group(1).replace("\\", "/")
    cf = re.search(r"cflags = ((?:.*\$\n)*.*)\n", body)
    flags = re.sub(r"\s+", " ", cf.group(1).replace("$\n", " ")).strip()
    info = {"rule": m.group("rule"), "mw": mw, "flags": flags}
    break
mwv = info["mw"]
if '--mw' in sys.argv:
    mwv = sys.argv[sys.argv.index('--mw') + 1]
src = os.path.abspath(sys.argv[1])
td = tempfile.mkdtemp(prefix="cc_")
out = os.path.join(td, "o.o")
cmd = cwexec.compile_prefix(NINJA, info["rule"], mwv) + shlex.split(info["flags"], posix=False) + ["-c", src, "-o", out]
cmd = [c.strip('"') if c.startswith('"') and c.endswith('"') else c for c in cmd]
r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
if not os.path.exists(out):
    print(r.stdout, r.stderr); sys.exit(1)
d = os.path.join(td, "o.s")
subprocess.run([ROOT + "/build/tools/dtk.exe", "elf", "disasm", out, d], capture_output=True)
txt = open(d).read()
txt = re.sub(r"/\* [0-9A-F]+ [0-9A-F]+ [0-9A-F ]+\*/\t", "", txt)
if len(sys.argv) > 2 and not sys.argv[2].startswith('--'):
    sym = sys.argv[2]
    i = txt.find(".fn " + sym)
    j = txt.find(".endfn " + sym, i)
    print(txt[i:j])
else:
    print(txt)
