"""alt.py <unit-stem e.g. rwsdk/world/bamatlst> <sym|-> [--mw 2.0p1] [--src altfile.c] [--show] [--all]
Compile the unit (optionally from an alternative source copy) with a given compiler, into scratch,
and diff against target. Never touches build/."""
import sys, os, re, subprocess, json, argparse, tempfile, hashlib
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))).replace("\\", "/")
OBJDIFF = ROOT + "/build/tools/objdiff-cli.exe"
sys.path.insert(0, ROOT + "/tools")
ap = argparse.ArgumentParser()
ap.add_argument("unit"); ap.add_argument("sym")
ap.add_argument("--mw", default=None); ap.add_argument("--src"); ap.add_argument("--show", action="store_true")
ap.add_argument("--all", action="store_true"); ap.add_argument("--extra", default="")
a = ap.parse_args()
obj = "build/GQPE78/src/" + a.unit + ".o"
cmd = subprocess.run(["ninja", "-t", "commands", obj], cwd=ROOT, capture_output=True, text=True).stdout.strip().splitlines()[-1]
SCR = os.path.dirname(os.path.abspath(__file__)) + "/w"
tag = hashlib.md5((a.unit + str(a.mw) + str(a.src) + a.extra).encode()).hexdigest()[:8]
out = SCR + "/" + tag + ".o"
c = cmd
if a.mw:
    c = re.sub(r"GC[/\\][^/\\]+[/\\]", lambda m: "GC/" + a.mw + "/", c)
c = re.sub(r" -o \S+", lambda m: " -o " + out.replace("/", "\\"), c).replace(" -MMD", "")
if a.src:
    orig = "src/" + a.unit + ".c"
    srcdir = os.path.dirname(orig).replace("/", "\\")
    origw = orig.replace("/", "\\")
    assert origw in c or orig in c, c
    c = c.replace(origw, os.path.abspath(a.src).replace("/", "\\")).replace(orig, os.path.abspath(a.src))
    c = c.replace(" -c ", " -I" + srcdir + " -c ", 1)
if a.extra:
    c = c.replace(" -c ", " " + a.extra + " -c ", 1)
if os.path.exists(out): os.remove(out)
p = subprocess.run(c, cwd=ROOT, shell=True, capture_output=True, text=True)
if not os.path.exists(out):
    print("COMPILE FAIL\n", c, p.stdout[-3000:], p.stderr[-3000:]); sys.exit(1)
cfg = json.load(open(ROOT + "/objdiff.json"))
u = [x for x in cfg["units"] if x["name"] == "main/" + a.unit][0]
d = json.loads(subprocess.run([OBJDIFF, "diff", "-1", ROOT + "/" + u["target_path"], "-2", out, "-o", "-", "--format", "json"], capture_output=True, text=True).stdout)
L = d["left"]["symbols"]; R = d.get("right", {}).get("symbols", [])
def fmt(ins):
    if not ins: return ""
    i = ins.get("instruction", {}); t = i.get("formatted", "")
    r = i.get("relocation")
    if r: t += "  <" + (r.get("target", {}).get("name", "?")) + ">"
    return t
for s in L:
    if s.get("kind") != "SYMBOL_FUNCTION": continue
    if a.sym != "-" and s["name"] != a.sym: continue
    print(f"{s.get('match_percent',0):7.2f} {s['name']}")
    if a.show and a.sym != "-":
        r = R[s["target_symbol"]] if "target_symbol" in s else {}
        li = s.get("instructions", []); ri = r.get("instructions", [])
        for k in range(max(len(li), len(ri))):
            x = li[k] if k < len(li) else None; y = ri[k] if k < len(ri) else None
            kind = (x or {}).get("diff_kind") or (y or {}).get("diff_kind") or ""
            mark = "  " if not kind or kind == "DIFF_NONE" else {"DIFF_REPLACE":"|","DIFF_DELETE":"<","DIFF_INSERT":">","DIFF_OP_MISMATCH":"|","DIFF_ARG_MISMATCH":"r"}.get(kind,"?")
            if mark == "  " and not a.all: continue
            print(f"{k:4d} {mark:2s} {fmt(x):55.55s} | {fmt(y)}")
