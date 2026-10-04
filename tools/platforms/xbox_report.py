"""Prepare a target-only, explicitly partial Xbox objdiff inventory.

This is not a reconstructed source build or a function-matching baseline.
No function symbols are invented for the unsplit executable. objdiff therefore
reports zero discovered functions/code bytes, while retaining the .text section
record and measuring the two included initialized-data sections. Its empty-code
percentages default to100%; these diagnostics must not be published as matching
progress until real function boundaries and a meaningful denominator exist.
"""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct

from .xbox import inspect_xbe


# XBE flags cannot distinguish code from data in this title: even .rdata and
# .data are executable. These PE-origin names establish the limited scope.
_SECTION_KINDS = {
    ".text": ("code", 0x60000020),
    ".rdata": ("initialized_data", 0x40000040),
    ".data": ("initialized_data", 0xC0000040),
}


def _target_coff(data: bytes, sections: list[dict]) -> bytes:
    """Wrap authenticated raw sections in i386 COFF, without inventing symbols.

    COFF offsets are zero-based within each section. Original virtual addresses
    and virtual zero-fill tails remain in coverage metadata. This object is for
    section inventory only; it has no recovered relocations and is not linkable
    as a replacement for the original XBE.
    """
    position = 20 + len(sections) * 40
    headers = []
    payloads = []
    for section in sections:
        name = section["name"]
        raw = data[section["raw_offset"]:section["raw_offset"] + section["raw_size"]]
        if len(raw) != section["raw_size"]:
            raise ValueError(f"Truncated XBE section {name}")
        headers.append(struct.pack(
            "<8sIIIIIIHHI", name.encode("ascii").ljust(8, b"\0"),
            0, 0, len(raw), position, 0, 0, 0, 0, _SECTION_KINDS[name][1],
        ))
        payloads.append(raw)
        position += len(raw)
    # IMAGE_FILE_MACHINE_I386, zero timestamp, no symbol/relocation tables.
    header = struct.pack("<HHIIIHH", 0x014C, len(sections), 0, 0, 0, 0, 0)
    return header + b"".join(headers) + b"".join(payloads)


def _write_registries(metadata: dict, output_dir: Path) -> None:
    identity = {"schema_version": 1, "executable_sha1": metadata["sha1"]}
    symbols = {**identity, "status": "confirmed", "coverage_complete": False,
               "provenance": "XBE supplies no original function symbol table; analyzer candidates are separate",
               "symbols": []}
    splits = {**identity, "status": "section_only", "symbol_file": "symbols.json",
              "translation_unit_ownership_recovered": False, "relocations_recovered": False,
              "retail_relink_verified": False, "source_build_available": False,
              "publish_matching_report": False,
              "notes": ["Authenticated executable sections, not translation-unit splits.",
                        "Analyzer candidates are in symbol-candidates.json and are not confirmed symbols.",
                        "Mixed SDK sections and embedded assets require further classification."],
              "sections": [{**section, "classification": _SECTION_KINDS.get(section["name"], ("unclassified", 0))[0]}
                           for section in metadata["sections"]]}
    output_dir.mkdir(parents=True, exist_ok=True)
    for name, value in (("symbols.json", symbols), ("splits.json", splits)):
        (output_dir / name).write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8", newline="\n")


def generate_registries(executable: Path, output_dir: Path) -> None:
    """Generate only binary-backed section metadata and the empty confirmed symbol registry."""
    _write_registries(inspect_xbe(Path(executable)), Path(output_dir))


def verify_registries(generated_dir: Path, committed_dir: Path) -> None:
    """Verify deterministic registries and candidate body hashes, without rerunning Ghidra.

    Byte validation authenticates what was analyzed; it does not promote
    heuristic function boundaries to original symbols or source matches.
    """
    generated_dir, committed_dir = Path(generated_dir), Path(committed_dir)
    for filename in ("symbols.json", "splits.json"):
        actual = json.loads((generated_dir / filename).read_text(encoding="utf-8"))
        expected = json.loads((committed_dir / filename).read_text(encoding="utf-8"))
        if actual != expected:
            raise ValueError(f"Generated Xbox registry differs from {committed_dir / filename}")
    candidates_path = committed_dir / "symbol-candidates.json"
    if not candidates_path.exists():
        return
    candidates = json.loads(candidates_path.read_text(encoding="utf-8"))
    splits = json.loads((generated_dir / "splits.json").read_text(encoding="utf-8"))
    if candidates["executable_sha1"] != splits["executable_sha1"] or candidates["status"] != "analysis_candidates":
        raise ValueError("Xbox candidate identity or status mismatch")
    section = next(s for s in splits["sections"] if s["name"] == ".text")
    obj = (generated_dir / "target.obj").read_bytes()
    if len(obj) < 20 or struct.unpack_from("<H", obj)[0] != 0x14C:
        raise ValueError("Invalid candidate-verification target COFF")
    text = None
    for index in range(struct.unpack_from("<H", obj, 2)[0]):
        fields = struct.unpack_from("<8sIIIIIIHHI", obj, 20 + index * 40)
        if fields[0].rstrip(b"\0") == b".text":
            text = obj[fields[4]:fields[4] + fields[3]]
    if text is None or len(text) != section["raw_size"] or hashlib.sha1(text).hexdigest() != section["sha1"]:
        raise ValueError("Candidate-verification .text bytes differ from original")
    start = section["virtual_address"]
    end = start + len(text)
    intervals = []
    entries = set()
    for function in candidates["functions"]:
        entry = function["entry"]
        if entry in entries or not start <= entry < end:
            raise ValueError("Duplicate or out-of-bounds candidate entry")
        entries.add(entry)
        digest = hashlib.sha1()
        size = 0
        previous_end = start
        contains_entry = False
        for low, high in function["ranges"]:
            if not start <= low < high <= end or low < previous_end:
                raise ValueError("Invalid candidate function body range")
            contains_entry |= low <= entry < high
            digest.update(text[low - start:high - start])
            size += high - low
            previous_end = high
            intervals.append((low, high))
        if not contains_entry or size != function["body_bytes"] or size != function["instruction_bytes"]:
            raise ValueError("Candidate function body accounting mismatch")
        if digest.hexdigest() != function["body_sha1"]:
            raise ValueError(f"Candidate body hash differs at {entry:#x}")
    intervals.sort()
    if any(right[0] < left[1] for left, right in zip(intervals, intervals[1:])):
        raise ValueError("Overlapping candidate function bodies")
    # Uploadable metadata only. Candidate bodies never enter matching measures.
    (generated_dir / "symbol-candidates.json").write_text(
        candidates_path.read_text(encoding="utf-8"), encoding="utf-8", newline="\n")


