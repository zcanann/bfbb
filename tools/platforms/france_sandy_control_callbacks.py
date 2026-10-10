"""Complete original French Sandy callbacks using typed player control fields."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES,TARGET,require
from platforms.france_tu_sequences import compare,digest,unique_template
from platforms.france_hangable_sequence import hangable_pair
from platforms.france_player_animation_context import OriginalData,closed
from platforms.france_boss_goal_kernels import model_field
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages

SOURCE="SB/Game/zNPCTypeBossSandy.cpp"
MEMBERS = [('getUpCB__FP5xGoalPvP11en_trantypefPv', 3345360, 428),
 ('elbowDropCB__FP5xGoalPvP11en_trantypefPv', 3346304, 384),
 ('tauntCB__FP5xGoalPvP11en_trantypefPv', 3348352, 332),
 ('idleCB__FP5xGoalPvP11en_trantypefPv', 3348688, 232)]
INDEPENDENT={0x2c70a0:("SB/Game/zNPCGoalStd.cpp","CalcNewDir",624),
             0x2d11e0:("SB/Game/zNPCTypeCommon.cpp","AnimTimeRemain",104)}
CALLS = {3345360: [(116, 2953696)], 3346304: [(172, 2953696)], 3348352: [(168, 2953696)], 3348688: []}
OPERANDS = {3345360: [(4, 56, 5426196), (248, 252, 5430504)],
 3346304: [(104, 116, 5426196), (208, 212, 5430504)],
 3348352: [(104, 116, 5426196), (204, 208, 5430504)],
 3348688: [(32, 36, 5430504), (88, 112, 5426196)]}


def control_fields(original,target,known):
    model=model_field(original,target,known)
    data=OriginalData(original)
    member=data.global_fields.get(6136)
    require(member and member["name"]=="ControlOff" and member["owner"]=="zPlayerGlobals" and
            member["offset"]==4344 and member["type_attributes"]=={"5":9},
            "Sandy callback control flag is not the original unsigned player member")
    require(data.globals[2]==model["globals"]["reference_address"],"Sandy callback globals owners disagree")
    return {"model_field":model,"control_field":{"reference_address":data.globals[2]+6136,
            "target_address":0x52dce8,"member":member,"path":["globals","player","ControlOff"],
            "component_offsets":[1792,4344],"field_size":4,"data_extent_promoted":False}}


def generate_unit(originals,registry_dir):
    target=originals[TARGET];known={}
    # Fixed dependency fence prevents later identities from changing this proof.
    for path in sorted(registry_dir.glob("*functions.json")):
        for f in json.loads(path.read_text())["functions"]:
            if f["address"] in INDEPENDENT:
                old=known.setdefault(f["address"],f)
                require(all(old[k]==f[k] for k in ("source","name","size","sha256")),"Conflicting Sandy control callback dependency")
    require(set(known)==set(INDEPENDENT),"Independent Sandy control callback dependency missing")
    for address,identity in INDEPENDENT.items():
        require(tuple(known[address][k] for k in ("source","name","size"))==identity,"Sandy control callback dependency identity differs")
    records={};sequences=[];data_proofs=[];call_proofs=[]
    for version in REFERENCES:
        original=originals[version];links=canonical_linkages(original.data,original.metadata)
        fields=control_fields(original,target,known);identities=[];operands=[];transfers=[]
        for linkage,b,size in MEMBERS:
            refs=[f for f in original.functions if f["source"]==SOURCE and links.get(f["low"])==linkage]
            require(len(refs)==1 and refs[0]["high"]-refs[0]["low"]==size,"Complete Sandy control callback original identity differs")
            f=refs[0];a=f["low"];require(a%16==b%16==0,"Sandy control callback entry alignment differs")
            padding=(-size)%16
            require(not any(original.read(a+size,padding)) and not any(target.read(b+size,padding)),"Sandy control callback padding differs")
            pairs,calls,masks=compare(original,target,a,b,size,address_resolver=hangable_pair)
            require([(p["hi_offset"],p["lo_offset"],p["target_address"]) for p in pairs]==OPERANDS[b],
                    "Sandy control callback data inventory differs")
            expected={}
            for p in pairs:
                candidates=[field for field in fields.values() if field["target_address"]==p["target_address"]]
                require(len(candidates)==1 and p["reference_address"]==candidates[0]["reference_address"] and
                        p["opcode"]==35 and p["storage"]=="zero_fill","Sandy callback operand lost its typed player path")
                path=candidates[0]["path"]
                expected[p["lo_offset"]]=0xffff0000
                if original.read(a+p["hi_offset"],4)!=target.read(b+p["hi_offset"],4):expected[p["hi_offset"]]=0xffff0000
                operands.append({"caller":linkage,"caller_address":b,"path":path,**p})
            require([(c["offset"],c["target_address"]) for c in calls]==CALLS[b] and all(c["opcode"]==3 for c in calls),
                    "Sandy control callback complete callee inventory differs")
            for c in calls:
                evidence=checked_identity(original,target,c["reference_address"],c["target_address"],known)
                expected[c["offset"]]=0xfc000000
                transfers.append({"caller":linkage,"caller_address":b,**c,
                    "callee":{k:evidence[k] for k in ("source","name","address","size","sha256")}})
            require(masks==expected,"Sandy control callback masks exceed typed operands and complete calls")
            boundary=closed(original,target,a,b,size);unique=unique_template(target,original.read(a,size),masks,b)
            identities.append({"source":SOURCE,"linkage_name":linkage,"source_address":a,"target_address":b,"size":size,
                               "uniqueness":unique,"terminal_alignment_bytes":padding})
            if b not in records:
                records[b]={"name":f["name"],"source":SOURCE,"address":b,"size":size,"sha256":digest(target.read(b,size)),
                    "boundary_confirmation":True,"confirmation_kind":CLUSTER_KIND,"provenance":[],"corroboration":{
                    "proof_scope":"complete_original_sandy_control_callbacks",
                    "local_control_flow":boundary,"whole_translation_unit_claimed":False}}
            records[b]["provenance"].append({"version":version,"executable_sha1":original.sha1,"source_address":a,
                "source":SOURCE,"name":f["name"],"linkage_name":linkage,"reference_sha256":digest(original.read(a,size)),
                "data_address_operands":pairs,"direct_transfers":calls})
        sequences.append({"version":version,"complete_bodies":identities,"whole_translation_unit_claimed":False})
        data_proofs.append({"version":version,"typed_player_fields":fields,"operands":operands})
        call_proofs.append({"version":version,"complete_callees":transfers})
    return {"functions":sorted(records.values(),key=lambda f:f["address"]),"sequence_proofs":sequences,
            "data_proofs":data_proofs,"call_proofs":call_proofs,
            "counts":{"functions":4,"code_bytes":1376,"source_units":1,"closed_return_bodies":4,
                      "reviewed_complete_caller_callee_clusters":4}}

