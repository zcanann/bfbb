"""Original-only complete sound listener, voice-query and delayed-insertion bodies."""
from __future__ import annotations

from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_tu_sequences import compare, digest, unique_template, named_data
from platforms.france_hangable_sequence import hangable_pair
from platforms.france_player_animation_context import closed, storage
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages
from platforms.ps2_type_layouts import aggregate_layouts
from platforms.dwarf1 import iter_dies

SOURCE = "SB/Core/x/xSnd.cpp"
MEMBERS = [("xSndStreamUnlock",0x209550,72), ("xSndStreamReady",0x2095a0,88),
           ("xSndParentDied",0x209c80,64), ("xSndIDIsPlaying",0x209cd0,88),
           ("xSndSetListenerData",0x20a380,232), ("xSndInternalUpdateVoicePos",0x20a530,284),
           ("xSndProcessSoundPos",0x20a650,616), ("xSndCalculateListenerPosition",0x20a8c0,416),
           ("xSndAddDelayed",0x20aa60,128)]


def original_data(original):
    section = next(s for s in original.metadata["sections"] if s["name"] == ".debug" and s["size"])
    debug = original.data[section["offset"]:section["offset"]+section["size"]]
    rows = list(iter_dies(debug))
    by = {off:(tag,owner,a) for off,tag,owner,a in rows}
    expected = {"xSndGlobals":5024,"xSndVoiceInfo":100,"xMat4x3":64,
                "xMat3x3":48,"xVec3":12,"_xSndDelayed":52}
    layouts = aggregate_layouts(debug,SOURCE,set(expected))
    require({n:r["size"] for n,r in layouts.items()} == expected,"Original sound component sizes differ")
    fields = {n:{m["name"]:m for m in r["members"]} for n,r in layouts.items()}

    def member(owner,name,offset,typ):
        m = fields[owner].get(name)
        require(m is not None and m["offset"] == offset and m["type_attributes"] == typ,
                f"Original sound member path differs: {owner}.{name}")
        return m

    def array(die,count,element):
        tag,_,a = by[die]
        descriptor = bytes.fromhex("000a0000000000")+(count-1).to_bytes(4,"little")+bytes.fromhex("087200")+element.to_bytes(4,"little")
        require(tag == 1 and a.get(9) == 0 and a.get(10) == descriptor,
                "Original sound array bound or element identity differs")
        return {"type_die":die,"count":count,"element_die":element,"descriptor":descriptor.hex()}

    def declaration(name,typ):
        candidates = [(off,a) for off,tag,owner,a in rows if tag in (7,12) and
                      owner.replace("\\","/").endswith(SOURCE) and a.get(3) == name]
        require(len(candidates) == 1 and candidates[0][1].get(7) == typ,
                "Original sound object declaration differs")
        off,a = candidates[0]
        return {"name":name,"reference_address":named_data(original,name,SOURCE),
                "declaration_die":off,"type_die":typ}

    vec = {"7":layouts["xVec3"]["die_offset"]}
    for i,n in enumerate(("x","y","z")):
        member("xVec3",n,4*i,{"5":14})
    for i,n in enumerate(("right","up","at")):
        member("xMat3x3",n,16*i,vec)
    for n,off,typ in (("flags",12,8),("pad1",28,9),("pad2",44,9)):
        member("xMat3x3",n,off,{"5":typ})
    member("xMat4x3","pos",48,vec)
    member("xMat4x3","pad3",60,{"5":9})
    die = layouts["xMat4x3"]["die_offset"]
    bases = [a for off,tag,_,a in rows if die<off<by[die][2][1] and tag == 28]
    require(len(bases) == 1 and bases[0].get(7) == layouts["xMat3x3"]["die_offset"] and
            bases[0].get(2) == bytes.fromhex("040000000007"),"Original listener matrix base differs")
    snd = declaration("gSnd",layouts["xSndGlobals"]["die_offset"])
    snd.update(size=5024,layouts=layouts,matrix_base_die=die)
    voice = fields["xSndGlobals"]["voice"]
    listener = fields["xSndGlobals"]["listenerMat"]
    require(voice["offset"] == 28 and listener["offset"] == 4832,"Original sound arrays moved")
    snd["voice_array"] = array(voice["type_attributes"]["7"],48,layouts["xSndVoiceInfo"]["die_offset"])
    snd["listener_array"] = array(listener["type_attributes"]["7"],2,layouts["xMat4x3"]["die_offset"])
    mode = fields["xSndGlobals"]["listenerMode"]
    tag,_,enum = by[mode["type_attributes"]["7"]]
    require(mode["offset"] == 4960 and tag == 4 and enum.get(11) == 4 and
            enum.get(15) == b"\0\0\0\0SND_LISTENER_MODE_PLAYER\0\1\0\0\0SND_LISTENER_MODE_CAMERA\0",
            "Original listener mode enumeration differs")
    snd["mode_enumeration"] = {"type_die":mode["type_attributes"]["7"],"values":enum[15].hex()}
    paths = {28:{"path":["voice"],"opcodes":[9]},4960:{"path":["listenerMode"],"opcodes":[35]}}
    for i in range(2):
        for n,off in (("right",0),("up",16),("at",32),("pos",48)):
            for k,axis in enumerate(("x","y","z")):
                paths[4832+64*i+off+4*k] = {"path":["listenerMat",i,n,axis],"opcodes":[9,49]}
            paths[4832+64*i+off+12] = {"path":["listenerMat",i,n,"one_past_vector"],"opcodes":[9]}
    for n,off in (("right",4968),("up",4980),("at",4992),("pos",5004)):
        member("xSndGlobals",n,off,vec)
        for k,axis in enumerate(("x","y","z")):
            paths[off+4*k] = {"path":[n,axis],"opcodes":[49,57]}
    snd["paths"] = {str(offset): path for offset, path in paths.items()}
    declarations = [(off,a) for off,tag,owner,a in rows if tag == 12 and
                    owner.replace("\\","/").endswith(SOURCE) and a.get(3) == "sDelayedSnd"]
    require(len(declarations) == 1,"Original delayed array declaration ambiguous")
    delayed = declaration("sDelayedSnd",declarations[0][1][7])
    delayed.update(size=832,array=array(delayed["type_die"],16,layouts["_xSndDelayed"]["die_offset"]))
    return snd,delayed


