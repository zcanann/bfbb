#!/usr/bin/env python3
"""Rebuild PAL/German metadata from the retail ELFs and curated US ownership.

Run after extracting the original files. This does not mark any source matching.
DTK reads native PAL symbols; ELF file/section/function anchors translate the US
split fixes, including common BSS and Bink pool ownership. Unknown or conflicting
boundaries fail closed. No address table or guessed global regional offset is used.
"""
from __future__ import annotations

import argparse
from bisect import bisect_left
from collections import Counter, defaultdict
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile

REGIONS = {
    "GQPP78": ("sys/main.dol", "6da9022f06bfb62a203017ec38046ba2566dc0cf"),
    "GU4Y78": ("files/Game.dol", "aeda497067bb53e4715db5b074535645a0ea9b3e"),
}
SPLIT = re.compile(r"(\s+)(\S+)(\s+)start:0x([\dA-Fa-f]+) end:0x([\dA-Fa-f]+)(.*)")
SYMBOL = re.compile(r"(.+?) = ([^:]+):0x([\dA-Fa-f]+);(.*)")
SIZE = re.compile(r"size:0x([\dA-Fa-f]+)")
# OS.c declares its first BSS object, DriveInfo, ALIGN(32) for DVD DMA.
# The linked ELF loses input sh_addralign; preceding retail tail padding used
# to hide this requirement when all objects were extracted from the DOL.
INPUT_SECTION_ALIGNMENT = {("dolphin/src/os/OS.c", ".bss"): 32}


class Elf:
    """The small ELF32 big-endian subset needed from the shipped retail ELFs."""

    def __init__(self, path: Path):
        self.data = path.read_bytes()
        if self.data[:6] != b"\x7fELF\x01\x02":
            raise ValueError(f"Expected big-endian ELF32: {path}")
        off = struct.unpack_from(">I", self.data, 32)[0]
        stride, count, names_index = struct.unpack_from(">HHH", self.data, 46)
        headers = [struct.unpack_from(">10I", self.data, off + i * stride) for i in range(count)]
        names = self.data[headers[names_index][4]:][:headers[names_index][5]]
        self.sections = []
        for h in headers:
            name = self.string(names, h[0])
            self.sections.append(dict(name=name, type=h[1], flags=h[2], address=h[3],
                                      offset=h[4], size=h[5], link=h[6], stride=h[9]))
        self.by_name = {s["name"]: s for s in self.sections}
        self.symbols = {}
        self.named = defaultdict(list)
        self.anchors = {}
        owner = ""
        occurrences = Counter()
        table = self.by_name[".symtab"]
        strings = self.body(self.sections[table["link"]])
        for pos in range(table["offset"], table["offset"] + table["size"], table["stride"]):
            name, address, size, info, _, section = struct.unpack_from(">IIIBBH", self.data, pos)
            name = self.string(strings, name)
            kind, binding = info & 15, info >> 4
            if kind == 4:  # STT_FILE
                owner = name
                continue
            if section >= len(self.sections) or not self.sections[section]["flags"] & 2:
                continue
            sec = self.sections[section]["name"]
            if kind == 3:  # STT_SECTION, original input section boundary
                base = ("section", owner, sec)
                key = (*base, occurrences[base])
                occurrences[base] += 1
            elif kind in (1, 2):
                key = (kind, owner if binding == 0 else "", sec, name)
            else:
                continue
            value = (sec, address, size)
            self.symbols[key] = value
            if kind in (1, 2):
                self.named[(sec, address, name)].append(key)
            if kind == 3 or not name.startswith("@"):
                self.anchors[key] = value

    @staticmethod
    def string(data: bytes, offset: int) -> str:
        return data[offset:data.index(0, offset)].decode("utf-8", errors="replace")

    def body(self, section: dict) -> bytes:
        return self.data[section["offset"]:][:section["size"]]

    def at(self, section: str, start: int, end: int) -> bytes:
        s = self.by_name[section]
        if s["type"] == 8 or not s["address"] <= start <= end <= s["address"] + s["size"]:
            return b""
        return self.body(s)[start - s["address"]:end - s["address"]]


