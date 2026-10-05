#!/usr/bin/env python3
"""Authenticate BFBB PS2 coarse load layout without inventing function bounds.

The executable has one merged RWX load, not surviving .text/.data sections.
Original DWARF linker/VU anchors, a complete DMA RET + VIF MPG packet walk,
startup BSS initialization, and all known code/data anchors corroborate the
recovered regions. France uniquely contains the complete identical VU block.

Packet formats: https://ps2dev.github.io/ps2sdk/dma__tags_8h.html
https://ps2dev.github.io/ps2sdk/vif__codes_8h.html
No microcode bytes or executable payloads are exported by this tool.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
if __package__ in (None, ""):
    sys.path.insert(0, str(ROOT / "tools"))
from platforms.ps2 import inspect_elf
from platforms.dwarf1 import iter_dies
from platforms.ps2_report import _function_ranges

VERSIONS = ("SLUS-20680", "SLES-51968", "SLES-51970", "SLES-53623")
REFERENCES = VERSIONS[:3]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


class Original:
    def __init__(self, version: str, executable: Path, manifest: dict):
        self.version = version
        self.binary = executable.read_bytes()
        self.metadata = inspect_elf(executable)
        expected = manifest["versions"][version]["executable"]["sha1"]
        require(self.metadata["sha1"] == expected, f"{version}: original SHA1 mismatch")
        loads = [s for s in self.metadata["segments"] if s["type"] == 1 and s["file_size"]]
        require(len(loads) == 1, f"{version}: expected one file-backed load")
        self.load = loads[0]
        require(self.load["address"] == 0x100000, f"{version}: unexpected load address")
        self.start = self.load["address"]
        self.end = self.start + self.load["file_size"]
        self.debug = next((s for s in self.metadata["sections"] if s["name"] == ".debug"), None)
        self.functions = []
        self.named = {}
        if self.debug:
            debug = self.binary[self.debug["offset"]:self.debug["offset"] + self.debug["size"]]
            self.functions = _function_ranges(debug)
            for offset, tag, owner, attrs in iter_dies(debug):
                name, location = attrs.get(3), attrs.get(2)
                if tag not in (7, 12) or name not in ("FXanimUVPRS", "FXgooPRS", "__data_start", "_end"):
                    continue
                if not isinstance(location, bytes) or len(location) != 5 or location[0] != 3:
                    continue
                address = struct.unpack_from("<I", location, 1)[0]
                previous = self.named.get(name)
                require(previous is None or previous["address"] == address, f"{version}: inconsistent {name}")
                self.named[name] = {"name": name, "address": address, "die_offset": offset, "declaration_source": owner}

    def read(self, address: int, size: int) -> bytes:
        require(self.start <= address <= address + size <= self.end, f"{self.version}: read outside load")
        offset = self.load["offset"] + address - self.start
        return self.binary[offset:offset + size]

    def word(self, address: int) -> int:
        return struct.unpack("<I", self.read(address, 4))[0]


def parse_vu_packets(original: Original, start: int, end: int) -> dict:
    """Consume every byte using DMA QWC and VIF MPG counts, not next symbols."""
    require(start % 16 == end % 16 == 0 and start < end, "Invalid VU block alignment")
    packets = []
    pc = start
    while pc < end:
        tag, address, _, _ = struct.unpack("<IIII", original.read(pc, 16))
        require(tag >> 16 == 0x6000 and address == 0, f"{original.version}: expected DMA RET at {pc:#x}")
        count = tag & 0xffff
        packet_end = pc + (count + 1) * 16
        require(count > 0 and packet_end <= end, "DMA packet escapes VU region")
        position, destination, commands, nop_words = pc + 8, 0, 0, 0
        while position < packet_end:
            command = original.word(position)
            position += 4
            if command == 0:
                nop_words += 1
                continue
            require(command >> 24 == 0x4a, f"Unexpected VIF command at {position - 4:#x}")
            instructions = (command >> 16) & 255 or 256
            require((command & 0xffff) == destination, "Noncontiguous VIF microprogram upload")
            require(position + instructions * 8 <= packet_end, "MPG payload escapes DMA packet")
            position += instructions * 8
            destination += instructions
            commands += 1
        require(position == packet_end and 0 < destination <= 2048, "Invalid VU microprogram extent")
        packets.append({"address": pc, "size": packet_end - pc, "microinstruction_count": destination,
                        "mpg_commands": commands, "nop_words": nop_words})
        pc = packet_end
    require(pc == end and len(packets) == 41, "Unexpected VU block extent or packet count")
    return {"packet_count": len(packets), "microcode_payload_bytes": sum(p["microinstruction_count"] * 8 for p in packets),
            "storage_bytes": end - start, "sha256": hashlib.sha256(original.read(start, end - start)).hexdigest(),
            "packets": packets}


def startup_bss(original: Original) -> dict:
    def pair(high: int, low: int, register: int) -> int:
        hi, lo = original.word(high), original.word(low)
        require(hi >> 16 == (15 << 10) | register, "Unexpected startup LUI")
        require(lo >> 16 == (9 << 10) | (register << 5) | register, "Unexpected startup ADDIU")
        immediate = lo & 0xffff
        return ((hi & 0xffff) << 16) + (immediate - 0x10000 if immediate & 0x8000 else immediate)
    start = pair(0x10011c, 0x100124, 2)
    end = pair(0x100120, 0x100128, 3)
    # These are the original SQ-zero loop and its count/branch/delay operations.
    expected = {0x10012c: 0x7c400000, 0x100130: 0, 0x100134: 0x0043082b,
                0x100138: 0, 0x10013c: 0, 0x100140: 0x1420fffa, 0x100144: 0x24420010}
    for pc, word in expected.items():
        require(original.word(pc) == word, f"{original.version}: changed startup clear loop {pc:#x}")
    require(start == original.end and end > start and end % 16 == 0, "BSS does not follow file-backed load")
    markers = [s["address"] for s in original.metadata["sections"] if s["flags"] == 3 and s["size"] == 0]
    require(markers == [end], "Empty writable section does not corroborate _end")
    if "_end" in original.named:
        require(original.named["_end"]["address"] == end, "DWARF _end disagrees with startup")
    memory_end = original.start + original.load["memory_size"]
    require(memory_end in (original.end, end), "Unexpected ELF memory end")
    return {"address": start, "end": end, "startup_loop_address": 0x10012c,
            "elf_memory_end": memory_end, "empty_writable_section_address": markers[0],
            "loop_note": "The original loop also clears one 16-byte word at _end; that heap word is not counted as BSS."}


def check_anchors(original: Original, config_dir: Path, cpu_end: int, bss_end: int) -> dict:
    for function in original.functions:
        require(original.start <= function["low"] < function["high"] <= cpu_end, "DWARF code outside CPU prefix")
    counts = {"dwarf_functions": len(original.functions), "function_entries": 0, "data_anchors": 0, "data_extents": 0}
    directory = config_dir / original.version
    for filename, key in (("address-anchors.json", "anchors"), ("data-extents.json", "extents")):
        path = directory / filename
        if not path.exists():
            continue
        registry = json.loads(path.read_text())
        require(registry["executable_sha1"] == original.metadata["sha1"], "Stale anchor registry")
        for anchor in registry[key]:
            address = anchor["address"]
            if filename == "address-anchors.json" and anchor["kind"] == "function_entry":
                require(original.start <= address < cpu_end, "Function entry outside CPU text")
                counts["function_entries"] += 1
            elif original.start <= address < original.end:
                require(cpu_end <= address, "Known data overlaps CPU text prefix")
                require(address + anchor.get("size", 1) <= original.end, "File-backed data escapes load")
                counts["data_extents" if filename == "data-extents.json" else "data_anchors"] += 1
    established = set()
    for filename in ("reviewed-functions.json", "corroborated-functions.json", "relocation-corroborated-functions.json"):
        path = directory / filename
        if not path.exists():
            continue
        registry = json.loads(path.read_text())
        require(registry["executable_sha1"] == original.metadata["sha1"], "Stale function registry")
        for function in registry["functions"]:
            address, size = function["address"], function["size"]
            require(original.start <= address < address + size <= cpu_end, "Established function escapes CPU text")
            established.add((address, size))
    for filename, code in (("reviewed-call-targets.json", True), ("reviewed-data-anchors.json", False)):
        path = directory / filename
        if not path.exists():
            continue
        registry = json.loads(path.read_text())
        require(registry["executable_sha1"] == original.metadata["sha1"], "Stale reviewed anchor registry")
        for anchor in registry["anchors"]:
            address = anchor["address"]
            require(original.start <= address < cpu_end if code else cpu_end <= address < bss_end,
                    "Reviewed anchor contradicts region classification")
    return counts


def generate_layouts(orig_dir: Path, config_dir: Path) -> dict[str, dict]:
    manifest = json.loads((config_dir / "versions.json").read_text())
    originals = {version: Original(version, orig_dir / version / "boot.elf", manifest) for version in VERSIONS}
    reference = originals[REFERENCES[0]]
    require(all(name in reference.named for name in ("FXanimUVPRS", "__data_start")), "Reference DWARF boundaries missing")
    reference_start = reference.named["FXanimUVPRS"]["address"]
    reference_end = reference.named["__data_start"]["address"]
    block = reference.read(reference_start, reference_end - reference_start)
    result = {}
    for version, original in originals.items():
        if version in REFERENCES:
            require(all(name in original.named for name in ("FXanimUVPRS", "FXgooPRS", "__data_start")), "DWARF boundaries missing")
            start, end = original.named["FXanimUVPRS"]["address"], original.named["__data_start"]["address"]
            require(original.read(start, end - start) == block, "Cross-version VU block differs")
            boundary_evidence = {"kind": "original_DWARF_declarations", "anchors": [original.named[n] for n in ("FXanimUVPRS", "FXgooPRS", "__data_start")]}
            last_end = max(f["high"] for f in original.functions)
            require(0 <= start - last_end < 128 and not any(original.read(last_end, start - last_end)), "Unexpected CPU/VU boundary padding")
            boundary_evidence["last_dwarf_function_end"] = last_end
            boundary_evidence["zero_alignment_bytes"] = start - last_end
        else:
            load = original.read(original.start, original.end - original.start)
            offset = load.find(block)
            require(offset >= 0 and load.find(block, offset + 1) < 0, "France VU block is not a unique exact match")
            start, end = original.start + offset, original.start + offset + len(block)
            boundary_evidence = {"kind": "unique_complete_cross_version_VU_block", "references": [
                {"version": v, "executable_sha1": originals[v].metadata["sha1"],
                 "address": originals[v].named["FXanimUVPRS"]["address"]} for v in REFERENCES]}
        packets = parse_vu_packets(original, start, end)
        if "FXgooPRS" in original.named:
            require(original.named["FXgooPRS"]["address"] in {p["address"] for p in packets["packets"]}, "Named second VU program is not a packet boundary")
        bss = startup_bss(original)
        counts = check_anchors(original, config_dir, start, bss["end"])
        def region(name, address, size, classification):
            return {"name": name, "address": address, "size": size, "classification": classification,
                    "file_offset": None if classification == "bss" else original.load["offset"] + address - original.start}
        regions = [region("cpu_text", original.start, start - original.start, "code"),
                   region("vu_packets", start, end - start, "data"),
                   region("initialized_data", end, original.end - end, "data"),
                   region("runtime_bss", bss["address"], bss["end"] - bss["address"], "bss")]
        require(sum(r["size"] for r in regions[:3]) == original.load["file_size"], "Regions do not exhaust load")
        require(all(a["address"] + a["size"] == b["address"] for a, b in zip(regions, regions[1:])), "Regions overlap or leave holes")
        result[version] = {"schema_version": 1, "version": version, "executable_sha1": original.metadata["sha1"],
            "classification_complete": True, "original_section_names_preserved": False,
            "source_and_link_complete": False, "regions": regions,
            "evidence": {"boundaries": boundary_evidence, "vu_packets": packets, "runtime_bss": bss, "anchor_checks": counts},
            "limitations": ["Coarse regions recover link layout; the original ELF merged section names and permissions.",
                "CPU text includes alignment and possible inline literals or jump tables; function ownership remains partial.",
                "VU packets are host storage classified as data, not matched R5900 instructions or compiled microcode.",
                "Initialized data can contain further VU programs stored as data; data subsection boundaries remain unresolved.",
                "Region classification does not establish source matching, relocations, or a complete retail link."]}
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--orig-dir", type=Path, default=ROOT / "orig")
    parser.add_argument("--config-dir", type=Path, default=ROOT / "config/platforms")
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    layouts = generate_layouts(args.orig_dir, args.config_dir)
    output = args.output_dir or args.config_dir
    for version, layout in layouts.items():
        path = output / version / "region-layout.json"
        if args.check:
            require(path.exists() and json.loads(path.read_text()) == layout, f"{path}: registry differs")
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(json.dumps(layout, indent=2) + "\n", encoding="utf-8")
        print(f"{version}: " + ", ".join(f"{r['name']}={r['size']}" for r in layout["regions"]))


if __name__ == "__main__":
    main()
