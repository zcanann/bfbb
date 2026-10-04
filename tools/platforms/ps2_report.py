"""Prepare a partial, target-only PS2 objdiff baseline from retail DWARF1.

Only explicit function low_pc/high_pc ranges enter code totals. Unclassified
load-image bytes are recorded separately, never guessed to be code or data.
These objects retain resolved retail instructions; source comparison/linking
will require a separate relocation-recovery and compiler integration step.
"""

from __future__ import annotations

from collections import defaultdict
import json
from pathlib import Path
import struct

from .ps2 import inspect_elf


def _function_ranges(debug: bytes) -> list[dict]:
    """Read the DWARF1 forms present in MW PS2 debug info using the stdlib."""
    offset = 0
    owner = None
    functions = []
    while offset < len(debug):
        if len(debug) - offset < 4:
            raise ValueError("Truncated DWARF1 record length")
        size = struct.unpack_from("<I", debug, offset)[0]
        if size < 4 or size > len(debug) - offset:
            raise ValueError("Invalid DWARF1 record length")
        end = offset + size
        if size < 8:
            offset = end
            continue
        tag = struct.unpack_from("<H", debug, offset + 4)[0]
        pos = offset + 6
        attrs = {}
        if tag == 0:
            offset = end
            continue
        while pos < end:
            if end - pos < 2:
                raise ValueError("Truncated DWARF1 attribute")
            attr = struct.unpack_from("<H", debug, pos)[0]
            pos += 2
            form, key = attr & 15, attr >> 4
            if form in (1, 2, 5, 6, 7):
                width = {1: 4, 2: 4, 5: 2, 6: 4, 7: 8}[form]
                if width > end - pos:
                    raise ValueError("Truncated DWARF1 scalar")
                value = int.from_bytes(debug[pos:pos + width], "little")
                pos += width
            elif form in (3, 4):
                width = 2 if form == 3 else 4
                if width > end - pos:
                    raise ValueError("Truncated DWARF1 block length")
                length = int.from_bytes(debug[pos:pos + width], "little")
                pos += width
                if length > end - pos:
                    raise ValueError("DWARF1 block exceeds record")
                value = None
                pos += length
            elif form == 8:
                terminator = debug.find(b"\0", pos, end)
                if terminator == -1:
                    raise ValueError("Unterminated DWARF1 string")
                value = debug[pos:terminator].decode("utf-8", errors="replace")
                pos = terminator + 1
            else:
                raise ValueError(f"Unsupported DWARF1 form {form}")
            if key in (3, 17, 18):  # name, low_pc, high_pc
                attrs[key] = value
        if tag == 0x11:  # compile_unit; MW emits multiple ranges per source file
            owner = attrs.get(3)
        elif tag in (0x06, 0x14) and 17 in attrs and 18 in attrs:
            low, high = attrs[17], attrs[18]
            if not isinstance(low, int) or not isinstance(high, int) or high < low:
                raise ValueError("Invalid DWARF1 function bounds")
            if high == low:
                offset = end
                continue
            if not owner or not isinstance(attrs.get(3), str):
                raise ValueError("DWARF1 function lacks source or name ownership")
            if low % 4 or high % 4:
                raise ValueError("Unaligned MIPS function range")
            functions.append({"source": owner, "name": attrs[3], "low": low, "high": high})
        offset = end
    functions.sort(key=lambda function: (function["low"], function["high"]))
    for previous, current in zip(functions, functions[1:]):
        if current["low"] < previous["high"]:
            raise ValueError("Overlapping DWARF1 functions require explicit ownership resolution")
    return functions


