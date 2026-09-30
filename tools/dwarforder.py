"""Find functions whose local declaration ORDER differs from dwarf/.

Unlike renaming, reordering local declarations changes generated code: under
both MW compilers declaration order drives stack slot assignment. Reordering
three locals to the dwarf order took zNPCTypeRobot's NPCMessage from 99.016%
to 100%.

So this is a worklist, not a patch. It ranks candidates by how close the
function already is, because a function at 99.x% whose only problem is stack
layout is the one most likely to flip; a function at 40% has bigger problems
and reordering it proves nothing.

Only the locals present in BOTH is compared, and only their relative order, so
extra or missing locals on either side do not create noise.

Usage:
  dwarforder.py [<path.cpp> ...]      default: every changed file with a peer
  dwarforder.py --min 95 [...]        only functions already at >= 95%
"""
import difflib
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

MIN = 0.0
if "--min" in sys.argv:
    i = sys.argv.index("--min")
    MIN = float(sys.argv[i + 1])
    del sys.argv[i:i + 2]
args = sys.argv[1:]

IDENT = r"[A-Za-z_~][A-Za-z_0-9]*"
DW_FUNC = re.compile(r"^(?P<sig>[A-Za-z_].*?)\((?P<params>.*)\)\s*(?:const\s*)?\{\s*$", re.M)
# Only genuine automatics. dwarf annotates each declaration with its storage:
#   // r18          register local
#   // r29+0x90     stack local
#   // @ 0x005CB880 function-scope STATIC, listed in DESCENDING ADDRESS order
# Statics are not declaration-ordered at all, so matching them is meaningless
# and actively harmful: zEntCruiseBubble::init_states is twelve statics, and
# "reordering to dwarf order" cost it 99.161% -> 94.699%. Compiler-generated
# `@NNNN` init-guard names are excluded for the same reason.
DW_LOCAL = re.compile(r"^\s+(?:class |struct |enum |union |signed |unsigned |static )*"
                      r"[A-Za-z_][A-Za-z_0-9:<>, ]*[ *&]+(?P<name>" + IDENT + r")"
                      r"(?:\[[^\]]*\])?;\s*//\s*r(?:[0-9]+)", re.M)
# our local: a declaration statement inside a body
OUR_LOCAL = re.compile(r"^\s+(?:const\s+|static\s+|volatile\s+|struct\s+|class\s+|unsigned\s+|signed\s+)*"
                       r"[A-Za-z_][A-Za-z_0-9:<>]*\s*[*&]?\s+(?P<name>" + IDENT + r")"
                       r"\s*(?:\[[^\]]*\])?\s*(?:=[^;]*)?;", re.M)

KEYWORDS = {"return", "if", "for", "while", "switch", "else", "do", "case", "break",
            "continue", "goto", "sizeof", "new", "delete", "typedef", "using"}


def dwarf_order(path):
    txt = open(path, encoding="utf-8", errors="replace").read()
    out = {}
    for m in DW_FUNC.finditer(txt):
        nm = m.group("sig").split("(")[0].strip().split()[-1].lstrip("*&")
        if nm in KEYWORDS:
            continue
        start = m.end()
        end = txt.find("\n}", start)
        body = txt[start:end if end > 0 else len(txt)]
        seq, seen = [], set()
        for x in DW_LOCAL.finditer(body):
            n = x.group("name")
            if n not in seen:
                seen.add(n)
                seq.append(n)
        if nm not in out or len(seq) > len(out[nm]):
            out[nm] = seq
    return out


def our_order(path):
    txt = open(path, encoding="utf-8", errors="replace").read()
    txt = re.sub(r"//[^\n]*", "", txt)
    txt = re.sub(r"/\*.*?\*/", "", txt, flags=re.S)
    out = {}
    pat = re.compile(r"(?:^|\n)[^\n;{}]*?(?:(" + IDENT + r")::)?(" + IDENT + r")\s*"
                     r"\(([^;{}()]*(?:\([^()]*\)[^;{}()]*)*)\)\s*(?:const\s*)?\{")
    for m in pat.finditer(txt):
        nm = m.group(2)
        if nm in KEYWORDS:
            continue
        i = m.end() - 1
        depth = 0
        for j in range(i, len(txt)):
            if txt[j] == "{":
                depth += 1
            elif txt[j] == "}":
                depth -= 1
                if depth == 0:
                    break
        body = txt[i:j]
        seq, seen = [], set()
        for x in OUR_LOCAL.finditer(body):
            n = x.group("name")
            if n in KEYWORDS or n in seen:
                continue
            seen.add(n)
            seq.append(n)
        out.setdefault(nm, (seq, body))
    return out


pct = {}
rep = "build/GQPE78/report.json"
if os.path.exists(rep):
    for u in json.load(open(rep))["units"]:
        for fn in u.get("functions") or []:
            base = fn["name"].split("__")[0]
            v = fn.get("fuzzy_match_percent", 0.0)
            if base not in pct or v > pct[base]:
                pct[base] = v

