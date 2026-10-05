"""Prepare partial PS2 objdiff targets from retail DWARF1 and reviewed extents.

Only explicit DWARF or independently corroborated function ranges enter code totals. Unclassified
load-image bytes are recorded separately, never guessed to be code or data.
Selected shared-source units restore independently verified relocations; other
objects retain resolved retail instructions. Whole-executable linking is pending.
"""

from __future__ import annotations

from collections import defaultdict
import json
import hashlib
from pathlib import Path
import struct

from .ps2 import inspect_elf
from .dwarf1 import iter_dies
from .ps2_anchors import write_anchors


def _function_ranges(debug: bytes) -> list[dict]:
    """Return only explicit, non-overlapping DWARF1 function extents."""
    functions = []
    for offset, tag, owner, attrs in iter_dies(debug):
        if tag in (0x06, 0x14) and 17 in attrs and 18 in attrs:
            low, high = attrs[17], attrs[18]
            if not isinstance(low, int) or not isinstance(high, int) or high < low:
                raise ValueError("Invalid DWARF1 function bounds")
            if high == low:
                continue
            if not owner or not isinstance(attrs.get(3), str):
                raise ValueError("DWARF1 function lacks source or name ownership")
            if low % 4 or high % 4:
                raise ValueError("Unaligned MIPS function range")
            functions.append({"source": owner, "name": attrs[3], "low": low, "high": high})
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
        # Match the compiler's code-section name even for a one-function subset.
        # objdiff also uses section names when pairing named functions.
        sections.append({"name": ".text", "type": 1,
                         "flags": 6, "address": 0,
                         "data": function["bytes"], "align": 4,
                         "link": 0, "info": 0, "entsize": 0})
        name_offset = len(strings)
        # DWARF1 gives human names, not necessarily unique linker names. Include
        # the proven address so overloads/weak copies cannot be falsely deduped.
        name = function.get("linkage_name", f"{function['name']}@{function['low']:08x}").encode("utf-8")
        strings.extend(name + b"\0")
        symbols.extend(struct.pack("<IIIBBH", name_offset, 0,
                                   len(function["bytes"]), 0x12, 0, index))
    external_symbols = {}
    relocation_sections = []
    for section_index, function in enumerate(functions, 1):
        relocations = bytearray()
        for relocation in function.get("relocations", []):
            name = relocation["symbol"]
            if name not in external_symbols:
                external_symbols[name] = len(symbols) // 16
                name_offset = len(strings)
                strings.extend(name.encode("utf-8") + b"\0")
                symbols.extend(struct.pack("<IIIBBH", name_offset, 0, 0, 0x10, 0, 0))
            relocations.extend(struct.pack("<II", relocation["offset"],
                                           (external_symbols[name] << 8) | relocation.get("type", 4)))
        if relocations:
            relocation_sections.append(len(sections))
            sections.append({"name": f".rel.text.{function['low']:08x}", "type": 9,
                             "flags": 0, "address": 0, "data": bytes(relocations),
                             "align": 4, "link": 0, "info": section_index, "entsize": 8})
    sym_index = len(sections)
    for index in relocation_sections:
        sections[index]["link"] = sym_index
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



