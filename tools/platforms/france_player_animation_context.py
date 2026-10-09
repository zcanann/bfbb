"""Complete, nonpromoted callback contexts for the player animation builders."""
from __future__ import annotations

from platforms.verify_reviewed import require
from platforms.france_corroborated import ControlFlow
from platforms.france_tu_sequences import compare, digest, unique_template, words
from platforms.france_hangable_sequence import hangable_pair
from platforms.france_update_cull_sequence import checked_identity
from platforms.ps2_type_layouts import aggregate_layouts
from platforms.dwarf1 import iter_dies

PLAYER = "SB/Game/zEntPlayer.cpp"
OOB = "SB/Game/zEntPlayerOOBState.cpp"
SOUND = "SB/Core/p2/iSnd.cpp"


def context_pair(body, address, offset, *, allow_return_store=False):
    """Recognize the one LBU in IdleCB's call delay without changing defaults."""
    if words(body)[offset // 4] >> 26 == 36:
        code = words(body)
        require(len(body) == 180 and offset == 0x9c and
                code[0x94 // 4] >> 16 == 0x3c01 and code[0x98 // 4] >> 26 == 3 and
                code[offset // 4] >> 16 == 0x9024,
                "Unreviewed callback byte-address pattern")
        flow = ControlFlow({address + 4*i: w for i, w in enumerate(code)})
        require(not any(address + 0x94 < flow.instruction(address + off).get("target", 0) <= address + offset
                        for off in range(0, len(body), 4)), "Edge bypasses callback byte-address producer")
        low = code[offset // 4] & 65535
        low -= 65536 if low & 32768 else 0
        return 0x94, (((code[0x94 // 4] & 65535) << 16) + low) & 0xffffffff
    return hangable_pair(body, address, offset, allow_return_store=allow_return_store)


class OriginalData:
    def __init__(self, original):
        section = next(s for s in original.metadata["sections"] if s["name"] == ".debug" and s["size"])
        self.debug = original.data[section["offset"]:section["offset"] + section["size"]]
        self.rows = list(iter_dies(self.debug))
        self.by = {off: (tag, owner, attrs) for off, tag, owner, attrs in self.rows}
        self.original = original
        self.contexts = {}
        self.layouts = aggregate_layouts(self.debug, PLAYER, {"zGlobals", "xGlobals", "zPlayerGlobals"})
        require({n: r["size"] for n, r in self.layouts.items()} ==
                {"zGlobals": 8272, "xGlobals": 1792, "zPlayerGlobals": 6464},
                "Player callback global component sizes differ")
        player = next(m for m in self.layouts["zGlobals"]["members"] if m["name"] == "player")
        require(player["offset"] == 1792 and player["type_attributes"].get("7") ==
                self.layouts["zPlayerGlobals"]["die_offset"], "Original player member path differs")
        die = self.layouts["zGlobals"]["die_offset"]
        bases = [a for off, tag, _, a in self.rows if die < off < self.by[die][2][1] and tag == 0x1c]
        require(len(bases) == 1 and bases[0].get(7) == self.layouts["xGlobals"]["die_offset"] and
                bases[0].get(2) == bytes.fromhex("040000000007"), "Original xGlobals base differs")
        self.globals = self.declaration(PLAYER, "globals")
        require(self.globals[1].get(7) == die, "Original globals declaration type differs")
        self.global_fields = {}
        for owner, base in (("xGlobals", 0), ("zGlobals", 0), ("zPlayerGlobals", 1792)):
            for m in self.layouts[owner]["members"]:
                types = m["type_attributes"]
                if m["offset"] is not None and ("5" in types or types.get("8", "").startswith("01")):
                    self.global_fields[base + m["offset"]] = {**m, "owner": owner, "absolute_offset": base + m["offset"]}

    def declaration(self, source, name):
        candidates = [(off, a) for off, tag, owner, a in self.rows if tag in (7, 12) and
                      owner.replace("\\", "/").endswith(source) and a.get(3) == name]
        require(len(candidates) == 1, f"Original context declaration ambiguous: {source} {name}")
        off, a = candidates[0]
        loc = a.get(2)
        require(isinstance(loc, bytes) and len(loc) == 5 and loc[0] == 3,
                "Context declaration has no absolute original address")
        return off, a, int.from_bytes(loc[1:], "little")

    def scalar_member(self, source, name, size, fields):
        off, a, address = self.declaration(source, name)
        die = a.get(7)
        tag, _, typ = self.by[die]
        require(tag in (2, 0x13) and typ.get(11) == size, "Context aggregate extent differs")
        members = {}
        for child, ct, _, ca in self.rows:
            if die < child < typ[1] and ct == 13 and ca.get(3) in fields:
                field = ca[3]
                require(ca.get(5) == 14 and ca.get(2) == b"\x04" + fields[field].to_bytes(4, "little") + b"\x07",
                        "Context scalar member is not the original float path")
                members[field] = {"name": field, "offset": fields[field], "member_die": child, "type": "float"}
        require(set(members) == set(fields), "Context scalar members missing")
        return {"name": name, "reference_address": address, "size": size,
                "declaration_die": off, "type_die": die, "members": members}

    def sound_array(self, name):
        off, a, address = self.declaration(PLAYER, name)
        tag, _, typ = self.by[a[7]]
        desc = typ.get(10)
        require(tag == 1 and typ.get(9) == 0 and isinstance(desc, bytes) and len(desc) == 18 and
                desc[:14] == bytes.fromhex("000a000000000002000000087200"),
                "Original sound array is not three rows")
        element = int.from_bytes(desc[14:], "little")
        etag, _, etyp = self.by[element]
        require(etag == 1 and etyp.get(9) == 0 and
                etyp.get(10) == bytes.fromhex("000a00000000002e0000000855000900"),
                "Original sound row is not uint32[47]")
        return {"name": name, "reference_address": address, "size": 564,
                "declaration_die": off, "type_die": a[7], "element_die": element,
                "shape": [3, 47], "stride": 188, "operand_offset": 184}


def storage(original, address, size, region):
    extent = original._stream_regions[region]
    require(extent["address"] <= address and address + size <= extent["address"] + extent["size"],
            "Complete typed callback context escapes original storage")


def closed(original, target, a, b, size):
    results = []
    for binary, entry in ((original, a), (target, b)):
        flow = ControlFlow({entry + i*4: w for i, w in enumerate(words(binary.read(entry, size + 16)))})
        bounds = flow.bounds(entry, size)
        require(bounds["passes"], f"Complete animation callback context does not close: {entry:#x}")
        results.append(bounds)
    return results[1]


def body_context(original, target, source, name, a, b, size):
    f = original.by_address.get(a)
    require(f is not None and (f["source"], f["name"], f["high"]-a) == (source, name, size),
            "Complete callback context original identity differs")
    pairs, calls, masks = compare(original, target, a, b, size, address_resolver=context_pair)
    bounds = closed(original, target, a, b, size)
    return {"name": name, "source": source, "source_address": a, "target_address": b, "size": size,
            "reference_sha256": digest(original.read(a, size)), "target_sha256": digest(target.read(b, size)),
            "data_address_operands": pairs, "direct_transfers": calls, "local_control_flow": bounds,
            "promoted_as_named_anchor": False}, masks


def oob_context(original, target, a, b, data):
    row, masks = body_context(original, target, OOB, "oob_timer", a, b, 52)
    require(len(row["data_address_operands"]) == 3 and not row["direct_transfers"], "OOB timer operand inventory differs")
    shared = data.scalar_member(OOB, "shared", 112, {"out_time": 28, "reset_time": 36})
    fixed = data.scalar_member(OOB, "fixed", 124, {"reset_time": 4})
    mapped = {}
    fields = []
    for p in row["data_address_operands"]:
        candidates = [(obj, member) for obj in (shared, fixed) for member in obj["members"].values()
                      if p["reference_address"] == obj["reference_address"] + member["offset"]]
        require(len(candidates) == 1 and p["opcode"] == 49, "Unreviewed OOB timer scalar operand")
        obj, member = candidates[0]
        base = p["target_address"] - member["offset"]
        require(mapped.setdefault(obj["name"], base) == base, "OOB context base changes between fields")
        region = "initialized_data" if obj["name"] == "shared" else "runtime_bss"
        storage(original, obj["reference_address"], obj["size"], region)
        storage(target, base, obj["size"], region)
        if obj["name"] == "shared":
            require(original.read(p["reference_address"], 4) == target.read(p["target_address"], 4),
                    "Initialized OOB scalar value differs")
        fields.append({**p, "object": obj["name"], "field": member, "complete_object": obj, "target_base": base})
    require(len(mapped) == 2 and len({(f["object"], f["field"]["name"]) for f in fields}) == 3,
            "OOB context scalar identities collapse")
    # This complete 52-byte leaf has a twenty-byte exact middle run. Search it
    # exhaustively with only its three proven scalar low halves masked; the
    # generic whole-unit helper's 32-byte anchor requirement stays unchanged.
    require(masks == {4: 0xffff0000, 12: 0xffff0000, 36: 0xffff0000},
            "OOB timer has unexpected instruction masks")
    body = original.read(a, 52)
    code = words(body)
    hits = []
    for segment in target.loaded:
        span = target.read(segment["address"], segment["file_size"])
        pos = span.find(body[16:36])
        while pos >= 0:
            begin = pos - 16
            if begin >= 0 and begin % 4 == 0 and begin + 52 <= len(span):
                other = words(span[begin:begin+52])
                if all((x & masks.get(i*4,0xffffffff)) == (y & masks.get(i*4,0xffffffff))
                       for i,(x,y) in enumerate(zip(code,other))):
                    hits.append(segment["address"]+begin)
            pos = span.find(body[16:36],pos+1)
    require(hits == [b], "Complete OOB timer context is not unique")
    row.update(typed_fields=fields, uniqueness={"matching_addresses": hits,
               "unchanged_anchor_offset":16,"unchanged_anchor_bytes":20,"masked_instruction_count":3})
    return row


def sound_context(original, target, a, b, data, known):
    f = original.by_address.get(a)
    require(f is not None and (f["name"], f["source"], f["high"]-a) ==
            ("xSndStop", "SB/Core/x/xSnd.cpp", 8), "Sound wrapper identity differs")
    require(original.word(a) >> 26 == target.word(b) >> 26 == 2 and
            original.word(a+4) == target.word(b+4) == 0, "Sound wrapper is not exactly J/NOP")
    ra, tb = (original.word(a) & 0x3ffffff)*4, (target.word(b) & 0x3ffffff)*4
    row, masks = body_context(original, target, SOUND, "iSndStop", ra, tb, 184)
    require(len(row["data_address_operands"]) == 2 and len(row["direct_transfers"]) == 1,
            "Platform sound stop inventory differs")
    call = row["direct_transfers"][0]
    callee = checked_identity(original, target, call["reference_address"], call["target_address"], known)
    require((callee["source"], callee["name"], callee["size"], call["opcode"], call["offset"]) ==
            ("SB/Core/p2/his/HISAPI.cpp", "HISStopVoiceAsync", 56, 3, 52),
            "Platform sound stop known callee differs")
    layouts = aggregate_layouts(data.debug, SOUND, {"xSndGlobals", "xSndVoiceInfo"})
    require(layouts["xSndVoiceInfo"]["size"] == 100, "Original sound voice stride differs")
    fields = {m["name"]: m for m in layouts["xSndVoiceInfo"]["members"]}
    require(fields["flags"]["offset"] == 20 and fields["sndID"]["offset"] == 4 and
            fields["flags"]["type_attributes"] == fields["sndID"]["type_attributes"] == {"5": 9},
            "Original sound voice unsigned fields differ")
    voice = next(m for m in layouts["xSndGlobals"]["members"] if m["name"] == "voice")
    require(voice["offset"] == 28, "Original sound voice array offset differs")
    tag, _, typ = data.by[voice["type_attributes"]["7"]]
    desc = typ.get(10)
    require(tag == 1 and typ.get(9) == 0 and isinstance(desc, bytes) and len(desc) == 18 and
            desc[:14] == bytes.fromhex("000a00000000002f000000087200") and
            int.from_bytes(desc[14:], "little") == layouts["xSndVoiceInfo"]["die_offset"],
            "Original sound voice array is not 48 complete records")
    off, declaration, base = data.declaration(SOUND, "gSnd")
    require(declaration.get(7) == layouts["xSndGlobals"]["die_offset"], "Original gSnd type differs")
    pairs = row["data_address_operands"]
    require([p["reference_address"]-base for p in pairs] == [0,48] and
            all(p["opcode"] == 9 and p["storage"] == "zero_fill" for p in pairs) and
            pairs[1]["target_address"] - pairs[0]["target_address"] == 48,
            "Sound voice base/flags operands differ")
    size = layouts["xSndGlobals"]["size"]
    for binary, address in ((original, base), (target, pairs[0]["target_address"])):
        storage(binary, address, size, "runtime_bss")
    row.update(typed_global={"name": "gSnd", "reference_address": base,
               "target_address": pairs[0]["target_address"], "size": size,
               "declaration_die": off, "layouts": layouts},
               uniqueness=unique_template(target, original.read(ra, 184), masks, tb))
    return {"name": "xSndStop", "source_address": a, "target_address": b, "size": 8,
            "reference_sha256": digest(original.read(a,8)), "target_sha256": digest(target.read(b,8)),
            "exact_j_nop": True, "complete_platform_callee": row, "promoted_as_named_anchor": False}


def callback_context(original, target, a, b, known, data):
    f = original.by_address.get(a)
    require(f is not None and f["source"] == PLAYER, "Unexpected player animation callback owner")
    row, masks = body_context(original, target, PLAYER, f["name"], a, b, f["high"]-a)
    globals_base = data.globals[2]
    global_target = 0x52c4f0
    arrays = [data.sound_array(n) for n in ("sPlayerSnd", "sPlayerSndID")] if f["name"] == "IdleCB" else []
    typed, bases = [], {}
    for p in row["data_address_operands"]:
        offset = p["reference_address"]-globals_base
        if offset in data.global_fields:
            field = data.global_fields[offset]
            types = field["type_attributes"]
            expected = {49} if types.get("5") == 14 else {36} if types.get("5") in (1,2,3) else {35,43}
            require(p["opcode"] in expected and p["target_address"] == global_target + offset and p["storage"] == "zero_fill",
                    "Original callback global field/type mapping differs")
            for binary, base in ((original, globals_base), (target, global_target)):
                storage(binary, base, 8272, "runtime_bss")
            typed.append({**p, "global": "globals", "field": field, "target_base": global_target})
        else:
            matches = [ar for ar in arrays if p["reference_address"] == ar["reference_address"] + 184]
            require(len(matches) == 1 and p["opcode"] == 9 and p["storage"] == "zero_fill",
                    f"Unreviewed callback typed operand: {f['name']} {p['reference_address']:#x}")
            ar = matches[0]
            base = p["target_address"] - 184
            require(bases.setdefault(ar["name"], base) == base, "Sound callback array mapping inconsistent")
            for binary, address in ((original, ar["reference_address"]), (target, base)):
                storage(binary, address, ar["size"], "runtime_bss")
            typed.append({**p, "array": ar, "target_base": base, "first_index": [0,46]})
    if arrays:
        require(set(bases) == {a["name"] for a in arrays} and
                bases["sPlayerSndID"] - bases["sPlayerSnd"] ==
                arrays[1]["reference_address"] - arrays[0]["reference_address"] == 1152,
                "Original sound arrays collapse or lose separation")
    neighbors = []
    for c in row["direct_transfers"]:
        ref = original.by_address.get(c["reference_address"])
        require(c["opcode"] == 3 and ref is not None, "Callback has an unreviewed external transfer")
        if (ref["source"], ref["name"]) == (OOB, "oob_timer"):
            key = (c["reference_address"],c["target_address"])
            if key not in data.contexts:
                data.contexts[key] = oob_context(original,target,*key,data)
            neighbors.append(data.contexts[key])
        elif (ref["source"], ref["name"]) == ("SB/Core/x/xSnd.cpp", "xSndStop"):
            key = (c["reference_address"],c["target_address"])
            if key not in data.contexts:
                data.contexts[key] = sound_context(original,target,*key,data,known)
            neighbors.append(data.contexts[key])
        else:
            callee = checked_identity(original,target,c["reference_address"],c["target_address"],known)
            require((callee["source"],callee["name"]) == ("SB/Core/x/xPad.cpp","xPadDestroyRumbleChain"),
                    "Unexpected independent callback callee")
    row.update(typed_data=typed, complete_callee_contexts=neighbors,
               identity_root="unique_complete_builder_code_pointer", data_extents_promoted=False)
    return row
