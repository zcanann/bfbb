"""Complete original NPC motion helpers and their closed robotic-goal consumers."""
from __future__ import annotations

import json
from platforms.verify_reviewed import REFERENCES,TARGET,require
from platforms.france_tu_sequences import compare,digest,unique_template
from platforms.france_player_animation_context import closed
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages

# All entries and extents below come from authenticated original DWARF bodies.
MEMBERS=[
    ('SB/Game/zNPCGoalRobo.cpp','Enter__17zNPCGoalTubeBirthFfPv',0x2af390,96),
    ('SB/Game/zNPCGoalRobo.cpp','StreakUpdate__13zNPCGoalKnockFv',0x2b1ea0,244),
    ('SB/Game/zNPCGoalRobo.cpp','Exit__13zNPCGoalKnockFfPv',0x2b2780,104),
    ('SB/Game/zNPCGoalRobo.cpp','Process__13zNPCGoalWoundFP11en_trantypefPvP6xScene',0x2b3090,360),
    ('SB/Game/zNPCGoalRobo.cpp','Exit__18zNPCGoalLassoThrowFfPv',0x2b4510,40),
    ('SB/Game/zNPCGoalRobo.cpp','Exit__17zNPCGoalLassoBaseFfPv',0x2b4a90,48),
    ('SB/Game/zNPCGoalRobo.cpp','Enter__15zNPCGoalStunnedFfPv',0x2b4db0,172),
    ('SB/Game/zNPCGoalRobo.cpp','NPCMessage__15zNPCGoalEvilPatFP6NPCMsg',0x2b4ee0,132),
    ('SB/Game/zNPCGoalRobo.cpp','Process__15zNPCGoalEvilPatFP11en_trantypefPvP6xScene',0x2b4f70,132),
    ('SB/Game/zNPCGoalRobo.cpp','Exit__16zNPCGoalTeleportFfPv',0x2b5790,176),
    ('SB/Game/zNPCGoalRobo.cpp','NPCMessage__17zNPCGoalDogPounceFP6NPCMsg',0x2b58f0,48),
    ('SB/Game/zNPCGoalRobo.cpp','Exit__22zNPCGoalAttackArfMeleeFfPv',0x2b7ec0,128),
    ('SB/Game/zNPCGoalRobo.cpp','Exit__20zNPCGoalAttackHammerFfPv',0x2b98c0,128),
    ('SB/Game/zNPCGoalRobo.cpp','Notify__Q220zNPCGoalAttackFodder12CattleNotifyF10en_haznoteP9NPCHazard',0x2b9f40,80),
    ('SB/Game/zNPCGoalRobo.cpp','Enter__18zNPCGoalAlertSlickFfPv',0x2baf10,100),
    ('SB/Game/zNPCGoalRobo.cpp','MoveToHome__20zNPCGoalAlertTubeletFf',0x2bb240,444),
    ('SB/Game/zNPCGoalRobo.cpp','ZoomMove__18zNPCGoalAlertChuckFf',0x2bb880,464),
    ('SB/Game/zNPCGoalRobo.cpp','Enter__18zNPCGoalAlertChuckFfPv',0x2bc3f0,88),
    ('SB/Game/zNPCGoalRobo.cpp','Enter__19zNPCGoalAlertTarTarFfPv',0x2bfda0,104),
    ('SB/Game/zNPCGoalRobo.cpp','Enter__13zNPCGoalEvadeFfPv',0x2c4cc0,128),
    ('SB/Game/zNPCTypeCommon.cpp','AnimTimeRemain__10zNPCCommonFP10xAnimState',0x2d11e0,104),
    ('SB/Game/zNPCTypeCommon.cpp','AnimCurStateID__10zNPCCommonFv',0x2d12c0,96),
    ('SB/Game/zNPCTypeCommon.cpp','IsAttackFrame__10zNPCCommonFfi',0x2d25d0,296),
    ('SB/Game/zNPCTypeCommon.cpp','ThrottleApply__10zNPCCommonFfPC5xVec3i',0x2d3600,372),
    ('SB/Game/zNPCTypeCommon.cpp','ThrottleAdjust__10zNPCCommonFfff',0x2d3810,264),
    ('SB/Game/zNPCTypeCommon.cpp','VelStop__10zNPCCommonFv',0x2d3920,192),
    ('SB/Game/zNPCTypeRobot.cpp','DstSqFromHome__8NPCArenaFP5xVec3P5xVec3',0x2d6bf0,144),
    ('SB/Game/zNPCTypeRobot.cpp','PctFromHome__8NPCArenaFP5xVec3',0x2d6c80,120),
    ('SB/Game/zNPCTypeRobot.cpp','IncludesPos__8NPCArenaFP5xVec3fP5xVec3',0x2d6d00,176),
    ('SB/Game/zNPCSupport.cpp','NPCC_Bounce__FP5xVec3P5xVec3f',0x31b120,184),
    ('SB/Game/zNPCHazard.cpp','Start__9NPCHazardFPC5xVec3f',0x3cb890,128),
]

