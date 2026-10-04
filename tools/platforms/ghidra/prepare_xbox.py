"""Prepare authenticated raw .text and metadata for the Ghidra Xbox scripts.

Run from the repository root:
    python -m tools.platforms.ghidra.prepare_xbox orig/XBOX-US/default.xbe build/xbox-analysis

Then run analyzeHeadless with BinaryLoader, -loader-baseAddr from the returned
metadata, -processor x86:LE:32:default, -cspec windows, and these scripts:
    -preScript MapXbeSections.py <metadata.json> <original.xbe>
    -postScript ExportXbeFunctions.py <metadata.json> <functions.json>
Set -scriptPath to this module's directory. Use a private build directory for
all extracted bytes and Ghidra project files. Function exports are candidates,
not original debug symbols or proof of matching source.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
from ..xbox import inspect_xbe


def prepare_analysis(executable: Path, output_dir: Path) -> dict:
    executable, output_dir = Path(executable), Path(output_dir)
    metadata = inspect_xbe(executable)
    registry = Path(__file__).resolve().parents[3] / "config/platforms/versions.json"
    versions = json.loads(registry.read_text(encoding="utf-8"))["versions"]
    accepted = {v["executable"]["sha1"] for v in versions.values() if v["platform"] == "xbox"}
    if metadata["sha1"] not in accepted:
        raise ValueError("XBE SHA-1 is not registered in config/platforms/versions.json")
    data = executable.read_bytes()
    if hashlib.sha1(data).hexdigest() != metadata["sha1"]:
        raise ValueError("XBE changed during analysis preparation")
    sections = [s for s in metadata["sections"] if s["name"] == ".text"]
    if len(sections) != 1:
        raise ValueError("Expected exactly one .text section")
    section = sections[0]
    output_dir.mkdir(parents=True, exist_ok=True)
    (output_dir / "text.bin").write_bytes(data[section["raw_offset"]:section["raw_offset"] + section["raw_size"]])
    (output_dir / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    return {"executable_sha1": metadata["sha1"], "text_base_address": section["virtual_address"], "text_size": section["raw_size"], "entry_point": metadata["entry_point"]}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("output_dir", type=Path)
    args = parser.parse_args()
    print(json.dumps(prepare_analysis(args.executable, args.output_dir), indent=2))


if __name__ == "__main__":
    main()
