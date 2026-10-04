#!/usr/bin/env python3
"""Inventory exact France function-byte candidates without claiming boundaries.

Run from the repository root:
    python tools/platforms/france_candidates.py
    python tools/platforms/france_candidates.py --check

Only complete DWARF1 function ranges of at least 32 bytes are considered. A
candidate must occur exactly once in France's loaded image, be instruction
aligned, and have consistent source/name ownership across reference versions.
Overlapping candidates are excluded. Exact bytes can still be a suffix of a
larger function or embedded data: this file is NOT a recovered symbol table,
source match, or progress denominator. No executable bytes are exported.
"""
from __future__ import annotations

import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT / "tools") not in sys.path:
    sys.path.insert(0, str(ROOT / "tools"))

from platforms.ps2 import inspect_elf
from platforms.ps2_report import _function_ranges, _source_name

TARGET = "SLES-53623"
REFERENCES = ("SLUS-20680", "SLES-51968", "SLES-51970")
MINIMUM_SIZE = 32


def _original(version: str, versions: dict, orig_dir: Path) -> tuple[bytes, dict]:
    record = versions[version]
    if record["platform"] != "ps2":
        raise ValueError(f"{version}: expected PS2 executable")
    path = orig_dir / version / Path(record["executable"]["path"]).name
    data = path.read_bytes()
    expected = record["executable"]["sha1"]
    if hashlib.sha1(data).hexdigest() != expected:
        raise ValueError(f"{version}: original executable SHA-1 does not match manifest")
    metadata = inspect_elf(path)
    if metadata["sha1"] != expected:
        raise ValueError(f"{version}: executable changed during inspection")
    return data, metadata


def _loaded(metadata: dict) -> list[dict]:
    segments = sorted((s for s in metadata["segments"]
                       if s["type"] == 1 and s["file_size"]),
                      key=lambda s: s["address"])
    for previous, current in zip(segments, segments[1:]):
        if current["address"] < previous["address"] + previous["file_size"]:
            raise ValueError("Overlapping loaded segments require explicit resolution")
    return segments


