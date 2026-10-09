"""Complete original NPC navigation, collision and robotic helper cluster."""
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
    ('SB/Game/zNPCTypeCommon.cpp', 'NPCC_BuildStandardAnimTran__FP10xAnimTablePPcPiif', 2947920, 516),
    ('SB/Game/zNPCTypeCommon.cpp', 'LassoSyncAnims__10zNPCCommonF11en_lassanim', 2949712, 140),
    ('SB/Game/zNPCTypeCommon.cpp', 'AnimDuration__10zNPCCommonFP10xAnimState', 2953808, 112),
    ('SB/Game/zNPCTypeCommon.cpp', 'AnimSetState__10zNPCCommonFUif', 2954192, 84),
    ('SB/Game/zNPCTypeCommon.cpp', 'MvptReset__10zNPCCommonFP10zMovePoint', 2955904, 108),
    ('SB/Game/zNPCTypeCommon.cpp', 'ParseProps__10zNPCCommonFv', 2960544, 140),
    ('SB/Game/zNPCTypeCommon.cpp', 'ThrottleAccel__10zNPCCommonFfif', 2963328, 136),
    ('SB/Game/zNPCTypeCommon.cpp', 'BoundAsRadius__10zNPCCommonCFi', 2967712, 216),
    ('SB/Game/zNPCTypeRobot.cpp', 'SetHome__8NPCArenaFP10zNPCCommonP5xVec3f', 2974912, 92),
    ('SB/Game/zNPCTypeRobot.cpp', 'NextBestNav__8NPCArenaFP10zNPCCommonP10zMovePoint', 2975264, 296),
    ('SB/Game/zNPCTypeRobot.cpp', 'NeedToCycle__8NPCArenaFP10zNPCCommon', 2976448, 304),
    ('SB/Game/zNPCTypeRobot.cpp', 'ShieldUpdate__9zNPCSlickFf', 2983904, 608),
    ('SB/Game/zNPCTypeRobot.cpp', 'PosStacked__13zNPCTubeSlaveFP5xVec3', 2986880, 92),
    ('SB/Game/zNPCTypeRobot.cpp', 'Notice__10TubeNoticeF10en_psynoteP5xGoalPv', 2988464, 264),
    ('SB/Game/zNPCTypeRobot.cpp', 'Chk_IsBonked__11zNPCTubeletFv', 2988752, 368),
    ('SB/Game/zNPCTypeRobot.cpp', 'Unbonk__11zNPCTubeletFv', 2989120, 108),
    ('SB/Game/zNPCTypeRobot.cpp', 'AdoptADoggie__10zNPCArfArfFv', 2996384, 88),
    ('SB/Game/zNPCTypeRobot.cpp', 'DuploNotice__10zNPCArfArfF13en_SM_NOTICESPv', 2996480, 168),
    ('SB/Game/zNPCTypeRobot.cpp', 'RepelMissile__10zNPCSleepyFf', 3002944, 120),
    ('SB/Game/zNPCTypeRobot.cpp', 'MoveTowardsArena__9zNPCRobotFff', 3019344, 272),
    ('SB/Game/zNPCSupport.cpp', 'NPCC_MakeArbPlane__FPC5xVec3P5xVec3P5xVec3', 3256016, 196),
    ('SB/Game/zNPCSupport.cpp', 'NPCC_rotHPB__FP7xMat3x3fff', 3256400, 196),
    ('SB/Game/zNPCSupport.cpp', 'NPCC_xBoundBack__FP6xBound', 3257504, 96),
    ('SB/Game/zNPCSupport.cpp', 'NPCC_xBoundAway__FP6xBound', 3257600, 96),
    ('SB/Game/zNPCSupport.cpp', 'NPCC_LineHitsBound__FP5xVec3P5xVec3P6xBoundP7xCollis', 3258016, 240),
    ('SB/Game/zNPCSupport.cpp', 'NPCC_chk_hitEnt__FP4xEntP6xBoundP7xCollis', 3258256, 200),
    ('SB/Game/zNPCSupport.cpp', 'Update__10NPCBlinkerFffff', 3262464, 140),
    ('SB/Game/zNPCSupport.cpp', 'Off__9NPCWidgetFPC10zNPCCommoni', 3266592, 168),
    ('SB/Game/zNPCSupport.cpp', 'On__9NPCWidgetFPC10zNPCCommoni', 3266768, 264),
    ('SB/Game/zNPCGoalRobo.cpp', 'DuckStackInterp__20zNPCGoalTubeDucklingFf', 2818032, 484),
    ('SB/Game/zNPCTypeRobot.cpp', 'BlinkerUpdate__11zNPCFodBombFff', 3016192, 88),
]