def _write_registries(metadata: dict, functions: list[dict], output_dir: Path,
                      provenance: str = "retail DWARF1 function ranges",
                      data_extents: list[dict] | None = None) -> None:
    """Export symbol names and exhaustive file-backed load splits, without bytes."""
    symbols = [{"name": function["name"], "source": _source_name(function["source"]),
                "address": function["low"], "size": function["high"] - function["low"],
                **({"provenance": function["provenance"]} if "provenance" in function else {})}
               for function in functions]
    segments = sorted((segment for segment in metadata["segments"]
                       if segment["type"] == 1 and segment["file_size"]),
                      key=lambda segment: segment["address"])
    data_ranges = {(e['address'], e['size']) for e in (data_extents or [])
                   if e['storage'] == 'file_backed'}
    classified = sorted([{'address': s['address'], 'size': s['size'], 'kind': 'function'}
                         for s in symbols] +
                        [{'address': a, 'size': size, 'kind': 'data'} for a, size in data_ranges],
                        key=lambda item: item['address'])
    ranges = []
    assigned = 0
    previous_end = None
    for segment_index, segment in enumerate(segments):
        begin, end = segment["address"], segment["address"] + segment["file_size"]
        if previous_end is not None and begin < previous_end:
            raise ValueError("Overlapping load segments cannot produce exhaustive splits")
        previous_end = end
        cursor = begin
        owned = [symbol for symbol in classified if begin <= symbol["address"] < end]
        for symbol in owned:
            address, size = symbol["address"], symbol["size"]
            if size <= 0 or address < cursor or address + size > end:
                raise ValueError("Classified splits overlap or exceed their load segment")
            if cursor < address:
                ranges.append({"segment": segment_index, "kind": "unclassified",
                               "address": cursor, "size": address - cursor,
                               "file_offset": segment["offset"] + cursor - begin})
            ranges.append({"segment": segment_index, "kind": symbol["kind"],
                           "address": address, "size": size,
                           "file_offset": segment["offset"] + address - begin})
            cursor = address + size
            assigned += 1
        if cursor < end:
            ranges.append({"segment": segment_index, "kind": "unclassified",
                           "address": cursor, "size": end - cursor,
                           "file_offset": segment["offset"] + cursor - begin})
    if assigned != len(classified):
        raise ValueError("A classified extent lacks a file-backed load segment")
    loaded_bytes = sum(segment["file_size"] for segment in segments)
    code_bytes = sum(symbol["size"] for symbol in symbols)
    data_bytes = sum(size for _, size in data_ranges)
    if sum(split["size"] for split in ranges) != loaded_bytes:
        raise ValueError("Splits do not exhaust file-backed load ranges")
    identity = {"schema_version": 1, "executable_sha1": metadata["sha1"]}
    symbol_registry = {**identity, "provenance": (provenance if symbols else
                                                  "no DWARF1 function ranges available"),
                       "coverage_complete": False, "symbols": symbols}
    split_registry = {
        **identity,
        "scope": "All file-backed PT_LOAD bytes; zero-fill memory and non-loadable ELF metadata excluded",
        "code_classification_complete": False,
        "loaded_file_bytes": loaded_bytes, "known_code_bytes": code_bytes,
        "known_data_bytes": data_bytes,
        "unclassified_loaded_bytes": loaded_bytes - code_bytes - data_bytes,
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
    for filename in ("symbols.json", "splits.json", "address-anchors.json", "data-extents.json"):
        actual = json.loads((Path(generated_dir) / filename).read_text(encoding="utf-8"))
        expected = json.loads((Path(committed_dir) / filename).read_text(encoding="utf-8"))
        if actual != expected:
            raise ValueError(f"Generated PS2 registry differs from {Path(committed_dir) / filename}")


def _merge_corroborated(functions: list[dict], registry: dict, expected: dict,
                       metadata: dict, binary: bytes, loaded: list[dict], kind: str) -> int:
    """Merge regenerated extents through shared identity, byte and overlap checks."""
    if registry != expected:
        raise ValueError("Corroborated registry differs from regenerated original evidence")
    if registry["executable_sha1"] != metadata["sha1"] or registry.get("coverage_complete") is not False:
        raise ValueError("Corroborated registry identity or partial scope differs")
    count = 0
    for entry in registry["functions"]:
        low, size = entry["address"], entry["size"]
        spans = [segment for segment in loaded if segment["address"] <= low and
                 low + size <= segment["address"] + segment["file_size"]]
        if (entry.get("boundary_confirmation") is not True or size <= 0 or low % 4 or size % 4 or
                entry.get("confirmation_kind") != kind or len(spans) != 1):
            raise ValueError("Invalid corroborated function bounds")
        offset = spans[0]["offset"] + low - spans[0]["address"]
        if hashlib.sha256(binary[offset:offset + size]).hexdigest() != entry["sha256"]:
            raise ValueError("Corroborated function bytes differ from the input original")
        overlaps = [function for function in functions
                    if low < function["high"] and function["low"] < low + size]
        if overlaps:
            if (len(overlaps) != 1 or overlaps[0]["low"] != low or overlaps[0]["high"] != low + size or
                    overlaps[0]["name"] != entry["name"] or
                    _source_name(overlaps[0]["source"]) != _source_name(entry["source"])):
                raise ValueError("Corroborated and existing function extents conflict")
            continue
        functions.append({"name": entry["name"], "source": entry["source"],
                          "low": low, "high": low + size, "provenance": kind})
        count += 1
    return count


def prepare_report(executable: Path, output_dir: Path, reviewed_functions: Path | None = None,
                   reviewed_call_targets: Path | None = None,
                   reviewed_data_anchors: Path | None = None,
                   corroborated_functions: Path | None = None,
                   relocation_corroborated_functions: Path | None = None,
                   tu_corroborated_functions: Path | None = None) -> dict:
    """Write genuine target objects/config, or pending metadata for stripped ELF.

    The returned coverage is partial, even for debug-bearing executables. Call
    objdiff report generate -p <output_dir> only when report_ready is true.
    Optional machine-corroborated bounds are fully regenerated against all four
    originals alongside the supplied manual registry before any are merged.
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
    binary = executable.read_bytes()
    if len(debug_sections) > 1:
        raise ValueError("Multiple DWARF1 debug sections are unsupported")
    functions = []
    if debug_sections:
        debug = debug_sections[0]
        functions = _function_ranges(binary[debug["offset"]:debug["offset"] + debug["size"]])
        if not functions:
            raise ValueError("DWARF1 contains no usable function ranges")
    reviewed_count = 0
    if reviewed_functions is not None:
        reviewed = json.loads(Path(reviewed_functions).read_text(encoding="utf-8"))
        if reviewed["executable_sha1"] != metadata["sha1"]:
            raise ValueError("Reviewed function registry targets a different executable")
        for entry in reviewed["functions"]:
            low, size = entry["address"], entry["size"]
            spans = [s for s in loaded if s["address"] <= low and
                     low + size <= s["address"] + s["file_size"]]
            if (not entry.get("boundary_confirmation") or size <= 0 or
                    low % 4 or size % 4 or len(spans) != 1):
                raise ValueError("Invalid reviewed function bounds")
            offset = spans[0]["offset"] + low - spans[0]["address"]
            if hashlib.sha256(binary[offset:offset + size]).hexdigest() != entry["sha256"]:
                raise ValueError("Reviewed function bytes differ from the original")
            functions.append({"name": entry["name"], "source": entry["source"],
                              "low": low, "high": low + size, "provenance": "manual-reviewed"})
            reviewed_count += 1
    corroborated_count = relocation_corroborated_count = tu_corroborated_count = 0
    checked_registries = []
    manifest = Path(__file__).resolve().parents[2] / "config/platforms/versions.json"
    orig_dir = executable.resolve().parent.parent
    if corroborated_functions is not None:
        if reviewed_functions is None:
            raise ValueError("Machine-corroborated bounds require their manual deduplication registry")
        from .france_corroborated import generate
        registry_path = Path(corroborated_functions)
        registry = json.loads(registry_path.read_text(encoding="utf-8"))
        regenerated = generate(manifest, orig_dir, registry_path.with_name("symbol-candidates.json"),
                               Path(reviewed_functions))
        corroborated_count = _merge_corroborated(functions, registry, regenerated, metadata, binary,
                                                loaded, "machine-corroborated-static-cfg")
        checked_registries.append(("corroborated-functions.json", regenerated))
    if relocation_corroborated_functions is not None:
        if reviewed_functions is None or corroborated_functions is None:
            raise ValueError("Relocation corroboration requires both starting extent registries")
        from .france_relocations import generate
        registry_path = Path(relocation_corroborated_functions)
        if (Path(reviewed_functions).resolve() != registry_path.with_name("reviewed-functions.json").resolve() or
                Path(corroborated_functions).resolve() != registry_path.with_name("corroborated-functions.json").resolve()):
            raise ValueError("Relocation corroboration must use its verified starting registries")
        registry = json.loads(registry_path.read_text(encoding="utf-8"))
        regenerated = generate(manifest, orig_dir, registry_path.parent)
        relocation_corroborated_count = _merge_corroborated(
            functions, registry, regenerated, metadata, binary, loaded, "machine-corroborated-explicit-transfers")
        checked_registries.append(("relocation-corroborated-functions.json", regenerated))
    if tu_corroborated_functions is not None:
        from .france_tu_sequences import generate, KIND
        registry_path = Path(tu_corroborated_functions)
        registry = json.loads(registry_path.read_text(encoding="utf-8"))
        regenerated = generate(manifest, orig_dir, registry_path.parent)
        tu_corroborated_count = _merge_corroborated(
            functions, registry, regenerated, metadata, binary, loaded, KIND)
        checked_registries.append(("tu-corroborated-functions.json", regenerated))
    functions.sort(key=lambda f: f["low"])
    if any(right["low"] < left["high"] for left, right in zip(functions, functions[1:])):
        raise ValueError("Reviewed, corroborated and debug function extents overlap")
    for filename, registry in checked_registries:
        (output_dir / filename).write_text(json.dumps(registry, indent=2) + "\n", encoding="utf-8", newline="\n")
    provenance = "retail DWARF1 function ranges"
    category_id, category_name = "debug_functions", "Debug-backed functions (partial coverage)"
    if reviewed_count:
        provenance = "Independently reviewed function ranges; see reviewed-functions.json"
        if debug_sections:
            provenance = "Retail DWARF1 plus independently reviewed function ranges"
        category_id, category_name = "known_functions", "Known function bounds (partial coverage)"
        coverage["scope"] = "Known function ranges only; not whole-executable progress"
        coverage["reviewed_function_count"] = reviewed_count
    if corroborated_count:
        provenance = "Manual-reviewed and machine-corroborated function ranges; see reviewed-functions.json and corroborated-functions.json"
        if debug_sections:
            provenance = "Retail DWARF1 plus manual-reviewed and machine-corroborated function ranges"
        category_id, category_name = "known_functions", "Known function bounds (partial coverage)"
        coverage["scope"] = "Known function ranges only; not whole-executable progress"
        coverage["corroborated_function_count"] = corroborated_count
    if relocation_corroborated_count:
        provenance += "; explicit-transfer corroboration: relocation-corroborated-functions.json"
        coverage["relocation_corroborated_function_count"] = relocation_corroborated_count
    if tu_corroborated_count:
        provenance += "; whole-TU sequence corroboration: tu-corroborated-functions.json"
        coverage["tu_corroborated_function_count"] = tu_corroborated_count
    data_registry = write_anchors(binary, metadata, functions, output_dir / 'address-anchors.json')
    _write_registries(metadata, functions, output_dir, provenance, data_registry['extents'])
    data_bytes = data_registry['counts']['file_backed_bytes']
    coverage['known_data_bytes'] = data_bytes
    coverage['unclassified_loaded_bytes'] = loaded_bytes - data_bytes
    if not functions:
        for name in ("objdiff.json", "report.json"):
            (output_dir / name).unlink(missing_ok=True)
        (output_dir / "coverage.json").write_text(json.dumps(coverage, indent=2) + "\n", encoding="utf-8")
        return coverage
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
    from .ps2_source import prepare_functions
    address_anchors = json.loads((output_dir / 'address-anchors.json').read_text(encoding='utf-8'))['anchors']
    if reviewed_data_anchors is not None:
        reviewed_data = json.loads(Path(reviewed_data_anchors).read_text(encoding='utf-8'))
        if reviewed_data['executable_sha1'] != metadata['sha1']:
            raise ValueError('Reviewed data anchors identify another executable')
        for anchor in reviewed_data['anchors']:
            if (not anchor.get('identity_confirmation') or anchor['size'] <= 0 or
                    anchor['address'] % 4 or anchor['storage'] != 'runtime_zero_fill'):
                raise ValueError('Unsupported reviewed data anchor')
            proof = anchor['runtime_zero_fill']
            if not proof['lower_bound'] <= anchor['address'] < anchor['address'] + anchor['size'] <= proof['upper_bound']:
                raise ValueError('Reviewed data anchor exceeds runtime-cleared memory')
            spans = [s for s in loaded if s['address'] <= proof['address'] and
                     proof['address'] + proof['size'] <= s['address'] + s['file_size']]
            if len(spans) != 1:
                raise ValueError('Runtime clearing proof lacks a file-backed owner')
            offset = spans[0]['offset'] + proof['address'] - spans[0]['address']
            if hashlib.sha256(binary[offset:offset + proof['size']]).hexdigest() != proof['sha256']:
                raise ValueError('Runtime clearing proof bytes differ')
            # The separate proof command checks startup instructions, named
            # reference declarations and uses; this does not invent an ELF BSS.
            address_anchors.append({**anchor, 'kind': 'data_address'})
    restored = prepare_functions(functions, binary, loaded, metadata["sha1"], reviewed_call_targets,
                                 metadata=metadata, address_anchors=address_anchors)
    (output_dir / "relocations.json").write_text(json.dumps({
        "executable_sha1": metadata["sha1"], "relocations": restored,
        "scope": "Validated calls only; complete relocation recovery remains pending",
    }, indent=2) + "\n", encoding="utf-8")
    targets = output_dir / "target"
    targets.mkdir(exist_ok=True)
    units = []
    for index, (name, group) in enumerate(sorted(groups.items())):
        target = targets / f"unit-{index:04d}.o"
        target.write_bytes(_target_object(group, metadata["flags"]))
        units.append({"name": name, "target_path": target.relative_to(output_dir).as_posix(),
                      "metadata": {"complete": False, "auto_generated": False,
                                   "progress_categories": [category_id]}})
    config = {
        "$schema": "https://raw.githubusercontent.com/encounter/objdiff/main/config.schema.json",
        "build_base": False, "build_target": False, "units": units,
        "progress_categories": [{"id": category_id, "name": category_name}],
    }
    (output_dir / "objdiff.json").write_text(json.dumps(config, indent=2) + "\n", encoding="utf-8")
    code_bytes = sum(function["high"] - function["low"] for function in functions)
    coverage.update({"report_ready": True, "status": "partial-target-only",
                     "known_code_bytes": code_bytes,
                     "unclassified_loaded_bytes": loaded_bytes - code_bytes - data_bytes,
                     "function_count": len(functions), "unit_count": len(units)})
    (output_dir / "coverage.json").write_text(json.dumps(coverage, indent=2) + "\n", encoding="utf-8")
    return coverage
