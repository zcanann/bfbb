"""Prove one large Dutchman body and its three complete parameter helpers.

This is a caller/callee cluster, not a claim about the complete Dutchman TU.
Only authenticated original bodies, DWARF identities and complete strings are
used. Fuzzy discovery and compiled source objects are not proof inputs.
"""
from __future__ import annotations

from collections import Counter
import json

from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow
from platforms.france_tu_sequences import compare, cstring, digest, unique_template, words
from platforms.france_update_cull_sequence import checked_identity
from platforms.ps2_source import canonical_linkages

CLUSTER_KIND = "reviewed-complete-caller-callee-cluster"
SOURCE = "SB/Game/zNPCTypeDutchman.cpp"
ANCHOR = 0x3A5CC0
MEMBERS = [(SOURCE, "register_tweaks", ANCHOR, 15588),
           ("SB/Game/zEnt.cpp", "zParamGetVector", 0x132F20, 328),
           ("SB/Game/zEnt.cpp", "zParamGetFloat", 0x133420, 168),
           ("SB/Game/zEnt.cpp", "zParamGetInt", 0x133580, 168)]


def generate_unit(originals, registry_dir):
    target = originals[TARGET]
    own = {row[2] for row in MEMBERS}
    known = {}
    for path in sorted(registry_dir.glob("*functions.json")):
        for record in json.loads(path.read_text())["functions"]:
            if record["address"] in own:
                continue  # Regeneration cannot bootstrap from its own records.
            prior = known.setdefault(record["address"], record)
            require(all(prior[k] == record[k] for k in ("name", "source", "size", "sha256")),
                    "Conflicting independent Dutchman context")
    functions, sequences, data_proofs, contexts = {}, [], [], []
    for version in REFERENCES:
        original = originals[version]
        links = canonical_linkages(original.data, original.metadata)
        anchor_original = [f for f in original.functions if f["source"] == SOURCE and f["name"] == "register_tweaks"]
        require(len(anchor_original) == 1 and anchor_original[0]["high"] - anchor_original[0]["low"] == 15588,
                "Original complete Dutchman tweak body is ambiguous or changed")
        original_callees = {c["reference_address"] for c in
            compare(original, target, anchor_original[0]["low"], ANCHOR, 15588)[1]}
        selected = []
        for source, name, address, size in MEMBERS:
            matches = [f for f in original.functions if f["source"] == source and f["name"] == name and
                       (address == ANCHOR or f["low"] in original_callees)]
            require(len(matches) == 1 and matches[0]["high"] - matches[0]["low"] == size,
                    f"Original Dutchman cluster ownership or extent differs: {version} {name} {[(f['low'], f['high'] - f['low']) for f in matches]}")
            selected.append((matches[0], address, size))
        inside = {f["low"]: (b, f["name"]) for f, b, _ in selected}
        caller_calls = []
        strings, mapping = [], {}
        for f, b, size in selected:
            a, name, source = f["low"], f["name"], f["source"]
            require(a in links and a % 16 == b % 16 == 0, "Dutchman cluster linkage/alignment differs")
            pairs, calls, mask = compare(original, target, a, b, size)
            for binary, entry in ((original, a), (target, b)):
                flow = ControlFlow({entry + i * 4: w for i, w in
                                    enumerate(words(binary.read(entry, size + 16)))})
                bounds = flow.bounds(entry, size)
                require(bounds["passes"], "Dutchman cluster requires a complete closed original body")
            if b == ANCHOR:
                require(len(pairs) == len(calls) == 120,
                        "Dutchman complete parameter list changed")
                for pair in pairs:
                    ra, tb = pair["reference_address"], pair["target_address"]
                    require(pair["opcode"] == 9 and pair["storage"] == "file_backed",
                            "Dutchman tweak operand is not a complete string pointer")
                    literal = cstring(original, ra)
                    require(len(literal) > 1 and literal == cstring(target, tb),
                            "Complete original Dutchman parameter string differs")
                    require(ra not in mapping or mapping[ra] == tb, "Dutchman string mapping inconsistent")
                    mapping[ra] = tb
                    strings.append({**pair, "size": len(literal), "sha256": digest(literal),
                                    "text": literal[:-1].decode("ascii")})
                for call in calls:
                    require(call["opcode"] == 3 and call["reference_address"] in inside and
                            inside[call["reference_address"]][0] == call["target_address"] != ANCHOR,
                            "Dutchman call does not preserve its complete named parameter helper")
                require(Counter(inside[c["reference_address"]][1] for c in calls) ==
                        {"zParamGetFloat": 111, "zParamGetVector": 6, "zParamGetInt": 3},
                        "Dutchman typed parameter helper usage differs")
                caller_calls = calls
                uniqueness = unique_template(target, original.read(a, size), mask, b)
                sequences.append({"version": version, "proof_scope": "one_complete_body_with_three_complete_helpers",
                    "source": SOURCE, "source_start": a, "target_start": b,
                    "complete_body_bytes": size, "uniqueness": uniqueness,
                    "whole_translation_unit_claimed": False})
            else:
                require(not pairs and len(calls) == 2, "Parameter helper address/call inventory differs")
                for call in calls:
                    ra, tb = call["reference_address"], call["target_address"]
                    require(call["opcode"] == 3, "Parameter helper contains an unexpected tail transfer")
                    if tb in known:
                        checked_identity(original, target, ra, tb, known)
                        expected = {("zParamGetVector", 60): "xStrHash",
                                    ("zParamGetVector", 232): "xStrParseFloatList",
                                    ("zParamGetFloat", 32): "xStrHash",
                                    ("zParamGetFloat", 136): "xatof",
                                    ("zParamGetInt", 32): "xStrHash"}
                        require(expected.get((name, call["offset"])) == known[tb]["name"],
                                "Parameter helper's independently known callee changed")
                    else:
                        require(name == "zParamGetInt" and call["offset"] == 136 and
                                ra == tb == 0x114BD0 and ra not in original.by_address and
                                original.read(a + call["offset"], 4) == target.read(b + call["offset"], 4) and
                                original.read(ra, 64) == target.read(tb, 64),
                                "Unreviewed or changed opaque parameter runtime context")
                        # This literal unchanged transfer is not an inferred relocation.
                        mask.pop(call["offset"], None)
                        call.update(transfer_word_unmasked=True, opaque_context_bytes=64,
                                    opaque_context_sha256=digest(target.read(tb, 64)),
                                    no_identity_or_extent_claim=True)
                        contexts.append({"version": version, "source": source, "caller": name,
                            "reference_address": ra, "target_address": tb, "compared_prefix_bytes": 64,
                            "sha256": digest(target.read(tb, 64)), "promoted_as_named_anchor": False})
            if b not in functions:
                functions[b] = {"name": name, "source": source, "address": b, "size": size,
                    "sha256": digest(target.read(b, size)), "boundary_confirmation": True,
                    "confirmation_kind": CLUSTER_KIND, "provenance": [],
                    "corroboration": {"proof_scope": "complete_caller_callee_cluster",
                        "local_control_flow": bounds, "unique_complete_caller": ANCHOR,
                        "whole_translation_unit_claimed": False,
                        "incoming_cluster_call_offsets": [c["offset"] for c in caller_calls
                                                          if c["target_address"] == b]}}
            record = functions[b]
            require(not record["provenance"] or record["provenance"][0]["linkage_name"] == links[a],
                    "Original Dutchman cluster canonical linkage differs")
            record["provenance"].append({"version": version, "executable_sha1": original.sha1,
                "source_address": a, "name": name, "source": source, "linkage_name": links[a],
                "reference_sha256": digest(original.read(a, size)), "data_address_operands": pairs,
                "direct_transfers": calls})
        require(len(mapping) == len(set(mapping.values())) == 120,
                "Distinct complete Dutchman parameter strings collapse")
        data_proofs.append({"version": version, "source": SOURCE, "complete_strings": strings,
                            "data_extents_promoted": False})
    require(len(functions) == 4 and sum(f["size"] for f in functions.values()) == 16252 and
            all(len(f["provenance"]) == 3 for f in functions.values()),
            "Dutchman cluster requires all four complete bodies in all three originals")
    return {"functions": sorted(functions.values(), key=lambda f: f["address"]),
            "sequence_proofs": sequences, "data_proofs": data_proofs, "call_neighbors": contexts,
            "counts": {"functions": 4, "code_bytes": 16252, "source_units": 2,
                       "closed_return_bodies": 4, "reviewed_complete_caller_callee_clusters": 1}}
