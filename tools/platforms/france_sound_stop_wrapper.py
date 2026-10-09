"""Original-only stop tail identified inside two complete sound anchors."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES,TARGET,require
from platforms.france_tu_sequences import compare,digest,unique_template,words
from platforms.france_hangable_sequence import hangable_pair
from platforms.france_player_animation_context import closed
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages

SOURCE="SB/Core/x/xSnd.cpp"
ANCHOR=0x209c80
SPAN=176
TAIL=0x209cc0
CALLEE=0x1b8b70
MEMBERS=[("xSndParentDied__FUi",ANCHOR,64),
         ("xSndStop__FUi",TAIL,8),
         ("xSndIDIsPlaying__FUi",0x209cd0,88)]
INDEPENDENT={ANCHOR:(SOURCE,"xSndParentDied",64),0x209cd0:(SOURCE,"xSndIDIsPlaying",88),
             CALLEE:("SB/Core/p2/iSnd.cpp","iSndStop",184)}


def generate_unit(originals,registry_dir):
    target=originals[TARGET];known={}
    for path in sorted(registry_dir.glob("*functions.json")):
        for f in json.loads(path.read_text())["functions"]:
            if f["address"] in INDEPENDENT:
                old=known.setdefault(f["address"],f)
                require(all(old[k]==f[k] for k in ("source","name","size","sha256")),"Conflicting sound stop dependency")
    require(set(known)==set(INDEPENDENT),"Complete sound stop dependencies missing")
    for a,identity in INDEPENDENT.items():
        require(tuple(known[a][k] for k in ("source","name","size"))==identity,"Sound stop dependency identity differs")
    record=None;sequences=[];contexts=[]
    for version in REFERENCES:
        original=originals[version];links=canonical_linkages(original.data,original.metadata)
        reference=next(p for p in known[ANCHOR]["provenance"] if p["version"]==version)
        start=reference["source_address"];expected=[];masks={};functions=[];dependencies=[]
        for linkage,b,size in MEMBERS:
            matches=[f for f in original.functions if f["source"]==SOURCE and links.get(f["low"])==linkage]
            require(len(matches)==1,"Sound stop original linkage/owner ambiguous")
            f=matches[0];a=f["low"]
            require(a==start+b-ANCHOR and f["high"]-a==size and a%16==b%16==0,
                    "Complete original sound stop order/extent differs")
            expected.append((a,a+size,linkage));padding=(-size)%16
            require(not any(original.read(a+size,padding)) and not any(target.read(b+size,padding)),"Sound stop cluster padding differs")
            pairs,calls,local=compare(original,target,a,b,size,address_resolver=hangable_pair)
            if b!=TAIL:
                evidence=checked_identity(original,target,a,b,known)
                prior=next(p for p in evidence["provenance"] if p["version"]==version)
                require(pairs==prior["data_address_operands"] and calls==prior["direct_transfers"],
                        "Sound stop anchor operand inventory differs from its independent proof")
                bounds=closed(original,target,a,b,size)
            else:
                require(not pairs and len(calls)==1 and calls[0]["offset"]==0 and calls[0]["opcode"]==2 and
                        calls[0]["target_address"]==CALLEE,"Sound stop must tail-transfer directly to complete iSndStop")
                evidence=checked_identity(original,target,calls[0]["reference_address"],CALLEE,known)
                require(words(target.read(b,size))==[0x0806e2dc,0] and
                        words(original.read(a,size))==[0x08000000|(calls[0]["reference_address"]>>2),0],
                        "Sound stop fixed J/nop body differs")
                require(local=={0:0xfc000000},"Sound stop masks exceed its single complete callee transfer")
                bounds={"passes":True,"proof":"fixed_J_nop_tail_inside_two_complete_original_anchors",
                        "terminal_transfer_offset":0,"delay_word":0,"stack_pointer_preserved":True,"return_address_preserved":True}
            dependencies.append({k:evidence[k] for k in ("source","name","address","size","sha256")})
            masks.update({a-start+off:mask for off,mask in local.items()})
            functions.append({"source_address":a,"target_address":b,"size":size,"linkage_name":linkage,
                              "terminal_alignment_bytes":padding,"bounds":bounds})
            if b!=TAIL:continue
            if record is None:
                record={"name":f["name"],"source":SOURCE,"address":b,"size":size,"sha256":digest(target.read(b,size)),
                    "boundary_confirmation":True,"confirmation_kind":CLUSTER_KIND,"provenance":[],"corroboration":{
                    "proof_scope":"complete_original_stop_tail_between_two_independently_proven_bodies",
                    "local_control_flow":bounds,"whole_translation_unit_claimed":False}}
            record["provenance"].append({"version":version,"executable_sha1":original.sha1,"source_address":a,
                "source":SOURCE,"name":f["name"],"linkage_name":linkage,"reference_sha256":digest(original.read(a,size)),
                "data_address_operands":pairs,"direct_transfers":calls})
        members=[f for f in original.functions if f["source"]==SOURCE and start<=f["low"]<start+SPAN]
        require(all(f["low"] in links for f in members),"Sound stop cluster member lacks original canonical linkage")
        actual=sorted((f["low"],f["high"],links[f["low"]]) for f in members)
        require(actual==expected,"Original sound stop cluster complete membership differs")
        unique=unique_template(target,original.read(start,SPAN),masks,ANCHOR)
        sequences.append({"version":version,"source_address":start,"target_address":ANCHOR,"span":SPAN,
                          "complete_members":functions,"unique_full_cluster_template":unique,
                          "new_code_bytes":8,"anchor_already_proved_bytes":152,"whole_translation_unit_claimed":False})
        contexts.append({"version":version,"fixed_complete_dependencies":dependencies})
    return {"functions":[record],"sequence_proofs":sequences,"data_proofs":contexts,
            "counts":{"functions":1,"code_bytes":8,"source_units":1,"closed_return_bodies":0,
                      "complete_tail_wrappers":1,"reviewed_complete_caller_callee_clusters":1}}
