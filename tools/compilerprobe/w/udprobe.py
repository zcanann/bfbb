import sys, os, re, subprocess, json
ROOT = "C:/Projects/bfbb"
src = sys.argv[1]
cmd = subprocess.run(["ninja", "-t", "commands", "build/GQPE78/src/rwsdk/plugin/userdata/rpusrdat.o"],
                     cwd=ROOT, capture_output=True, text=True).stdout.strip().splitlines()[-1]
out = os.path.abspath(src)[:-2] + ".o"
c = re.sub(r" -o \S+", lambda m: " -o " + out, cmd).replace(" -MMD", "")
c = c.replace(os.path.normpath("src/rwsdk/plugin/userdata/rpusrdat.c"), os.path.abspath(src))
c = c.replace(" -c ", " -I" + os.path.normpath("src/rwsdk/plugin/userdata") + " " + " ".join(sys.argv[2:]) + " -c ", 1)
if os.path.exists(out):
    os.remove(out)
p = subprocess.run(c, cwd=ROOT, shell=True, capture_output=True, text=True)
if not os.path.exists(out):
    print("FAIL", p.stdout[-2000:], p.stderr[-2000:])
    sys.exit(1)
cfg = json.load(open(ROOT + "/objdiff.json"))
u = [x for x in cfg["units"] if x["name"] == "main/rwsdk/plugin/userdata/rpusrdat"][0]
d = json.loads(subprocess.run([ROOT + "/build/tools/objdiff-cli.exe", "diff", "-1", ROOT + "/" + u["target_path"],
                               "-2", out, "-o", "-", "--format", "json"], capture_output=True, text=True).stdout)
L = {s["name"]: s for s in d["left"]["symbols"]}
for s in d["right"]["symbols"]:
    if s.get("kind") != "SYMBOL_FUNCTION":
        continue
    m = L.get(s["name"], {}).get("match_percent")
    if "-q" in os.environ.get("UDQ", "") and m == 100:
        continue
    print("%-28s size=%-5s match=%s" % (s["name"], s.get("size"), m))
