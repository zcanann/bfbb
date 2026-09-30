"""For each symbol name given, print which unit owns it in the retail link.

Resolves config/GQPE78/symbols.txt addresses against splits.txt ranges.
Usage: b3a_owner.py <symbol> [<symbol> ...]
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

syms = {}
for line in open("config/GQPE78/symbols.txt"):
    m = re.match(r"(\S+) = (\.\w+):(0x[0-9A-Fa-f]+);(.*)", line.strip())
    if m:
        syms[m.group(1)] = (m.group(2), int(m.group(3), 16), m.group(4))

ranges = []
unit = None
for line in open("config/GQPE78/splits.txt"):
    s = line.rstrip("\n")
    if s and not s[0].isspace() and s.endswith(":"):
        unit = s[:-1]
        continue
    m = re.match(r"\s+(\.\w+)\s+start:(0x[0-9A-Fa-f]+) end:(0x[0-9A-Fa-f]+)", s)
    if m and unit:
        ranges.append((m.group(1), int(m.group(2), 16), int(m.group(3), 16), unit))


def owner(sec, addr):
    for s, a, b, u in ranges:
        if s == sec and a <= addr < b:
            return u
    return "(not in any split)"


for name in sys.argv[1:]:
    if name not in syms:
        print("%-46s  ABSENT from retail symbols.txt" % name)
        continue
    sec, addr, rest = syms[name]
    print("%-46s  %s:0x%08X  %-34s %s" % (name, sec, addr, owner(sec, addr), rest.strip()))
