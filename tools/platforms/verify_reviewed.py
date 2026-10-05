#!/usr/bin/env python3
"""Recheck France reviewed evidence against all four authenticated PS2 originals.

Run: python tools/platforms/verify_reviewed.py --orig-dir orig

This read-only proof command checks named DWARF1 extents, explicit relocation
exceptions, recorded direct calls, frame/return/padding evidence, and unique
serial-callee correspondences. It does not rerun the entry CFG or replace the
individual semantic/boundary review. It never generates progress or symbols.
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
from platforms.ps2 import inspect_elf
from platforms.ps2_report import _function_ranges, _source_name

TARGET = "SLES-53623"
REFERENCES = ("SLUS-20680", "SLES-51968", "SLES-51970")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def normalized(data: bytes) -> tuple[int, ...]:
    """Preserve every bit except actual MIPS J/JAL destination fields."""
    return tuple(word & 0xfc000000 if word >> 26 in (2, 3) else word
                 for (word,) in struct.iter_unpack("<I", data))


class Original:
    def __init__(self, version: str, record: dict, orig_dir: Path):
        self.version = version
        require(record["platform"] == "ps2", f"{version}: not a PS2 original")
        path = orig_dir / version / Path(record["executable"]["path"]).name
        self.data = path.read_bytes()
        self.sha1 = hashlib.sha1(self.data).hexdigest()
        require(self.sha1 == record["executable"]["sha1"], f"{version}: executable SHA-1 mismatch")
        self.metadata = inspect_elf(path)
        require(self.metadata["sha1"] == self.sha1, f"{version}: original changed during inspection")
        self.loaded = [s for s in self.metadata["segments"] if s["type"] == 1 and s["file_size"]]
        self.functions = []
        if version in REFERENCES:
            debug = [s for s in self.metadata["sections"] if s["name"] == ".debug" and s["size"]]
            require(len(debug) == 1, f"{version}: expected one DWARF1 debug section")
            section = debug[0]
            self.functions = _function_ranges(self.data[section["offset"]:section["offset"] + section["size"]])
            for function in self.functions:
                function["source"] = _source_name(function["source"])
            require(bool(self.functions), f"{version}: no DWARF1 functions")
        self.by_address = {f["low"]: f for f in self.functions}

    def read(self, address: int, size: int) -> bytes:
        require(isinstance(address, int) and isinstance(size, int) and size >= 0,
                f"{self.version}: invalid address/size")
        spans = [s for s in self.loaded if s["address"] <= address and
                 address + size <= s["address"] + s["file_size"]]
        require(len(spans) == 1, f"{self.version}: range {address:#x}+{size:#x} lacks unique loaded owner")
        offset = spans[0]["offset"] + address - spans[0]["address"]
        return self.data[offset:offset + size]

    def word(self, address: int) -> int:
        require(address % 4 == 0, f"{self.version}: unaligned instruction {address:#x}")
        return struct.unpack("<I", self.read(address, 4))[0]

    def transfer(self, address: int, target: int, opcode: int = 3) -> None:
        word = self.word(address)
        decoded = ((address + 4) & 0xf0000000) | ((word & 0x3ffffff) << 2)
        require(word >> 26 == opcode and decoded == target,
                f"{self.version}: direct transfer {address:#x} does not target {target:#x}")


def verified_body(entry: dict, original: Original) -> bytes:
    address, size = entry["address"], entry["size"]
    require(size > 0 and size % 4 == 0 and address % 4 == 0, f"{entry['name']}: invalid extent")
    body = original.read(address, size)
    require(sha256(body) == entry["sha256"], f"{entry['name']}: reviewed byte hash mismatch")
    return body


def reference_body(entry: dict, proof: dict, originals: dict) -> tuple[Original, bytes]:
    original = originals[proof["version"]]
    require(original.version in REFERENCES and proof["executable_sha1"] == original.sha1,
            f"{entry['name']}: reference executable identity mismatch")
    function = original.by_address.get(proof["source_address"])
    require(function is not None and function["name"] == proof["name"] == entry["name"] and
            function["source"] == proof["source"] == entry["source"] and
            function["high"] - function["low"] == entry["size"],
            f"{entry['name']}: reference name/source/extent differs from actual DWARF1")
    body = original.read(function["low"], entry["size"])
    if "reference_sha256" in proof:
        require(sha256(body) == proof["reference_sha256"], f"{entry['name']}: reference byte hash mismatch")
    return original, body


def provenance(entry: dict) -> list[dict]:
    proofs = entry["provenance"]
    require(len(proofs) == len(REFERENCES) and {p["version"] for p in proofs} == set(REFERENCES),
            f"{entry['name']}: expected provenance from each of the three debug versions")
    return proofs


def compare_explicit(entry: dict, french: bytes, reference: bytes, offsets: list[int]) -> None:
    differences = [i for i in range(0, len(french), 4) if french[i:i+4] != reference[i:i+4]]
    require(sorted(offsets) == differences and len(offsets) == len(set(offsets)),
            f"{entry['name']}: changed instructions differ from explicit relocation exceptions")
    for offset in offsets:
        require(0 <= offset < len(french) and offset % 4 == 0, f"{entry['name']}: invalid relocation offset")
        left, right = struct.unpack("<I", french[offset:offset+4])[0], struct.unpack("<I", reference[offset:offset+4])[0]
        require(left >> 26 == right >> 26 and left >> 26 in (2, 3),
                f"{entry['name']}: exception changes more than a J/JAL destination")


def corroboration(entry: dict, target: Original) -> None:
    evidence = entry.get("corroboration", {})
    start, end = entry["address"], entry["address"] + entry["size"]
    calls = evidence.get("direct_entry_calls", [])
    if "direct_entry_call" in evidence:
        calls = [*calls, evidence["direct_entry_call"]]
    for call in calls:
        target.transfer(call, start)
    cfg = evidence.get("entry_cfg")
    if cfg:
        require(cfg["entry_point"] == target.metadata["entry_point"], "Recorded ELF entry differs")
        chain = cfg["direct_call_chain"]
        require(bool(chain) and chain[-1]["target"] == start, f"{entry['name']}: incomplete direct-call witness")
        for edge in chain:
            target.transfer(edge["call_site"], edge["target"])
    padding = evidence.get("following_padding")
    if padding:
        require(padding["address"] == end and padding["all_zero"] is True and
                target.read(end, padding["size"]) == bytes(padding["size"]), f"{entry['name']}: padding differs")
    terminal = evidence.get("terminal_return")
    control = evidence.get("local_control_flow")
    if isinstance(control, dict):
        terminal = {"address": control["return_instruction"], "delay_slot": control["return_instruction"] + 4,
                    "end_exclusive": control["end_exclusive"]}
        branch = target.word(control["conditional_branch"])
        displacement = (branch & 65535) - (65536 if branch & 32768 else 0)
        require(branch >> 26 in (4, 5, 6, 7) and
                control["conditional_branch"] + 4 + displacement * 4 == control["conditional_target"],
                f"{entry['name']}: recorded conditional branch differs")
        target.transfer(control["conditional_call"], control["conditional_call_target"])
    if terminal:
        require(target.word(terminal["address"]) == 0x03e00008 and
                terminal["delay_slot"] == terminal["address"] + 4 and
                terminal["end_exclusive"] == terminal["address"] + 8 == end,
                f"{entry['name']}: return/delay-slot boundary differs")
    frame = evidence.get("stack_frame")
    if frame:
        for address, adjustment in ((frame["allocation_address"], -frame["allocation_bytes"]),
                                    (frame["deallocation_address"], frame.get("deallocation_bytes", frame["allocation_bytes"]))):
            word = target.word(address)
            require(word >> 26 in (9, 25) and (word >> 21) & 31 == 29 and (word >> 16) & 31 == 29 and
                    word & 65535 == adjustment & 65535, f"{entry['name']}: stack adjustment differs")
        for field, opcodes in (("return_address_save", (43, 63)), ("return_address_restore", (35, 55))):
            word = target.word(frame[field])
            require(word >> 26 in opcodes and (word >> 21) & 31 == 29 and (word >> 16) & 31 == 31 and
                    word & 65535 == frame["return_address_stack_offset"] & 65535,
                    f"{entry['name']}: return-address save/restore differs")
    caller = evidence.get("independent_caller")
    if caller:
        require(sha256(target.read(caller["address"], caller["size"])) == caller["sha256"],
                f"{entry['name']}: independent caller bytes differ")
        target.transfer(caller["call_site"], start)


def verify(manifest: Path, orig_dir: Path, registry_dir: Path) -> dict:
    versions = json.loads(manifest.read_text(encoding="utf-8"))["versions"]
    originals = {version: Original(version, versions[version], orig_dir) for version in (TARGET, *REFERENCES)}
    target = originals[TARGET]
    reviewed = json.loads((registry_dir / "reviewed-functions.json").read_text(encoding="utf-8"))
    call_registry = json.loads((registry_dir / "reviewed-call-targets.json").read_text(encoding="utf-8"))
    for registry in (reviewed, call_registry):
        require(registry["version"] == TARGET and registry["executable_sha1"] == target.sha1,
                "Registry targets another executable")
        require(registry["coverage_complete"] is False, "Partial reviewed coverage must remain explicit")
    functions = reviewed["functions"]
    anchors = call_registry["anchors"]
    by_address = {entry["address"]: entry for entry in functions}
    by_name = {(entry["source"], entry["name"]): entry for entry in functions}
    anchor_by_address = {entry["address"]: entry for entry in anchors}
    require(len(by_address) == len(functions) and len(anchor_by_address) == len(anchors), "Duplicate reviewed address")
    ordered = sorted(functions, key=lambda entry: entry["address"])
    require(all(a["address"] + a["size"] <= b["address"] for a, b in zip(ordered, ordered[1:])), "Reviewed functions overlap")
    for entry in functions:
        require(entry["boundary_confirmation"] is True, f"{entry['name']}: boundary not confirmed")
        body = verified_body(entry, target)
        for proof in provenance(entry):
            reference, reference_bytes = reference_body(entry, proof, originals)
            layout = entry.get("corroboration", {}).get("tu_layout")
            if layout:
                base = by_name.get((entry["source"], layout["anchor_function"]))
                require(base is not None and base["address"] == layout["anchor_address"] and
                        entry["address"] - base["address"] == layout["offset_from_anchor"],
                        f"{entry['name']}: reviewed TU layout anchor differs")
                base_proof = next(p for p in provenance(base) if p["version"] == proof["version"])
                require(proof["source_address"] - base_proof["source_address"] == layout["offset_from_anchor"],
                        f"{entry['name']}: reference TU relative position differs")
            changes = proof.get("call_target_differences", [])
            compare_explicit(entry, body, reference_bytes, [change["offset"] for change in changes])
            for change in changes:
                offset = change["offset"]
                target.transfer(entry["address"] + offset, change["french_target"])
                reference.transfer(proof["source_address"] + offset, change["reference_target"])
                callee = reference.by_address.get(change["reference_target"])
                anchor = anchor_by_address.get(change["french_target"])
                require(callee is not None and anchor is not None and
                        callee["name"] == anchor["name"] == change["reference_target_name"] and
                        callee["source"] == anchor["source"] == change["reference_target_source"],
                        f"{entry['name']}: explicit relocation has no independently named callee")
        corroboration(entry, target)
    for anchor in anchors:
        require(anchor["identity_confirmation"] is True and anchor["eligible_for_progress"] is False,
                f"{anchor['name']}: identity or progress scope differs")
        body = verified_body(anchor, target)
        occurrences = 0
        for segment in target.loaded:
            span = target.read(segment["address"], segment["file_size"])
            position = span.find(body)
            while position >= 0:
                occurrences += 1
                position = span.find(body, position + 1)
        require(occurrences == anchor["corroboration"]["loaded_image_exact_occurrences"] == 1,
                f"{anchor['name']}: target bytes are not unique in the loaded image")
        for proof in provenance(anchor):
            reference, reference_bytes = reference_body(anchor, proof, originals)
            compare_explicit(anchor, body, reference_bytes, proof["changed_instruction_offsets"])
            group = anchor["corroboration"]["wrapper_group"]
            require(group["first_address"] + group["position"] * group["stride"] == anchor["address"],
                    f"{anchor['name']}: wrapper group position differs")
            for index in range(group["wrapper_count"]):
                french_address = group["first_address"] + index * group["stride"]
                reference_address = proof["source_address"] + french_address - anchor["address"]
                neighbor = reference.by_address.get(reference_address)
                require(neighbor is not None and neighbor["source"] == anchor["source"] and
                        neighbor["high"] - neighbor["low"] == group["wrapper_size"] and
                        normalized(target.read(french_address, group["wrapper_size"])) ==
                        normalized(reference.read(reference_address, group["wrapper_size"])),
                        f"{anchor['name']}: reference wrapper neighborhood differs")
            matches = [f for f in reference.functions if f["high"] - f["low"] == anchor["size"] and
                       normalized(reference.read(f["low"], anchor["size"])) == normalized(body)]
            require(len(matches) == proof["matching_named_dwarf_ranges"] == 1 and
                    matches[0]["low"] == proof["source_address"], f"{anchor['name']}: reference identity not unique")
        transfer = anchor["corroboration"]["internal_transfer"]
        target.transfer(anchor["address"] + transfer["offset"], transfer["address"],
                        2 if transfer["kind"] == "tail_jump" else 3)
        for use in anchor["reviewed_uses"]:
            owner = by_address.get(use["function_address"])
            require(owner is not None and owner["name"] == use["function"] and use["relocation_type"] == "R_MIPS_26" and
                    0 <= use["offset"] < owner["size"] and use["call_address"] == owner["address"] + use["offset"],
                    f"{anchor['name']}: reviewed call site lacks function ownership")
            target.transfer(use["call_address"], anchor["address"])
    return {"originals": len(originals), "reviewed_functions": len(functions),
            "reviewed_code_bytes": sum(entry["size"] for entry in functions), "call_anchors": len(anchors)}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=ROOT / "config/platforms/versions.json")
    parser.add_argument("--orig-dir", type=Path, default=ROOT / "orig")
    parser.add_argument("--registry-dir", type=Path, default=ROOT / "config/platforms" / TARGET)
    args = parser.parse_args()
    try:
        result = verify(args.manifest, args.orig_dir, args.registry_dir)
        print(f"Verified {result['originals']} original hashes, {result['reviewed_functions']} reviewed functions "
              f"({result['reviewed_code_bytes']} bytes), and {result['call_anchors']} independent call anchors; "
              "no progress or source-link claim.")
    except (OSError, ValueError, KeyError, TypeError, struct.error) as error:
        parser.exit(1, f"error: {error}\n")


if __name__ == "__main__":
    main()
