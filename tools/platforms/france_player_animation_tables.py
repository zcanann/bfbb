"""Prove five complete player animation builders using original-only operands.

The main SpongeBob builder still requires insertion-helper and pointer-table
evidence and is deliberately excluded from this scoped cluster proof.
"""
from __future__ import annotations

from collections import Counter
import json

from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow
from platforms.france_tu_sequences import compare, digest, unique_template, words
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages

SOURCE = "SB/Game/zEntPlayerAnimationTables.h"
# name, target entry, original complete size, literal and code-pointer counts,
# complete known call inventory (New, NewState, NewTransition, AddTransition).
MEMBERS = [
    ("zEntPlayer_TreeDomeSBAnimTable", 0x159F20, 6536, 91, 90, (1, 90, 0, 0)),
    ("zEntPlayer_BoulderVehicleAnimTable", 0x15B8B0, 352, 7, 2, (1, 2, 2, 0)),
    ("zSpongeBobTongue_AnimTable", 0x15BA10, 560, 8, 7, (1, 7, 0, 0)),
    ("zPatrick_AnimTable", 0x161880, 11056, 243, 56, (1, 56, 87, 12)),
    ("zSandy_AnimTable", 0x1643B0, 14400, 315, 61, (1, 61, 127, 0)),
]
CALLEES = ("xAnimTableNew", "xAnimTableNewState", "xAnimTableNewTransition", "xAnimTableAddTransition")


def animation_literal(original, address):
    """Read a complete bounded ASCII animation expression, including long lists."""
    value = bytearray()
    for offset in range(4096):
        byte = original.read(address + offset, 1)
        require(byte == b"\0" or 32 <= byte[0] <= 126, "Animation expression is not printable ASCII")
        value.extend(byte)
        if byte == b"\0":
            return bytes(value)
    raise ValueError("Animation expression lacks a bounded terminator")


