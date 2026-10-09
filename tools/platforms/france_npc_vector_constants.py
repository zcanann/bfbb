"""Original NPC callers using complete typed vector and scalar objects."""
from __future__ import annotations

import json
from platforms.verify_reviewed import REFERENCES,TARGET,require
from platforms.france_tu_sequences import compare,digest,unique_template
from platforms.france_player_animation_context import closed
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages
from platforms.france_goal_sequence import original_data,goal_pair
from platforms.france_hangable_sequence import hangable_pair
from platforms.france_player_animation_context import storage

# All entries and extents below come from authenticated original DWARF bodies.
MEMBERS = [('SB/Game/zNPCTypeRobot.cpp', 'SetHome__8NPCArenaFP10zNPCCommonP10zMovePoint', 2975008, 244),
 ('SB/Game/zNPCTypeRobot.cpp', 'CornerOfArena__9zNPCRobotFP5xVec3f', 3019616, 896),
 ('SB/Game/zNPCSupport.cpp', 'NPCC_aimVary__FP5xVec3P5xVec3P5xVec3fiP5xVec3', 3258496, 1248),
 ('SB/Game/zNPCGoalRobo.cpp', 'MoveTryToEscape__17zNPCGoalTubeLassoFf', 2814960, 1132),
 ('SB/Game/zNPCGoalRobo.cpp', 'MoveFrolic__20zNPCGoalTubeDucklingFf', 2818528, 1240),
 ('SB/Game/zNPCGoalRobo.cpp', 'Enter__20zNPCGoalAttackTarTarFfPv', 2853312, 144),
 ('SB/Game/zNPCGoalRobo.cpp', 'MoveCorner__18zNPCGoalAlertSlickFf', 2860352, 608),
 ('SB/Game/zNPCGoalRobo.cpp', 'MoveCorner__20zNPCGoalAlertMonsoonFf', 2871648, 608),
 ('SB/Game/zNPCGoalRobo.cpp', 'Enter__20zNPCGoalAlertChomperFfPv', 2890896, 132),
 ('SB/Game/zNPCGoalRobo.cpp', 'Enter__20zNPCGoalAlertFodBzztFfPv', 2896336, 240)]

INDEPENDENT = {2024672: ('SB/Core/x/xMath.cpp', 'xurand', 88),
 2024768: ('SB/Core/x/xMath.cpp', 'xrand', 32),
 2166032: ('SB/Core/x/xVec3.cpp', 'xVec3Normalize', 224),
 2925472: ('SB/Game/zNPCGoalStd.cpp', 'Enter', 32),
 2929808: ('SB/Game/zNPCGoalCommon.cpp', 'Enter', 216),
 2962592: ('SB/Game/zNPCTypeCommon.cpp', 'TurnToFace', 340),
 2962944: ('SB/Game/zNPCTypeCommon.cpp', 'ThrottleApply', 372),
 2963472: ('SB/Game/zNPCTypeCommon.cpp', 'ThrottleAdjust', 264),
 2963744: ('SB/Game/zNPCTypeCommon.cpp', 'VelStop', 192),
 2975264: ('SB/Game/zNPCTypeRobot.cpp', 'NextBestNav', 296),
 3256288: ('SB/Game/zNPCSupport.cpp', 'NPCC_TmrCycle', 104),
 2912416: ('SB/Game/zNPCGoalStd.cpp', 'CalcNewDir', 624)}

CALLS = {2975008: [(80, 2975264)],
 3019616: [(824, 2024768)],
 3258496: [(460, 2166032), (696, 2024672), (756, 2024672), (812, 2024672), (844, 2024672), (1128, 2166032)],
 2814960: [(552, 2166032), (1104, 2962592)],
 2818528: [(572, 3256288), (592, 3256288), (632, 1132848), (652, 1132848), (932, 1132848)],
 2853312: [(36, 2963744), (112, 2925472)],
 2860352: [(232, 3019616), (388, 2963472), (448, 2963472), (484, 2963472), (556, 2166032), (576, 2962944)],
 2871648: [(232, 3019616), (388, 2963472), (448, 2963472), (484, 2963472), (556, 2166032), (576, 2962944)],
 2890896: [(40, 2963744), (100, 2929808)],
 2896336: [(44, 2024768), (208, 2929808)]}

