"""Complete original NPC facing helpers and their closed robotic-goal consumers."""
from __future__ import annotations

import json
from platforms.verify_reviewed import REFERENCES,TARGET,require
from platforms.france_tu_sequences import compare,digest,unique_template,words
from platforms.france_player_animation_context import closed
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages

# All entries and extents below come from authenticated original DWARF bodies.
MEMBERS=[
    ('SB/Game/zNPCSupport.cpp', 'NPCC_dir_toXZAng__FPC5xVec3', 3259760, 40),
    ('SB/Game/zNPCTypeCommon.cpp', 'TurnToFace__10zNPCCommonFfPC5xVec3f', 2962592, 340),
    ('SB/Game/zNPCTypeRobot.cpp', 'FacePos__9zNPCRobotFP5xVec3ff', 3020768, 220),
    ('SB/Game/zNPCGoalRobo.cpp', 'Process__13zNPCGoalChaseFP11en_trantypefPvP6xScene', 2859872, 476),
    ('SB/Game/zNPCGoalRobo.cpp', 'Process__14zNPCGoalGoHomeFP11en_trantypefPvP6xScene', 2902384, 420),
]

INDEPENDENT={
    0x1edef0:('SB/Core/x/xMath.cpp', 'xDangleClamp', 132),
    0x1edf80:('SB/Core/x/xMath.cpp', 'xAngleClampFast', 80),
    0x210d10:('SB/Core/x/xVec3.cpp', 'xVec3Normalize', 224),
    0x2d3600:('SB/Game/zNPCTypeCommon.cpp', 'ThrottleApply', 372),
    0x2d3810:('SB/Game/zNPCTypeCommon.cpp', 'ThrottleAdjust', 264),
    0x2f2250:('SB/Core/x/xBehaviour.cpp', 'Process', 56),
}

CALLS={
    0x31bd70:[(12, 1133248), (20, 2023296)],
    0x2d34a0:[(64, 3259760), (84, 3259760), (128, 2023152)],
    0x2e17e0:[(188, 2962592)],
    0x2ba360:[(352, 2962592), (392, 2963472), (412, 2962944), (436, 3088976)],
    0x2c4970:[(220, 2963472), (276, 2166032), (300, 2962592), (328, 2963472), (348, 2962944), (372, 3088976)],
}


def small_helper_unique(target,body,masks,address):
    """Only this complete 40-byte helper uses a shorter, exhaustive seed."""
    require(address==0x31bd70 and len(body)==40 and masks=={20:0xfc000000},
            "Unreviewed small facing helper uniqueness scope")
    expected=words(body)
    require(expected[3]==0x0c0452b0 and expected[5]>>26==3,
            "Facing helper literal opaque call differs")
    hits=[]
    for segment in target.loaded:
        data=target.read(segment["address"],segment["file_size"])
        pos=data.find(body[:20])
        while pos>=0:
            if (segment["address"]+pos)%4==0 and pos+40<=len(data):
                candidate=words(data[pos:pos+40])
                if all(((a^b)&masks.get(i*4,0xffffffff))==0
                       for i,(a,b) in enumerate(zip(expected,candidate))):
                    hits.append(segment["address"]+pos)
            pos=data.find(body[:20],pos+1)
    require(hits==[address],"Complete small facing helper is not uniquely located")
    return {"matching_addresses":hits,"unchanged_anchor_offset":0,"unchanged_anchor_bytes":20,
            "masked_instruction_count":1,"full_body_bytes_checked":40,
            "scope":"one_complete_original_forty_byte_helper_all_loaded_spans"}


