"""Prove the complete Plankton parameter body and its typed sound-array operands."""
from __future__ import annotations

from collections import Counter
import json

from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow
from platforms.france_tu_sequences import compare, cstring, digest, named_data, unique_template, words
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND, generate_unit as parameter_helpers
from platforms.ps2_source import canonical_linkages
from platforms.dwarf1 import iter_dies
from platforms.ps2_type_layouts import aggregate_layouts

SOURCE = "SB/Game/zNPCTypeBossPlankton.cpp"
ANCHOR, SIZE = 0x36A0F0, 12572


def sound_arrays(original):
    section = next(s for s in original.metadata["sections"] if s["name"] == ".debug" and s["size"])
    debug = original.data[section["offset"]:section["offset"] + section["size"]]
    rows = list(iter_dies(debug))
    by = {off: (tag, attrs) for off, tag, _, attrs in rows}
    layout = aggregate_layouts(debug, SOURCE, {"sound_data_type"})["sound_data_type"]
    require(layout["size"] == 16 and {m["name"]: m["offset"] for m in layout["members"]} ==
            {"id": 0, "handle": 4, "loc": 8, "volume": 12} and
            next(m for m in layout["members"] if m["name"] == "id")["type_attributes"] == {"5": 9},
            "Original Plankton sound record layout differs")
    result = {}
    for name in ("sound_asset_ids", "sound_data"):
        matches = [(off, attrs) for off, tag, owner, attrs in rows if tag == 12 and
                   owner.replace("\\", "/").endswith(SOURCE) and attrs.get(3) == name]
        require(len(matches) == 1, "Original Plankton sound array declaration is ambiguous")
        declaration, attrs = matches[0]
        type_die = attrs.get(7)
        tag, array = by[type_die]
        desc = array.get(10)
        require(tag == 1 and array.get(9) == 0 and isinstance(desc, bytes) and len(desc) == 18 and
                desc[:14] == bytes.fromhex("000a000000000005000000087200"),
                "Original Plankton sound array is not six elements")
        element_die = int.from_bytes(desc[14:], "little")
        if name == "sound_asset_ids":
            element_tag, element = by[element_die]
            require(element_tag == 1 and element.get(9) == 0 and
                    element.get(10) == bytes.fromhex("000a0000000000090000000855000900"),
                    "Original Plankton sound IDs are not uint32[6][10]")
        else:
            require(element_die == layout["die_offset"], "Plankton sound array element type differs")
        stride = 40 if name == "sound_asset_ids" else 16
        result[name] = {"reference_address": named_data(original, name, SOURCE), "size": stride * 6,
            "count": 6, "stride": stride, "declaration_die": declaration, "type_die": type_die,
            "element_die": element_die, "subscript_descriptor": desc.hex()}
    result["sound_data"]["element_layout"] = layout
    require(result["sound_data"]["reference_address"] - result["sound_asset_ids"]["reference_address"] == 272,
            "Original Plankton sound array separation differs")
    return result