def generate_unit(originals, registry_dir):
    target = originals[TARGET]
    known = {}
    for path in sorted(registry_dir.glob("*functions.json")):
        for row in json.loads(path.read_text())["functions"]:
            if row["source"] == SOURCE:
                continue  # Future table proofs cannot bootstrap this replay.
            prior = known.setdefault(row["address"], row)
            require(all(prior[k] == row[k] for k in ("name", "source", "size", "sha256")),
                    "Conflicting independent animation-table context")
    records, sequences, data_proofs = {}, [], []
    # German relocates these player callbacks while USA/PAL keep their address
    # words literal. Both large builders root the complete nonpromoted contexts.
    from platforms.france_player_animation_context import OriginalData, callback_context, PLAYER
    callbacks = {}
    german = originals["SLES-51970"]
    for name, b, size, *_ in MEMBERS[3:]:
        f = next(f for f in german.functions if (f["source"],f["name"]) == (SOURCE,name))
        pairs, _, _ = compare(german,target,f["low"],b,size)
        for p in pairs:
            ref = german.by_address.get(p["reference_address"])
            if ref is not None and ref["source"] == PLAYER:
                identity = (ref["name"],ref["high"]-ref["low"])
                require(callbacks.setdefault(p["target_address"],identity) == identity,
                        "Player callback identities collapse")
    require(len(callbacks) == 34, "Complete builder callback inventory differs")
    for version in REFERENCES:
        original = originals[version]
        links = canonical_linkages(original.data, original.metadata)
        owned = [f for f in original.functions if f["source"] == SOURCE]
        require(len(owned) == 6 and sum(f["high"] - f["low"] for f in owned) == 56516,
                "Original six-builder animation ownership differs")
        mapping = {}
        context_data = OriginalData(original)
        contexts = {}
        for entry,(name,size) in sorted(callbacks.items()):
            refs = [f for f in original.functions if (f["source"],f["name"],f["high"]-f["low"]) == (PLAYER,name,size)]
            require(len(refs) == 1, "Original full player callback identity is ambiguous")
            contexts[entry] = callback_context(original,target,refs[0]["low"],entry,known,context_data)
        data_proofs.append({"version":version,"source":SOURCE,
                            "complete_nonpromoted_player_callback_contexts":list(contexts.values()),
                            "data_extents_promoted":False})
        for name, b, size, literal_count, pointer_count, inventory in MEMBERS:
            if version == "SLES-51970":
                pointer_count += {"zPatrick_AnimTable":67,"zSandy_AnimTable":83}.get(name,0)
            matches = [f for f in owned if f["name"] == name]
            require(len(matches) == 1 and matches[0]["high"] - matches[0]["low"] == size,
                    "Complete original animation-builder extent differs")
            a = matches[0]["low"]
            require(a in links and a % 16 == b % 16 == 0, "Animation-builder linkage/alignment differs")
            pairs, calls, masks = compare(original, target, a, b, size)
            require(len(pairs) == literal_count + pointer_count and len(calls) == sum(inventory),
                    f"Complete animation-builder operand inventory differs: {version} {name}: "
                    f"{len(pairs)} pairs, {len(calls)} calls")
            strings, pointers = [], []
            for pair in pairs:
                ra, tb = pair["reference_address"], pair["target_address"]
                require(pair["opcode"] == 9 and pair["storage"] == "file_backed",
                        "Animation-builder pointer has an unreviewed operand kind")
                require(ra not in mapping or mapping[ra] == tb, "Animation-builder address mapping inconsistent")
                mapping[ra] = tb
                if ra in original.by_address:
                    if tb in contexts:
                        context = contexts[tb]
                        require(context["source_address"] == ra,
                                "Builder pointer does not address its complete original callback context")
                        callback = {**context,"sha256":context["target_sha256"]}
                    else:
                        callback = checked_identity(original, target, ra, tb, known)
                        require((callback["source"], callback["name"]) ==
                                ("SB/Core/x/xAnim.cpp", "xAnimDefaultBeforeEnter"),
                                "Animation-builder callback differs from its independently known original")
                    pointers.append({**pair, "name": callback["name"], "size": callback["size"],
                                     "complete_target_sha256": callback["sha256"]})
                else:
                    value = animation_literal(original, ra)
                    require(value == animation_literal(target, tb), "Complete animation expression differs")
                    strings.append({**pair, "size": len(value), "sha256": digest(value),
                                    "text": value[:-1].decode("ascii")})
            require(len(strings) == literal_count and len(pointers) == pointer_count and
                    len(mapping) == len(set(mapping.values())), "Animation literal/code-pointer identity collapses")
            callee_names = []
            for call in calls:
                require(call["opcode"] == 3, "Animation builder contains an unexpected tail transfer")
                callee = checked_identity(original, target, call["reference_address"], call["target_address"], known)
                require(callee["source"] == "SB/Core/x/xAnim.cpp", "Animation builder calls an unreviewed source")
                callee_names.append(callee["name"])
            require(Counter(callee_names) == {k: n for k, n in zip(CALLEES, inventory) if n},
                    "Animation-builder complete callee inventory differs")
            for binary, entry in ((original, a), (target, b)):
                flow = ControlFlow({entry + i * 4: w for i, w in enumerate(words(binary.read(entry, size + 16)))})
                bounds = flow.bounds(entry, size)
                require(bounds["passes"], "Animation builder lacks complete closed original bounds")
            unique = unique_template(target, original.read(a, size), masks, b)
            if b not in records:
                scope = ("complete_caller_with_proven_callback_contexts_and_independently_proven_callees"
                         if name in ("zPatrick_AnimTable", "zSandy_AnimTable") else
                         "complete_caller_with_independently_proven_callees_and_callback")
                records[b] = {"name": name, "source": SOURCE, "address": b, "size": size,
                    "sha256": digest(target.read(b, size)), "boundary_confirmation": True,
                    "confirmation_kind": CLUSTER_KIND, "provenance": [],
                    "corroboration": {"proof_scope": scope,
                        "unique_complete_caller": b, "local_control_flow": bounds,
                        "whole_translation_unit_claimed": False}}
            record = records[b]
            require(not record["provenance"] or record["provenance"][0]["linkage_name"] == links[a],
                    "Animation-builder canonical linkage differs across originals")
            record["provenance"].append({"version": version, "executable_sha1": original.sha1,
                "source_address": a, "source": SOURCE, "name": name, "linkage_name": links[a],
                "reference_sha256": digest(original.read(a, size)), "data_address_operands": pairs,
                "direct_transfers": calls})
            sequences.append({"version": version, "source": SOURCE, "name": name, "source_start": a,
                "target_start": b, "complete_body_bytes": size, "uniqueness": unique,
                "whole_translation_unit_claimed": False})
            data_proofs.append({"version": version, "source": SOURCE, "name": name,
                "complete_strings": strings, "known_code_pointers": pointers, "data_extents_promoted": False})
    require(len(records) == 5 and all(len(f["provenance"]) == 3 for f in records.values()),
            "Animation subset requires all five bodies from all three originals")
    return {"functions": sorted(records.values(), key=lambda f: f["address"]),
            "sequence_proofs": sequences, "data_proofs": data_proofs,
            "counts": {"functions": 5, "code_bytes": 32904, "source_units": 1,
                       "closed_return_bodies": 5, "reviewed_complete_caller_callee_clusters": 5}}
