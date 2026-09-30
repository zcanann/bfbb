#!/usr/bin/env python3
"""Compare the STRINGS each function references, target vs ours.

A function can name the wrong string literal and still score 100.000%. objdiff
normalises `@stringBase0` offsets away -- it has to, because the pool is ordered
by first use, so any unrelated string moving shifts every offset after it -- and
the casualty is that it cannot compare which string an instruction actually
names. zNPCTypeBossSB2 sat at 100% with "RSB_foor_impact" where retail has
"RSB_foot_impact"; xStrHash of the misspelling matches nothing, so that anim
event simply never fired.

datamulti.py only catches a wrong string when it changes the .rodata contents.
It is blind to a reference aimed at the wrong *existing* string, and blind to two
functions swapping which one they use. This resolves every reference to the bytes
it names, so both show up.

Resolving takes two steps and missing the second is the trap. CW materialises the
pool base with a relocated pair whose addends are zero

    lis  rX,0        R_PPC_ADDR16_HA  @stringBase0
    addi rX,rX,0     R_PPC_ADDR16_LO  @stringBase0

and then reaches an individual string with a SEPARATE, unrelocated

    addi rY,rX,75

The first version of this tool read only the relocations, and reported every
function in zNPCFXCinematic as referencing the same string -- the one sitting at
offset 0 of the pool. The offset has to come from tracking which register holds
the base and reading the immediate off the addi that consumes it.

Attribution is by address range with overlapping symbols dropped, for the same
reason as calldiff.py and poolmulti.py: dtk's reconstructed symbols overlap
(zNPCSupplement has a weak NextAvail 0x78 bytes inside NPCC_MakeLightningInfo),
so attributing by the labels objdump prints splits one function's references
across two names.

Usage:
  strdiff.py [unit-frag ...]
  strdiff.py --self            compare each object against itself; must report 0
  strdiff.py --list <frag>     print every string each function names
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

SELF = "--self" in sys.argv
LIST = "--list" in sys.argv
FRAGS = [a for a in sys.argv[1:] if not a.startswith("-")]

STT_FUNC = 2
SECT = re.compile(r"^Disassembly of section (\S+):$")
LINE = re.compile(r"^\s*([0-9a-f]+):\t(?:[0-9a-f]{2} ){4}\t(\S+)\s*(.*)$")
RELOC = re.compile(r"R_PPC_(\S+)\s+(\S+)")
ADDI = re.compile(r"^(r\d+),(r\d+),(-?\d+)$")
# instructions after which the first operand is NOT a freshly written register
NOT_A_DEF = ("stw", "stb", "sth", "stfs", "stfd", "stmw", "stwu", "stwx", "stbx",
             "sthx", "stfsx", "stfdx", "psq_st", "cmp", "cmpw", "cmpwi", "cmplw",
             "cmplwi", "fcmpo", "fcmpu", "b", "bl", "bctrl", "mtctr", "mtlr")


class Elf:
    """Just enough ELF32-BE to find where a symbol's bytes live."""

    def __init__(self, path):
        self.d = open(path, "rb").read()
        (shoff,) = struct.unpack_from(">I", self.d, 0x20)
        shentsize, shnum, shstrndx = struct.unpack_from(">HHH", self.d, 0x2E)
        self.sh = []
        for i in range(shnum):
            off = shoff + i * shentsize
            nm, typ, _fl, _ad, offset, size, link, info, _al, _es = \
                struct.unpack_from(">10I", self.d, off)
            self.sh.append(dict(nm=nm, type=typ, offset=offset, size=size,
                                link=link, info=info))
        base = self.sh[shstrndx]["offset"]
        for s in self.sh:
            s["str"] = self._cstr(base + s["nm"])

    def _cstr(self, off):
        end = self.d.index(b"\0", off)
        return self.d[off:end].decode("ascii", "replace")

    def data(self, s):
        if s["type"] == 8:                      # SHT_NOBITS
            return b"\0" * s["size"]
        return self.d[s["offset"]:s["offset"] + s["size"]]

    def symbols(self):
        for s in self.sh:
            if s["type"] != 2:                  # SHT_SYMTAB
                continue
            strtab = self.sh[s["link"]]
            out = []
            for k in range(s["size"] // 16):
                off = s["offset"] + k * 16
                nm, val, sz, info, _o, shndx = struct.unpack_from(
                    ">IIIBBH", self.d, off)
                out.append(dict(name=self._cstr(strtab["offset"] + nm),
                                value=val, size=sz, info=info, shndx=shndx))
            return out
        return []


def strings_referenced(path):
    """{function: [strings it names, in order]}"""
    e = Elf(path)
    syms = e.symbols()

    pools = {}
    for s in syms:
        if s["shndx"] < len(e.sh):
            host = e.sh[s["shndx"]]
            if host["str"].startswith((".rodata", ".data", ".sdata")):
                pools[s["name"]] = (host, s["value"])

    # Key ranges by section INDEX, not by section name. dtk gives the target
    # object many separate sections all called ".text", so keying by name makes
    # functions in different sections look like they overlap at the same offset
    # and the overlap filter silently drops them. That produced a confident and
    # completely wrong reading of zNPCFXCinematic before it was noticed.
    fns = [(s["shndx"], s["value"], s["value"] + s["size"], s["name"])
           for s in syms
           if (s["info"] & 0xF) == STT_FUNC and s["size"] and s["shndx"] < len(e.sh)]
    keep = [f for f in fns
            if not any(g is not f and g[0] == f[0] and g[1] < f[2] and f[1] < g[2]
                       for g in fns)]
    by_sec = {}
    for sec, a, b, n in keep:
        by_sec.setdefault(sec, []).append((a, b, n))
    res = {n: [] for _, _, _, n in keep}

    def resolve(symname, off):
        ent = pools.get(symname)
        if not ent:
            return None
        host, base = ent
        blob = e.data(host)
        a = base + off
        if not (0 <= a < len(blob)):
            return None
        end = blob.find(b"\0", a)
        if end < 0:
            return None
        t = blob[a:end]
        if len(t) < 2 or not all(32 <= c < 127 for c in t):
            return None
        return t.decode("ascii")

    def owner(sec, addr):
        for a, b, n in by_sec.get(sec, ()):
            if a <= addr < b:
                return n
        return None

    # objdump emits "Disassembly of section X:" in section-header order, so the
    # nth such line is the nth code section; that is the only way to tell two
    # sections with the same name apart.
    code_secs = [i for i, sh in enumerate(e.sh)
                 if sh["str"].startswith(".text") and sh["size"]]

    out = subprocess.run([OD, "-d", "-r", "-M", "broadway", os.path.abspath(path)],
                         capture_output=True, text=True).stdout

    sec_iter = iter(code_secs)
    sec = None
    held = {}            # register -> (pool symbol, offset into it)
    pending = {}         # register -> (function, pool symbol) awaiting a verdict
    prev = None          # the instruction a following relocation line applies to

    def settle_one(reg):
        """A base that was never offset named the pool's first string after all."""
        ent = pending.pop(reg, None)
        if not ent:
            return
        fname, symname = ent
        txt = resolve(symname, 0)
        if txt and fname:
            res[fname].append(txt)

    def settle():
        for reg in list(pending):
            settle_one(reg)
    for line in out.splitlines():
        m = SECT.match(line)
        if m:
            settle()
            sec = next(sec_iter, None)
            held, prev = {}, None
            continue

        m = RELOC.search(line)
        if m and prev is not None:
            kind, symname = m.group(1), m.group(2)
            if kind == "ADDR16_LO" and symname in pools and prev[1] == "addi":
                am = ADDI.match(prev[2])
                if am:
                    settle_one(am.group(1))
                    held[am.group(1)] = (symname, 0)
                    # The pair may BE the reference (a string at pool offset 0)
                    # or may just be establishing a base that a later addi
                    # offsets. Record it provisionally and withdraw it if the
                    # register turns out to be a base -- otherwise every function
                    # in the TU looks like it references the pool's first string.
                    pending[am.group(1)] = (owner(sec, prev[0]), symname)
            elif kind not in ("ADDR16_HA",):
                # any other relocation retargets the register; forget it
                am = ADDI.match(prev[2]) if prev[1] == "addi" else None
                if am:
                    held.pop(am.group(1), None)
            continue

        m = LINE.match(line)
        if not m:
            continue
        addr, mn, ops = int(m.group(1), 16), m.group(2), m.group(3).strip()
        prev = (addr, mn, ops)

        if mn == "addi":
            am = ADDI.match(ops)
            if am:
                d, b, imm = am.group(1), am.group(2), int(am.group(3))
                if b in held and imm:
                    symname, off = held[b]
                    held[d] = (symname, off + imm)
                    pending.pop(b, None)      # it was a base, not a reference
                    txt = resolve(symname, off + imm)
                    o = owner(sec, addr)
                    if txt and o:
                        res[o].append(txt)
                    continue
                if b in held:
                    held[d] = held[b]
                    continue
            # otherwise it is a fresh value (possibly the relocated base pair)
            am = ADDI.match(ops)
            if am:
                settle_one(am.group(1))
                held.pop(am.group(1), None)
            continue

        if mn not in NOT_A_DEF:
            d = ops.split(",")[0].strip()
            if re.match(r"^r\d+$", d):
                settle_one(d)
                held.pop(d, None)
    settle()
    return res


rep = json.load(open(os.path.join(ROOT, "build/GQPE78/report.json")))
units = [u["name"][len("main/"):] for u in rep["units"]
         if u["name"].startswith("main/SB/")]
if FRAGS:
    units = [u for u in units if any(f in u for f in FRAGS)]

if LIST:
    for u in units:
        o = os.path.join(ROOT, "build/GQPE78/src", u + ".o")
        if not os.path.exists(o):
            continue
        for fn, v in sorted(strings_referenced(o).items()):
            if v:
                print("  %-52s %s" % (fn[:52], v))
    sys.exit(0)

hits = 0
for u in units:
    t = os.path.join(ROOT, "build/GQPE78/obj", u + ".o")
    o = os.path.join(ROOT, "build/GQPE78/src", u + ".o")
    if not (os.path.exists(t) and os.path.exists(o)):
        continue
    T = strings_referenced(t)
    O = strings_referenced(t if SELF else o)
    for fn in sorted(T):
        if fn not in O:
            continue
        a, b = Counter(T[fn]), Counter(O[fn])
        if a == b:
            continue
        hits += 1
        print("%s  [%s]" % (fn, u))
        for s, c in sorted((a - b).items()):
            print("     target only: %r x%d" % (s, c))
        for s, c in sorted((b - a).items()):
            print("     ours   only: %r x%d" % (s, c))

print("\n%d function(s) reference different strings%s"
      % (hits, "  [SELF-CHECK]" if SELF else ""))