def _target_object(functions: list[dict], elf_flags: int) -> bytes:
    """Write ELF32 ET_REL with one text section per proven function range."""
    sections = [{"name": "", "type": 0, "flags": 0, "address": 0,
                 "data": b"", "align": 0, "link": 0, "info": 0, "entsize": 0}]
    symbols = bytearray(16)
    strings = bytearray(b"\0")
    for function in functions:
        index = len(sections)
        sections.append({"name": f".text.{function['low']:08x}", "type": 1,
                         "flags": 6, "address": 0,
                         "data": function["bytes"], "align": 4,
                         "link": 0, "info": 0, "entsize": 0})
        name_offset = len(strings)
        # DWARF1 gives human names, not necessarily unique linker names. Include
        # the proven address so overloads/weak copies cannot be falsely deduped.
        name = f"{function['name']}@{function['low']:08x}".encode("utf-8")
        strings.extend(name + b"\0")
        symbols.extend(struct.pack("<IIIBBH", name_offset, 0,
                                   len(function["bytes"]), 0x12, 0, index))
    sym_index = len(sections)
    sections.extend([
        {"name": ".symtab", "type": 2, "flags": 0, "address": 0,
         "data": bytes(symbols), "align": 4, "link": sym_index + 1,
         "info": 1, "entsize": 16},
        {"name": ".strtab", "type": 3, "flags": 0, "address": 0,
         "data": bytes(strings), "align": 1, "link": 0, "info": 0, "entsize": 0},
        {"name": ".shstrtab", "type": 3, "flags": 0, "address": 0,
         "data": b"", "align": 1, "link": 0, "info": 0, "entsize": 0},
    ])
    names = bytearray(b"\0")
    for section in sections:
        section["name_offset"] = len(names) if section["name"] else 0
        if section["name"]:
            names.extend(section["name"].encode("ascii") + b"\0")
    sections[-1]["data"] = bytes(names)
    output = bytearray(52)
    for section in sections:
        alignment = max(1, section["align"])
        output.extend(b"\0" * (-len(output) % alignment))
        section["offset"] = len(output) if section["type"] else 0
        output.extend(section["data"])
    output.extend(b"\0" * (-len(output) % 4))
    shoff = len(output)
    for section in sections:
        output.extend(struct.pack("<IIIIIIIIII", section["name_offset"], section["type"],
                                  section["flags"], section["address"], section["offset"],
                                  len(section["data"]), section["link"], section["info"],
                                  section["align"], section["entsize"]))
    output[:52] = struct.pack("<16sHHIIIIIHHHHHH", b"\x7fELF\x01\x01\x01" + b"\0" * 9,
                              1, 8, 1, 0, 0, shoff, elf_flags, 52, 0, 0,
                              40, len(sections), len(sections) - 1)
    return bytes(output)


def _source_name(path: str) -> str:
    path = path.replace("\\", "/")
    if len(path) > 2 and path[1:3] == ":/":
        path = path[3:]
    return path.lstrip("/")



def _write_registries(metadata: dict, functions: list[dict], output_dir: Path) -> None:
    """Export symbol names and exhaustive file-backed load splits, without bytes."""
    symbols = [{"name": function["name"], "source": _source_name(function["source"]),
                "address": function["low"], "size": function["high"] - function["low"]}
               for function in functions]
    segments = sorted((segment for segment in metadata["segments"]
                       if segment["type"] == 1 and segment["file_size"]),
                      key=lambda segment: segment["address"])
    ranges = []
    assigned = 0
    previous_end = None
    for segment_index, segment in enumerate(segments):
        begin, end = segment["address"], segment["address"] + segment["file_size"]
        if previous_end is not None and begin < previous_end:
            raise ValueError("Overlapping load segments cannot produce exhaustive splits")
        previous_end = end
        cursor = begin
        owned = [symbol for symbol in symbols if begin <= symbol["address"] < end]
        for symbol in owned:
            address, size = symbol["address"], symbol["size"]
            if size <= 0 or address < cursor or address + size > end:
                raise ValueError("Function splits overlap or exceed their load segment")
            if cursor < address:
                ranges.append({"segment": segment_index, "kind": "unclassified",
                               "address": cursor, "size": address - cursor,
                               "file_offset": segment["offset"] + cursor - begin})
            ranges.append({"segment": segment_index, "kind": "function",
                           "address": address, "size": size,
                           "file_offset": segment["offset"] + address - begin})
            cursor = address + size
            assigned += 1
        if cursor < end:
            ranges.append({"segment": segment_index, "kind": "unclassified",
                           "address": cursor, "size": end - cursor,
                           "file_offset": segment["offset"] + cursor - begin})
    if assigned != len(symbols):
        raise ValueError("A function lacks a file-backed load segment")
    loaded_bytes = sum(segment["file_size"] for segment in segments)
    code_bytes = sum(symbol["size"] for symbol in symbols)
    if sum(split["size"] for split in ranges) != loaded_bytes:
        raise ValueError("Splits do not exhaust file-backed load ranges")
    identity = {"schema_version": 1, "executable_sha1": metadata["sha1"]}
    symbol_registry = {**identity, "provenance": ("retail DWARF1 function ranges" if symbols else
                                                  "no DWARF1 function ranges available"),
                       "coverage_complete": False, "symbols": symbols}
    split_registry = {
        **identity,
        "scope": "All file-backed PT_LOAD bytes; zero-fill memory and non-loadable ELF metadata excluded",
        "code_classification_complete": False,
        "loaded_file_bytes": loaded_bytes, "known_code_bytes": code_bytes,
        "unclassified_loaded_bytes": loaded_bytes - code_bytes,
        "segments": [{"address": segment["address"], "file_offset": segment["offset"],
                      "file_size": segment["file_size"], "memory_size": segment["memory_size"],
                      "flags": segment["flags"]} for segment in segments],
        "ranges": ranges,
    }
    for filename, registry in (("symbols.json", symbol_registry), ("splits.json", split_registry)):
        (output_dir / filename).write_text(json.dumps(registry, indent=2) + "\n",
                                           encoding="utf-8", newline="\n")


