"""Original-only French Hangable TU proof, anchored by the existing Setup body."""
from __future__ import annotations

import json

from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow
from platforms.france_tu_sequences import (KIND, compare, cstring, digest, gpr_writes,
                                          named_data, unique_template, words)
from platforms.france_update_cull_sequence import checked_identity
from platforms.ps2_source import canonical_linkages
from platforms.dwarf1 import iter_dies
from platforms.ps2_type_layouts import aggregate_layouts

SOURCE = "SB/Game/zEntHangable.cpp"
START = 0x136EE0
MEMBERS = [("zEntHangable_FollowUpdate", 376), ("zEntHangable_Reset", 84),
           ("zEntHangable_Load", 8), ("zEntHangable_Save", 8),
           ("zEntHangable_SetMatrix", 1612), ("zEntHangableEventCB", 812),
           ("zEntHangable_Update", 1080), ("zEntHangable_UpdateFX", 448),
           ("zEntHangable_Init", 104), ("HangableSetup", 252),
           ("zEntHangable_SetupFX", 84)]


def hangable_gpr_writes(word):
    # Observed single-precision arithmetic/comparisons write FPR/ACC/FCC state,
    # never a GPR. MFC/CFC transfer encodings still use the strict shared decoder.
    if (word >> 26 == 17 and word >> 21 & 31 == 16 and
            word & 63 in (0, 1, 2, 3, 4, 6, 7, 24, 25, 26, 28, 29, 48, 50, 60, 62)):
        return set()
    return gpr_writes(word)