INDEPENDENT={
    0x1c0c50:('SB/Core/x/xBound.cpp', 'xRayHitsBound', 208),
    0x1c10e0:('SB/Core/x/xBound.cpp', 'xBoundHitsBound', 400),
    0x1ef150:('SB/Core/x/xMath3.cpp', 'xMat3x3Mul', 476),
    0x1ef550:('SB/Core/x/xMath3.cpp', 'xMat3x3RotZ', 128),
    0x1ef5d0:('SB/Core/x/xMath3.cpp', 'xMat3x3RotY', 128),
    0x1ef650:('SB/Core/x/xMath3.cpp', 'xMat3x3RotX', 128),
    0x1fa8c0:('SB/Core/x/xQuickCull.cpp', 'xQuickCullForEverything', 44),
    0x210d10:('SB/Core/x/xVec3.cpp', 'xVec3Normalize', 224),
    0x2125d0:('SB/Core/x/xAnim.cpp', 'xAnimPlaySetState', 264),
    0x212f90:('SB/Core/x/xAnim.cpp', 'xAnimTableGetStateID', 72),
    0x2131f0:('SB/Core/x/xAnim.cpp', 'xAnimTableNewTransition', 824),
    0x213530:('SB/Core/x/xAnim.cpp', 'xAnimTableAddTransition', 8),
    0x283ab0:('SB/Core/x/xEvent.cpp', 'zEntEvent', 32),
    0x2d1570:('SB/Game/zNPCTypeCommon.cpp', 'ModelAtomicFind', 80),
    0x2d15c0:('SB/Game/zNPCTypeCommon.cpp', 'ModelAtomicShow', 100),
    0x2d1630:('SB/Game/zNPCTypeCommon.cpp', 'ModelAtomicHide', 84),
    0x2d34a0:('SB/Game/zNPCTypeCommon.cpp', 'TurnToFace', 340),
    0x2d3600:('SB/Game/zNPCTypeCommon.cpp', 'ThrottleApply', 372),
    0x2d3810:('SB/Game/zNPCTypeCommon.cpp', 'ThrottleAdjust', 264),
}

CALLS={
    0x2cfb50:[(180, 2175472), (216, 2176304), (368, 2175472), (436, 2175472)],
    0x2d0250:[(120, 2172368)],
    0x2d1250:[],
    0x2d13d0:[(32, 2174864), (56, 2172368)],
    0x2d1a80:[],
    0x2d2ca0:[],
    0x2d3780:[],
    0x2d48a0:[],
    0x2d64c0:[],
    0x2d6620:[],
    0x2d6ac0:[],
    0x2d87e0:[(400, 2954608), (448, 2954800), (472, 2954688)],
    0x2d9380:[],
    0x2d99b0:[],
    0x2d9ad0:[(260, 2954688), (276, 2954800), (292, 2954800)],
    0x2d9c40:[(20, 2954688), (36, 2954800), (52, 2954800)],
    0x2db8a0:[],
    0x2db900:[],
    0x2dd240:[],
    0x2e1250:[(212, 2963472), (232, 2962944)],
    0x31aed0:[(80, 2166032)],
    0x31b050:[(108, 2028880), (120, 2029136), (136, 2027856), (148, 2029008), (164, 2027856)],
    0x31b4a0:[],
    0x31b500:[],
    0x31b6a0:[(208, 1838160)],
    0x31b790:[(120, 2074816), (136, 1839328)],
    0x31c800:[],
    0x31d820:[(124, 2636464), (136, 2636464)],
    0x31d8d0:[(224, 2636464), (236, 2636464)],
    0x2afff0:[(124, 2962592), (220, 2986880)],
    0x2e0600:[(52, 3262464)],
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
        functions={}
        for source,linkage,b,size in MEMBERS:
            refs=[f for f in original.functions if f["source"]==source and links.get(f["low"])==linkage]
            require(len(refs)==1 and refs[0]["high"]-refs[0]["low"]==size,"Complete NPC helper/goal original identity differs")
            f=refs[0];a=f["low"]
            require(a%16==b%16==0,"NPC helper/goal original entry alignment differs")
            pairs,calls,masks=compare(original,target,a,b,size)
            require(not pairs and all(c["opcode"]==3 for c in calls),"Unreviewed NPC helper data or tail operand")
            require(set(masks)=={c["offset"] for c in calls} and all(mask==0xfc000000 for mask in masks.values()),
                    "Only explicitly proved NPC helper JAL destinations may be masked")
            require([(c["offset"],c["target_address"]) for c in calls]==CALLS[b],
                    "Complete NPC helper call inventory differs")
            functions[b]=dict(source=source,name=f["name"],linkage=linkage,a=a,b=b,size=size,
                              calls=calls,masks=masks,bounds=closed(original,target,a,b,size))
        unique,transfers=[],[]
        used_independent=set()
        for b,f in functions.items():
            for c in f["calls"]:
                ra,tb=c["reference_address"],c["target_address"]
                if tb in functions:
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
                    "proof_scope":"complete_literal_npc_navigation_collision_and_robotic_helpers",
                    "local_control_flow":f["bounds"],"whole_translation_unit_claimed":False}}
            records[b]["provenance"].append({"version":version,"executable_sha1":original.sha1,
                "source_address":f["a"],"source":f["source"],"name":f["name"],"linkage_name":f["linkage"],
                "reference_sha256":digest(original.read(f["a"],f["size"])),
                "data_address_operands":[],"direct_transfers":f["calls"]})
        require(len(transfers)==34 and used_independent==set(INDEPENDENT),"Complete NPC helper call inventory differs")
        sequences.append({"version":version,"complete_bodies":unique,"whole_translation_units_claimed":False})
        call_proofs.append({"version":version,"complete_direct_transfers":transfers,"changed_data_operands":0})
    return {"functions":sorted(records.values(),key=lambda f:f["address"]),"sequence_proofs":sequences,
            "data_proofs":call_proofs,"counts":{"functions":31,"code_bytes":6400,"source_units":4,
            "closed_return_bodies":31,"reviewed_complete_caller_callee_clusters":1}}
