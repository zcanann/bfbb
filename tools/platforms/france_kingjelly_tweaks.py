"""Original-only King Jelly parameter caller and complete float-list helper."""
from __future__ import annotations

from collections import Counter
import json

from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow
from platforms.france_tu_sequences import compare, cstring, digest, unique_template, words
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND, generate_unit as parameter_helpers
from platforms.ps2_source import canonical_linkages

SOURCE = "SB/Game/zNPCTypeKingJelly.cpp"
ANCHOR, SIZE = 0x342FB0, 23444
HELPER = 0x1331F0
COLOR = 0x348B50


def generate_unit(originals, registry_dir):
    target = originals[TARGET]
    known = {}
    for path in sorted(registry_dir.glob("*functions.json")):
        for row in json.loads(path.read_text())["functions"]:
            if row["address"] not in (ANCHOR, HELPER):
                prior = known.setdefault(row["address"], row)
                require(all(prior[k] == row[k] for k in ("name", "source", "size", "sha256")),
                        "Conflicting independent King Jelly context")
    # Independently regenerate the parameter helpers; no circular reliance on
    # the committed Dutchman cluster or on this module's own new records.
    for row in parameter_helpers(originals, registry_dir)["functions"]:
        known[row["address"]] = row
    functions, sequences, data_proofs, contexts = {}, [], [], []
    for version in REFERENCES:
        original = originals[version]
        links = canonical_linkages(original.data, original.metadata)
        candidates = [f for f in original.functions if f["source"] == SOURCE and f["name"] == "register_tweaks"]
        require(len(candidates) == 1 and candidates[0]["high"] - candidates[0]["low"] == SIZE,
                "Original complete King Jelly tweak body differs")
        caller = candidates[0]
        pairs, calls, mask = compare(original, target, caller["low"], ANCHOR, SIZE)
        require(len(pairs) == 160 and len(calls) == 168, "King Jelly parameter inventory differs")
        strings, mapping, callmap = [], {}, {}
        for pair in pairs:
            ra, tb = pair["reference_address"], pair["target_address"]
            require(pair["opcode"] == 9 and pair["storage"] == "file_backed",
                    "King Jelly parameter is not a complete string pointer")
            value = cstring(original, ra)
            require(len(value) > 1 and value == cstring(target, tb), "Complete King Jelly parameter string differs")
            require(ra not in mapping or mapping[ra] == tb, "King Jelly string mapping inconsistent")
            mapping[ra] = tb
            strings.append({**pair, "size": len(value), "sha256": digest(value), "text": value[:-1].decode("ascii")})
        require(len(mapping) == len(set(mapping.values())) == 159, "Distinct King Jelly strings collapse")
        for call in calls:
            ra, tb = call["reference_address"], call["target_address"]
            require(call["opcode"] == 3 and (ra not in callmap or callmap[ra] == tb),
                    "King Jelly direct helper relationship differs")
            callmap[ra] = tb
        require(len(callmap) == len(set(callmap.values())) == 4 and
                Counter(c["target_address"] for c in calls) ==
                {0x133420: 132, 0x133580: 20, HELPER: 8, COLOR: 8},
                "King Jelly complete parameter helper usage differs")
        helper_refs = [ra for ra, tb in callmap.items() if tb == HELPER]
        helper = original.by_address.get(helper_refs[0])
        require(helper is not None and (helper["source"], helper["name"], helper["high"] - helper["low"]) ==
                ("SB/Game/zEnt.cpp", "zParamGetFloatList", 376), "King Jelly float-list helper identity differs")
        color = original.by_address.get(next(ra for ra, tb in callmap.items() if tb == COLOR))
        require(color is not None and (color["source"], color["name"], color["high"] - color["low"]) ==
                ("SB/Core/x/xColor.h", "xColorFromRGBA", 64) and color["low"] in links,
                "King Jelly complete color helper identity differs")
        require(color["low"] - caller["high"] == COLOR - (ANCHOR + SIZE) == 12 and
                original.read(caller["high"], 12) == target.read(ANCHOR + SIZE, 12) == bytes(12),
                "King Jelly color helper adjacency or zero alignment differs")
        require(original.read(color["low"], 64) == target.read(COLOR, 64),
                "Complete King Jelly color helper is not byte-identical")
        for binary, entry in ((original, color["low"]), (target, COLOR)):
            flow = ControlFlow({entry + i * 4: w for i, w in enumerate(words(binary.read(entry, 80)))})
            require(flow.bounds(entry, 64)["passes"], "King Jelly color context is not a complete closed body")
        contexts.append({"version": version, "source": color["source"], "name": color["name"],
            "linkage_name": links[color["low"]], "reference_address": color["low"], "target_address": COLOR,
            "complete_original_bytes_compared": 64, "sha256": digest(target.read(COLOR, 64)),
            "promoted_as_named_anchor": False})
        for call in calls:
            if call["target_address"] not in (HELPER, COLOR):
                checked_identity(original, target, call["reference_address"], call["target_address"], known)
        unique = unique_template(target, original.read(caller["low"], SIZE), mask, ANCHOR)
        sequences.append({"version": version, "source": SOURCE, "source_start": caller["low"],
            "target_start": ANCHOR, "complete_body_bytes": SIZE, "uniqueness": unique,
            "proof_scope": "one_complete_body_with_complete_parameter_helpers",
            "whole_translation_unit_claimed": False})
        for f, b, size in ((caller, ANCHOR, SIZE), (helper, HELPER, 376)):
            a, name, source = f["low"], f["name"], f["source"]
            require(a in links and a % 16 == b % 16 == 0, "King Jelly cluster linkage/alignment differs")
            fpairs, fcalls, _ = compare(original, target, a, b, size)
            for binary, entry in ((original, a), (target, b)):
                flow = ControlFlow({entry + i * 4: w for i, w in enumerate(words(binary.read(entry, size + 16)))})
                bounds = flow.bounds(entry, size)
                require(bounds["passes"], "King Jelly cluster needs closed complete original bodies")
            if b == HELPER:
                require(not fpairs and len(fcalls) == 2, "Float-list helper address/call inventory differs")
                for c in fcalls:
                    require(c["opcode"] == 3 and c["target_address"] in known,
                            "Float-list helper requires independently named complete callees")
                    checked_identity(original, target, c["reference_address"], c["target_address"], known)
                require([known[c["target_address"]]["name"] for c in fcalls] ==
                        ["xStrHash", "xStrParseFloatList"], "Float-list helper callee identities differ")
            record = functions.setdefault(b, {"name": name, "source": source, "address": b, "size": size,
                "sha256": digest(target.read(b, size)), "boundary_confirmation": True,
                "confirmation_kind": CLUSTER_KIND, "provenance": [],
                "corroboration": {"proof_scope": "complete_caller_callee_cluster", "local_control_flow": bounds,
                    "unique_complete_caller": ANCHOR, "whole_translation_unit_claimed": False,
                    "incoming_cluster_call_offsets": [c["offset"] for c in calls if c["target_address"] == b]}})
            require(not record["provenance"] or record["provenance"][0]["linkage_name"] == links[a],
                    "King Jelly cluster canonical linkage differs across originals")
            record["provenance"].append({"version": version, "executable_sha1": original.sha1,
                "source_address": a, "name": name, "source": source, "linkage_name": links[a],
                "reference_sha256": digest(original.read(a, size)), "data_address_operands": fpairs,
                "direct_transfers": fcalls})
        data_proofs.append({"version": version, "source": SOURCE, "complete_strings": strings,
                            "data_extents_promoted": False})
    require(len(functions) == 2 and sum(f["size"] for f in functions.values()) == 23820 and
            all(len(f["provenance"]) == 3 for f in functions.values()) and
            len({c["linkage_name"] for c in contexts}) == 1,
            "King Jelly cluster requires all three originals")
    return {"functions": sorted(functions.values(), key=lambda f: f["address"]),
            "sequence_proofs": sequences, "data_proofs": data_proofs, "call_neighbors": contexts,
            "counts": {"functions": 2, "code_bytes": 23820, "source_units": 2,
                       "closed_return_bodies": 2, "reviewed_complete_caller_callee_clusters": 1}}