def generate_unit(originals,registry_dir):
    target=originals[TARGET]
    known={}
    # Only these independently confirmed external destinations contribute
    # evidence. Later NPC recoveries cannot change this module's dependency set.
    for path in sorted(registry_dir.glob("*functions.json")):
        for f in json.loads(path.read_text())["functions"]:
            if f["address"] in INDEPENDENT:
                old=known.setdefault(f["address"],f)
                require(all(old[k]==f[k] for k in ("source","name","size","sha256")),"Conflicting facing dependency")
    require(set(known)==set(INDEPENDENT),"Independent facing/streak dependencies missing")
    for address,identity in INDEPENDENT.items():
        require(tuple(known[address][k] for k in ("source","name","size"))==identity,
                "Facing dependency identity differs")
    records,sequences,call_proofs={},[],[]
    for version in REFERENCES:
        original=originals[version]
        links=canonical_linkages(original.data,original.metadata)
        functions={}
        for source,linkage,b,size in MEMBERS:
            refs=[f for f in original.functions if f["source"]==source and links.get(f["low"])==linkage]
            require(len(refs)==1 and refs[0]["high"]-refs[0]["low"]==size,"Complete facing/goal original identity differs")
            f=refs[0];a=f["low"]
            require(a%16==b%16==0,"Facing/goal original entry alignment differs")
            pairs,calls,masks=compare(original,target,a,b,size)
            require(not pairs and all(c["opcode"]==3 for c in calls),"Unreviewed facing data or tail operand")
            require(set(masks)=={c["offset"] for c in calls} and all(mask==0xfc000000 for mask in masks.values()),
                    "Only explicitly proved facing JAL destinations may be masked")
            require([(c["offset"],c["target_address"]) for c in calls]==CALLS[b],
                    "Complete facing call inventory differs")
            if b==0x31bd70:
                require(masks=={12:0xfc000000,20:0xfc000000} and
                        original.read(a+12,4)==target.read(b+12,4),"Facing helper opaque call must stay literal")
                masks.pop(12)
            functions[b]=dict(source=source,name=f["name"],linkage=linkage,a=a,b=b,size=size,
                              calls=calls,masks=masks,bounds=closed(original,target,a,b,size))
        unique,transfers,contexts=[],[],[]
        used_independent=set()
        for b,f in functions.items():
            for c in f["calls"]:
                ra,tb=c["reference_address"],c["target_address"]
                if b==0x31bd70 and c["offset"]==12:
                    require(ra==tb==0x114ac0,"Facing opaque call destination differs")
                    prefix=original.read(ra,64)
                    require(prefix==target.read(tb,64),"Facing opaque runtime context differs")
                    context={"address":tb,"context_bytes":64,"sha256":digest(prefix),
                             "scope":"unchanged_opaque_prefix","no_identity_or_extent_claim":True,
                             "uniqueness":unique_template(target,prefix,{},tb)}
                    contexts.append(context)
                    identity={"scope":"unchanged_opaque_prefix","address":tb,"context_bytes":64,
                              "sha256":digest(prefix),"no_identity_or_extent_claim":True}
                elif tb in functions:
                    callee=functions[tb]
                    require(ra==callee["a"],
                            "Internal facing call loses its independently closed facing helper")
                    identity={"source":callee["source"],"name":callee["name"],"size":callee["size"],
                              "linkage_name":callee["linkage"]}
                else:
                    require(tb in known,"Facing call has no independently confirmed complete callee")
                    checked_identity(original,target,ra,tb,known)
                    identity={k:known[tb][k] for k in ("source","name","size")}
                    used_independent.add(tb)
                transfers.append({"caller":f["linkage"],"caller_address":b,**c,"complete_callee_identity":identity})
            u=(small_helper_unique(target,original.read(f["a"],f["size"]),f["masks"],b)
               if b==0x31bd70 else unique_template(target,original.read(f["a"],f["size"]),f["masks"],b))
            unique.append({"source":f["source"],"linkage_name":f["linkage"],"address":b,"size":f["size"],"uniqueness":u})
            if b not in records:
                records[b]={"name":f["name"],"source":f["source"],"address":b,"size":f["size"],
                    "sha256":digest(target.read(b,f["size"])),"boundary_confirmation":True,
                    "confirmation_kind":CLUSTER_KIND,"provenance":[],"corroboration":{
                    "proof_scope":"complete_npc_facing_helpers_and_closed_robotic_goal_consumers",
                    "local_control_flow":f["bounds"],"whole_translation_unit_claimed":False}}
            records[b]["provenance"].append({"version":version,"executable_sha1":original.sha1,
                "source_address":f["a"],"source":f["source"],"name":f["name"],"linkage_name":f["linkage"],
                "reference_sha256":digest(original.read(f["a"],f["size"])),
                "data_address_operands":[],"direct_transfers":f["calls"]})
        require(len(transfers)==16 and len(contexts)==1 and used_independent==set(INDEPENDENT),"Complete facing call inventory differs")
        sequences.append({"version":version,"complete_bodies":unique,"whole_translation_units_claimed":False})
        call_proofs.append({"version":version,"complete_direct_transfers":transfers,"changed_data_operands":0,"opaque_runtime_contexts":contexts})
    return {"functions":sorted(records.values(),key=lambda f:f["address"]),"sequence_proofs":sequences,
            "data_proofs":call_proofs,"counts":{"functions":5,"code_bytes":1496,"source_units":4,
            "closed_return_bodies":5,"reviewed_complete_caller_callee_clusters":1}}