def verify_registries(generated_dir: Path, committed_dir: Path) -> None:
    """Fail when freshly extracted symbol/split metadata differs from the registry."""
    for filename in ("symbols.json", "splits.json"):
        actual = json.loads((Path(generated_dir) / filename).read_text(encoding="utf-8"))
        expected = json.loads((Path(committed_dir) / filename).read_text(encoding="utf-8"))
        if actual != expected:
            raise ValueError(f"Generated PS2 registry differs from {Path(committed_dir) / filename}")


def prepare_report(executable: Path, output_dir: Path) -> dict:
    """Write genuine target objects/config, or pending metadata for stripped ELF.

    The returned coverage is partial, even for debug-bearing executables. Call
    objdiff report generate -p <output_dir> only when report_ready is true.
    """
    executable, output_dir = Path(executable), Path(output_dir)
    metadata = inspect_elf(executable)
    loaded = [s for s in metadata["segments"] if s["type"] == 1 and s["file_size"]]
    intervals = sorted((s["address"], s["address"] + s["file_size"]) for s in loaded)
    if any(second[0] < first[1] for first, second in zip(intervals, intervals[1:])):
        raise ValueError("Overlapping ELF load segments require explicit ownership resolution")
    loaded_bytes = sum(end - start for start, end in intervals)
    debug_sections = [s for s in metadata["sections"] if s["name"] == ".debug" and s["size"]]
    coverage = {
        "schema_version": 1, "executable_sha1": metadata["sha1"],
        "report_ready": False, "coverage_complete": False,
        "scope": "DWARF1 function ranges only; not whole-executable progress",
        "status": "pending-symbol-recovery", "loaded_file_bytes": loaded_bytes,
        "known_code_bytes": 0, "unclassified_loaded_bytes": loaded_bytes,
        "function_count": 0, "unit_count": 0,
        "source_compilation_available": False, "source_link_verified": False,
    }
    output_dir.mkdir(parents=True, exist_ok=True)
    if not debug_sections:
        _write_registries(metadata, [], output_dir)
        # Avoid reusing an old report/config if this output directory is switched
        # from a debug-bearing input to a stripped executable.
        for name in ("objdiff.json", "report.json"):
            (output_dir / name).unlink(missing_ok=True)
        (output_dir / "coverage.json").write_text(json.dumps(coverage, indent=2) + "\n", encoding="utf-8")
        return coverage
    if len(debug_sections) != 1:
        raise ValueError("Multiple DWARF1 debug sections are unsupported")
    binary = executable.read_bytes()
    debug = debug_sections[0]
    functions = _function_ranges(binary[debug["offset"]:debug["offset"] + debug["size"]])
    if not functions:
        raise ValueError("DWARF1 contains no usable function ranges")
    _write_registries(metadata, functions, output_dir)
    groups = defaultdict(list)
    for function in functions:
        spans = [s for s in loaded if s["address"] <= function["low"] and
                 function["high"] <= s["address"] + s["file_size"]]
        if len(spans) != 1:
            raise ValueError("DWARF1 function does not belong to exactly one file-backed segment")
        segment = spans[0]
        offset = segment["offset"] + function["low"] - segment["address"]
        function["bytes"] = binary[offset:offset + function["high"] - function["low"]]
        groups[_source_name(function["source"])].append(function)
    targets = output_dir / "target"
    targets.mkdir(exist_ok=True)
    units = []
    for index, (name, group) in enumerate(sorted(groups.items())):
        target = targets / f"unit-{index:04d}.o"
        target.write_bytes(_target_object(group, metadata["flags"]))
        units.append({"name": name, "target_path": target.relative_to(output_dir).as_posix(),
                      "metadata": {"complete": False, "auto_generated": False,
                                   "progress_categories": ["debug_functions"]}})
    config = {
        "$schema": "https://raw.githubusercontent.com/encounter/objdiff/main/config.schema.json",
        "build_base": False, "build_target": False, "units": units,
        "progress_categories": [{"id": "debug_functions", "name": "Debug-backed functions (partial coverage)"}],
    }
    (output_dir / "objdiff.json").write_text(json.dumps(config, indent=2) + "\n", encoding="utf-8")
    code_bytes = sum(function["high"] - function["low"] for function in functions)
    coverage.update({"report_ready": True, "status": "partial-target-only",
                     "known_code_bytes": code_bytes,
                     "unclassified_loaded_bytes": loaded_bytes - code_bytes,
                     "function_count": len(functions), "unit_count": len(units)})
    (output_dir / "coverage.json").write_text(json.dumps(coverage, indent=2) + "\n", encoding="utf-8")
    return coverage