if args:
    files = args
else:
    files = subprocess.run(["git", "diff", "--name-only", "bfbbdecomp/main...HEAD",
                            "--", "src/*.cpp"], capture_output=True, text=True).stdout.split()

OBJDUMP = os.path.join(ROOT, "build/binutils/powerpc-eabi-objdump")
STACKREF = re.compile(r"-?\d+\(r1\)")


def _stream(obj, mangled):
    out = subprocess.run([OBJDUMP, "-d", "--section=.text", obj],
                         capture_output=True, text=True).stdout.splitlines()
    start = None
    for i, l in enumerate(out):
        if re.match(r"^[0-9a-f]{8} <%s>:" % re.escape(mangled), l):
            start = i
            break
    if start is None:
        return None
    res = []
    for l in out[start + 1:]:
        if re.match(r"^[0-9a-f]{8} <", l):
            break
        m = re.match(r"^\s+[0-9a-f]+:\s+(?:[0-9a-f]{2} ){4}\s*(.*)$", l)
        if m:
            res.append(m.group(1).strip())
    return res


def stack_touching(relcpp, name):
    """How many stack slots the two objects disagree about.

    Declaration order assigns stack slots and does nothing else. So the question
    is not whether the differing instructions mention r1 -- register allocation
    moves those around constantly while the frame stays put -- but whether the
    two objects reference a DIFFERENT SET of offsets from r1. If the sets match,
    the frame is laid out identically and no amount of reordering will help.

    xShadowVertical_DrawCache is the case that forced this: eight differing
    instructions, four of them addressing r1, and every offset the same on both
    sides (196(r1), 176(r1)). Its residual is register allocation, and moving
    `tri` to dwarf's position changed nothing at all.
    """
    o = os.path.join(ROOT, "build/GQPE78/src", relcpp[:-4] + ".o")
    t = os.path.join(ROOT, "build/GQPE78/obj", relcpp[:-4] + ".o")
    if not (os.path.exists(o) and os.path.exists(t)):
        return None
    syms = subprocess.run([OBJDUMP, "-t", t], capture_output=True, text=True).stdout
    cand = [l.split()[-1] for l in syms.splitlines()
            if " F .text" in l and l.split()[-1].startswith(name)]
    if not cand:
        return None
    A, B = _stream(t, cand[0]), _stream(o, cand[0])
    if not A or not B:
        return None
    sa = set(m.group(0) for x in A for m in [STACKREF.search(x)] if m)
    sb = set(m.group(0) for x in B for m in [STACKREF.search(x)] if m)
    return len(sa ^ sb)


rows = []
for f in files:
    dw = "dwarf/" + f[len("src/"):]
    if not (os.path.exists(dw) and os.path.exists(f)):
        continue
    D, O = dwarf_order(dw), our_order(f)
    for nm, dseq in D.items():
        if nm not in O:
            continue
        oseq, _ = O[nm]
        common = [n for n in dseq if n in oseq]
        ours = [n for n in oseq if n in common]
        if len(common) < 2 or common == ours:
            continue
        p = pct.get(nm)
        if p is None or p < MIN or p >= 100.0:
            continue
        rows.append((p, f[len("src/"):], nm, common, ours))

rows.sort(reverse=True)
print("%-7s %-6s %-30s %s" % ("fuzzy", "stack", "file", "function"))
live = 0
for p, f, nm, d, o in rows:
    n = stack_touching(f, nm)
    if n:
        live += 1
    print("%6.2f%% %-6s %-30s %s"
          % (p, ("%d" % n) if n is not None else "?", f, nm))
    print("        dwarf order: %s" % ", ".join(d))
    print("        our order  : %s" % ", ".join(o))
print("\n%d candidate functions (>= %.0f%%, order differs, >=2 shared locals)" % (len(rows), MIN))
print("%d of them address a stack slot the target does not, or vice versa." % live)
print("""
stack = how many r1 offsets one object references and the other does not.
Declaration order assigns stack slots and nothing else, so a candidate showing 0
has the same frame layout as the target and cannot be explained by order however
much the two declaration lists disagree. Its residual is register allocation,
scheduling or data placement, and reordering will only churn it.

Do not read `differing instructions that mention r1` as the signal -- register
allocation moves those constantly while the frame stays put.
xShadowVertical_DrawCache has four such instructions and every offset identical
on both sides; moving `tri` to dwarf's position changed nothing.

Three worked examples, all reverted: MoveFrolic disagrees on two locals and is
201/203 instructions from the target, all of it one expression computed into
f1-then-f27 instead of f27-then-f1. NPCC_aimVary's residual is the
zero-initialiser blob at a different .rodata offset. SlideTrackUpdate compiles
identically either way -- 99.4236% both -- because triIndex never reaches the
stack. Applying dwarf's order made the first two measurably worse.""")
