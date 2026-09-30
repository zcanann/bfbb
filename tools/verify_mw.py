#!/usr/bin/env python3
"""Compile ONE unit with a CHOSEN mw_version into a private temp dir and report
one function's match percent.  Isolated: never touches the shared compiler."""
import json, os, re, shlex, subprocess, sys, tempfile
ROOT = "/home/user/bfbb"
sys.path.insert(0, os.path.join(ROOT, "tools"))
import cwexec

CLI  = cwexec.objdiff_cli(ROOT)
CFG  = json.load(open(os.path.join(ROOT, "objdiff.json")))
NINJA= open(os.path.join(ROOT, "build.ninja")).read()
BUILD_RE = re.compile(r"^build (?P<obj>\S+\.o): (?P<rule>mwcc_sjis|mwcc) (?P<body>(?:.*\n)*?)  basedir", re.M)
SRC_RE   = re.compile(r"(?:^|\s)(src[\\/]\S+\.(?:c|cp|cpp))(?:\s|$)")

def rules():
    out={}
    for m in BUILD_RE.finditer(NINJA):
        b=m.group("body")
        mw=re.search(r"mw_version = (\S+)", b); cf=re.search(r"cflags = ((?:.*\$\n)*.*)\n", b); s=SRC_RE.search(b)
        if not(mw and cf and s): continue
        out[m.group("obj").replace("\\","/")]={"src":s.group(1).replace("\\","/"),
            "mw":mw.group(1).replace("\\","/"),"rule":m.group("rule"),
            "flags":re.sub(r"\s+"," ",cf.group(1).replace("$\n"," ")).strip()}
    return out
R=rules()

def find(frag):
    h=[u for u in CFG["units"] if frag in u["name"]]
    if len(h)!=1:
        e=[u for u in h if u["name"].endswith(frag)]
        if len(e)==1: return e[0]
        raise SystemExit("ambiguous: "+", ".join(u["name"] for u in h[:12]))
    return h[0]

def run(unit_frag, mw_override, symbol):
    u=find(unit_frag); obj=u["base_path"].replace("\\","/"); info=R[obj]
    mw = mw_override or info["mw"]
    td=tempfile.mkdtemp(prefix="vmw_"); out=os.path.join(td,"o.o")
    cmd=cwexec.compile_prefix(NINJA,info["rule"],mw)+shlex.split(info["flags"],posix=False)+["-c",info["src"],"-o",out]
    cmd=[c.strip('"') if c.startswith('"') and c.endswith('"') else c for c in cmd]
    r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=900)
    if not os.path.exists(out): raise SystemExit("COMPILE FAILED:\n"+(r.stdout+r.stderr)[-2500:])
    dj=os.path.join(td,"d.json")
    subprocess.run([CLI,"diff","-1",os.path.join(ROOT,u["target_path"]),"-2",out,
                    "-o",dj,"--format","json","-c","functionRelocDiffs=none"],cwd=ROOT,capture_output=True)
    d=json.load(open(dj))
    left={s["name"]:s for s in d.get("left",{}).get("symbols",[])}
    if symbol:
        s=left.get(symbol)
        return None if s is None else s.get("match_percent",0.0)
    return {n:s.get("match_percent",0.0) for n,s in left.items()}

if __name__=="__main__":
    unit,mw,sym = sys.argv[1], (None if sys.argv[2]=="-" else sys.argv[2]), (sys.argv[3] if len(sys.argv)>3 else None)
    res=run(unit,mw,sym)
    if isinstance(res,dict):
        for n,p in sorted(res.items(),key=lambda kv:kv[1]):
            if p<100.0: print("%8.3f  %s"%(p,n))
    else:
        print("%s  mw=%s  %s"%(sym, mw or "(tree default)", "NOT FOUND" if res is None else "%.3f%%"%res))