def generate_unit(originals, registry_dir):
    target = originals[TARGET]
    known = {}
    for path in sorted(registry_dir.glob("*functions.json")):
        for row in json.loads(path.read_text())["functions"]:
            if row["address"] != ANCHOR:
                prior = known.setdefault(row["address"], row)
                require(all(prior[k] == row[k] for k in ("name", "source", "size", "sha256")),
                        "Conflicting independent Plankton context")
    for row in parameter_helpers(originals, registry_dir)["functions"]:
        known[row["address"]] = row
    record, sequences, data_proofs = None, [], []
    for version in REFERENCES:
        original = originals[version]
        links = canonical_linkages(original.data, original.metadata)
        matches = [f for f in original.functions if f["source"] == SOURCE and f["name"] == "register_tweaks"]
        require(len(matches) == 1 and matches[0]["high"] - matches[0]["low"] == SIZE,
                "Original Plankton complete parameter body differs")
        a = matches[0]["low"]
        require(a in links and a % 16 == ANCHOR % 16 == 0, "Plankton linkage/alignment differs")
        arrays = sound_arrays(original)
        pairs, calls, mask = compare(original, target, a, ANCHOR, SIZE)
        require(len(pairs) == 113 and len(calls) == 101, "Complete Plankton operand inventory differs")
        strings, fields, mapping = [], [], {}
        bases = {name: set() for name in arrays}
        offsets = {name: [] for name in arrays}
        for pair in pairs:
            ra, tb = pair["reference_address"], pair["target_address"]
            require(ra not in mapping or mapping[ra] == tb, "Plankton data address mapping inconsistent")
            mapping[ra] = tb
            if pair["storage"] == "file_backed":
                require(pair["opcode"] == 9, "Plankton literal operand is not a complete string pointer")
                value = cstring(original, ra)
                require(value == cstring(target, tb), "Complete Plankton parameter string differs")
                strings.append({**pair, "size": len(value), "sha256": digest(value), "text": value[:-1].decode("ascii")})
            else:
                require(pair["storage"] == "zero_fill" and pair["opcode"] in (35, 43),
                        "Unreviewed Plankton typed data operand")
                name = "sound_asset_ids" if pair["opcode"] == 35 else "sound_data"
                offset = ra - arrays[name]["reference_address"]
                allowed = ([16, 52, 80, 132, 172, 212] if name == "sound_asset_ids" else
                           [i * 16 for i in range(6)])
                require(offset in allowed, "Plankton sound field/index differs from original typed array")
                base = tb - offset
                bases[name].add(base)
                offsets[name].append(offset)
                for binary, address in ((original, arrays[name]["reference_address"]), (target, base)):
                    region = binary._stream_regions["runtime_bss"]
                    require(region["address"] <= address and address + arrays[name]["size"] <= region["address"] + region["size"],
                            "Complete typed Plankton array escapes zero-fill storage")
                fields.append({**pair, "array": name, "array_offset": offset, "complete_array_size": arrays[name]["size"]})
        require(len(strings) == 101 and sum(p["size"] == 1 for p in strings) == 6 and
                len(set(p["reference_address"] for p in strings)) == 96 and
                len(fields) == 12 and len(mapping) == len(set(mapping.values())) == 108,
                "Distinct Plankton strings and typed fields collapse")
        require(all(len(bases[name]) == 1 and len(offsets[name]) == len(set(offsets[name])) == 6 for name in arrays),
                "Plankton typed array bases or complete six-index inventory differ")
        require(next(iter(bases["sound_data"])) - next(iter(bases["sound_asset_ids"])) == 272,
                "French Plankton typed arrays overlap or lose their original separation")
        callees = []
        for call in calls:
            require(call["opcode"] == 3 and call["target_address"] in known,
                    "Plankton parameter call lacks an independently proven complete target")
            checked_identity(original, target, call["reference_address"], call["target_address"], known)
            callees.append(known[call["target_address"]]["name"])
        require(Counter(callees) == {"zParamGetFloat": 89, "zParamGetInt": 1, "zParamGetVector": 5, "xStrHash": 6},
                "Plankton complete parameter helper identities differ")
        for binary, entry in ((original, a), (target, ANCHOR)):
            flow = ControlFlow({entry + i * 4: w for i, w in enumerate(words(binary.read(entry, SIZE + 16)))})
            bounds = flow.bounds(entry, SIZE)
            require(bounds["passes"], "Plankton requires a complete closed original body")
        unique = unique_template(target, original.read(a, SIZE), mask, ANCHOR)
        if record is None:
            record = {"name": "register_tweaks", "source": SOURCE, "address": ANCHOR, "size": SIZE,
                "sha256": digest(target.read(ANCHOR, SIZE)), "boundary_confirmation": True,
                "confirmation_kind": CLUSTER_KIND, "provenance": [],
                "corroboration": {"proof_scope": "complete_caller_with_independently_proven_callees",
                    "unique_complete_caller": ANCHOR, "local_control_flow": bounds,
                    "whole_translation_unit_claimed": False}}
        require(not record["provenance"] or record["provenance"][0]["linkage_name"] == links[a],
                "Plankton canonical linkage differs across originals")
        record["provenance"].append({"version": version, "executable_sha1": original.sha1,
            "source_address": a, "source": SOURCE, "name": "register_tweaks", "linkage_name": links[a],
            "reference_sha256": digest(original.read(a, SIZE)), "data_address_operands": pairs, "direct_transfers": calls})
        sequences.append({"version": version, "source": SOURCE, "source_start": a, "target_start": ANCHOR,
            "complete_body_bytes": SIZE, "uniqueness": unique, "whole_translation_unit_claimed": False})
        data_proofs.append({"version": version, "source": SOURCE, "complete_strings": strings,
            "typed_original_arrays": arrays, "array_fields": fields,
            "target_array_bases": {name: next(iter(values)) for name, values in bases.items()},
            "data_extents_promoted": False})
    require(record is not None and len(record["provenance"]) == 3, "Plankton requires all three originals")
    return {"functions": [record], "sequence_proofs": sequences, "data_proofs": data_proofs,
            "counts": {"functions": 1, "code_bytes": SIZE, "source_units": 1, "closed_return_bodies": 1,
                       "reviewed_complete_caller_callee_clusters": 1}}