def generate_unit(originals, registry_dir):
    # No mutable registry identity is consumed: every body and the sole internal
    # call are proved directly from all three authenticated originals.
    target = originals[TARGET]
    records,sequences,data_proofs = {},[],[]
    for version in REFERENCES:
        original = originals[version]
        links = canonical_linkages(original.data,original.metadata)
        snd,delayed = original_data(original)
        functions = {}
        for name,b,size in MEMBERS:
            refs = [f for f in original.functions if (f["source"],f["name"],f["high"]-f["low"]) == (SOURCE,name,size)]
            require(len(refs) == 1 and refs[0]["low"] in links,"Complete sound identity differs")
            a = refs[0]["low"]
            require(a % 16 == b % 16 == 0,"Original sound entry alignment differs")
            pairs,calls,masks = compare(original,target,a,b,size,address_resolver=hangable_pair)
            functions[b] = dict(name=name,a=a,b=b,size=size,pairs=pairs,calls=calls,masks=masks,
                                bounds=closed(original,target,a,b,size))
        fields,mapping,bases,unique = [],{},{},[]
        for b,f in functions.items():
            for p in f["pairs"]:
                ra,tb = p["reference_address"],p["target_address"]
                obj = delayed if f["name"] == "xSndAddDelayed" else snd
                off = ra-obj["reference_address"]
                if obj is delayed:
                    require(off == 0 and p["opcode"] == 9,"Delayed sound operand is not its original array base")
                    path = {"path":["sDelayedSnd"],"opcodes":[9]}
                else:
                    path = snd["paths"].get(str(off))
                    require(path is not None and p["opcode"] in path["opcodes"],"Unreviewed sound field operand")
                require(p["storage"] == "zero_fill","Sound global storage differs")
                require(mapping.setdefault(ra,tb) == tb,"Sound operand mapping is inconsistent")
                base = tb-off
                require(bases.setdefault(obj["name"],base) == base,"Sound object base differs between fields")
                storage(original,obj["reference_address"],obj["size"],"runtime_bss")
                storage(target,base,obj["size"],"runtime_bss")
                fields.append({**p,"caller":f["name"],"object":obj["name"],"path":path["path"],"target_base":base})
            expected = [(260,3,functions[0x20a650]["a"],0x20a650)] if b == 0x20a530 else []
            require([(c["offset"],c["opcode"],c["reference_address"],c["target_address"]) for c in f["calls"]] == expected,
                    "Complete sound internal call inventory differs")
            u = unique_template(target,original.read(f["a"],f["size"]),f["masks"],b)
            unique.append({"name":f["name"],"address":b,"size":f["size"],"uniqueness":u})
            if b not in records:
                records[b] = {"name":f["name"],"source":SOURCE,"address":b,"size":f["size"],
                    "sha256":digest(target.read(b,f["size"])),"boundary_confirmation":True,
                    "confirmation_kind":CLUSTER_KIND,"provenance":[],"corroboration":{
                    "proof_scope":"complete_sound_listener_voice_and_delayed_insertion_cluster",
                    "local_control_flow":f["bounds"],"whole_translation_unit_claimed":False}}
            require(not records[b]["provenance"] or records[b]["provenance"][0]["linkage_name"] == links[f["a"]],
                    "Sound linkage differs across originals")
            records[b]["provenance"].append({"version":version,"executable_sha1":original.sha1,
                "source_address":f["a"],"source":SOURCE,"name":f["name"],"linkage_name":links[f["a"]],
                "reference_sha256":digest(original.read(f["a"],f["size"])),
                "data_address_operands":f["pairs"],"direct_transfers":f["calls"]})
        require(len(mapping) == len(set(mapping.values())) and len(fields) == 58 and
                bases["gSnd"]-bases["sDelayedSnd"] == snd["reference_address"]-delayed["reference_address"],
                "Sound data inventory or distinct object separation differs")
        sequences.append({"version":version,"source":SOURCE,"complete_bodies":unique,
                          "whole_translation_unit_claimed":False})
        data_proofs.append({"version":version,"source":SOURCE,"typed_objects":[snd,delayed],
                            "typed_fields":fields,"data_extents_promoted":False})
    return {"functions":sorted(records.values(),key=lambda f:f["address"]),"sequence_proofs":sequences,
            "data_proofs":data_proofs,"counts":{"functions":9,"code_bytes":1988,"source_units":1,
            "closed_return_bodies":9,"reviewed_complete_caller_callee_clusters":1}}
