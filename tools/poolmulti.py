#!/usr/bin/env python3
"""Compare the SET of float constants each function loads, target vs ours.

tools/pooldiff.py walks the two instruction streams in lockstep and gives up on
any function whose streams differ in shape -- which is exactly the set of
functions most likely to be wrong. xhud::widget::hide loaded 255.0f where retail
loads 0.5f, and pooldiff never looked at it because the surrounding code had
already diverged.

This compares the multiset of resolved lfs/lfd values instead, so it does not
care about order or scheduling. A value present on one side and absent on the
other is a wrong literal.

Two things have to be right or the output is all noise:

  * Attribute by ADDRESS RANGE, not by the labels objdump prints. dtk's
    reconstructed symbols overlap in the target objects -- zNPCSupplement has a
    weak NextAvail sitting 0x78 bytes inside NPCC_MakeLightningInfo -- so
    objdump prints a label in the middle of a body and splits one function's
    constants across two names, inventing a difference on both.
  * Drop any symbol that overlaps another rather than guess which owns the code.

Usage:
  poolmulti.py [unit-frag ...] [--counts]

--counts also reports values present on both sides in differing quantities;
those are usually rematerialisation, not bugs.
"""

import json
import os
import re
import struct
import subprocess
import sys
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OD = os.path.join(ROOT, "build", "binutils", "powerpc-eabi-objdump.exe")
SHOW_COUNTS = "--counts" in sys.argv
FRAGS = [a for a in sys.argv[1:] if not a.startswith("-")]

SYM = re.compile(r"^([0-9a-f]{8})\s.{7}\s+(\S+)\s+([0-9a-f]{8})\s+(\S+)$")
INSN = re.compile(r"^\s*([0-9a-f]+):\t")
SECT = re.compile(r"^Disassembly of section (\S+):$")
RELOC = re.compile(r"R_PPC_EMB_SDA21\s+(\S+)")


def run(*args):
    return subprocess.run([OD] + list(args), capture_output=True, text=True).stdout


def sections(obj):
    cur, data = None, {}
    for l in run("-s", obj).splitlines():
        m = re.match(r"Contents of section (\S+):", l)
        if m:
            cur = m.group(1)
            data[cur] = bytearray()
            continue
        if cur:
            m = re.match(r"\s*([0-9a-f]+) ((?:[0-9a-f]{2,8} ){1,4})", l)
            if m:
                off = int(m.group(1), 16)
                b = bytes.fromhex(m.group(2).replace(" ", ""))
                if len(data[cur]) < off:
                    data[cur] += b"\0" * (off - len(data[cur]))
                data[cur][off:off + len(b)] = b
    return data


def symtab(obj):
    """{name: (section, addr)} for every sized symbol, and the .text ranges."""
    named, fns = {}, []
    for l in run("-t", obj).splitlines():
        m = SYM.match(l)
        if not m:
            continue
        addr, sec, size, name = (int(m.group(1), 16), m.group(2),
                                 int(m.group(3), 16), m.group(4))
        named[name] = (sec, addr)
        if ".text" in sec and size:
            fns.append((sec, addr, addr + size, name))
    keep = [f for f in fns
            if not any(g is not f and g[0] == f[0] and g[1] < f[2] and f[1] < g[2]
                       for g in fns)]
    return named, keep


def value_of(data, sec, addr, is_double):
    if sec not in data:
        return None
    b = bytes(data[sec][addr:addr + 8])
    try:
        if is_double:
            return "%g" % struct.unpack(">d", b)[0]
        return "%g" % struct.unpack(">f", b[:4])[0]
    except struct.error:
        return None


def consts(obj):
    data, (named, ranges) = sections(obj), symtab(obj)
    res = {nm: Counter() for _, _, _, nm in ranges}
    by_sec = {}
    for sec, st, en, nm in ranges:
        by_sec.setdefault(sec, []).append((st, en, nm))

    def owner(sec, a):
        for st, en, nm in by_sec.get(sec, ()):
            if st <= a < en:
                return nm
        return None

    sec, last, cur = None, None, None
    for l in run("-d", "-r", "-M", "broadway", obj).splitlines():
        m = SECT.match(l)
        if m:
            sec, last, cur = m.group(1), None, None
            continue
        m = INSN.match(l)
        if m:
            cur = owner(sec, int(m.group(1), 16))
            last = l
            continue
        m = RELOC.search(l)
        if m and cur and last and re.search(r"\blf[sd]\b", last):
            s = named.get(m.group(1))
            if s:
                v = value_of(data, s[0], s[1], "lfd" in last)
                if v is not None:
                    res[cur][v] += 1
    return res


rep = json.load(open(os.path.join(ROOT, "build/GQPE78/report.json")))
units = [u["name"][len("main/"):] for u in rep["units"]
         if u["name"].startswith("main/SB/")]
if FRAGS:
    units = [u for u in units if any(f in u for f in FRAGS)]

score = {}
for u in rep["units"]:
    if u["name"].startswith("main/SB/"):
        for f in u.get("functions", []):
            score[(u["name"][len("main/"):], f["name"])] = f.get("fuzzy_match_percent", 0.0)

hits = 0
for u in units:
    t = os.path.join(ROOT, "build/GQPE78/obj", u + ".o")
    o = os.path.join(ROOT, "build/GQPE78/src", u + ".o")
    if not (os.path.exists(t) and os.path.exists(o)):
        continue
    T, O = consts(t), consts(o)
    for fn, tc in sorted(T.items()):
        oc = O.get(fn)
        if oc is None or tc == oc:
            continue
        miss = Counter({k: v for k, v in (tc - oc).items() if k not in oc})
        extra = Counter({k: v for k, v in (oc - tc).items() if k not in tc})
        if not (miss or extra) and not SHOW_COUNTS:
            continue
        hits += 1
        print("%s  [%s]  %.2f%%" % (fn, u, score.get((u, fn), 0.0)))
        if miss:
            print("     target only: %s" % ", ".join(
                "%s x%d" % (k, v) for k, v in sorted(miss.items())))
        if extra:
            print("     ours   only: %s" % ", ".join(
                "%s x%d" % (k, v) for k, v in sorted(extra.items())))

print("\n%d function(s) load a constant the other side never loads" % hits)
