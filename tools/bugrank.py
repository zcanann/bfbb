#!/usr/bin/env python3
"""Rank functions most likely to contain a BUG, rather than by how badly they match.

Lowest-match-first is the wrong order for bug hunting. Every behavioural defect
found on 2026-08-26 sat between 93% and 100%: xhud::widget::hide at 94.6%
(255.0f where retail has 0.5f), zTalkBox at 93-96% (a <sound> tag falling
through and playing), and two wrong string literals inside functions scoring a
clean 100.000%. Functions in the 40-70% band were almost all scheduling.

So the interesting set is "high match AND a semantic difference": the score says
finished, the instruction multiset says otherwise.

semdiff's raw output cannot be ranked directly, because one very common
difference is cosmetic and very loud. When a function loads a constant block
from .rodata, the target names an anonymous pool symbol at one offset and we
name .rodata.0 at another. Every word in the block becomes two terms, so a
function with an identity matrix in it reports 24 terms while being byte-for-byte
equivalent. Verified twice by resolving the values: xhud::render_model (10 words)
and zNPCTypeBossPlankton's update_move_orbit (12 words) are both identical.

This tool cancels that class: in any term of the form

    lis R, SYM@ha / addi R, R, SYM@l / lwz R, 0xNNN(R) <SYM>

the offset and the symbol name are erased before the target-only and ours-only
multisets are differenced. Terms that cancel are reported separately rather than
dropped, since the cancellation assumes the pooled VALUES agree -- true in both
cases checked, but not proven in general. Use tools/poolmulti.py to confirm the
float values of any function you care about.

Usage:
  bugrank.py [unit-frag ...]      rank by match %, bug-shaped terms only
  bugrank.py --pool               also list the functions that fully cancelled
"""

import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHOW_POOL = "--pool" in sys.argv
FRAGS = [a for a in sys.argv[1:] if not a.startswith("-")]

HEAD = re.compile(r"^(\S+)\s+(\d+)\s+([\d.]+)%\s+(\d+) term\(s\)\s+\[(\S+)\]")
TERM = re.compile(r"^\s+(target|ours)\s+only : (.+?)\s*$")

# a pooled-constant reference: the symbol it names and the offset into it both
# shift freely between builds and carry no meaning on their own
POOLISH = re.compile(
    r"^(lis R, )\S+@ha( <\S+>)?$"
    r"|^(addi R, R, )\S+@l( <\S+>)?$"
    r"|^(lwz R, )0x[0-9a-f]+\(R\)( <\S+>)?$"
)


def canon(term):
    """Erase the parts of a pooled reference that are free to differ."""
    if not POOLISH.match(term):
        return term
    t = re.sub(r"\S+@(ha|l)", "SYM@\\1", term)
    t = re.sub(r"0x[0-9a-f]+\(R\)", "OFF(R)", t)
    t = re.sub(r"<\S+>", "<SYM>", t)
    return t


OD = os.path.join(ROOT, "build", "binutils", "powerpc-eabi-objdump.exe")
FRAME = re.compile(r"^\s*[0-9a-f]+:	(?:[0-9a-f]{2} ){4}	stwu\s+r1,(-\d+)\(r1\)$")
FNLBL = re.compile(r"^[0-9a-f]+ <(.+)>:$")


def frame_sizes(obj):
    """{function: stack frame size}, from its `stwu r1,-N(r1)` prologue.

    A different frame size makes every r1-relative operand differ, so a function
    whose frames disagree reports a pile of terms that are all one difference:
    Process__14zNPCGoalWander showed 22 of them (target 0x70, ours 0x60). That is
    worth knowing before spending time reading the terms, but it is NOT grounds
    for cancelling the function -- extra stack can equally mean a local retail has
    and we do not. So it is reported, not filtered.
    """
    out = subprocess.run([OD, "-d", "-M", "broadway", os.path.abspath(obj)],
                         capture_output=True, text=True).stdout
    res, cur = {}, None
    for line in out.splitlines():
        m = FNLBL.match(line)
        if m:
            cur = m.group(1)
            continue
        if cur and cur not in res:
            m = FRAME.match(line)
            if m:
                res[cur] = -int(m.group(1))
    return res


cmd = [sys.executable, os.path.join(ROOT, "tools", "semdiff.py"), "-v"] + FRAGS
out = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT).stdout

from collections import Counter

rows, cur, terms = [], None, None
def flush():
    if cur is None:
        return
    t = Counter(canon(x) for s, x in terms if s == "target")
    o = Counter(canon(x) for s, x in terms if s == "ours")
    left = sum(((t - o) + (o - t)).values())
    rows.append((cur, left, len(terms)))

for line in out.splitlines():
    m = HEAD.match(line)
    if m:
        flush()
        cur = (float(m.group(3)), int(m.group(2)), m.group(1), m.group(5))
        terms = []
        continue
    m = TERM.match(line)
    if m and cur:
        terms.append((m.group(1), m.group(2)))
flush()

real = [r for r in rows if r[1]]
pool = [r for r in rows if not r[1]]
real.sort(key=lambda r: -r[0][0])
pool.sort(key=lambda r: -r[0][0])

print("BUG-SHAPED semantic differences, highest match first")
print("(pool-placement-only differences cancelled out)\n")
# annotate frame-size disagreements: one difference that wears many hats
frames = {}
for unit in sorted({u for (_, _, _, u), _, _ in real}):
    rel = unit.replace("main/", "")
    t = os.path.join(ROOT, "build/GQPE78/obj", rel + ".o")
    o = os.path.join(ROOT, "build/GQPE78/src", rel + ".o")
    if os.path.exists(t) and os.path.exists(o):
        frames[unit] = (frame_sizes(t), frame_sizes(o))

print("  %-9s %-6s %-7s %-48s %s"
      % ("match", "terms", "bytes", "function", "unit"))
for (p, b, n, u), left, tot in real:
    ft, fo = frames.get(u, ({}, {}))
    note = ""
    if n in ft and n in fo and ft[n] != fo[n]:
        note = "   [frame 0x%x vs 0x%x]" % (ft[n], fo[n])
    print("  %7.3f%%  %-6d %-7d %-48s %s%s"
          % (p, left, b, n[:48], u.replace("main/SB/", ""), note))
print("\n  %d bug-shaped, %d pool-placement-only, %d total"
      % (len(real), len(pool), len(rows)))

if SHOW_POOL:
    print("\nPOOL-PLACEMENT ONLY (values assumed equal; verify with poolmulti.py)")
    for (p, b, n, u), left, tot in pool:
        print("  %7.3f%%  %-6d %-7d %-48s %s"
              % (p, tot, b, n[:48], u.replace("main/SB/", "")))
