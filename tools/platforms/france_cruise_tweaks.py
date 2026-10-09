"""Prove CruiseBubble's complete parameter body using independently known callees."""
from __future__ import annotations

from collections import Counter
import json

from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow
from platforms.france_tu_sequences import compare, cstring, digest, unique_template, words
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND, generate_unit as parameter_helpers
from platforms.ps2_source import canonical_linkages

SOURCE = "SB/Game/zEntCruiseBubble.cpp"
ANCHOR, SIZE = 0x2A1540, 11980


def generate_unit(originals, registry_dir):
    target = originals[TARGET]
    known = {}
    for path in sorted(registry_dir.glob("*functions.json")):
        for row in json.loads(path.read_text())["functions"]:
            if row["address"] == ANCHOR:
                continue
            prior = known.setdefault(row["address"], row)
            require(all(prior[k] == row[k] for k in ("name", "source", "size", "sha256")),
                    "Conflicting independent CruiseBubble context")
    for row in parameter_helpers(originals, registry_dir)["functions"]:
        known[row["address"]] = row
    functions, sequences, data_proofs = {}, [], []
    for version in REFERENCES:
        original = originals[version]
        links = canonical_linkages(original.data, original.metadata)
        matches = [f for f in original.functions if f["source"] == SOURCE and f["name"] == "register_tweaks"]
        require(len(matches) == 1 and matches[0]["high"] - matches[0]["low"] == SIZE,
                "Original CruiseBubble complete parameter body differs")
        caller = matches[0]
        pairs, calls, mask = compare(original, target, caller["low"], ANCHOR, SIZE)
        require(len(pairs) == len(calls) == 94, "Complete CruiseBubble operand inventory differs")
        strings, mapping = [], {}
        for pair in pairs:
            ra, tb = pair["reference_address"], pair["target_address"]
            require(pair["opcode"] == 9 and pair["storage"] == "file_backed",
                    "CruiseBubble parameter operand is not a complete string pointer")
            literal = cstring(original, ra)
            require(len(literal) > 1 and literal == cstring(target, tb),
                    "Complete CruiseBubble parameter string differs")
            require(ra not in mapping or mapping[ra] == tb, "CruiseBubble string mapping inconsistent")
            mapping[ra] = tb
            strings.append({**pair, "size": len(literal), "sha256": digest(literal),
                            "text": literal[:-1].decode("ascii")})
        require(len(mapping) == len(set(mapping.values())) == 94,
                "Distinct complete CruiseBubble strings collapse")
        callees = []
        for call in calls:
            require(call["opcode"] == 3, "CruiseBubble parameter body contains an unexpected tail call")
            callee = checked_identity(original, target, call["reference_address"], call["target_address"], known)
            if callee["name"] == "xStrHash":
                require((callee["source"], callee["size"], call["target_address"]) ==
                        ("SB/Core/x/xString.cpp", 88, 0x20F260),
                        "CruiseBubble bounded hash overload differs")
            callees.append(callee["name"])
        require(Counter(callees) == {"zParamGetFloat": 86, "zParamGetInt": 5, "zParamGetVector": 1, "xStrHash": 2},
                "CruiseBubble complete parameter helper identities differ")
        for f, b, size in ((caller, ANCHOR, SIZE),):
            a = f["low"]
            require(a in links and a % 16 == b % 16 == 0, "CruiseBubble linkage/alignment differs")
            fpairs, fcalls, fmask = compare(original, target, a, b, size)
            for binary, entry in ((original, a), (target, b)):
                flow = ControlFlow({entry + i * 4: w for i, w in enumerate(words(binary.read(entry, size + 16)))})
                bounds = flow.bounds(entry, size)
                require(bounds["passes"], "CruiseBubble cluster requires a complete closed original body")
            unique = unique_template(target, original.read(a, size), fmask, b)
            if b not in functions:
                functions[b] = {"name": f["name"], "source": f["source"], "address": b, "size": size,
                    "sha256": digest(target.read(b, size)), "boundary_confirmation": True,
                    "confirmation_kind": CLUSTER_KIND, "provenance": [],
                    "corroboration": {"proof_scope": "complete_caller_callee_cluster",
                        "unique_complete_caller": ANCHOR, "local_control_flow": bounds,
                        "whole_translation_unit_claimed": False}}
            record = functions[b]
            require(not record["provenance"] or record["provenance"][0]["linkage_name"] == links[a],
                    "CruiseBubble canonical linkage differs across originals")
            record["provenance"].append({"version": version, "executable_sha1": original.sha1,
                "source_address": a, "source": f["source"], "name": f["name"], "linkage_name": links[a],
                "reference_sha256": digest(original.read(a, size)), "data_address_operands": fpairs,
                "direct_transfers": fcalls})
            sequences.append({"version": version, "source": f["source"], "source_start": a,
                "target_start": b, "complete_body_bytes": size, "uniqueness": unique,
                "whole_translation_unit_claimed": False})
        data_proofs.append({"version": version, "source": SOURCE, "complete_strings": strings,
                            "data_extents_promoted": False})
    require(len(functions) == 1 and all(len(f["provenance"]) == 3 for f in functions.values()),
            "CruiseBubble requires the complete body from all three originals")
    return {"functions": sorted(functions.values(), key=lambda f: f["address"]),
            "sequence_proofs": sequences, "data_proofs": data_proofs,
            "counts": {"functions": 1, "code_bytes": SIZE, "source_units": 1,
                       "closed_return_bodies": 1, "reviewed_complete_caller_callee_clusters": 1}}
