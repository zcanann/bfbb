"""Inspect authenticated PS2 DWARF aggregate layouts without inventing declarations.

Reports direct members and their recorded displacement expressions. It does not
infer alignment, tail fields, macros, or definitions for incomplete types.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

from .dwarf1 import iter_dies
from .verify_reviewed import Original


def aggregate_layouts(debug: bytes, source: str, names: set[str]) -> dict:
    rows = list(iter_dies(debug))
    by_offset = {row[0]: row for row in rows}
    next_offset = {a[0]: b[0] for a, b in zip(rows, rows[1:])}
    result = {}
    source = source.replace("\\", "/").lstrip("/")
    for offset, tag, owner, attrs in rows:
        name = attrs.get(3)
        owner = str(owner).replace("\\", "/")
        if (tag not in (2, 0x13, 0x17) or name not in names or
                not attrs.get(11) or not owner.endswith("/" + source)):
            continue
        end = attrs.get(1)
        if not isinstance(end, int) or end <= offset:
            raise ValueError(f"Missing aggregate sibling for {name}")
        members = []
        cursor = next_offset[offset]
        while cursor < end:
            if cursor not in by_offset:
                # iter_dies omits null/sibling terminators, including four-byte
                # records immediately before the final aggregate sibling.
                size = int.from_bytes(debug[cursor:cursor + 4], "little")
                null_tag = int.from_bytes(debug[cursor + 4:cursor + 6], "little") == 0
                if size < 4 or cursor + size > end or (size >= 8 and not null_tag):
                    raise ValueError(f"Invalid aggregate terminator for {name}")
                cursor += size
                continue
            child_offset, child_tag, _, child = by_offset[cursor]
            if child_tag == 0x0d:
                location = child.get(2)
                # DW_OP_const (u32), DW_OP_add. Keep other expressions unknown.
                displacement = None
                if (isinstance(location, bytes) and len(location) == 6 and
                        location[0] == 4 and location[-1] == 7):
                    displacement = int.from_bytes(location[1:5], "little")
                type_attrs = {str(k): v.hex() if isinstance(v, bytes) else v
                              for k, v in child.items() if k in (5, 6, 7, 8)}
                members.append({"name": child.get(3), "offset": displacement,
                                "die_offset": child_offset,
                                "location": location.hex() if isinstance(location, bytes) else location,
                                "type_attributes": type_attrs})
            following = child.get(1, next_offset.get(cursor, end))
            if not isinstance(following, int) or following <= cursor:
                raise ValueError(f"Invalid sibling in {name}")
            cursor = following
        record = {"die_offset": offset, "tag": tag, "size": attrs[11], "members": members}
        if name in result:
            old = result[name]
            if (old["size"], [(m["name"], m["offset"]) for m in old["members"]]) != (
                    record["size"], [(m["name"], m["offset"]) for m in members]):
                raise ValueError(f"Conflicting aggregate declarations for {name}")
        result[name] = record
    missing = names - result.keys()
    if missing:
        raise ValueError("Missing concrete types: " + ", ".join(sorted(missing)))
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", action="append", required=True)
    parser.add_argument("--orig-root", type=Path, default=Path("orig"))
    parser.add_argument("--source", required=True, help="DWARF source, e.g. SB/Core/x/xEnv.cpp")
    parser.add_argument("--type", dest="types", action="append", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    versions = json.loads((root / "config/platforms/versions.json").read_text())["versions"]
    result = {}
    for version in args.version:
        original = Original(version, versions[version], args.orig_root)
        sections = [s for s in original.metadata["sections"] if s["name"] == ".debug" and s["size"]]
        if len(sections) != 1:
            raise ValueError(f"{version}: expected one original DWARF section")
        section = sections[0]
        debug = original.data[section["offset"]:section["offset"] + section["size"]]
        result[version] = {"original_sha1": original.sha1, "source": args.source,
                           "types": aggregate_layouts(debug, args.source, set(args.types))}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n")


if __name__ == "__main__":
    main()
