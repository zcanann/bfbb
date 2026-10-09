"""Complete original French voice controls and playing-state leaves."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES,TARGET,require
from platforms.france_tu_sequences import compare,digest,unique_template
from platforms.france_hangable_sequence import hangable_pair
from platforms.france_player_animation_context import closed,storage
from platforms.france_sound_playback import playback_data
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages

SOURCE="SB/Core/p2/iSnd.cpp"
API="SB/Core/p2/his/HISAPI.cpp"
MEMBERS = [('iSndGetVol__FUi', 1804528, 144),
 ('iSndSetVol__FUif', 1804672, 492),
 ('iSndStop__FUi', 1805168, 184),
 ('iSndPause__FUiUi', 1805360, 140),
 ('iSndIsReady__FUi', 1806880, 120),
 ('iSndIsPlayingByHandle__FUi', 1808128, 112),
 ('iSndIsPlaying__FUiUi', 1808240, 112),
 ('iSndIsPlaying__FUi', 1808352, 88)]
INDEPENDENT={0x1b8cc0:(SOURCE,"iSndFindFreeVoice",900),
 0x34bd30:(API,"HISSetVoiceVolumeAsync",64),0x34bcb0:(API,"HISStopVoiceAsync",56),
 0x34bc70:(API,"HISPauseVoiceAsync",56),0x34bc30:(API,"HISResumeVoiceAsync",56)}
CALLS = {1804528: [],
 1804672: [(432, 3456304)],
 1805168: [(52, 3456176)],
 1805360: [(68, 3456112), (88, 3456048)],
 1806880: [],
 1808128: [],
 1808240: [],
 1808352: []}
OPERANDS = {1804528: [(0, 8, 28), (48, 52, 8)],
 1804672: [(24, 32, 28), (96, 100, 0)],
 1805168: [(4, 12, 0), (64, 72, 48)],
 1805360: [(4, 24, 0)],
 1806880: [(0, 8, 0)],
 1808128: [(0, 8, 0), (32, 40, 48)],
 1808240: [(0, 8, 0)],
 1808352: [(0, 8, 0)]}


def sound_object(original,target,known):
    snd=playback_data(original)[0]
    # The complete independently proved allocator pins the object's target base.
    anchor=known[0x1b8cc0]
    prior=next(p for p in anchor["provenance"] if p["version"]==original.version)
    a=prior["source_address"]
    identity=checked_identity(original,target,a,0x1b8cc0,known)
    pairs,calls,masks=compare(original,target,a,0x1b8cc0,900,address_resolver=hangable_pair)
    require(pairs==prior["data_address_operands"] and calls==[{k:c[k] for k in ("offset","opcode","reference_address","target_address")} for c in prior["direct_transfers"]],
            "Sound controls allocator anchor operands differ")
    witnesses=[p for p in pairs if p["reference_address"]==snd["reference_address"] and p["target_address"]==0x5b7400]
    require(witnesses,"Sound controls allocator does not pin the typed global base")
    for binary,base in ((original,snd["reference_address"]),(target,0x5b7400)):
        storage(binary,base,snd["size"],"runtime_bss")
    snd["target_address"]=0x5b7400
    return snd,{"identity":{k:identity[k] for k in ("source","name","address","size","sha256")},"operands":witnesses}


def generate_unit(originals,registry_dir):
    target=originals[TARGET];known={}
    # Fixed dependency fence prevents later identities from changing this proof.
    for path in sorted(registry_dir.glob("*functions.json")):
        for f in json.loads(path.read_text())["functions"]:
            if f["address"] in INDEPENDENT:
                old=known.setdefault(f["address"],f)
                require(all(old[k]==f[k] for k in ("source","name","size","sha256")),"Conflicting voice control dependency")
    require(set(known)==set(INDEPENDENT),"Independent voice control dependency missing")
    for address,identity in INDEPENDENT.items():
        require(tuple(known[address][k] for k in ("source","name","size"))==identity,"Voice control dependency identity differs")
    records={};sequences=[];data_proofs=[];call_proofs=[]
    for version in REFERENCES:
        original=originals[version];links=canonical_linkages(original.data,original.metadata)
        snd,anchor=sound_object(original,target,known);identities=[];operands=[];transfers=[]
        for linkage,b,size in MEMBERS:
            refs=[f for f in original.functions if f["source"]==SOURCE and links.get(f["low"])==linkage]
            require(len(refs)==1 and refs[0]["high"]-refs[0]["low"]==size,"Complete voice control original identity differs")
            f=refs[0];a=f["low"];require(a%16==b%16==0,"Voice control entry alignment differs")
            padding=(-size)%16
            require(not any(original.read(a+size,padding)) and not any(target.read(b+size,padding)),"Voice control padding differs")
            pairs,calls,masks=compare(original,target,a,b,size,address_resolver=hangable_pair)
            require([(p["hi_offset"],p["lo_offset"],p["target_address"]-0x5b7400) for p in pairs]==OPERANDS[b],
                    "Voice control data inventory differs")
            expected={}
            for p in pairs:
                offset=p["target_address"]-0x5b7400;path=snd["paths"].get(str(offset))
                require(path and p["reference_address"]==snd["reference_address"]+offset and
                        p["opcode"]==9 and p["opcode"] in path["opcodes"] and p["storage"]=="zero_fill",
                        "Voice control operand lost its typed global path")
                expected[p["lo_offset"]]=0xffff0000
                if original.read(a+p["hi_offset"],4)!=target.read(b+p["hi_offset"],4):expected[p["hi_offset"]]=0xffff0000
                operands.append({"caller":linkage,"caller_address":b,"path":path["path"],**p})
            require([(c["offset"],c["target_address"]) for c in calls]==CALLS[b] and all(c["opcode"]==3 for c in calls),
                    "Voice control complete callee inventory differs")
            for c in calls:
                evidence=checked_identity(original,target,c["reference_address"],c["target_address"],known)
                expected[c["offset"]]=0xfc000000
                transfers.append({"caller":linkage,"caller_address":b,**c,
                    "callee":{k:evidence[k] for k in ("source","name","address","size","sha256")}})
            require(masks==expected,"Voice control masks exceed typed operands and complete calls")
            boundary=closed(original,target,a,b,size);unique=unique_template(target,original.read(a,size),masks,b)
            identities.append({"source":SOURCE,"linkage_name":linkage,"source_address":a,"target_address":b,"size":size,
                               "uniqueness":unique,"terminal_alignment_bytes":padding})
            if b not in records:
                records[b]={"name":f["name"],"source":SOURCE,"address":b,"size":size,"sha256":digest(target.read(b,size)),
                    "boundary_confirmation":True,"confirmation_kind":CLUSTER_KIND,"provenance":[],"corroboration":{
                    "proof_scope":"complete_original_voice_controls_and_playing_state_leaves",
                    "local_control_flow":boundary,"whole_translation_unit_claimed":False}}
            records[b]["provenance"].append({"version":version,"executable_sha1":original.sha1,"source_address":a,
                "source":SOURCE,"name":f["name"],"linkage_name":linkage,"reference_sha256":digest(original.read(a,size)),
                "data_address_operands":pairs,"direct_transfers":calls})
        sequences.append({"version":version,"complete_bodies":identities,"whole_translation_unit_claimed":False})
        data_proofs.append({"version":version,"typed_sound_global":snd,"independent_base_anchor":anchor,"operands":operands})
        call_proofs.append({"version":version,"complete_callees":transfers})
    return {"functions":sorted(records.values(),key=lambda f:f["address"]),"sequence_proofs":sequences,
            "data_proofs":data_proofs,"call_proofs":call_proofs,
            "counts":{"functions":8,"code_bytes":1392,"source_units":1,"closed_return_bodies":8,
                      "reviewed_complete_caller_callee_clusters":8}}

