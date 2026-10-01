#!/usr/bin/env python3
"""Classify every non-matching function in a progress category.

For each function below 100%, compile its unit with the project compiler and
with alternative compilers, and classify the diff against the target:

  shape (project compiler):
    REG     identical opcodes and operands except register numbers
    SCHED   same multiset of normalised instructions, different order
    STRUCT  different instructions (count or content)
  compilers: which alternative compilers (if any) give 100% from the SAME source

Usage: python tools/residue.py [--category RW] [--compilers 2.0p1,2.5,2.6,2.7]
                               [--json out.json] [--md out.md]
Must be run from the repo root after a build (uses objdiff.json, build.ninja).
"""
import argparse, collections, json, os, re, subprocess, sys, tempfile
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OBJDIFF = os.environ.get("OBJDIFF") or os.path.join(ROOT, "build", "tools", "objdiff-cli.exe")
REG = re.compile(r"\b(?:r\d+|f\d+|cr\d)\b")


def compile_cmd(unit):
    obj = unit["base_path"]
    out = subprocess.run(["ninja", "-t", "commands", obj], cwd=ROOT, capture_output=True, text=True).stdout
    lines = out.strip().splitlines()
    return lines[-1] if lines else None


def build_variant(cmd, compiler, out):
    c = cmd
    if compiler:
        c = re.sub(r"compilers\\GC\\[^\\]+\\", lambda m: "compilers\\GC\\" + compiler + "\\", c)
    c = re.sub(r" -o \S+", lambda m: " -o " + out, c).replace(" -MMD", "")
    return subprocess.run(c, cwd=ROOT, shell=True, capture_output=True, text=True).returncode == 0


def diff(target, base):
    p = subprocess.run([OBJDIFF, "diff", "-1", target, "-2", base, "-o", "-", "--format", "json",
                        "-c", "functionRelocDiffs=none"], capture_output=True, text=True)
    return json.loads(p.stdout)


def insns(sym):
    out = []
    for i in sym.get("instructions", []):
        ins = i.get("instruction")
        if ins:
            out.append(ins.get("formatted", ""))
    return out


def norm(s, regs=True):
    s = re.sub(r"\s+<.*$", "", s)  # relocation annotation
    s = re.sub(r"@\d+", "@N", s)  # compiler-numbered labels/constants
    s = re.sub(r"\$\d+", "$N", s)
    if s.split(" ", 1)[0].startswith("b"):  # branch targets move with code size
        s = re.sub(r"0x[0-9a-fA-F]+", "ADDR", s)
    return REG.sub("R", s) if regs else s


def mnem(s):
    return s.split(" ", 1)[0]


def classify(left, right):
    """Return (shape, first_diff, opdelta, ndelta).

    REG    same normalised sequence, only register numbers differ
    SCHED  same normalised multiset, different order
    OPS    same instruction count, some instructions differ
    COUNT  different instruction count
    opdelta: mnemonic -> (ours - target) for mnemonics whose counts differ
    """
    a, b = insns(left), insns(right)
    na, nb = [norm(x) for x in a], [norm(x) for x in b]
    first = next((i for i, (x, y) in enumerate(zip(na, nb)) if x != y), None)
    ca, cb = collections.Counter(mnem(x) for x in a), collections.Counter(mnem(x) for x in b)
    opdelta = {k: cb[k] - ca[k] for k in set(ca) | set(cb) if cb[k] != ca[k]}
    if len(a) != len(b):
        return "COUNT", first, opdelta, len(b) - len(a)
    if na == nb:
        return "REG", next((i for i, (x, y) in enumerate(zip(a, b)) if x != y), None), opdelta, 0
    if collections.Counter(na) == collections.Counter(nb):
        return "SCHED", first, opdelta, 0
    return "OPS", first, opdelta, 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--category", default="RW")
    ap.add_argument("--compilers", default="2.0p1,2.5,2.6,2.7")
    ap.add_argument("--json")
    ap.add_argument("--md")
    args = ap.parse_args()
    alts = [c for c in args.compilers.split(",") if c]
    cfg = json.load(open(os.path.join(ROOT, "objdiff.json")))
    report = json.load(open(os.path.join(ROOT, "build", "GQPE78", "report.json")))
    want = {}
    for u in report["units"]:
        if args.category not in u["metadata"].get("progress_categories", []):
            continue
        bad = [f for f in u.get("functions", []) if f.get("fuzzy_match_percent", 0) < 100]
        if bad:
            want[u["name"]] = bad
    units = [u for u in cfg["units"] if u["name"] in want]
    tmp = tempfile.mkdtemp(prefix="residue_")

    def do_unit(u):
        cmd = compile_cmd(u)
        res = {}
        target = os.path.join(ROOT, u["target_path"])
        base = os.path.join(ROOT, u["base_path"])
        d0 = diff(target, base)
        lsyms = {s["name"]: s for s in d0["left"]["symbols"]}
        rsyms = d0.get("right", {}).get("symbols", [])
        alt_scores = {}
        for c in alts:
            out = os.path.join(tmp, re.sub(r"\W", "_", u["name"]) + "_" + c + ".o")
            if cmd and build_variant(cmd, c, out):
                d = diff(target, out)
                alt_scores[c] = {s["name"]: s.get("match_percent", 0) for s in d["left"]["symbols"]
                                 if s.get("kind") == "SYMBOL_FUNCTION"}
        for f in want[u["name"]]:
            name = f["name"]
            l = lsyms.get(name)
            r = rsyms[l["target_symbol"]] if l and "target_symbol" in l else None
            if r is None:
                shape, first, opdelta, ndelta = "MISSING", None, {}, 0
            else:
                shape, first, opdelta, ndelta = classify(l, r)
            res[name] = {
                "unit": u["name"].replace("main/", ""),
                "size": int(f["size"]),
                "pct": round(f.get("fuzzy_match_percent", 0), 3),
                "shape": shape,
                "first_diff": first,
                "ndelta": ndelta,
                "opdelta": opdelta,
                "match_under": [c for c in alts if alt_scores.get(c, {}).get(name) == 100],
                "alt": {c: round(alt_scores.get(c, {}).get(name, 0), 2) for c in alts},
            }
        return res

    results = {}
    with ThreadPoolExecutor(4) as ex:
        for r in ex.map(do_unit, units):
            results.update(r)
    if args.json:
        json.dump(results, open(args.json, "w"), indent=1)
    rows = sorted(results.items(), key=lambda kv: (kv[1]["unit"], kv[0]))
    lines = ["| unit | function | size | % | shape | insn delta | opcode delta (ours - target) | matches 100% under |",
             "|---|---|---|---|---|---|---|---|"]
    for n, r in rows:
        od = " ".join(f"{k}{v:+d}" for k, v in sorted(r["opdelta"].items())) or "-"
        lines.append(f"| {r['unit']} | `{n}` | {r['size']} | {r['pct']} | {r['shape']} | {r['ndelta']:+d} | {od} | {', '.join(r['match_under']) or '-'} |")
    summ = collections.Counter(r["shape"] for r in results.values())
    comp = sum(1 for r in results.values() if r["match_under"])
    text = "\n".join(lines) + f"\n\nshapes: {dict(summ)}; match under another compiler: {comp}/{len(results)}\n"
    if args.md:
        open(args.md, "w").write(text)
    print(text)


if __name__ == "__main__":
    main()