INDEPENDENT={
    0x1e8170:('SB/Core/x/xFX.cpp','xFXStreakUpdate',208),
    0x1e8240:('SB/Core/x/xFX.cpp','xFXStreakStop',96),
    0x1ee4e0:('SB/Core/x/xMath.cpp','xurand',88),
    0x1ee540:('SB/Core/x/xMath.cpp','xrand',32),
    0x2ca1d0:('SB/Game/zNPCGoalStd.cpp','Process',420),
    0x2ca390:('SB/Game/zNPCGoalStd.cpp','Exit',12),
    0x2cb490:('SB/Game/zNPCGoalCommon.cpp','Enter',216),
    0x2f2250:('SB/Core/x/xBehaviour.cpp','Process',56),
}


def generate_unit(originals,registry_dir):
    target=originals[TARGET]
    known={}
    # Only these independently confirmed external destinations contribute
    # evidence. Later NPC recoveries cannot change this module's dependency set.
    for path in sorted(registry_dir.glob("*functions.json")):
        for f in json.loads(path.read_text())["functions"]:
            if f["address"] in INDEPENDENT:
                old=known.setdefault(f["address"],f)
                require(all(old[k]==f[k] for k in ("source","name","size","sha256")),"Conflicting motion dependency")
    require(set(known)==set(INDEPENDENT),"Independent motion/streak dependencies missing")
    for address,identity in INDEPENDENT.items():
        require(tuple(known[address][k] for k in ("source","name","size"))==identity,
                "Motion dependency identity differs")
    records,sequences,call_proofs={},[],[]
    for version in REFERENCES:
        original=originals[version]
        links=canonical_linkages(original.data,original.metadata)
        functions={}
        for source,linkage,b,size in MEMBERS:
            refs=[f for f in original.functions if f["source"]==source and links.get(f["low"])==linkage]
            require(len(refs)==1 and refs[0]["high"]-refs[0]["low"]==size,"Complete motion/goal original identity differs")
            f=refs[0];a=f["low"]
            require(a%16==b%16==0,"Motion/goal original entry alignment differs")
            pairs,calls,masks=compare(original,target,a,b,size)
            require(not pairs and all(c["opcode"]==3 for c in calls),"Unreviewed motion data or tail operand")
            require(set(masks)=={c["offset"] for c in calls} and all(mask==0xfc000000 for mask in masks.values()),
                    "Only explicitly proved motion JAL destinations may be masked")
            require(source=="SB/Game/zNPCGoalRobo.cpp" or not calls,"Motion helper is not a complete literal leaf")
            functions[b]=dict(source=source,name=f["name"],linkage=linkage,a=a,b=b,size=size,
                              calls=calls,masks=masks,bounds=closed(original,target,a,b,size))
        unique,transfers=[],[]
        used_independent=set()
        for b,f in functions.items():
            for c in f["calls"]:
                ra,tb=c["reference_address"],c["target_address"]
                if tb in functions:
                    callee=functions[tb]
                    require(ra==callee["a"] and not callee["calls"],
                            "Internal motion call loses its independently closed literal helper")
                    identity={"source":callee["source"],"name":callee["name"],"size":callee["size"],
                              "linkage_name":callee["linkage"]}
                else:
                    require(tb in known,"Motion call has no independently confirmed complete callee")
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
                    "proof_scope":"complete_literal_npc_motion_helpers_and_closed_robotic_goal_consumers",
                    "local_control_flow":f["bounds"],"whole_translation_unit_claimed":False}}
            records[b]["provenance"].append({"version":version,"executable_sha1":original.sha1,
                "source_address":f["a"],"source":f["source"],"name":f["name"],"linkage_name":f["linkage"],
                "reference_sha256":digest(original.read(f["a"],f["size"])),
                "data_address_operands":[],"direct_transfers":f["calls"]})
        require(len(transfers)==34 and used_independent==set(INDEPENDENT),"Complete motion call inventory differs")
        sequences.append({"version":version,"complete_bodies":unique,"whole_translation_units_claimed":False})
        call_proofs.append({"version":version,"complete_direct_transfers":transfers,"changed_data_operands":0})
    return {"functions":sorted(records.values(),key=lambda f:f["address"]),"sequence_proofs":sequences,
            "data_proofs":call_proofs,"counts":{"functions":31,"code_bytes":5292,"source_units":5,
            "closed_return_bodies":31,"reviewed_complete_caller_callee_clusters":1}}