def generate(manifest: Path, orig_dir: Path) -> dict:
    versions = json.loads(manifest.read_text(encoding="utf-8"))["versions"]
    target, target_metadata = _original(TARGET, versions, orig_dir)
    target_segments = _loaded(target_metadata)
    target_spans = [(segment, target[segment["offset"]:
                                   segment["offset"] + segment["file_size"]])
                    for segment in target_segments]
    found = defaultdict(list)
    hashes = {}
    reference_metadata = []
    for version in REFERENCES:
        data, metadata = _original(version, versions, orig_dir)
        loaded = _loaded(metadata)
        sections = [s for s in metadata["sections"] if s["name"] == ".debug" and s["size"]]
        if len(sections) != 1:
            raise ValueError(f"{version}: expected exactly one DWARF1 debug section")
        section = sections[0]
        functions = _function_ranges(data[section["offset"]:section["offset"] + section["size"]])
        if not functions:
            raise ValueError(f"{version}: no debug-backed function ranges")
        counts = {"below_minimum_size": 0, "no_exact_match": 0,
                  "ambiguous_or_unaligned": 0, "unique_exact_match": 0}
        for function in functions:
            size = function["high"] - function["low"]
            if size < MINIMUM_SIZE:
                counts["below_minimum_size"] += 1
                continue
            owners = [s for s in loaded if s["address"] <= function["low"]
                      and function["high"] <= s["address"] + s["file_size"]]
            if len(owners) != 1:
                raise ValueError(f"{version}: function has no unique file-backed segment")
            owner = owners[0]
            offset = owner["offset"] + function["low"] - owner["address"]
            needle = data[offset:offset + size]
            matches = []
            for segment, span in target_spans:
                start = span.find(needle)
                if start < 0:
                    continue
                matches.append(segment["address"] + start)
                # A second occurrence anywhere invalidates uniqueness, including
                # unaligned occurrences and occurrences in other loaded segments.
                if span.find(needle, start + 1) >= 0:
                    matches.append(None)
            if not matches:
                counts["no_exact_match"] += 1
                continue
            if len(matches) != 1 or matches[0] % 4:
                counts["ambiguous_or_unaligned"] += 1
                continue
            address = matches[0]
            key = (address, address + size)
            found[key].append({"version": version,
                               "source_address": f"0x{function['low']:08x}",
                               "name": function["name"],
                               "source": _source_name(function["source"])})
            hashes[key] = hashlib.sha256(needle).hexdigest()
            counts["unique_exact_match"] += 1
        reference_metadata.append({"version": version, "executable_sha1": metadata["sha1"],
                                   "debug_function_count": len(functions), "search_counts": counts})

    overlapping = set()
    ordered = sorted(found)
    for index, first in enumerate(ordered):
        for second in ordered[index + 1:]:
            if second[0] >= first[1]:
                break
            overlapping.update((first, second))
    candidates = []
    conflicting = 0
    for span in ordered:
        references = found[span]
        names = {(reference["source"], reference["name"]) for reference in references}
        if len(names) != 1:
            conflicting += 1
        if span in overlapping or len(names) != 1:
            continue
        source, name = next(iter(names))
        candidates.append({"name": name, "source": source,
                           "french_address": f"0x{span[0]:08x}",
                           "size": span[1] - span[0], "exact_bytes_sha256": hashes[span],
                           "boundary_confirmation": False,
                           "provenance": sorted(references, key=lambda r: (r["version"], r["source_address"]))})
    return {
        "schema_version": 1, "version": TARGET,
        "executable_sha1": target_metadata["sha1"],
        "status": "unconfirmed-exact-byte-candidates",
        "boundary_confirmation": False, "eligible_for_progress": False,
        "method": {"name": "unique-full-dwarf1-function-bytes", "minimum_size": MINIMUM_SIZE,
                   "instruction_alignment": 4, "search_scope": "all file-backed loaded segments",
                   "reject_overlaps": True, "reject_conflicting_names_or_sources": True},
        "limitations": [
            "Exact bytes alone do not establish France function boundaries or code ownership.",
            "A match may be part of a larger function or embedded data; boundary corroboration is required.",
            "Candidate names are inherited from reference DWARF1, not recovered France symbols.",
            "These candidates must not enter symbols.json or matching-progress denominators without confirmation.",
            "No source comparison, relocation recovery, or retail relink is claimed.",
        ],
        "references": reference_metadata,
        "candidate_count": len(candidates),
        "candidate_byte_count": sum(candidate["size"] for candidate in candidates),
        "excluded_overlapping_ranges": len(overlapping),
        "excluded_conflicting_ranges": conflicting,
        "candidates": candidates,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=ROOT / "config/platforms/versions.json")
    parser.add_argument("--orig-dir", type=Path, default=ROOT / "orig")
    parser.add_argument("--output", type=Path,
                        default=ROOT / "config/platforms/SLES-53623/symbol-candidates.json")
    parser.add_argument("--check", action="store_true", help="Regenerate in memory and require identical committed metadata")
    args = parser.parse_args()
    try:
        document = generate(args.manifest, args.orig_dir)
        encoded = (json.dumps(document, indent=2, ensure_ascii=False) + "\n").encode("utf-8")
        if args.check:
            if json.loads(args.output.read_text(encoding="utf-8")) != document:
                raise ValueError(f"{args.output}: candidate metadata differs; regenerate explicitly")
        else:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            temporary = None
            try:
                with tempfile.NamedTemporaryFile(dir=args.output.parent, delete=False) as stream:
                    temporary = Path(stream.name)
                    stream.write(encoded)
                temporary.replace(args.output)
            finally:
                if temporary is not None:
                    temporary.unlink(missing_ok=True)
        action = "Verified" if args.check else "Wrote"
        print(f"{action} {document['candidate_count']} unconfirmed France candidates; not counted as progress")
    except (OSError, ValueError, KeyError) as error:
        parser.exit(1, f"error: {error}\n")


if __name__ == "__main__":
    main()