class Mapping:
    def __init__(self, source: Elf, target: Elf):
        self.source, self.target = source, target
        self.points = defaultdict(lambda: defaultdict(set))
        for key, (sec, old, size) in source.anchors.items():
            if key not in target.anchors:
                continue
            _, new, new_size = target.anchors[key]
            if old and new:
                self.points[sec][old].add(new)
                if size and new_size:
                    self.points[sec][old + size].add(new + new_size)
        for sec, s in source.by_name.items():
            if not s["flags"] & 2:
                continue
            t = target.by_name[sec]
            self.points[sec][s["address"]].add(t["address"])
            self.points[sec][s["address"] + s["size"]].add(t["address"] + t["size"])
        self.keys = {sec: sorted(points) for sec, points in self.points.items()}
        self.counts = Counter()
        self.intervals = []

    def address(self, section: str, address: int) -> int:
        points = self.points[section]
        values = points.get(address)
        if values and len(values) == 1:
            self.counts["exact"] += 1
            return next(iter(values))
        if values:
            raise ValueError(f"Conflicting ELF anchors: {section}:{address:#x}")
        keys = self.keys[section]
        index = bisect_left(keys, address)
        if 0 < index < len(keys):
            left, right = keys[index-1:index+1]
            ls, rs = points[left], points[right]
            if len(ls) == len(rs) == 1:
                delta = next(iter(ls)) - left
                if next(iter(rs)) - right == delta:
                    self.counts["bounded"] += 1
                    return address + delta
        raise ValueError(f"No proven ELF mapping: {section}:{address:#x}")

    def splits(self, text: str) -> str:
        result = []
        owner = ""
        for line in text.splitlines():
            if line and not line[0].isspace() and line.endswith(":"):
                owner = line[:-1]
            match = SPLIT.fullmatch(line)
            if match:
                indent, sec, space, lo, hi, suffix = match.groups()
                lo, hi = int(lo, 16), int(hi, 16)
                new_lo, new_hi = self.address(sec, lo), self.address(sec, hi)
                if new_hi < new_lo:
                    raise ValueError(f"Reversed regional interval: {line}")
                self.intervals.append((sec, lo, hi, new_lo, new_hi))
                alignment = INPUT_SECTION_ALIGNMENT.get((owner, sec))
                if alignment:
                    if new_lo % alignment:
                        raise ValueError(f"Retail address violates declared alignment: {line}")
                    suffix = re.sub(r" align:\d+", "", suffix) + f" align:{alignment}"
                line = f"{indent}{sec}{space}start:0x{new_lo:08X} end:0x{new_hi:08X}{suffix}"
            result.append(line)
        return "\n".join(result) + "\n"

    def symbol(self, name: str, sec: str, start: int, size: int | None):
        # Named ELF identity handles changed-size regional functions and pools.
        choices = []
        for key in self.source.named.get((sec, start, name), []):
            if key in self.target.symbols:
                _, address, original_size = self.source.symbols[key]
                _, new, target_size = self.target.symbols[key]
                if size is None or size == original_size:
                    choices.append((new, target_size if size is not None else None))
        if choices and len(set(choices)) == 1:
            return choices[0], "identity"
        # Curated names inside an unchanged input range (not present in the ELF).
        if size is not None:
            values = set()
            for s, lo, hi, new_lo, new_hi in self.intervals:
                if s != sec or not lo <= start < start + size <= hi:
                    continue
                new = new_lo + start - lo
                if new + size > new_hi:
                    continue
                old_bytes = self.source.at(sec, lo, start + size)
                if old_bytes and old_bytes == self.target.at(sec, new_lo, new + size):
                    values.add((new, size))
            if len(values) == 1:
                return next(iter(values)), "unchanged_bytes"
        try:
            new = self.address(sec, start)
            end = self.address(sec, start + size) if size is not None else None
            return (new, end-new if end is not None else None), "anchors"
        except ValueError:
            return None, "native_only"



