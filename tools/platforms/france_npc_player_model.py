"""Original NPC consumers of the typed player model field."""
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
MEMBERS=[
    ('SB/Game/zNPCTypeRobot.cpp', 'FaceAntiPlayer__9zNPCRobotFff', 3020512, 252),
    ('SB/Game/zNPCTypeRobot.cpp', 'TurnThemHeads__9zNPCRobotFv', 3020992, 444),
    ('SB/Game/zNPCSupport.cpp', 'NPCC_DstSqPlyrToPos__FPC5xVec3', 3256976, 104),
    ('SB/Game/zNPCGoalRobo.cpp', 'Process__13zNPCGoalKnockFP11en_trantypefPvP6xScene', 2827792, 360),
    ('SB/Game/zNPCGoalRobo.cpp', 'CalcAttackVector__18zNPCGoalAlertGloveFv', 2876240, 300),
    ('SB/Game/zNPCGoalRobo.cpp', 'MoveEvade__19zNPCGoalAlertHammerFf', 2883088, 360),
    ('SB/Game/zNPCGoalRobo.cpp', 'CalcEvadePos__20zNPCGoalAlertChomperFP5xVec3', 2887936, 836),
    ('SB/Game/zNPCGoalRobo.cpp', 'Process__14zNPCGoalNoticeFP11en_trantypefPvP6xScene', 2903856, 292),
]

INDEPENDENT={
    0x1ee4e0:('SB/Core/x/xMath.cpp', 'xurand', 88),
    0x1ef150:('SB/Core/x/xMath3.cpp', 'xMat3x3Mul', 476),
    0x1ef330:('SB/Core/x/xMath3.cpp', 'xMat3x3Transpose', 152),
    0x1ef910:('SB/Core/x/xMath3.cpp', 'xMat3x3LookVec', 616),
    0x210d10:('SB/Core/x/xVec3.cpp', 'xVec3Normalize', 224),
    0x2b1ea0:('SB/Game/zNPCGoalRobo.cpp', 'StreakUpdate', 244),
    0x2ca1d0:('SB/Game/zNPCGoalStd.cpp', 'Process', 420),
    0x2d11e0:('SB/Game/zNPCTypeCommon.cpp', 'AnimTimeRemain', 104),
    0x2d34a0:('SB/Game/zNPCTypeCommon.cpp', 'TurnToFace', 340),
    0x2d3600:('SB/Game/zNPCTypeCommon.cpp', 'ThrottleApply', 372),
    0x2d3810:('SB/Game/zNPCTypeCommon.cpp', 'ThrottleAdjust', 264),
    0x2d3920:('SB/Game/zNPCTypeCommon.cpp', 'VelStop', 192),
    0x2e17e0:('SB/Game/zNPCTypeRobot.cpp', 'FacePos', 220),
    0x2f2250:('SB/Core/x/xBehaviour.cpp', 'Process', 56),
    0x2c70a0:('SB/Game/zNPCGoalStd.cpp', 'CalcNewDir', 624),
}

CALLS={
    0x2e16e0:[(224, 2962592)],
    0x2e18c0:[(224, 2166032), (372, 2029840), (388, 2028336), (404, 2027856)],
    0x31b290:[],
    0x2b2610:[(140, 2953696), (256, 3020768), (284, 2825888), (308, 3088976)],
    0x2be350:[(104, 2166032)],
    0x2bfe10:[(312, 2963472), (332, 2962944)],
    0x2c1100:[(152, 2024672), (180, 1132176), (192, 1132848)],
    0x2c4f30:[(184, 2166032), (212, 2962592), (224, 2963744), (248, 2925008)],
}
RANK={member[2]:i for i,member in enumerate(MEMBERS)}


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
        functions={}
        for source,linkage,b,size in MEMBERS:
            refs=[f for f in original.functions if f["source"]==source and links.get(f["low"])==linkage]
            require(len(refs)==1 and refs[0]["high"]-refs[0]["low"]==size,"Complete NPC helper/goal original identity differs")
            f=refs[0];a=f["low"]
            require(a%16==b%16==0,"NPC helper/goal original entry alignment differs")
            pairs,calls,masks=compare(original,target,a,b,size,address_resolver=hangable_pair)
            require(all(c["opcode"]==3 for c in calls),"Unreviewed NPC tail operand")
            require(all(p["reference_address"]==field and p["target_address"]==0x52cc14 and
                        p["storage"]=="zero_fill" and p["opcode"] in (9,35) for p in pairs),
                    "Unreviewed typed player model data operand")
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
                    "proof_scope":"complete_npc_consumers_of_independently_anchored_typed_player_model_field",
                    "local_control_flow":f["bounds"],"whole_translation_unit_claimed":False}}
            records[b]["provenance"].append({"version":version,"executable_sha1":original.sha1,
                "source_address":f["a"],"source":f["source"],"name":f["name"],"linkage_name":f["linkage"],
                "reference_sha256":digest(original.read(f["a"],f["size"])),
                "data_address_operands":f["pairs"],"direct_transfers":f["calls"]})
        require(len(transfers)==19 and used_independent==set(INDEPENDENT)-{0x2c70a0} and len(data_pairs)==9 and len(contexts)==2,"Complete NPC helper call inventory differs")
        sequences.append({"version":version,"complete_bodies":unique,"whole_translation_units_claimed":False})
        call_proofs.append({"version":version,"complete_direct_transfers":transfers,"changed_data_operands":9,
                            "typed_field_operands":data_pairs,"opaque_runtime_contexts":contexts,
                            "typed_globals":{"declaration":globals_data,"target_address":0x52cc14-1828,
                                             "component_path":paths[0],"component_offset":1828,
                                             "independent_complete_anchor":{k:anchor[k] for k in ("source","name","address","size","sha256")},
                                             "data_extent_promoted":False}})
    return {"functions":sorted(records.values(),key=lambda f:f["address"]),"sequence_proofs":sequences,
            "data_proofs":call_proofs,"counts":{"functions":8,"code_bytes":2948,"source_units":3,
            "closed_return_bodies":8,"reviewed_complete_caller_callee_clusters":1}}
