#!/usr/bin/env python3
"""Authenticate reviewed PS2 runtime entries and their original caller evidence.

This command checks hashes, recorded instructions, named DWARF caller ownership,
and cross-version equality. It does not repeat the human semantic review, infer
new function extents, generate progress, or prove a source-selected ELF link.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct

from platforms.ps2 import inspect_elf
from platforms.ps2_report import _function_ranges, _source_name

ROOT = Path(__file__).resolve().parents[1]
VERSIONS = ("SLUS-20680", "SLES-51968", "SLES-51970")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def verify(manifest: Path, orig_dir: Path, registry_dir: Path) -> dict:
    versions = json.loads(manifest.read_text(encoding="utf-8"))["versions"]
    reviewed = {}
    caller_count = 0
    for version in VERSIONS:
        record = versions[version]
        require(record["platform"] == "ps2", f"{version}: expected PS2 platform")
        path = orig_dir / version / Path(record["executable"]["path"]).name
        binary = path.read_bytes()
        executable_sha1 = hashlib.sha1(binary).hexdigest()
        require(executable_sha1 == record["executable"]["sha1"], f"{version}: original hash mismatch")
        metadata = inspect_elf(path)
        require(metadata["sha1"] == executable_sha1, f"{version}: original changed during inspection")
        segments = [s for s in metadata["segments"] if s["type"] == 1 and s["file_size"]]

        def read(address: int, size: int) -> bytes:
            require(isinstance(address, int) and isinstance(size, int) and size > 0,
                    f"{version}: invalid loaded range")
            owners = [s for s in segments if s["address"] <= address and
                      address + size <= s["address"] + s["file_size"]]
            require(len(owners) == 1, f"{version}: range lacks a unique file-backed owner")
            offset = owners[0]["offset"] + address - owners[0]["address"]
            return binary[offset:offset + size]

        debug = [s for s in metadata["sections"] if s["name"] == ".debug" and s["size"]]
        require(len(debug) == 1, f"{version}: expected one original DWARF section")
        section = debug[0]
        functions = _function_ranges(binary[section["offset"]:section["offset"] + section["size"]])
        by_address = {f["low"]: f for f in functions}
        document = json.loads((registry_dir / version / "reviewed-call-targets.json").read_text(encoding="utf-8"))
        require(document["version"] == version and document["executable_sha1"] == executable_sha1,
                f"{version}: reviewed registry identifies another original")
        require(document["eligible_for_progress"] is False and document["coverage_complete"] is False,
                f"{version}: runtime anchor must not claim progress or complete coverage")
        anchors = [a for a in document["anchors"] if a["source"] == "runtime/memset"]
        require(len(anchors) == 1, f"{version}: expected exactly one reviewed memset entry")
        anchor = anchors[0]
        require(anchor["name"] == anchor["symbol"] == "memset" and anchor["identity_confirmation"] is True
                and anchor["eligible_for_progress"] is False and bool(anchor["semantic_review"]),
                f"{version}: incomplete reviewed runtime identity")
        address, size = anchor["address"], anchor["size"]
        require(address % 4 == 0 and size % 4 == 0, f"{version}: unaligned reviewed instructions")
        body = read(address, size)
        require(hashlib.sha256(body).hexdigest() == anchor["sha256"], f"{version}: reviewed runtime bytes differ")
        require(struct.unpack_from("<II", body, len(body) - 8) == (0x03e00008, 0x0080102d),
                f"{version}: recorded return of original destination differs")
        # Authenticate the recorded local-control-flow property, not general function discovery.
        for offset, (word,) in enumerate(struct.iter_unpack("<I", body)):
            opcode = word >> 26
            require(opcode not in (2, 3), f"{version}: unexpected runtime call or direct tail transfer")
            if opcode in (1, 4, 5, 6, 7, 20, 21, 22, 23):
                displacement = struct.unpack("<h", struct.pack("<H", word & 0xffff))[0] * 4
                destination = address + offset * 4 + 4 + displacement
                require(address <= destination < address + size, f"{version}: branch leaves reviewed span")
        values = set()
        for witness in anchor["reviewed_callers"]:
            owner = by_address.get(witness["function_address"])
            require(owner is not None and owner["name"] == witness["function"] and
                    _source_name(owner["source"]) == witness["source"] and
                    owner["high"] - owner["low"] == witness["function_size"],
                    f"{version}: caller lacks named original DWARF ownership")
            call = witness["call_address"]
            require(call % 4 == 0 and call == owner["low"] + witness["call_offset"] and
                    owner["low"] <= call and call + 8 <= owner["high"],
                    f"{version}: call lies outside its original function")
            word = struct.unpack("<I", read(call, 4))[0]
            require(word >> 26 == 3 and (((call + 4) & 0xf0000000) | ((word & 0x03ffffff) << 2)) == address,
                    f"{version}: recorded caller does not call the reviewed entry")
            window = witness["window_address"]
            end = window + witness["window_size"]
            require(owner["low"] <= window <= call and call + 8 <= end <= owner["high"],
                    f"{version}: caller evidence window exceeds its function")
            require(hashlib.sha256(read(window, witness["window_size"])).hexdigest() == witness["window_sha256"],
                    f"{version}: caller instruction evidence differs")
            fill_address = witness["fill_instruction_address"]
            require(window <= fill_address < call and fill_address % 4 == 0,
                    f"{version}: fill instruction lies outside its evidence window")
            fill = struct.unpack("<I", read(fill_address, 4))[0]
            value = witness["fill_value"]
            require(0 < value <= 255 and fill == witness["fill_instruction"] == (0x24050000 | value),
                    f"{version}: recorded nonzero a1 fill value differs")
            values.add(value)
            caller_count += 1
        require(values == {0x77, 0xcd}, f"{version}: expected both reviewed nonzero-fill callers")
        known_calls = sum(1 for f in functions for (word,) in struct.iter_unpack("<I", read(f["low"], f["high"] - f["low"]))
                          if word == (0x0c000000 | (address >> 2)))
        require(known_calls == anchor["known_dwarf_call_count"], f"{version}: recorded original call count differs")
        reviewed[version] = (document, anchor, body)
    for version, (_, anchor, body) in reviewed.items():
        proofs = anchor["cross_version_corroboration"]
        require(len(proofs) == len(VERSIONS) and {p["version"] for p in proofs} == set(VERSIONS),
                f"{version}: incomplete cross-version corroboration")
        for proof in proofs:
            document, reference, other = reviewed[proof["version"]]
            require(proof["executable_sha1"] == document["executable_sha1"] and
                    proof["address"] == reference["address"] and proof["size"] == reference["size"] and
                    proof["sha256"] == reference["sha256"] and other == body,
                    f"{version}: cross-version runtime evidence differs")
    return {"originals": len(reviewed), "runtime_entries": len(reviewed), "caller_witnesses": caller_count}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=ROOT / "config/platforms/versions.json")
    parser.add_argument("--orig-dir", type=Path, default=ROOT / "orig")
    parser.add_argument("--registry-dir", type=Path, default=ROOT / "config/platforms")
    args = parser.parse_args()
    try:
        result = verify(args.manifest, args.orig_dir, args.registry_dir)
        print(f"Verified {result['originals']} original hashes, {result['runtime_entries']} reviewed runtime entries, "
              f"and {result['caller_witnesses']} original caller witnesses; no progress or source-link claim.")
    except (OSError, ValueError, KeyError, TypeError, struct.error) as error:
        parser.exit(1, f"error: {error}\n")


if __name__ == "__main__":
    main()