def restore_regional_sections(splits: str, target: Elf) -> str:
    """Restore input sections added in PAL that USA ownership cannot project."""
    # iDraw's PAL-only display-offset function adds a constant pool between
    # iCollide and iFMV. Projecting USA's iCollide endpoint swallowed that pool.
    donor, owner, section = "SB/Core/gc/iCollide.cpp", "SB/Core/gc/iDraw.cpp", ".sdata2"
    key = ("section", Path(owner).name, section, 0)
    start = target.symbols[key][1]
    lines = splits.splitlines()
    current = ""
    end = None
    for index, line in enumerate(lines):
        if line and not line[0].isspace() and line.endswith(":"):
            current = line[:-1]
        match = SPLIT.fullmatch(line)
        if current != donor or not match or match[2] != section:
            continue
        indent, sec, space, lo, hi, suffix = match.groups()
        if not int(lo, 16) < start < int(hi, 16):
            raise ValueError("PAL iDraw pool is not inside the projected iCollide interval")
        end = int(hi, 16)
        following = min(value[1] for anchor, value in target.symbols.items()
                        if anchor[0] == "section" and value[0] == section and value[1] > start)
        if end != following:
            raise ValueError("PAL iDraw pool endpoint disagrees with the next ELF input section")
        lines[index] = f"{indent}{sec}{space}start:0x{int(lo, 16):08X} end:0x{start:08X}{suffix}"
    if end is None or f"{owner}:" not in lines:
        raise ValueError("Missing projected iCollide or iDraw ownership")
    insert = lines.index(f"{owner}:") + 1
    while insert < len(lines) and lines[insert].strip():
        insert += 1
    lines.insert(insert, f"\t{section:<12}start:0x{start:08X} end:0x{end:08X}")
    return "\n".join(lines) + "\n"


def normalize(dtk: Path, original: Path, splits: str, symbols: str):
    """Use DTK's actual relocation/data analysis, without touching project files."""
    with tempfile.TemporaryDirectory(prefix="bfbb-pal-normalize-") as temp:
        scratch = Path(temp)
        split_path, symbol_path = scratch / "splits.txt", scratch / "symbols.txt"
        split_path.write_text(splits, newline="\n")
        symbol_path.write_text(symbols, newline="\n")
        # Keep VFS object names relative and all paths on the scratch drive.
        # In particular, Windows TEMP and --orig-root may use different drives.
        (scratch / "original.dol").write_bytes(original.read_bytes())
        config = scratch / "config.yml"
        config.write_text(
            "name: main\nobject: original.dol\n"
            "symbols: symbols.txt\nsplits: splits.txt\n"
            "symbols_known: true\nfill_gaps: false\n", newline="\n")
        subprocess.run([str(dtk), "dol", "split", str(config), str(scratch / "out")],
                       cwd=scratch, check=True, capture_output=True, text=True)
        if split_path.read_text() != splits:
            raise ValueError("DTK changed translated split ownership during normalization")
        return symbol_path.read_text()