def prepare_report(executable: Path, output_dir: Path) -> dict:
    """Write target.obj, objdiff.json and coverage.json; return coverage metadata.

    Outputs contain original executable bytes and must remain in an ignored
    private build directory. There are no base/source objects or completion
    claims. The caller runs objdiff report generation against this directory.
    """
    executable, output_dir = Path(executable), Path(output_dir)
    metadata = inspect_xbe(executable)
    data = executable.read_bytes()
    if hashlib.sha1(data).hexdigest() != metadata["sha1"]:
        raise ValueError("XBE changed during report preparation")
    sections = metadata["sections"]
    included = [section for section in sections if section["name"] in _SECTION_KINDS]
    names = [section["name"] for section in included]
    if len(names) != len(_SECTION_KINDS) or set(names) != set(_SECTION_KINDS):
        raise ValueError("Expected exactly one .text, .rdata and .data XBE section")

    target = _target_coff(data, included)
    coverage = {
        "schema_version": 1,
        "status": "target_only_unsplit",
        "report_ready": False,
        "format": "XBE",
        "architecture": "i386",
        "executable_sha1": metadata["sha1"],
        "executable_size": metadata["size"],
        "target_object_sha1": hashlib.sha1(target).hexdigest(),
        "source_build_available": False,
        "source_comparison_available": False,
        "function_boundaries_recovered": False,
        "relocations_recovered": False,
        "retail_relink_verified": False,
        "coverage_complete": False,
        "publish_matching_report": False,
        "scope": ".text plus initialized .rdata/.data section bytes only",
        "included_code_section_bytes": sum(
            section["raw_size"] for section in included if section["name"] == ".text"
        ),
        "included_initialized_data_bytes": sum(
            section["raw_size"] for section in included if section["name"] != ".text"
        ),
        "included_sections": [
            {**section, "classification": _SECTION_KINDS[section["name"]][0],
             "unmeasured_virtual_tail_bytes": max(0, section["virtual_size"] - section["raw_size"])}
            for section in included
        ],
        "unclassified_sections": [
            {**section, "reason": "Requires explicit code/data or asset classification"}
            for section in sections if section["name"] not in _SECTION_KINDS
        ],
        "report_limitations": [
            "No source objects have been compiled or compared.",
            "No function symbols are synthesized for the unsplit .text section.",
            "objdiff total_code and total_functions remain zero until function boundaries are supplied.",
            "objdiff defaults zero-denominator code/function percentages to100%; this is not measured progress.",
            "Do not publish this diagnostic report as a matching-progress baseline.",
            "Initialized data bytes are inventoried; virtual zero-fill tails are not measured.",
            "Mixed SDK sections and embedded assets are excluded from matching measures.",
            "This COFF is an inventory container, not a relocation-restored link input.",
        ],
    }
    config = {
        "$schema": "https://raw.githubusercontent.com/encounter/objdiff/main/config.schema.json",
        "build_target": False,
        "build_base": False,
        "units": [{
            "name": "main/xbox_unsplit",
            "target_path": "target.obj",
            "metadata": {"complete": False},
        }],
    }
    output_dir.mkdir(parents=True, exist_ok=True)
    _write_registries(metadata, output_dir)
    (output_dir / "target.obj").write_bytes(target)
    (output_dir / "objdiff.json").write_text(json.dumps(config, indent=2) + "\n", encoding="utf-8")
    (output_dir / "coverage.json").write_text(json.dumps(coverage, indent=2) + "\n", encoding="utf-8")
    return coverage