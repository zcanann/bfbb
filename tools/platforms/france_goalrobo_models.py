"""Complete original NPC model helpers and their closed robotic-goal consumers."""
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
    ('SB/Game/zNPCTypeCommon.cpp', 'ModelAtomicFind__10zNPCCommonFiiP14xModelInstance', 2954608, 80),
    ('SB/Game/zNPCTypeCommon.cpp', 'ModelAtomicShow__10zNPCCommonFiP14xModelInstance', 2954688, 100),
    ('SB/Game/zNPCTypeCommon.cpp', 'ModelAtomicHide__10zNPCCommonFiP14xModelInstance', 2954800, 84),
    ('SB/Game/zNPCGoalRobo.cpp', 'Exit__15zNPCGoalDeflateFfPv', 2810768, 180),
    ('SB/Game/zNPCGoalRobo.cpp', 'Enter__15zNPCGoalDeflateFfPv', 2811040, 240),
    ('SB/Game/zNPCGoalRobo.cpp', 'Exit__17zNPCGoalTubeDyingFfPv', 2812096, 184),
    ('SB/Game/zNPCGoalRobo.cpp', 'Enter__17zNPCGoalTubeDyingFfPv', 2812288, 368),
    ('SB/Game/zNPCGoalRobo.cpp', 'Enter__15zNPCGoalTubePalFfPv', 2821456, 288),
    ('SB/Game/zNPCGoalRobo.cpp', 'Exit__19zNPCGoalAttackChuckFfPv', 2846320, 92),
]

INDEPENDENT={
    0x2ca390:('SB/Game/zNPCGoalStd.cpp', 'Exit', 12),
    0x2cb490:('SB/Game/zNPCGoalCommon.cpp', 'Enter', 216),
    0x2d3920:('SB/Game/zNPCTypeCommon.cpp', 'VelStop', 192),
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
                require(all(old[k]==f[k] for k in ("source","name","size","sha256")),"Conflicting model dependency")
    require(set(known)==set(INDEPENDENT),"Independent model/streak dependencies missing")
    for address,identity in INDEPENDENT.items():
        require(tuple(known[address][k] for k in ("source","name","size"))==identity,
                "Model dependency identity differs")
    records,sequences,call_proofs={},[],[]
    for version in REFERENCES:
        original=originals[version]
        links=canonical_linkages(original.data,original.metadata)
        functions={}
        for source,linkage,b,size in MEMBERS:
            refs=[f for f in original.functions if f["source"]==source and links.get(f["low"])==linkage]
            require(len(refs)==1 and refs[0]["high"]-refs[0]["low"]==size,"Complete model/goal original identity differs")
            f=refs[0];a=f["low"]
            require(a%16==b%16==0,"Model/goal original entry alignment differs")
            pairs,calls,masks=compare(original,target,a,b,size)
            require(not pairs and all(c["opcode"]==3 for c in calls),"Unreviewed model data or tail operand")
            require(set(masks)=={c["offset"] for c in calls} and all(mask==0xfc000000 for mask in masks.values()),
                    "Only explicitly proved model JAL destinations may be masked")
            if source=="SB/Game/zNPCTypeCommon.cpp":
                require((b==0x2d1570 and not calls) or
                        (b in (0x2d15c0,0x2d1630) and len(calls)==1 and calls[0]["offset"]==24 and
                         calls[0]["target_address"]==0x2d1570),"Complete model helper call inventory differs")
            functions[b]=dict(source=source,name=f["name"],linkage=linkage,a=a,b=b,size=size,
                              calls=calls,masks=masks,bounds=closed(original,target,a,b,size))
        unique,transfers=[],[]
        used_independent=set()
        for b,f in functions.items():
            for c in f["calls"]:
                ra,tb=c["reference_address"],c["target_address"]
                if tb in functions:
                    callee=functions[tb]
                    require(ra==callee["a"] and callee["source"]=="SB/Game/zNPCTypeCommon.cpp",
                            "Internal model call loses its independently closed model helper")
                    identity={"source":callee["source"],"name":callee["name"],"size":callee["size"],
                              "linkage_name":callee["linkage"]}
                else:
                    require(tb in known,"Model call has no independently confirmed complete callee")
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
                    "proof_scope":"complete_npc_model_helpers_and_closed_robotic_goal_consumers",
                    "local_control_flow":f["bounds"],"whole_translation_unit_claimed":False}}
            records[b]["provenance"].append({"version":version,"executable_sha1":original.sha1,
                "source_address":f["a"],"source":f["source"],"name":f["name"],"linkage_name":f["linkage"],
                "reference_sha256":digest(original.read(f["a"],f["size"])),
                "data_address_operands":[],"direct_transfers":f["calls"]})
        require(len(transfers)==19 and used_independent==set(INDEPENDENT),"Complete model call inventory differs")
        sequences.append({"version":version,"complete_bodies":unique,"whole_translation_units_claimed":False})
        call_proofs.append({"version":version,"complete_direct_transfers":transfers,"changed_data_operands":0})
    return {"functions":sorted(records.values(),key=lambda f:f["address"]),"sequence_proofs":sequences,
            "data_proofs":call_proofs,"counts":{"functions":9,"code_bytes":1616,"source_units":2,
            "closed_return_bodies":9,"reviewed_complete_caller_callee_clusters":1}}
