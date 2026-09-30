#!/usr/bin/env python3
"""Rank units by genuinely wrong static data, using a per-section word multiset.

Two lenses that look right and are not:

  * report.json's `matched_data` is symbol-attribution based. xpkrsvc scores
    0.14% with a .data that is byte-identical to the target.
  * Positional byte comparison (what datadiff.py counts) is dominated by shift.
    CodeWarrior pools constants in order of first use, so one absent or extra
    blob makes every byte after it differ. zEntCruiseBubble reports 2555
    differing .rodata bytes while being 359 bytes short -- almost all of that is
    the shift, not wrong values.

The lens that survives both is a multiset (Counter) difference of the 4-byte
words in a section, with 00000000 dropped because alignment padding is noise.
Order-independent, so pooling order and insertions cost nothing; a word present
on one side and absent on the other is a real difference in the data.

This complements datadiff.py rather than replacing it: datadiff still shows you
WHERE and which relocations differ, which is what you need once this says a unit
is worth looking at.

Usage:
  datamulti.py [unit-frag ...]
"""

import json
import os
import re
import subprocess
import sys
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OD = os.path.join(ROOT, "build", "binutils", "powerpc-eabi-objdump.exe")
HEX = re.compile(r"^[0-9a-f]{2,8}$")
FRAGS = [a for a in sys.argv[1:] if not a.startswith("-")]

DATA_SECTIONS = (".data", ".rodata", ".sdata", ".sdata2")


def words_by_section(obj):
    """{section: Counter(word)} -- one objdump run for the whole object."""
    out = subprocess.run([OD, "-s", os.path.abspath(obj)],
                         capture_output=True, text=True).stdout
    cur, raw = None, {}
    for line in out.splitlines():
        m = re.match(r"Contents of section (\S+):", line)
        if m:
            cur = m.group(1)
            raw.setdefault(cur, [])
            continue
        if cur is None or not cur.startswith(DATA_SECTIONS):
            continue
        # cols 2..5 are the hex groups; the ASCII column contains spaces and
        # would otherwise leak in, so every field is validated as hex digits.
        # The last group of a section can be short -- one side is padded to the
        # section alignment and the other is not, and dropping the short group
        # instead of padding it reported the padded side's word as target-only
        # in 40 units that were byte-identical.
        for f in line.split()[1:5]:
            if HEX.match(f):
                raw[cur].append(f)
    res = {}
    for sec, groups in raw.items():
        h = "".join(groups)
        h += "0" * (-len(h) % 8)
        c = Counter(h[i:i + 8] for i in range(0, len(h), 8))
        c.pop("00000000", None)
        res[sec] = c
    return res


rep = json.load(open(os.path.join(ROOT, "build/GQPE78/report.json")))
units = [u["name"][len("main/"):] for u in rep["units"]
         if u["name"].startswith("main/SB/")]
if FRAGS:
    units = [u for u in units if any(f in u for f in FRAGS)]

rows = []
for u in units:
    t = os.path.join(ROOT, "build/GQPE78/obj", u + ".o")
    o = os.path.join(ROOT, "build/GQPE78/src", u + ".o")
    if not (os.path.exists(t) and os.path.exists(o)):
        continue
    T, O = words_by_section(t), words_by_section(o)
    per, total = [], 0
    for sec in sorted(set(T) | set(O)):
        if not sec.startswith(DATA_SECTIONS):
            continue
        a, b = T.get(sec, Counter()), O.get(sec, Counter())
        n = sum(((a - b) + (b - a)).values())
        if n:
            per.append((sec, n, a - b, b - a))
            total += n
    if total:
        rows.append((total, u, per))

rows.sort(reverse=True)
for total, u, per in rows:
    print("%6d  %s" % (total, u))
    for sec, n, miss, extra in per:
        print("          %-9s %4d   target-only %s%s"
              % (sec, n,
                 ", ".join(sorted(miss)[:6]) or "-",
                 " ..." if len(miss) > 6 else ""))
        if extra:
            print("          %-9s        ours-only   %s%s"
                  % ("", ", ".join(sorted(extra)[:6]),
                     " ..." if len(extra) > 6 else ""))
print("\n%d unit(s) with differing data words" % len(rows))
