#!/usr/bin/env python3
"""Recheck the reviewed Xbox function extents against both authenticated originals.

This read-only evidence check validates region payload equivalence, exact extent
hashes, recorded short branches/returns, saved-register instructions, direct-call
witnesses, caller hashes, and suffix-string arguments. It does not repeat Ghidra
analysis, infer names, prove source matching, or claim an executable relink.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from platforms.xbox import inspect_xbe

VERSIONS = ("XBOX-US", "XBOX-EU")
REGISTERS = {name: index for index, name in enumerate(("eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"))}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


class Original:
    def __init__(self, version: str, record: dict, orig_dir: Path):
        self.version = version
        require(record["platform"] == "xbox", f"{version}: not an Xbox original")
        path = orig_dir / version / Path(record["executable"]["path"]).name
        self.data = path.read_bytes()
        self.sha1 = hashlib.sha1(self.data).hexdigest()
        require(self.sha1 == record["executable"]["sha1"], f"{version}: original SHA-1 mismatch")
        self.metadata = inspect_xbe(path)
        require(self.metadata["sha1"] == self.sha1, f"{version}: original changed during inspection")

    def section(self, address: int, size: int) -> dict:
        require(isinstance(address, int) and isinstance(size, int) and size >= 0, "Invalid address/size")
        owners = [section for section in self.metadata["sections"] if
                  section["virtual_address"] <= address and address + size <=
                  section["virtual_address"] + min(section["raw_size"], section["virtual_size"])]
        require(len(owners) == 1, f"{self.version}: extent has no unique mapped file owner")
        return owners[0]

    def read(self, address: int, size: int) -> bytes:
        section = self.section(address, size)
        offset = section["raw_offset"] + address - section["virtual_address"]
        return self.data[offset:offset + size]

    def check_hash(self, record: dict) -> None:
        require(hashlib.sha256(self.read(record["address"], record["size"])).hexdigest() == record["sha256"],
                f"{self.version}: byte hash differs at {record['address']:#x}")

    def call(self, address: int, target: int) -> None:
        instruction = self.read(address, 5)
        require(instruction[0] == 0xe8 and address + 5 + struct.unpack("<i", instruction[1:])[0] == target,
                f"{self.version}: CALL rel32 at {address:#x} does not target {target:#x}")


def verify_functions(original: Original, document: dict, anchors: dict | None = None) -> tuple[int, int]:
    require(document["version"] == original.version and document["executable_sha1"] == original.sha1,
            "Reviewed metadata targets another executable")
    require(document["coverage_complete"] is False, "Partial reviewed coverage must remain explicit")
    functions = document["functions"]
    require(bool(functions) and len({f["canonical_identifier"] for f in functions}) == len(functions),
            "Canonical function identifiers are missing or duplicated")
    ordered = sorted(functions, key=lambda f: f["address"])
    require(all(a["address"] + a["size"] <= b["address"] for a, b in zip(ordered, ordered[1:])),
            "Reviewed function extents overlap")
    calls_checked = 0
    call_targets = {f['canonical_identifier']: f['address'] for f in functions
                    if f.get('boundary_confirmation') and f.get('identity_confirmation')}
    for function in functions:
        label = function["canonical_identifier"]
        start, size = function["address"], function["size"]
        end = start + size
        require(size > 0 and function["boundary_confirmation"] is True and function["identity_confirmation"] is True,
                f"{label}: invalid or unreviewed function")
        require(original.section(start, size)["name"] == ".text", f"{label}: not in .text")
        original.check_hash(function)
        if function.get('address_expressions'):
            from platforms.xbox_relocations import normalize
            normalize(original.read(start, size), function['address_expressions'], anchors or {})
        if function.get('direct_calls'):
            from platforms.xbox_calls import normalize_calls
            normalize_calls(original.read(start, size), start, function['direct_calls'], call_targets)
        evidence = function["corroboration"]
        if "callback_registration" in evidence:
            from platforms.xbox_particle_commands import verify_callback
            verify_callback(original, function)
        require(evidence["return_address"] == end - 1 and original.read(end - 1, 1) == b"\xc3",
                f"{label}: terminal RET boundary differs")
        for branch in evidence["branches"]:
            instruction = original.read(branch["address"], 2)
            opcode = instruction[0]
            target = branch["address"] + 2 + struct.unpack("<b", instruction[1:])[0]
            require(branch["size"] == 2 and opcode == branch["opcode"] and
                    (opcode == 0xeb or 0x70 <= opcode <= 0x7f) and target == branch["target"] and
                    start <= branch["address"] < end - 1 and start <= target < end,
                    f"{label}: recorded short branch differs or leaves extent")
        for saved in evidence["saved_registers"]:
            register = REGISTERS[saved["register"]]
            require(start <= saved["push_address"] < saved["pop_address"] < end and
                    original.read(saved["push_address"], 1) == bytes([0x50 + register]) and
                    original.read(saved["pop_address"], 1) == bytes([0x58 + register]),
                    f"{label}: saved-register boundary evidence differs")
        for call in evidence["reviewed_call_witnesses"]:
            require(call["kind"] == "x86_call_rel32" and call["target"] == start, f"{label}: invalid call witness")
            original.call(call["address"], start)
            calls_checked += 1
        for padding in evidence.get("internal_alignment", []):
            require(start <= padding["address"] and padding["address"] + padding["size"] <= end,
                    f"{label}: internal alignment lies outside extent")
            original.check_hash(padding)
            require(any(b["opcode"] == 0xeb and b["address"] + 2 == padding["address"] and
                        b["target"] == padding["address"] + padding["size"] for b in evidence["branches"]),
                    f"{label}: internal alignment is not skipped by its recorded jump")
        for alignment in evidence.get("executed_alignment", []):
            original.check_hash(alignment)
            instruction = original.read(alignment["address"], alignment["size"])
            require(start <= alignment["address"] and alignment["address"] + alignment["size"] <= end and
                    len(instruction) == 7 and instruction[:3] == b"\x8d\xa4\x24" and
                    struct.unpack("<i", instruction[3:])[0] == 0, f"{label}: executed LEA ESP alignment differs")
        previous = evidence.get("previous_boundary")
        if previous:
            require(original.read(previous["return_address"], 1) == b"\xc3" and
                    previous["padding_address"] == previous["return_address"] + 1 and
                    previous["padding_address"] + previous["padding_size"] == start and
                    original.read(previous["padding_address"], previous["padding_size"]) ==
                    bytes([previous["padding_byte"]]) * previous["padding_size"], f"{label}: preceding boundary differs")
        padding = evidence.get("following_padding")
        if padding:
            require(padding["address"] == end and padding["address"] + padding["size"] == evidence["next_function_address"] and
                    original.read(end, padding["size"]) == bytes([padding["byte"]]) * padding["size"],
                    f"{label}: following alignment differs")
        context = evidence.get("caller_context")
        if context:
            original.check_hash(context)
            for string in context.get("string_witnesses", []):
                encoded = string["value"].encode("ascii") + b"\0"
                require(original.read(string["address"], len(encoded)) == encoded, f"{label}: caller suffix string differs")
                original.call(string["call_site"], start)
                # These reviewed callers load ECX=string, then EAX=prefix before CALL.
                load = original.read(string["call_site"] - 7, 5)
                require(string["argument_register"] == "ecx" and load[0] == 0xb9 and
                        struct.unpack("<I", load[1:])[0] == string["address"], f"{label}: suffix argument load differs")
    return len(functions), calls_checked


def verify(manifest: Path, orig_dir: Path, registry_root: Path) -> dict:
    versions = json.loads(manifest.read_text(encoding="utf-8"))["versions"]
    originals = {version: Original(version, versions[version], orig_dir) for version in VERSIONS}
    left, right = (originals[version] for version in VERSIONS)
    a, b = left.metadata["sections"], right.metadata["sections"]
    require(len(a) == len(b), "Xbox region section counts differ")
    for first, second in zip(a, b):
        fields = ("name", "flags", "virtual_address", "virtual_size", "raw_offset", "raw_size", "sha1")
        require(all(first[key] == second[key] for key in fields), "Xbox region section layouts/hashes differ")
        require(left.data[first["raw_offset"]:first["raw_offset"] + first["raw_size"]] ==
                right.data[second["raw_offset"]:second["raw_offset"] + second["raw_size"]],
                "Xbox region section payload bytes differ")
    documents = {}
    results = {}
    for version, original in originals.items():
        document = json.loads((registry_root / version / "reviewed-functions.json").read_text(encoding="utf-8"))
        proof = document["cross_version_proof"]
        peer = originals[proof["peer_version"]]
        require(peer.version != version and peer.sha1 == proof["peer_executable_sha1"] and proof["section_count"] == len(a),
                "Recorded cross-version identity differs")
        anchors_path = registry_root / version / 'reviewed-data-anchors.json'
        anchors = None
        if anchors_path.is_file():
            from platforms.xbox_relocations import verify_original_anchors
            anchors = verify_original_anchors(anchors_path, original)
        count, calls = verify_functions(original, document, anchors)
        results[version] = {"functions": count, "bytes": sum(f["size"] for f in document["functions"]), "call_witnesses": calls}
        documents[version] = document
    require(documents[VERSIONS[0]]["functions"] == documents[VERSIONS[1]]["functions"],
            "Identical Xbox payloads have different reviewed metadata")
    return results


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=ROOT / "config/platforms/versions.json")
    parser.add_argument("--orig-dir", type=Path, default=ROOT / "orig")
    parser.add_argument("--registry-root", type=Path, default=ROOT / "config/platforms")
    args = parser.parse_args()
    try:
        results = verify(args.manifest, args.orig_dir, args.registry_root)
        for version, result in results.items():
            print(f"{version}: verified {result['functions']} reviewed functions, {result['bytes']} extent bytes, "
                  f"{result['call_witnesses']} direct-call witnesses; no source-match or relink claim.")
    except (OSError, ValueError, KeyError, TypeError, struct.error) as error:
        parser.exit(1, f"error: {error}\n")


if __name__ == "__main__":
    main()
