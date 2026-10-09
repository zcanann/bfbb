"""Original anchored playback-wrapper cluster; tiny tails are never weak seeds."""
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
ANCHOR=0x209d60
SPAN=1520
MEMBERS=[("xSndPlayInternal__FUiffUiUiUiP4xEntPC5xVec3ff14sound_categoryf",ANCHOR,1376),
         ("xSndPlay3D__FUiffUiUiPC5xVec3ff14sound_categoryf",0x20a2c0,72),
         ("xSndPlay3D__FUiffUiUiP4xEntff14sound_categoryf",0x20a310,24),
         ("xSndPlay__FUiffUiUiUi14sound_categoryf",0x20a330,28)]
TAIL_WORDS={0x20a310:(0x00e0102d,0x0100502d,0x0000382d,0x0040402d,0x08082758,0x0000482d),
            0x20a330:(0x46007406,0x0100502d,0x44807000,0x0000402d,0x0000482d,0x08082758,0x460073c6)}
CALLS={0x20a2c0:[(32,3),(52,3)],0x20a310:[(16,2)],0x20a330:[(20,2)]}


def tail(original,target,a,b,size):
    require(b in TAIL_WORDS and len(TAIL_WORDS[b])*4==size,"Unreviewed sound wrapper tail")
    expected=list(TAIL_WORDS[b]);offset=CALLS[b][0][0]
    require(words(target.read(b,size))==expected,"Sound wrapper fixed argument/tail instructions differ")
    ref=words(original.read(a,size));expected[offset//4]=ref[offset//4]
    require(ref==expected and ref[offset//4]>>26==2,"Original sound wrapper fixed tail instructions differ")
    # Both inventories contain only argument moves, the terminal J, and its
    # reviewed delay instruction. SP and RA are neither read nor written.
    return {"passes":True,"proof":"fixed_argument_moves_then_complete_playback_tail",
            "terminal_transfer_offset":offset,"delay_word":TAIL_WORDS[b][-1],
            "stack_pointer_preserved":True,"return_address_preserved":True}


def generate_unit(originals,registry_dir):
    target=originals[TARGET];known={}
    for path in sorted(registry_dir.glob("*functions.json")):
        for f in json.loads(path.read_text())["functions"]:
            if f["address"]==ANCHOR:
                old=known.setdefault(ANCHOR,f)
                require(all(old[k]==f[k] for k in ("source","name","size","sha256")),"Conflicting sound wrapper anchor")
    require(set(known)=={ANCHOR} and tuple(known[ANCHOR][k] for k in ("source","name","size"))==
            (SOURCE,"xSndPlayInternal",1376),"Complete playback wrapper anchor missing or changed")
    records={};sequences=[];contexts=[]
    for version in REFERENCES:
        original=originals[version];links=canonical_linkages(original.data,original.metadata)
        anchor=known[ANCHOR];reference=next(p for p in anchor["provenance"] if p["version"]==version)
        start=reference["source_address"];checked_identity(original,target,start,ANCHOR,known)
        expected=[];masks={};functions=[]
        for linkage,b,size in MEMBERS:
            matches=[f for f in original.functions if f["source"]==SOURCE and links.get(f["low"])==linkage]
            require(len(matches)==1,"Sound wrapper original linkage/owner ambiguous")
            f=matches[0];a=f["low"]
            require(a==start+b-ANCHOR and f["high"]-a==size and a%16==b%16==0,
                    "Complete original sound wrapper order/extent differs")
            expected.append((a,a+size,linkage))
            padding=(-size)%16
            require(not any(original.read(a+size,padding)) and not any(target.read(b+size,padding)),"Sound wrapper cluster padding differs")
            pairs,calls,local=compare(original,target,a,b,size,address_resolver=hangable_pair)
            if b==ANCHOR:
                require(pairs==reference["data_address_operands"] and calls==reference["direct_transfers"],
                        "Playback anchor operand inventory differs from its independent proof")
                bounds=closed(original,target,a,b,size)
            else:
                require(not pairs and [(c["offset"],c["opcode"]) for c in calls]==CALLS[b] and
                        all(c["reference_address"]==start and c["target_address"]==ANCHOR for c in calls),
                        "Sound wrapper must transfer directly to its complete anchor")
                require(local=={offset:0xfc000000 for offset,_ in CALLS[b]},"Sound wrapper masks exceed complete playback transfers")
                bounds=closed(original,target,a,b,size) if b==0x20a2c0 else tail(original,target,a,b,size)
            masks.update({a-start+off:mask for off,mask in local.items()})
            functions.append({"source_address":a,"target_address":b,"size":size,"linkage_name":linkage,
                              "terminal_alignment_bytes":padding,"bounds":bounds})
            if b==ANCHOR:continue
            if b not in records:
                records[b]={"name":f["name"],"source":SOURCE,"address":b,"size":size,"sha256":digest(target.read(b,size)),
                    "boundary_confirmation":True,"confirmation_kind":CLUSTER_KIND,"provenance":[],"corroboration":{
                    "proof_scope":"complete_original_playback_wrapper_cluster_anchored_by_proven_internal_body",
                    "local_control_flow":bounds,"whole_translation_unit_claimed":False}}
            records[b]["provenance"].append({"version":version,"executable_sha1":original.sha1,"source_address":a,
                "source":SOURCE,"name":f["name"],"linkage_name":linkage,"reference_sha256":digest(original.read(a,size)),
                "data_address_operands":pairs,"direct_transfers":calls})
        members=[f for f in original.functions if f["source"]==SOURCE and start<=f["low"]<start+SPAN]
        require(all(f["low"] in links for f in members),"Playback cluster member lacks original canonical linkage")
        actual=sorted((f["low"],f["high"],links[f["low"]]) for f in members)
        require(actual==expected,"Original playback cluster complete membership differs")
        unique=unique_template(target,original.read(start,SPAN),masks,ANCHOR)
        sequences.append({"version":version,"source_address":start,"target_address":ANCHOR,"span":SPAN,
                          "complete_members":functions,"unique_full_cluster_template":unique,
                          "new_code_bytes":124,"anchor_already_proved_bytes":1376,"whole_translation_unit_claimed":False})
        contexts.append({"version":version,"fixed_complete_anchor":{k:anchor[k] for k in ("source","name","address","size","sha256")}})
    return {"functions":sorted(records.values(),key=lambda f:f["address"]),"sequence_proofs":sequences,"data_proofs":contexts,
            "counts":{"functions":3,"code_bytes":124,"source_units":1,"closed_return_bodies":1,
                      "complete_tail_wrappers":2,"reviewed_complete_caller_callee_clusters":1}}
