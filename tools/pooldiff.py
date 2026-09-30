#!/usr/bin/env python3
"""Compare the VALUES behind small-data pool references, instruction by instruction.

A float literal never appears in a PowerPC instruction. `x <= 0.001f` compiles to

    lfs  f0, 0(0)        R_PPC_EMB_SDA21   @784

and the number itself lives in .sdata2, named by an anonymous pool symbol. The
ordinal in that name is assigned in first-use order within the translation unit,
so it differs freely between builds and every differ has to normalise it away.

objdiff does exactly that, which means the four bytes the instruction actually
names are never compared. Two functions can be byte-identical in .text, score a
clean 100%, and still load different constants. Every wrong-float-literal bug in
the game hides in that hole -- and being at 100% is precisely what keeps anyone
from looking. datadiff --consts cannot see them either: it compares .sdata2 as a
multiset, so a value that is present on both sides but wired to the wrong *site*
cancels out.

This tool resolves each pool reference to the bytes it names, then walks the two
instruction streams in lockstep and reports the value mismatches.

Only lfs/lfd are considered. Integer small-data references name real globals
rather than pool constants, and the lis/addi pairs that build a .rodata address
name @stringBase0, whose contents legitimately differ whenever a string moves.

Functions whose instruction streams differ in length or shape are skipped -- our
source already says something different there, and the ordinary differ shows it.
The interesting case is the one that looks perfect.

Usage:
  pooldiff.py                    every unit, worst first
  pooldiff.py <unit-frag> ...    restrict to matching units
"""

import os
import re
import struct
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OD = os.path.join(ROOT, "build/binutils/powerpc-eabi-objdump")
NM = os.path.join(ROOT, "build/binutils/powerpc-eabi-nm")
TARGET = os.path.join(ROOT, "build/GQPE78/obj")
OURS = os.path.join(ROOT, "build/GQPE78/src")

POOL_SECTIONS = (".sdata2", ".sdata", ".rodata", ".data")
FLOAT_LOADS = ("lfs", "lfd")


def run(*args):
    return subprocess.run(args, capture_output=True, text=True).stdout


def section_words(obj, sec):
    """offset -> 4 raw bytes, for one section."""
    words = {}
    for line in run(OD, "-s", "-j", sec, obj).splitlines():
        parts = line.split()
        if len(parts) < 2 or not re.fullmatch(r"[0-9a-f]{4,8}", parts[0]):
            continue
        base, off = int(parts[0], 16), 0
        for tok in parts[1:]:
            if not re.fullmatch(r"[0-9a-f]{8}", tok):
                break
            words[base + off] = int(tok, 16)
            off += 4
    return words


def pool_values(obj):
    """anonymous pool symbol -> the word it names.

    A pool ordinal is only unique within its section: @1548 has been seen in
    both .sdata2 and .rodata in the same object with different contents, so the
    section has to come from the symbol table rather than be guessed from an
    offset. objdump -t names it outright; nm's one-letter class does not
    distinguish .sdata2 from .rodata.
    """
    words = {sec: section_words(obj, sec) for sec in POOL_SECTIONS}
    values = {}
    for line in run(OD, "-t", obj).splitlines():
        parts = line.split()
        if len(parts) < 5 or not parts[-1].startswith("@"):
            continue
        name, sec = parts[-1], parts[-3]
        if sec not in words:
            continue
        off = int(parts[0], 16)
        if off in words[sec]:
            values[name] = words[sec][off]
    return values


def streams(obj):
    """function -> [(mnemonic, pool value or None)]"""
    values = pool_values(obj)
    fns, cur, pending = {}, None, None
    for line in run(OD, "-dr", "--section=.text", obj).splitlines():
        head = re.match(r"^[0-9a-f]{8} <(.+)>:", line)
        if head:
            cur, pending = head.group(1), None
            fns[cur] = []
            continue
        if cur is None:
            continue
        rel = re.match(r"^\s+[0-9a-f]+: (\S+)\s+(\S+)", line)
        if rel and pending is not None:
            v = values.get(rel.group(2)) if rel.group(1) == "R_PPC_EMB_SDA21" else None
            fns[cur].append((pending, v))
            pending = None
            continue
        ins = re.match(r"^\s+[0-9a-f]+:\s+(?:[0-9a-f]{2} ){4}\s*(\S+)", line)
        if ins:
            if pending is not None:
                fns[cur].append((pending, None))
            pending = ins.group(1)
    return fns


def f32(word):
    return struct.unpack(">f", struct.pack(">I", word))[0]


def compare(target, ours):
    """[(index, mnemonic, target word, our word)] for one function pair."""
    if len(target) != len(ours):
        return []
    bad = []
    for i, (t, o) in enumerate(zip(target, ours)):
        if t[0] != o[0]:
            return []               # shape differs; not our business
        if t[1] is None or o[1] is None or t[1] == o[1]:
            continue
        if not t[0].startswith(FLOAT_LOADS):
            continue
        bad.append((i, t[0], t[1], o[1]))
    return bad


def main():
    want = sys.argv[1:]
    findings, total = [], 0
    for dirpath, _, files in os.walk(TARGET):
        for fn in sorted(files):
            if not fn.endswith(".o"):
                continue
            tp = os.path.join(dirpath, fn)
            rel = os.path.relpath(tp, TARGET).replace("\\", "/")
            op = os.path.join(OURS, os.path.relpath(tp, TARGET))
            if not os.path.exists(op):
                continue
            if want and not any(w in rel for w in want):
                continue
            t, o = streams(tp), streams(op)
            for name in t:
                if name not in o:
                    continue
                bad = compare(t[name], o[name])
                if bad:
                    findings.append((rel, name, bad))
                    total += len(bad)

    findings.sort(key=lambda f: -len(f[2]))
    for rel, name, bad in findings:
        print("=" * 74)
        print("%s  %s" % (rel, name))
        for i, mnem, tv, ov in bad:
            print("   insn %-5d %-4s target %08x %-14g | ours %08x %-14g"
                  % (i, mnem, tv, f32(tv), ov, f32(ov)))
    print()
    print("pool-value mismatches: %d in %d function(s)" % (total, len(findings)))


main()
