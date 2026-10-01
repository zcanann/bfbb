"""regonly.py <unit> <symbol> [--src copy.c]: is the diff register-only? Prints the non-register rows.

Normalises r/f/cr register numbers on both sides of every differing row (vfdiff output) and lists
rows that still differ (= instruction/order/size differences, not allocation)."""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
args = [sys.executable, os.path.join(HERE, 'vfdiff.py')] + sys.argv[1:]
out = subprocess.run(args, capture_output=True, text=True).stdout.splitlines()
REG = re.compile(r'\b(?:r\d+|f\d+|cr\d)\b')
rows = nonreg = 0
for line in out[1:]:
    m = re.match(r'\s*(\d+) (.)\s+(.*?)\s*\|\s(.*)$', line)
    if not m or m.group(2) != '|':
        continue
    rows += 1
    a, b = m.group(3).strip(), m.group(4).strip()
    if REG.sub('R', a) != REG.sub('R', b):
        nonreg += 1
        print('%4s  %-45s | %s' % (m.group(1), a, b))
print('%s: %d differing rows, %d not register-only' % (out[0] if out else '?', rows, nonreg))