def hangable_pair(body, address, offset, *, allow_return_store=False):
    """Strict reaching-LUI proof with the TU's explicitly decoded COP1 effects."""
    code = words(body)
    low = code[offset // 4]
    register = low >> 21 & 31
    require(low >> 26 in (9, 35, 43, 49, 57) and register not in (0, 31),
            "Hangable data operand has no ordinary address base")
    high_offset = None
    for off in range(offset - 4, -1, -4):
        if register in hangable_gpr_writes(code[off // 4]):
            require(code[off // 4] >> 26 == 15 and code[off // 4] >> 21 & 31 == 0,
                    "Hangable address base is not produced by LUI")
            high_offset = off
            break
    require(high_offset is not None, "Hangable address has no reaching LUI")
    flow = ControlFlow({address + i * 4: w for i, w in enumerate(code)})
    if high_offset >= 4:
        previous = flow.instruction(address + high_offset - 4)
        require(previous["kind"] == "normal" or
                previous["kind"] == "branch" and not previous.get("likely"),
                "Hangable LUI occurs in an unproved delay slot")
    for off in range(high_offset + 4, offset, 4):
        instruction = flow.instruction(address + off)
        require(instruction["kind"] in ("normal", "branch") or
                instruction["kind"] == "call" and off == offset - 4,
                "Transfer interrupts Hangable address lifetime")
    for off in range(0, len(body), 4):
        instruction = flow.instruction(address + off)
        destination = instruction.get("target")
        if destination is not None and address + high_offset < destination <= address + offset:
            require(off == high_offset - 4 and instruction["kind"] == "branch",
                    "A direct edge bypasses the Hangable LUI definition")
    immediate = (low & 65535) - (65536 if low & 32768 else 0)
    return high_offset, (((code[high_offset // 4] & 65535) << 16) + immediate) & 0xffffffff


def original_data(original):
    section = next(s for s in original.metadata["sections"] if s["name"] == ".debug" and s["size"])
    debug = original.data[section["offset"]:section["offset"] + section["size"]]
    names = {"zGlobals", "xGlobals", "zPlayerGlobals", "zEnt", "xEnt", "xBase",
             "xBound", "xCylinder", "xVec3"}
    layouts = aggregate_layouts(debug, SOURCE, names)
    expected = {"zGlobals": (8272, {"player": 1792, "sceneCur": 8264}),
                "xGlobals": (1792, {"updateMgr": 1604}),
                "zPlayerGlobals": (6464, {"ent": 0, "HangEnt": 4976, "HangPivot": 4984}),
                "zEnt": (212, {"atbl": 208}),
                "xEnt": (208, {"model": 36, "bound": 100}),
                "xBase": (16, {}), "xBound": (76, {"cyl": 36}),
                "xCylinder": (20, {"r": 12}), "xVec3": (12, {"x": 0, "y": 4, "z": 8})}
    for name, (size, fields) in expected.items():
        layout = layouts[name]
        offsets = {m["name"]: m["offset"] for m in layout["members"]}
        require(layout["size"] == size and all(offsets.get(k) == v for k, v in fields.items()),
                "Original Hangable consumer/global component layout changed")
    rows = list(iter_dies(debug))
    by = {off: (tag, owner, attrs) for off, tag, owner, attrs in rows}
    for derived, parent in (("zGlobals", "xGlobals"), ("zEnt", "xEnt"), ("xEnt", "xBase")):
        die = layouts[derived]["die_offset"]
        bases = [attrs for off, tag, _, attrs in rows if die < off < by[die][2][1] and tag == 0x1c]
        require(len(bases) == 1 and bases[0].get(7) == layouts[parent]["die_offset"] and
                bases[0].get(2) == bytes.fromhex("040000000007"),
                "Original Hangable consumer inheritance is not at offset zero")
    globals_decl = [(off, attrs) for off, tag, owner, attrs in rows if tag in (7, 12) and
                    owner.replace("\\", "/").endswith(SOURCE) and attrs.get(3) == "globals"]
    require(len(globals_decl) == 1 and globals_decl[0][1].get(7) == layouts["zGlobals"]["die_offset"],
            "Original globals declaration lacks its zGlobals type")
    local = [(off, attrs) for off, tag, owner, attrs in rows if tag == 12 and
             owner.replace("\\", "/").endswith(SOURCE) and attrs.get(3) == "offset_rlii0006"]
    require(len(local) == 1, "Original Hangable circle array declaration is ambiguous")
    array_die = local[0][1].get(7)
    tag, _, attrs = by[array_die]
    descriptor = attrs.get(10)
    require(tag == 1 and attrs.get(9) == 0 and isinstance(descriptor, bytes) and
            len(descriptor) == 18 and descriptor[:14] == bytes.fromhex("000a000000000007000000087200") and
            int.from_bytes(descriptor[14:], "little") == layouts["xVec3"]["die_offset"],
            "Original circle is not the eight-element xVec3 array")
    return {"reference_address": named_data(original, "globals", SOURCE), "size": 8272,
            "declaration_die": globals_decl[0][0], "layouts": layouts,
            "circle_array": {"declaration_die": local[0][0], "type_die": array_die,
                             "count": 8, "element_size": 12, "size": 96,
                             "subscript_descriptor": descriptor.hex()}}


def checked_tail(original, target, reference, address, name, known):
    """Two exact J/NOP wrappers terminate in an independently known xEnt body."""
    operation = "Load" if name == "zEntHangable_Load" else "Save"
    require(name in ("zEntHangable_Load", "zEntHangable_Save"), "Unreviewed Hangable tail")
    r, t, steps = reference, address, []
    for expected_name, expected_source in ((name, SOURCE), ("zEnt" + operation, "SB/Game/zEnt.cpp")):
        function = original.by_address.get(r)
        require(function is not None and
                (function["name"], function["source"], function["high"] - r) ==
                (expected_name, expected_source, 8), "Original Hangable tail-chain identity changed")
        destinations = []
        for binary, entry in ((original, r), (target, t)):
            word = binary.word(entry)
            require(word >> 26 == 2 and binary.word(entry + 4) == 0,
                    "Hangable tail is not exactly J/NOP")
            destinations.append(((entry + 4) & 0xf0000000) | ((word & 0x3ffffff) << 2))
        steps.append({"reference_name": expected_name, "reference_source": expected_source,
                      "reference_address": r, "target_address": t, "size": 8,
                      "target_sha256": digest(target.read(t, 8)),
                      "reference_sha256": digest(original.read(r, 8)),
                      "promoted_as_named_anchor": False})
        r, t = destinations
    callee = checked_identity(original, target, r, t, known)
    require((callee["name"], callee["source"]) == ("xEnt" + operation, "SB/Core/x/xEnt.cpp"),
            "Hangable tail does not terminate in the corresponding independently known entity helper")
    for binary, entry in ((original, r), (target, t)):
        flow = ControlFlow({entry + 4 * i: w for i, w in enumerate(words(binary.read(entry, callee["size"] + 16)))})
        require(flow.bounds(entry, callee["size"])["passes"], "Independent entity tail callee does not close")
    return {"passes": True, "boundary_kind": "reviewed-two-j-nop-tail-chain",
            "unchanged_sp_ra": True, "context_only_wrappers": steps,
            "terminal_target_name": callee["name"], "terminal_target_address": t,
            "terminal_size": callee["size"], "both_original_terminal_bodies_closed": True}


def generate_unit(originals, registry_dir):
    target = originals[TARGET]
    known = {}
    for path in sorted(registry_dir.glob("*functions.json")):
        for f in json.loads(path.read_text())["functions"]:
            if f["source"] == SOURCE and f["name"] != "HangableSetup":
                continue  # Regeneration must not prove new members using itself.
            if f["address"] in known:
                require(all(known[f["address"]][k] == f[k] for k in ("name", "source", "size", "sha256")),
                        "Conflicting independent Hangable context")
            known[f["address"]] = f
    anchors = [f for f in known.values() if f["source"] == SOURCE]
    require(len(anchors) == 1 and (anchors[0]["name"], anchors[0]["address"], anchors[0]["size"]) ==
            ("HangableSetup", 0x1380D0, 252), "Expected the independently confirmed Setup anchor")
    functions, sequences, data_proofs, contexts = {}, [], [], []
    # External entry prefixes corroborate relationships only. They do not become
    # recovered functions, named relocation anchors or source-comparison units.
    prefix_identities = {("xVec3Normalize", "SB/Core/x/xVec3.cpp", 224),
                         ("zEntEvent", "SB/Core/x/xEvent.cpp", 32),
                         ("xParEmitterEmitCustom", "SB/Core/x/xParEmitter.cpp", 1104),
                         ("zEntInit", "SB/Game/zEnt.cpp", 1880),
                         ("zParEmitterFind", "SB/Game/zParEmitter.cpp", 120)}
    for version in REFERENCES:
        original = originals[version]
        members = sorted((f for f in original.functions if f["source"] == SOURCE), key=lambda f: f["low"])
        require([(f["name"], f["high"] - f["low"]) for f in members] == MEMBERS,
                "Original complete Hangable membership changed")
        low, end = members[0]["low"], members[-1]["high"]
        require(end - low == 4932 and sum(size for _, size in MEMBERS) == 4868,
                "Original complete Hangable sequence size changed")
        inside = {f["low"]: f for f in members}
        shift = START - low
        setup = next(f for f in members if f["name"] == "HangableSetup")
        checked_identity(original, target, setup["low"], setup["low"] + shift, known)
        linkages = canonical_linkages(original.data, original.metadata)
        data = original_data(original)
        masks, pairmap, callee_map, globals_bases, data_rows = {}, {}, {}, set(), []
        prefix_seen = set()
        callbacks = set()
        strings, circle = [], []

        def data_operand(pair, name, reference_function):
            ra, tb = pair["reference_address"], pair["target_address"]
            require(ra not in pairmap or pairmap[ra] == tb, "Hangable data mapping is inconsistent")
            pairmap[ra] = tb
            if pair["storage"] == "zero_fill":
                offset = ra - data["reference_address"]
                # Named original component offsets, including sceneCur used by
                # the nonpromoted zParEmitterFind entry context.
                allowed = {1604: "updateMgr", 1828: "player.ent.model", 1940: "player.ent.bound.cyl.r",
                           6768: "player.HangEnt", 6776: "player.HangPivot.x", 6780: "player.HangPivot.y",
                           6784: "player.HangPivot.z", 8264: "sceneCur"}
                require(offset in allowed, "Unreviewed Hangable globals component")
                base = tb - offset
                globals_bases.add(base)
                for binary, address in ((original, data["reference_address"]), (target, base)):
                    region = binary._stream_regions["runtime_bss"]
                    require(region["address"] <= address and address + data["size"] <= region["address"] + region["size"],
                            "Complete original typed globals object escapes BSS")
                data_rows.append({"kind": "typed_globals_component", "component": allowed[offset],
                                  "offset": offset, **pair})
            elif tb in known and known[tb]["source"] == "SB/Core/x/xUpdateCull.cpp":
                callback = checked_identity(original, target, ra, tb, known)
                require(pair["opcode"] == 9 and name == "zEntHangableEventCB" and callback["name"] in
                        ("xUpdateCull_AlwaysTrueCB", "xUpdateCull_DistanceSquaredCB"),
                        "Hangable callback address changed independently reviewed ownership")
                callbacks.add(callback["name"])
                data_rows.append({"kind": "independently_verified_callback", "name": callback["name"], **pair})
            else:
                for binary, address in ((original, ra), (target, tb)):
                    region = binary._stream_regions["initialized_data"]
                    require(region["address"] <= address < region["address"] + region["size"],
                            "Hangable immutable operand is outside initialized data")
                if name == "zEntHangable_SetupFX":
                    first, second = cstring(original, ra), cstring(target, tb)
                    require(first == second, "Hangable emitter/hash string bytes differ")
                    strings.append(ra)
                    data_rows.append({"kind": "complete_equal_cstring", "size": len(first),
                                      "sha256": digest(first), **pair})
                else:
                    require(name == "zEntHangable_UpdateFX" and pair["opcode"] == 9,
                            "Unreviewed Hangable immutable operand")
                    code = words(original.read(reference_function["low"], reference_function["high"] - reference_function["low"]))
                    base_reg = code[pair["lo_offset"] // 4] >> 16 & 31
                    loads = [(w & 65535) for w in code if w >> 26 == 30 and w >> 21 & 31 == base_reg]
                    require(sorted(loads) == list(range(0, 96, 16)),
                            "Original typed circle template is not copied by exactly six 16-byte loads")
                    require(original.read(ra, 96) == target.read(tb, 96),
                            "Complete 96-byte original circle initializer differs")
                    circle.append(ra)
                    data_rows.append({"kind": "complete_typed_array_initializer", "size": 96,
                                      "sha256": digest(original.read(ra, 96)), **pair})

        for index, f in enumerate(members):
            a, b, size, name = f["low"], f["low"] + shift, f["high"] - f["low"], f["name"]
            require(a in linkages and a % 16 == b % 16 == 0, "Original Hangable entry linkage/alignment changed")
            if index:
                gap = a - members[index - 1]["high"]
                require(0 <= gap < 16 and not any(original.read(a - gap, gap)) and not any(target.read(b - gap, gap)),
                        "Unrelated bytes or nonzero alignment interrupt the complete Hangable sequence")
            pairs, calls, mask = compare(original, target, a, b, size, address_resolver=hangable_pair)
            for binary, entry in ((original, a), (target, b)):
                flow = ControlFlow({entry + i * 4: w for i, w in enumerate(words(binary.read(entry, size + 16)))})
                bounds = flow.bounds(entry, size)
                if not bounds["passes"]:
                    require(size == 8, "Only the two reviewed Hangable tail wrappers may leave their extents")
                    bounds = checked_tail(original, target, a, b, name, known)
            for pair in pairs:
                data_operand(pair, name, f)
            for call in calls:
                ra, tb = call["reference_address"], call["target_address"]
                require(ra not in callee_map or callee_map[ra] == tb, "Hangable callee mapping is inconsistent")
                callee_map[ra] = tb
                if ra in inside:
                    require(tb == ra + shift, "Hangable internal call changes complete member ownership")
                elif tb in known:
                    checked_identity(original, target, ra, tb, known)
                elif size == 8:
                    # The complete two-wrapper chain was just independently
                    # checked through a known, closed xEnt terminal above.
                    require(call["offset"] == 0 and call["opcode"] == 2, "Unexpected Hangable tail call")
                elif ra not in prefix_seen:
                    ref = original.by_address.get(ra)
                    require(ref is not None and (ref["name"], ref["source"], ref["high"] - ra) in prefix_identities,
                            "Unreviewed Hangable external entry context")
                    cpairs, ccalls, _ = compare(original, target, ra, tb, 32, address_resolver=hangable_pair)
                    for pair in cpairs:
                        data_operand(pair, ref["name"], ref)
                    for nested in ccalls:
                        if nested["target_address"] in known:
                            checked_identity(original, target, nested["reference_address"], nested["target_address"], known)
                        else:
                            # The one unpromoted zEntEvent tail is preserved as
                            # context, including a literal equal 32-byte callee prefix.
                            require(ref["name"] == "zEntEvent" and nested["offset"] == 24 and
                                    nested["opcode"] == 2 and original.read(nested["reference_address"], 32) ==
                                    target.read(nested["target_address"], 32),
                                    "Unreviewed transfer inside external Hangable entry context")
                    contexts.append({"version": version, "reference_name": ref["name"],
                        "reference_source": ref["source"], "reference_address": ra, "target_address": tb,
                        "size": ref["high"] - ra, "compared_entry_prefix_bytes": 32,
                        "reference_sha256": digest(original.read(ra, 32)),
                        "target_sha256": digest(target.read(tb, 32)),
                        "data_address_operands": cpairs, "direct_transfers": ccalls,
                        "promoted_as_named_anchor": False})
                    prefix_seen.add(ra)
            masks.update({a - low + off: value for off, value in mask.items()})
            if name == "HangableSetup":
                continue
            if b not in functions:
                functions[b] = {"name": name, "source": SOURCE, "address": b, "size": size,
                    "sha256": digest(target.read(b, size)), "boundary_confirmation": True,
                    "confirmation_kind": KIND, "provenance": [],
                    "corroboration": {"local_control_flow": bounds,
                                      "previously_confirmed_neighbor_entries": [anchors[0]["address"]]}}
            record = functions[b]
            require((record["name"], record["size"]) == (name, size) and (not record["provenance"] or
                    record["provenance"][0]["linkage_name"] == linkages[a]), "Original Hangable identities disagree")
            record["provenance"].append({"version": version, "executable_sha1": original.sha1,
                "source_address": a, "name": name, "source": SOURCE, "linkage_name": linkages[a],
                "reference_sha256": digest(original.read(a, size)), "data_address_operands": pairs,
                "direct_transfers": calls})
        require(len(globals_bases) == 1 and len(set(pairmap.values())) == len(pairmap) and
                len(set(callee_map.values())) == len(callee_map), "Distinct Hangable data/callees collapse")
        require(len(strings) == 4 and len(circle) == 1 and callbacks ==
                {"xUpdateCull_AlwaysTrueCB", "xUpdateCull_DistanceSquaredCB"},
                "Original Hangable strings, circle initializer or callbacks changed")
        sequences.append({"version": version, "source": SOURCE, "source_start": low,
            "target_start": START, "complete_original_members": 11, "previously_confirmed_members": 1,
            "sequence_bytes_with_alignment": 4932,
            "uniqueness": unique_template(target, original.read(low, end - low), masks, START)})
        data_proofs.append({"version": version, "source": SOURCE, "typed_original_globals": data,
                           "target_globals_address": next(iter(globals_bases)), "operands": data_rows,
                           "data_extents_promoted": False})
    require(len(functions) == 10 and sum(f["size"] for f in functions.values()) == 4616 and
            all(len(f["provenance"]) == 3 for f in functions.values()),
            "All ten missing Hangable members require all three originals")
    context_groups = {}
    for context in contexts:
        context_groups.setdefault(context["target_address"], []).append(context)
    for group in context_groups.values():
        require(len(group) == 3 and {r["version"] for r in group} == set(REFERENCES) and
                len({(r["reference_name"], r["reference_source"], r["size"]) for r in group}) == 1,
                "Hangable external contexts disagree across originals")
    return {"functions": sorted(functions.values(), key=lambda f: f["address"]),
            "sequence_proofs": sequences, "data_proofs": data_proofs, "call_neighbors": contexts,
            "counts": {"functions": 10, "code_bytes": 4616, "source_units": 1,
                       "closed_return_bodies": 8, "reviewed_hangable_tail_chains": 2}}
