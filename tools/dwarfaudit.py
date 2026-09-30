"""Audit our source against dwarf/ for parameter and local variable names.

dwarf/ is DWARF-derived source from the PS2 build. It carries the original
identifier names, so where our names disagree we are almost always the ones who
invented something. Register annotations are MIPS and are ignored here;
signatures can legitimately differ between the PS2 and GC builds, so a
disagreement is a lead, not a verdict.

Checks, per function present in both trees:
  PARAM  our parameter names vs DWARF's, positionally (equal arity only)
  LOCAL  DWARF locals whose name appears nowhere in our function body
  ARITY  DWARF named fewer parameters than we declare -- it omits unused ones,
         so positional matching would assign wrong names. Resolve by hand
         against the full declaration near the top of the DWARF file.

Usage:
  dwarfaudit.py                 audit every changed file that has a dwarf peer
  dwarfaudit.py <path.cpp> ...  audit specific files
  dwarfaudit.py --detail <path.cpp>   show every finding for one file
"""
import os
import re
import subprocess
import sys

DETAIL = "--detail" in sys.argv
args = [a for a in sys.argv[1:] if a != "--detail"]

IDENT = r"[A-Za-z_~][A-Za-z_0-9]*"
TYPEWORDS = {"float","int","char","double","short","long","bool","void",
             "unsigned","signed","const","struct","class","enum","union"}
# DWARF function line: "<ret and name>(<params>) {" at column 0, brace at EOL.
DW_FUNC = re.compile(r"^(?P<sig>[A-Za-z_].*?)\((?P<params>.*)\)\s*(?:const\s*)?\{\s*$", re.M)
# DWARF local: "    <type> <name>; // rNN" indented, inside a block.
DW_LOCAL = re.compile(r"^\s+(?:class |struct |enum |union |signed |unsigned |static )*"
                      r"[A-Za-z_][A-Za-z_0-9:<>, ]*[ *&]+(?P<name>" + IDENT + r")"
                      r"(?:\[[^\]]*\])?;\s*//", re.M)


def dwarf_functions(path):
    """name -> (ordered param names, set of local names)"""
    txt = open(path, encoding="utf-8", errors="replace").read()
    out = {}
    for m in DW_FUNC.finditer(txt):
        sig = m.group("sig").strip()
        nm = sig.split("(")[0].strip().split()[-1].lstrip("*&")
        if nm in ("if", "for", "while", "switch", "return", "else"):
            continue
        params = []
        for p in m.group("params").split(","):
            p = re.sub(r"/\*.*?\*/", "", p).strip()
            if not p or p == "void":
                continue
            mm = re.search(r"(" + IDENT + r")\s*(?:\[[^\]]*\])?$", p)
            # A trailing type keyword means the parameter was unnamed; a name we
            # invented for it is not a mismatch, so record it as anonymous.
            params.append(mm.group(1) if mm and mm.group(1) not in TYPEWORDS else None)
        # body = from the opening brace to the matching close at column 0
        start = m.end()
        end = txt.find("\n}", start)
        body = txt[start:end if end > 0 else len(txt)]
        locals_ = set(x.group("name") for x in DW_LOCAL.finditer(body))
        locals_ -= set(params)
        prev = out.get(nm)
        if prev is None or len(params) > len(prev[0]):
            out[nm] = (params, locals_)
    return out


def our_functions(path):
    """name -> (ordered param names, body text). Brace-matched, best effort."""
    txt = open(path, encoding="utf-8", errors="replace").read()
    txt = re.sub(r"//[^\n]*", "", txt)
    txt = re.sub(r"/\*.*?\*/", "", txt, flags=re.S)
    out = {}
    pat = re.compile(r"(?:^|\n)[^\n;{}]*?(?:(" + IDENT + r")::)?(" + IDENT + r")\s*"
                     r"\(([^;{}()]*(?:\([^()]*\)[^;{}()]*)*)\)\s*(?:const\s*)?\{")
    for m in pat.finditer(txt):
        nm = m.group(2)
        if nm in ("if", "for", "while", "switch", "catch", "return", "sizeof", "else"):
            continue
        params = []
        for p in m.group(3).split(","):
            p = p.strip()
            if not p or p == "void":
                continue
            mm = re.search(r"(" + IDENT + r")\s*(?:\[[^\]]*\])?$", p)
            params.append(mm.group(1) if mm and mm.group(1) not in TYPEWORDS else None)
        i = m.end() - 1
        depth = 0
        for j in range(i, len(txt)):
            if txt[j] == "{":
                depth += 1
            elif txt[j] == "}":
                depth -= 1
                if depth == 0:
                    break
        out.setdefault(nm, (params, txt[i:j]))
    return out


if args:
    files = args
else:
    files = subprocess.run(["git", "diff", "--name-only", "bfbbdecomp/main...HEAD",
                            "--", "src/*.cpp"], capture_output=True, text=True).stdout.split()

tot_p = tot_l = tot_f = 0
rows = []
for f in files:
    dw = "dwarf/" + f[len("src/"):]
    if not (os.path.exists(dw) and os.path.exists(f)):
        continue
    D, O = dwarf_functions(dw), our_functions(f)
    pmis, lmis, shortr, checked = [], [], [], 0
    for nm, (dp, dl) in D.items():
        if nm not in O:
            continue
        op, body = O[nm]
        checked += 1
        dp2 = [x for x in dp if x != "this"]
        # A dwarf DEFINITION lists only the parameters that were named and used;
        # unused ones are dropped, which shifts every later position. Comparing
        # positionally across a length mismatch renames things to the wrong
        # name, and because renames are byte-neutral nothing downstream catches
        # it. Only compare equal-length lists, and flag the rest for a human.
        if len(dp2) != len(op):
            shortr.append((nm, len(dp2), len(op)))
        elif dp2 != op:
            bad = [(a, b) for a, b in zip(dp2, op) if a and b and a != b]
            if bad:
                pmis.append((nm, bad))
        missing = sorted(n for n in dl
                         if not re.search(r"\b" + re.escape(n) + r"\b", body))
        if missing:
            lmis.append((nm, missing))
    if pmis or lmis or shortr:
        rows.append((f, checked, pmis, lmis, shortr))
    tot_p += len(pmis); tot_l += len(lmis); tot_f += checked

rows.sort(key=lambda r: -(len(r[2]) * 10 + len(r[3])))
print("audited %d files, %d functions matched to dwarf" % (len(files), tot_f))
print("functions with differing PARAM names: %d" % tot_p)
print("functions with dwarf LOCALs absent from our body: %d\n" % tot_l)
print("%-46s %6s %6s %6s %6s" % ("file", "fns", "param", "local", "arity"))
for f, c, p, l, sh in rows:
    print("%-46s %6d %6d %6d %6d" % (f[len("src/"):], c, len(p), len(l), len(sh)))
    if DETAIL:
        for nm, bad in p:
            print("    PARAM %s: " % nm + ", ".join("dwarf %s / ours %s" % b for b in bad))
        for nm, miss in l:
            print("    LOCAL %s: missing %s" % (nm, ", ".join(miss[:12])))
        for nm, dn, on in sh:
            print("    ARITY %s: dwarf names %d param(s), we declare %d -- dwarf omits"
                  " unused params, so DO NOT match positionally; use the full"
                  " declaration near the top of the dwarf file" % (nm, dn, on))