PAIRS = {2975008: [(176, 180, 5212576, 49), (192, 196, 5212580, 49), (204, 208, 5212584, 49)],
 3019616: [(144, 148, 5212616, 49), (184, 188, 5212612, 49), (196, 200, 5212608, 49), (336, 344, 5426196, 9)],
 3258496: [(480, 484, 5212592, 49),
           (492, 496, 5212596, 49),
           (504, 508, 5212600, 49),
           (568, 612, 5212616, 49),
           (628, 632, 5212612, 49),
           (636, 640, 5212608, 49),
           (904, 968, 5212608, 49),
           (980, 984, 5212612, 49),
           (1000, 1016, 5212616, 49)],
 2814960: [(92, 104, 5227784, 57), (640, 644, 5227784, 49), (676, 748, 5227784, 49)],
 2818528: [(660, 668, 5212624, 9), (664, 700, 5212592, 9)],
 2853312: [(52, 84, 5212576, 49), (92, 96, 5212580, 49), (104, 108, 5212584, 49)],
 2860352: [],
 2871648: [],
 2890896: [(48, 56, 5212576, 49), (76, 80, 5212580, 49), (88, 92, 5212584, 49)],
 2896336: [(100, 144, 5212576, 49),
           (152, 156, 5212580, 49),
           (164, 168, 5212584, 49),
           (176, 180, 5212576, 49),
           (188, 192, 5212580, 49),
           (200, 204, 5212584, 49)]}

RANK={member[2]:i for i,member in enumerate(MEMBERS)}


def original_constants(original,target):
    from platforms.dwarf1 import iter_dies
    from platforms.ps2_type_layouts import aggregate_layouts
    section=next(s for s in original.metadata["sections"] if s["name"]==".debug")
    debug=original.data[section["offset"]:section["offset"]+section["size"]]
    source="SB/Game/zNPCGoalRobo.cpp"
    layout=aggregate_layouts(debug,source,{"xVec3"})["xVec3"]
    require(layout["size"]==12 and
            [(m["name"],m["offset"],m["type_attributes"]) for m in layout["members"]]==
            [("x",0,{"5":14}),("y",4,{"5":14}),("z",8,{"5":14})],
            "NPC vector original layout differs")
    targets={"g_O3":0x4f89a0,"g_X3":0x4f89b0,"g_Y3":0x4f89c0,"g_Z3":0x4f89d0,
             "dst_tetherMax":0x4fc508}
    payloads={"g_O3":"000000000000000000000000","g_X3":"0000803f0000000000000000",
              "g_Y3":"000000000000803f00000000","g_Z3":"00000000000000000000803f",
              "dst_tetherMax":"00000000"}
    declarations={}
    for off,tag,owner,attrs in iter_dies(debug):
        name=attrs.get(3)
        if name not in targets or not owner.replace("\\","/").endswith(source):
            continue
        loc=attrs.get(2)
        require(name not in declarations and isinstance(loc,bytes) and len(loc)==5 and loc[0]==3,
                "NPC constant declaration is missing, ambiguous or not absolute")
        if name=="dst_tetherMax":
            require(tag==12 and attrs.get(5)==14 and 7 not in attrs and
                    attrs.get(512)=="dst_tetherMax$12202","NPC local original is not the reviewed float")
            size=4
        else:
            require(tag==7 and attrs.get(7)==layout["die_offset"],"NPC vector declaration type differs")
            size=12
        address=int.from_bytes(loc[1:],"little");target_address=targets[name]
        storage(original,address,size,"initialized_data")
        storage(target,target_address,size,"initialized_data")
        require(original.read(address,size).hex()==payloads[name] and
                target.read(target_address,size)==original.read(address,size),
                "NPC complete typed data payload differs")
        declarations[name]={"reference_address":address,"target_address":target_address,"size":size,
                            "declaration_die":off,"type":"float" if size==4 else "xVec3",
                            "payload":payloads[name],"data_extent_promoted":False}
    require(set(declarations)==set(targets),"NPC typed data declaration missing")
    return declarations


