"""Original-only CruiseBubble animation insertion and bounded callback cluster."""
from __future__ import annotations

import json
from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_tu_sequences import compare, digest, unique_template, words
from platforms.france_player_animation_tables import animation_literal
from platforms.france_player_animation_context import closed, storage
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages
from platforms.dwarf1 import iter_dies

SOURCE = "SB/Game/zEntCruiseBubble.cpp"
MEMBERS = [("insert_player_animations",0x2a09e0,744),
           ("load_cheat_tweak",0x2a4410,396),("check_anim_aim",0x2a45a0,8)]
PARAMETERS = 0x2a1540
OPAQUE_RUNTIME = {0x118c08,0x11b938,0x11be58}


class TypedObjects:
    def __init__(self, original):
        section = next(s for s in original.metadata["sections"] if s["name"] == ".debug" and s["size"])
        self.debug = original.data[section["offset"]:section["offset"]+section["size"]]
        self.rows = list(iter_dies(self.debug))
        self.by = {off:(tag,owner,a) for off,tag,owner,a in self.rows}
        self.next = {a[0]:b[0] for a,b in zip(self.rows,self.rows[1:])}

    def declaration(self, name):
        matches = [(off,a) for off,tag,owner,a in self.rows if tag in (7,12) and
                   owner.replace("\\","/").endswith(SOURCE) and a.get(3) == name]
        require(len(matches) == 1, "Original animation helper declaration ambiguous")
        off,a = matches[0]
        loc = a.get(2)
        require(isinstance(loc,bytes) and len(loc) == 5 and loc[0] == 3,
                "Animation helper data has no original absolute location")
        return off,a,int.from_bytes(loc[1:],"little")

    def leaves(self, die, prefix=(), base=0, ancestors=()):
        require(die not in ancestors and len(ancestors) < 10, "Recursive animation helper value type")
        tag,_,a = self.by[die]
        require(tag in (2,0x13) and a.get(11), "Animation helper value is not a complete aggregate")
        result = {}
        cursor,end = self.next[die],a[1]
        while cursor < end:
            if cursor not in self.by:
                length = int.from_bytes(self.debug[cursor:cursor+4],"little")
                require(length >= 4 and cursor+length <= end and
                        (length < 8 or self.debug[cursor+4:cursor+6] == b"\0\0"),
                        "Invalid aggregate terminator in animation helper")
                cursor += length
                continue
            ct,_,ca = self.by[cursor]
            if ct == 13:
                loc = ca.get(2)
                require(isinstance(loc,bytes) and len(loc) == 6 and loc[0] == 4 and loc[-1] == 7,
                        "Animation helper member has an unreviewed displacement")
                offset = int.from_bytes(loc[1:5],"little")
                path = prefix+(ca[3],)
                require(offset < a[11], "Animation helper member escapes its complete parent")
                if 5 in ca and ca[5] in (8,9,14):
                    leaf = {"path":list(path),"offset":base+offset,"size":4,"member_die":cursor,
                            "fundamental_type":ca[5]}
                    require(offset+4 <= a[11], "Animation scalar exceeds original parent")
                    result[base+offset] = leaf
                elif any(isinstance(ca.get(k),bytes) and ca[k].startswith(b"\x01") for k in (6,8)):
                    require(offset+4 <= a[11], "Animation pointer exceeds original parent")
                    result[base+offset] = {"path":list(path),"offset":base+offset,"size":4,
                        "member_die":cursor,"pointer_type":{str(k):ca[k].hex() for k in (6,8) if k in ca}}
                elif 7 in ca and self.by[ca[7]][0] in (2,0x13):
                    require(offset+self.by[ca[7]][2][11] <= a[11], "Nested animation value escapes parent")
                    children = self.leaves(ca[7],path,base+offset,ancestors+(die,))
                    require(not set(result)&set(children), "Animation helper leaf paths overlap")
                    result.update(children)
            following = ca.get(1,self.next.get(cursor,end))
            require(isinstance(following,int) and following > cursor, "Invalid animation aggregate sibling")
            cursor = following
        return result

    def object(self,name,size):
        off,a,address = self.declaration(name)
        die = a.get(7)
        require(die in self.by and self.by[die][2].get(11) == size,
                "Complete original animation helper object size differs")
        return {"name":name,"reference_address":address,"size":size,"declaration_die":off,
                "type_die":die,"fields":self.leaves(die)}