def generate(args):
    root = args.root.resolve()
    orig = args.orig_root.resolve() if args.orig_root else root / "orig"
    output = args.output_root.resolve() if args.output_root else root / "config"
    source = Elf(orig / "GQPE78/files/sbgcM.elf")
    if hashlib.sha1(source.data).hexdigest() != "9243acc83d8d0d99e7583bdba9d0766dd063273d":
        raise ValueError("Unexpected US retail ELF")
    target = Elf(orig / "GQPP78/files/sbpeM.elf")
    if hashlib.sha1(target.data).hexdigest() != "9741e0d932a0ce441753843d659ed5371fbdc584":
        raise ValueError("Unexpected PAL retail ELF")
    mapping = Mapping(source, target)
    splits = mapping.splits((root / "config/GQPE78/splits.txt").read_text())
    splits = restore_regional_sections(splits, target)
    split_counts = dict(mapping.counts)
    with tempfile.TemporaryDirectory(prefix="bfbb-pal-config-") as scratch:
        subprocess.run([str(args.dtk.resolve()), "elf", "config", str(orig / "GQPP78/files/sbpeM.elf"), scratch], check=True)
        native = Path(scratch, "symbols.txt").read_text()
    entries = {}
    for line in native.splitlines():
        match = SYMBOL.fullmatch(line)
        if match:
            name, sec, address, attrs = match.groups()
            entries[(sec, int(address, 16), name)] = line
    carried, skipped = Counter(), []
    native_keys = defaultdict(list)
    for key in entries:
        native_keys[key[:2]].append(key)
    replaced_addresses = set()
    for line in (root / "config/GQPE78/symbols.txt").read_text().splitlines():
        match = SYMBOL.fullmatch(line)
        if not match:
            continue
        name, sec, raw, attrs = match.groups()
        size_match = SIZE.search(attrs)
        size = int(size_match[1], 16) if size_match else None
        value, method = mapping.symbol(name, sec, int(raw, 16), size)
        carried[method] += 1
        if value is None:
            skipped.append((name, sec, raw))
            continue
        address, new_size = value
        if new_size is not None:
            attrs = SIZE.sub(f"size:0x{new_size:X}", attrs)
        # Replace native metadata as a complete address group, retaining all
        # curated aliases at that address (functions and their RAS labels too).
        if (sec, address) not in replaced_addresses:
            for key in native_keys[(sec, address)]:
                del entries[key]
            replaced_addresses.add((sec, address))
        entries[(sec, address, name)] = f"{name} = {sec}:0x{address:08X};{attrs}"
    section_order = {s["name"]: i for i, s in enumerate(target.sections)}
    symbols = "\n".join(entries[key] for key in sorted(entries, key=lambda k: (section_order[k[0]], k[1]))) + "\n"
    symbols = normalize(args.dtk.resolve(), orig / "GQPP78/sys/main.dol", splits, symbols)
    summary = dict(split_boundaries=split_counts, curated_symbols=dict(carried), native_only=skipped)
    for version, (object_path, digest) in REGIONS.items():
        if hashlib.sha1((orig / version / object_path).read_bytes()).hexdigest() != digest:
            raise ValueError(f"Unexpected retail executable: {version}")
    for version, (object_path, digest) in REGIONS.items():
        directory = output / version
        directory.mkdir(parents=True, exist_ok=True)
        (directory / "splits.txt").write_text(splits, newline="\n")
        (directory / "symbols.txt").write_text(symbols, newline="\n")
        (directory / "ldscript.tpl").write_bytes((root / "config/GQPE78/ldscript.tpl").read_bytes())
        (directory / "config.yml").write_text(
            f"# Generated by tools/regional_config.py from retail ELF ownership.\n"
            f"name: main\nobject: orig/{version}/{object_path}\nhash: {digest}\n"
            f"symbols: config/{version}/symbols.txt\nsplits: config/{version}/splits.txt\n"
            f"symbols_known: true\nfill_gaps: false\nldscript_template: config/{version}/ldscript.tpl\n",
            newline="\n")
        (directory / "build.sha1").write_text(f"{digest}  build/{version}/main.dol\n", newline="\n")
    print(json.dumps(summary, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument("--orig-root", type=Path)
    parser.add_argument("--output-root", type=Path)
    parser.add_argument("--dtk", type=Path, default=Path("build/tools/dtk.exe" if os.name == "nt" else "build/tools/dtk"))
    generate(parser.parse_args())


if __name__ == "__main__":
    main()