def generate_unit(originals,registry_dir):
    target=originals[TARGET]
    known={}
    # Only these independently confirmed external destinations contribute
    # evidence. Later NPC recoveries cannot change this module's dependency set.
    for path in sorted(registry_dir.glob("*functions.json")):
        for f in json.loads(path.read_text())["functions"]:
            if f["address"] in INDEPENDENT:
                old=known.setdefault(f["address"],f)
                require(all(old[k]==f[k] for k in ("source","name","size","sha256")),"Conflicting NPC helper dependency")
    require(set(known)==set(INDEPENDENT),"Independent NPC helper dependencies missing")
    for address,identity in INDEPENDENT.items():
        require(tuple(known[address][k] for k in ("source","name","size"))==identity,
                "NPC helper dependency identity differs")
    records,sequences,call_proofs={},[],[]
    for version in REFERENCES:
        original=originals[version]
        links=canonical_linkages(original.data,original.metadata)
        objects,tables,paths,layouts=original_data(original)
        globals_data=objects["globals"]
        require([p["offset"] for p in paths[0]]==[1792,0,0,36],"Player model component path differs")
        anchor=known[0x2c70a0]
        ref=next(p for p in anchor["provenance"] if p["version"]==version)
        checked_identity(original,target,ref["source_address"],0x2c70a0,known)
        ap,ac,am=compare(original,target,ref["source_address"],0x2c70a0,624,address_resolver=goal_pair)
        field=globals_data["reference_address"]+1828
        require(any(p["reference_address"]==field and p["target_address"]==0x52cc14 and p["opcode"]==35 for p in ap),
                "Independent complete goal no longer anchors the player model field")
        storage(original,globals_data["reference_address"],8272,"runtime_bss")
        storage(target,0x52cc14-1828,8272,"runtime_bss")
        typed_objects=original_constants(original,target)
        functions={}
        for source,linkage,b,size in MEMBERS:
            refs=[f for f in original.functions if f["source"]==source and links.get(f["low"])==linkage]
            require(len(refs)==1 and refs[0]["high"]-refs[0]["low"]==size,"Complete NPC helper/goal original identity differs")
            f=refs[0];a=f["low"]
            require(a%16==b%16==0,"NPC helper/goal original entry alignment differs")
            pairs,calls,masks=compare(original,target,a,b,size,address_resolver=hangable_pair)
            require(all(c["opcode"]==3 for c in calls),"Unreviewed NPC tail operand")
            require([(p["hi_offset"],p["lo_offset"],p["target_address"],p["opcode"]) for p in pairs]==PAIRS[b],
                    "Typed NPC data operand inventory differs")
            for p in pairs:
                if p["target_address"]==0x52cc14:
                    require(p["reference_address"]==field and p["storage"]=="zero_fill" and p["opcode"] in (9,35),
                            "Typed player model field differs")
                else:
                    candidates=[(name,obj,p["target_address"]-obj["target_address"]) for name,obj in typed_objects.items()
                                if 0<=p["target_address"]-obj["target_address"]<obj["size"]]
                    require(len(candidates)==1,"NPC operand lacks a complete typed object")
                    name,obj,offset=candidates[0]
                    require(offset in (0,4,8) and p["reference_address"]==obj["reference_address"]+offset and
                            p["storage"]=="file_backed","NPC typed component or storage differs")
                    if name=="dst_tetherMax":
                        require(b==0x2af3f0 and offset==0 and p["opcode"] in (49,57),"Unreviewed local float consumer")
                    else:
                        require(p["opcode"]==9 and offset==0 or p["opcode"]==49,
                                "NPC vector operand is not an address or scalar load")
            expected_masks={c["offset"]:0xfc000000 for c in calls}
            for p in pairs:
                expected_masks[p["lo_offset"]]=0xffff0000
                if original.read(a+p["hi_offset"],4)!=target.read(b+p["hi_offset"],4):
                    expected_masks[p["hi_offset"]]=0xffff0000
            require(masks==expected_masks,"Typed NPC mask inventory differs")
            for c in calls:
                if c["target_address"] in (0x114690,0x114930):
                    require(c["reference_address"]==c["target_address"] and
                            original.read(a+c["offset"],4)==target.read(b+c["offset"],4),
                            "NPC opaque math call must stay literal")
                    masks.pop(c["offset"])
            require([(c["offset"],c["target_address"]) for c in calls]==CALLS[b],
                    "Complete NPC helper call inventory differs")
            functions[b]=dict(source=source,name=f["name"],linkage=linkage,a=a,b=b,size=size,
                              calls=calls,pairs=pairs,masks=masks,bounds=closed(original,target,a,b,size))
        unique,transfers,contexts,data_pairs=[],[],[],[]
        used_independent=set()
        for b,f in functions.items():
            data_pairs.extend({"caller":f["linkage"],"caller_address":b,**p} for p in f["pairs"])
            for c in f["calls"]:
                ra,tb=c["reference_address"],c["target_address"]
                if tb in (0x114690,0x114930):
                    require(ra==tb,"NPC opaque runtime address differs")
                    prefix=original.read(ra,64)
                    require(prefix==target.read(tb,64),"NPC opaque runtime context differs")
                    identity={"address":tb,"context_bytes":64,"sha256":digest(prefix),
                              "scope":"unchanged_opaque_prefix","no_identity_or_extent_claim":True}
                    contexts.append({**identity,"uniqueness":unique_template(target,prefix,{},tb)})
                elif tb in functions:
                    callee=functions[tb]
                    require(ra==callee["a"] and RANK[tb]<RANK[b],
                            "Internal NPC call loses its closed helper or acyclic dependency order")
                    identity={"source":callee["source"],"name":callee["name"],"size":callee["size"],
                              "linkage_name":callee["linkage"]}
                else:
                    require(tb in known,"NPC helper call has no independently confirmed complete callee")
                    checked_identity(original,target,ra,tb,known)
                    identity={k:known[tb][k] for k in ("source","name","size")}
                    used_independent.add(tb)
                transfers.append({"caller":f["linkage"],"caller_address":b,**c,"complete_callee_identity":identity})
            u=unique_template(target,original.read(f["a"],f["size"]),f["masks"],b)
            unique.append({"source":f["source"],"linkage_name":f["linkage"],"address":b,"size":f["size"],"uniqueness":u})
            if b not in records:
                records[b]={"name":f["name"],"source":f["source"],"address":b,"size":f["size"],
                    "sha256":digest(target.read(b,f["size"])),"boundary_confirmation":True,
                    "confirmation_kind":CLUSTER_KIND,"provenance":[],"corroboration":{
                    "proof_scope":"complete_npc_callers_with_typed_vector_constants_local_float_and_anchored_player_model",
                    "local_control_flow":f["bounds"],"whole_translation_unit_claimed":False}}
            records[b]["provenance"].append({"version":version,"executable_sha1":original.sha1,
                "source_address":f["a"],"source":f["source"],"name":f["name"],"linkage_name":f["linkage"],
                "reference_sha256":digest(original.read(f["a"],f["size"])),
                "data_address_operands":f["pairs"],"direct_transfers":f["calls"]})
        require(len(transfers)==33 and used_independent==set(INDEPENDENT)-{0x2c70a0} and len(data_pairs)==33 and len(contexts)==3,"Complete NPC helper call inventory differs")
        sequences.append({"version":version,"complete_bodies":unique,"whole_translation_units_claimed":False})
        call_proofs.append({"version":version,"complete_direct_transfers":transfers,"changed_data_operands":33,
                            "typed_field_operands":data_pairs,"opaque_runtime_contexts":contexts,
                            "typed_constants":typed_objects,"typed_globals":{"declaration":globals_data,"target_address":0x52cc14-1828,
                                             "component_path":paths[0],"component_offset":1828,
                                             "independent_complete_anchor":{k:anchor[k] for k in ("source","name","address","size","sha256")},
                                             "data_extent_promoted":False}})
    return {"functions":sorted(records.values(),key=lambda f:f["address"]),"sequence_proofs":sequences,
            "data_proofs":call_proofs,"counts":{"functions":10,"code_bytes":6492,"source_units":3,
            "closed_return_bodies":10,"reviewed_complete_caller_callee_clusters":1}}
