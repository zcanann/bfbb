#!/usr/bin/env python3
"""Dump the bytes of a data symbol out of a target object.

The original objects still carry all of their static data: goal sequence
tables, sound asset lists, hook name arrays, tweak defaults, jump tables. When
a function you are reconstructing indexes into one of those, the contents are
recoverable exactly - there is no need to invent them, and inventing them
produces source that looks authoritative and is wrong.

Usage:
  symdump.py <unit-fragment>                    data symbols, largest first
  symdump.py <unit-fragment> <symbol-fragment> [fmt]

  fmt is one of:
    mix  (default)  word and float side by side - use when unsure
    u32             hex words, for enums / ids / pointers
    f32             floats, for tweak blocks and curves
    raw             plain hex bytes

Reads the ELF directly; no toolchain binary required.
"""
import json
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CFG = json.load(open(os.path.join(ROOT, "objdiff.json")))

BE = ">"
SHT_SYMTAB = 2
STT_OBJECT = 1


def read_elf(path):
    d = open(path, "rb").read()
    if d[:4] != b"\x7fELF":
        raise SystemExit("%s is not an ELF object" % path)
    shoff, = struct.unpack_from(BE + "I", d, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from(BE + "HHH", d, 0x2E)
    secs = []
    for i in range(shnum):
        o = shoff + i * shentsize
        name, typ, flags, addr, off, size, link, info, align, entsize = \
            struct.unpack_from(BE + "10I", d, o)
        secs.append(dict(name=name, type=typ, off=off, size=size, link=link,
                         addr=addr))
    shstr = secs[shstrndx]
    for s in secs:
        end = d.index(b"\0", shstr["off"] + s["name"])
        s["sname"] = d[shstr["off"] + s["name"]:end].decode()
    return d, secs


def symbols(d, secs):
    out = []
    for s in secs:
        if s["type"] != SHT_SYMTAB:
            continue
        strtab = secs[s["link"]]
        for i in range(s["size"] // 16):
            o = s["off"] + i * 16
            nm, val, sz, info, other, shndx = struct.unpack_from(BE + "IIIBBH", d, o)
            end = d.index(b"\0", strtab["off"] + nm)
            name = d[strtab["off"] + nm:end].decode("utf-8", "replace")
            out.append(dict(name=name, value=val, size=sz, type=info & 0xF,
                            shndx=shndx))
    return out


def find_unit(frag):
    hits = [u for u in CFG["units"] if frag in u["name"]]
    if len(hits) != 1:
        exact = [u for u in hits if u["name"].endswith(frag)]
        if len(exact) != 1:
            raise SystemExit("ambiguous or no match: " +
                             ", ".join(u["name"] for u in hits[:12]))
        return exact[0]
    return hits[0]


def main():
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    unit = find_unit(sys.argv[1])
    d, secs = read_elf(os.path.join(ROOT, unit["target_path"]))
    syms = symbols(d, secs)

    if len(sys.argv) < 3:
        for s in sorted(syms, key=lambda x: -x["size"]):
            if s["type"] == STT_OBJECT and s["size"]:
                sec = secs[s["shndx"]]["sname"] if s["shndx"] < len(secs) else "?"
                print("%7d  %-12s %s" % (s["size"], sec, s["name"]))
        return

    want = sys.argv[2]
    fmt = sys.argv[3] if len(sys.argv) > 3 else "mix"
    found = False
    for s in syms:
        if want not in s["name"] or s["type"] != STT_OBJECT:
            continue
        found = True
        sec = secs[s["shndx"]]
        print("=== %s  %d bytes in %s ===" % (s["name"], s["size"], sec["sname"]))
        if sec["sname"] == ".bss":
            print("(.bss - zero initialised, nothing to read)")
            continue
        blob = d[sec["off"] + s["value"]:sec["off"] + s["value"] + s["size"]]
        for i in range(0, len(blob), 16):
            row = blob[i:i + 16]
            nw = len(row) // 4
            words = struct.unpack(BE + "%dI" % nw, row[:nw * 4])
            floats = struct.unpack(BE + "%df" % nw, row[:nw * 4])
            if fmt == "u32":
                cells = "  ".join("0x%08x" % w for w in words)
            elif fmt == "f32":
                cells = "  ".join("%12g" % f for f in floats)
            elif fmt == "raw":
                cells = row.hex(" ")
            else:
                cells = "  ".join("0x%08x/%-10.4g" % (w, f)
                                  for w, f in zip(words, floats))
            print("%6d: %s" % (i, cells))
    if not found:
        raise SystemExit("no data symbol matching %r in %s" % (want, unit["name"]))


main()