def generate_unit(originals, registry_dir):
    target = originals[TARGET]
    known = {}
    entries = {b for _,b,_ in MEMBERS}
    for path in sorted(registry_dir.glob("*functions.json")):
        for f in json.loads(path.read_text())["functions"]:
            # Keep the literal runtime witnesses stable if later work recovers
            # complete identities for these currently opaque SDK entries.
            if f["address"] not in entries | OPAQUE_RUNTIME:
                prior = known.setdefault(f["address"],f)
                require(all(prior[k] == f[k] for k in ("name","source","size","sha256")),
                        "Conflicting independent animation helper identities")
    reviewed = json.loads((registry_dir/"reviewed-call-targets.json").read_text())
    require(reviewed["executable_sha1"] == target.sha1, "Allocator context targets another original")
    for f in reviewed["anchors"]:
        if f["name"] in ("xMemPushTemp","xMemPopTemp"):
            require(f["identity_confirmation"] and f["eligible_for_progress"] is False,
                    "Allocator context lacks reviewed identity")
            known.setdefault(f["address"],f)
    records,sequences,data_proofs = {},[],[]
    for version in REFERENCES:
        original = originals[version]
        links = canonical_linkages(original.data,original.metadata)
        types = TypedObjects(original)
        cheat,shared = types.object("cheat_tweak",440),types.object("shared",416)
        off,arr,arrbase = types.declaration("start_anim_states")
        tag,_,atyp = types.by[arr[7]]
        require(tag == 1 and atyp.get(9) == 0 and atyp.get(10) ==
                bytes.fromhex("000a0000000000240000000863000300010100"),
                "Start-animation table is not original const char*[37]")
        functions = {}
        for name,b,size in MEMBERS:
            refs = [f for f in original.functions if (f["source"],f["name"],f["high"]-f["low"]) == (SOURCE,name,size)]
            require(len(refs) == 1 and refs[0]["low"] in links, "Animation helper full original identity differs")
            a = refs[0]["low"]
            require(a % 16 == b % 16 == 0, "Animation helper original entry alignment differs")
            pairs,calls,masks = compare(original,target,a,b,size)
            require((len(pairs),len(calls)) == {744:(23,12),396:(30,2),8:(0,0)}[size],
                    "Complete helper address/call inventory differs")
            functions[b] = {"name":name,"a":a,"b":b,"size":size,"pairs":pairs,"calls":calls,
                            "masks":masks,"bounds":closed(original,target,a,b,size)}
        anchor = known.get(PARAMETERS)
        require(anchor is not None and (anchor["source"],anchor["name"],anchor["size"]) ==
                (SOURCE,"register_tweaks",11980), "Independent Cruise parameter anchor missing")
        provenance = next(p for p in anchor["provenance"] if p["version"] == version)
        checked_identity(original,target,provenance["source_address"],PARAMETERS,known)
        _,_,anchor_masks = compare(original,target,provenance["source_address"],PARAMETERS,11980)
        load,aim = functions[0x2a4410],functions[0x2a45a0]
        require(load["a"] == provenance["source_address"]+11984 and aim["a"] == load["a"]+400 and
                not any(original.read(load["a"]-4,4)+target.read(load["b"]-4,4)+
                        original.read(aim["a"]-4,4)+target.read(aim["b"]-4,4)),
                "Complete callback neighborhood loses original members or zero alignment")
        require(not aim["pairs"] and not aim["calls"], "Aim callback literal complete body differs")
        strings,fields,pointers,contexts,arrays = [],[],[],[],[]
        mapping,bases = {},{}
        for b,f in functions.items():
            for p in f["pairs"]:
                ra,tb = p["reference_address"],p["target_address"]
                require(mapping.setdefault(ra,tb) == tb,"Animation helper operands map inconsistently")
                if ra in original.by_address:
                    if tb == aim["b"]:
                        require(ra == aim["a"],"Animation aim pointer loses its complete original neighbor")
                        name,size = "check_anim_aim",8
                    else:
                        cb = checked_identity(original,target,ra,tb,known)
                        require((cb["source"],cb["name"]) == ("SB/Core/x/xAnim.cpp","xAnimDefaultBeforeEnter"),
                                "Unexpected independent insertion callback")
                        name,size = cb["name"],cb["size"]
                    require(p["opcode"] == 9,"Animation code pointer has an unexpected opcode")
                    pointers.append({**p,"name":name,"complete_size":size,"target_sha256":digest(target.read(tb,size))})
                elif ra == arrbase:
                    require(p["opcode"] == 9 and p["storage"] == "file_backed", "Animation string array operand differs")
                    values = []
                    for index,(x,y) in enumerate(zip(words(original.read(ra,148)),words(target.read(tb,148)))):
                        require(bool(x) == bool(y),"Animation string-array terminator differs")
                        if x:
                            literal = animation_literal(original,x)
                            require(literal == animation_literal(target,y),"Complete animation array string differs")
                            values.append({"index":index,"reference_address":x,"target_address":y,
                                           "size":len(literal),"sha256":digest(literal),"text":literal[:-1].decode("ascii")})
                    require(len(values) == 37 and [v["index"] for v in values] == list(range(37)),
                            "Animation array must contain all 37 complete original strings")
                    arrays.append({**p,"name":"start_anim_states","count":37,"size":148,
                                   "declaration_die":off,"type_die":arr[7],"strings":values})
                elif p["opcode"] == 9:
                    literal = animation_literal(original,ra)
                    require(literal == animation_literal(target,tb),"Complete insertion literal differs")
                    strings.append({**p,"size":len(literal),"sha256":digest(literal),"text":literal[:-1].decode("ascii")})
                else:
                    obj = cheat if b == load["b"] else shared
                    offset = ra-obj["reference_address"]
                    require(offset in obj["fields"] and p["opcode"] in (35,43),"Unreviewed helper typed field")
                    field = obj["fields"][offset]
                    require((b == load["b"] and p["opcode"] == 43 and "fundamental_type" in field) or
                            (b != load["b"] and "pointer_type" in field and field["path"][0] in ("astate","atran")),
                            "Helper operand field type or path differs")
                    base = tb-offset
                    require(bases.setdefault(obj["name"],base) == base,"Helper typed object base is inconsistent")
                    region = "runtime_bss" if obj is cheat else "initialized_data"
                    for binary,address in ((original,obj["reference_address"]),(target,base)):
                        storage(binary,address,obj["size"],region)
                    if obj is shared:
                        require(original.read(ra,4) == target.read(tb,4) == b"\0"*4,
                                "Shared animation pointer has a changed original initializer")
                    fields.append({**p,"object":obj["name"],"complete_size":obj["size"],
                                   "declaration_die":obj["declaration_die"],"field":field,"target_base":base})
            for c in f["calls"]:
                ra,tb = c["reference_address"],c["target_address"]
                require(c["opcode"] == 3,"Unexpected animation helper tail transfer")
                if tb in known:
                    callee = checked_identity(original,target,ra,tb,known)
                    require(callee["source"] in ("SB/Core/x/xAnim.cpp","SB/Core/x/xMemMgr.cpp","SB/Core/x/xString.cpp"),
                            "Animation helper calls an unreviewed source")
                else:
                    require(f["name"] == "insert_player_animations" and
                            {300:0x118c08,332:0x11b938,340:0x11be58}.get(c["offset"]) == ra == tb and
                            ra not in original.by_address and original.read(ra,64) == target.read(tb,64) and
                            original.word(f["a"]+c["offset"]) == target.word(b+c["offset"]),
                            "Opaque insertion runtime context changed")
                    f["masks"].pop(c["offset"])
                    c.update(transfer_word_unmasked=True,no_identity_or_extent_claim=True,
                             opaque_context_bytes=64,opaque_context_sha256=digest(target.read(tb,64)))
                    contexts.append({**c,"caller":f["name"],"promoted_as_named_anchor":False})
        require(len(strings)==12 and len(fields)==36 and len(pointers)==4 and len(arrays)==1 and len(contexts)==3,
                f"Complete Cruise insertion inventory differs: {len(strings)}/{len(fields)}/{len(pointers)}/{len(arrays)}")
        require(len(mapping)==len(set(mapping.values())),"Distinct helper data operands collapse")
        sequence_masks = {**anchor_masks,**{11984+k:v for k,v in load["masks"].items()}}
        unique = unique_template(target,original.read(provenance["source_address"],12392),sequence_masks,PARAMETERS)
        insertion = functions[0x2a09e0]
        insertion_unique = unique_template(target,original.read(insertion["a"],744),insertion["masks"],insertion["b"])
        for b,f in functions.items():
            if b not in records:
                records[b] = {"name":f["name"],"source":SOURCE,"address":b,"size":f["size"],
                    "sha256":digest(target.read(b,f["size"])),"boundary_confirmation":True,
                    "confirmation_kind":CLUSTER_KIND,"provenance":[],"corroboration":{
                    "proof_scope":"complete_animation_insertion_and_anchored_callback_cluster",
                    "local_control_flow":f["bounds"],"whole_translation_unit_claimed":False}}
            require(not records[b]["provenance"] or records[b]["provenance"][0]["linkage_name"] == links[f["a"]],
                    "Animation helper canonical linkage differs across originals")
            records[b]["provenance"].append({"version":version,"executable_sha1":original.sha1,
                "source_address":f["a"],"source":SOURCE,"name":f["name"],"linkage_name":links[f["a"]],
                "reference_sha256":digest(original.read(f["a"],f["size"])),
                "data_address_operands":f["pairs"],"direct_transfers":f["calls"]})
        sequences.append({"version":version,"source":SOURCE,"source_start":provenance["source_address"],
            "target_start":PARAMETERS,"complete_context_bytes":12392,"uniqueness":unique,
            "insertion_uniqueness":insertion_unique,"whole_translation_unit_claimed":False})
        data_proofs.append({"version":version,"source":SOURCE,"complete_strings":strings,
            "typed_fields":fields,"code_pointers":pointers,"string_pointer_arrays":arrays,
            "opaque_runtime_contexts":contexts,"data_extents_promoted":False})
    require(len(records)==3 and all(len(f["provenance"])==3 for f in records.values()),
            "Cruise animation cluster needs all three original identities")
    return {"functions":sorted(records.values(),key=lambda f:f["address"]),"sequence_proofs":sequences,
            "data_proofs":data_proofs,"counts":{"functions":3,"code_bytes":1148,"source_units":1,
            "closed_return_bodies":3,"reviewed_complete_caller_callee_clusters":1}}
