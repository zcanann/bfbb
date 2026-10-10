"""Complete original French particle builders and hazard kernels."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES,TARGET,require
from platforms.france_tu_sequences import compare,digest,unique_template
from platforms.france_hangable_sequence import hangable_pair
from platforms.france_player_animation_context import OriginalData,closed,storage
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages
from platforms.ps2_type_layouts import aggregate_layouts

SUPPLEMENT="SB/Game/zNPCSupplement.cpp"
HAZARD="SB/Game/zNPCHazard.cpp"
MEMBERS=[(SUPPLEMENT,"ConfigPar__17NPARParmVisSplashCFP8NPARData11en_nparmodePC5xVec3PC5xVec3",0x3b80f0,284),
 (SUPPLEMENT,"ConfigPar__19NPARParmChuckSplashCFP8NPARData11en_nparmodePC5xVec3PC5xVec3",0x3b89d0,540),
 (SUPPLEMENT,"ConfigPar__20NPARParmTubeConfettiCFP8NPARData11en_nparmodePC5xVec3PC5xVec3",0x3baf00,1144),
 (SUPPLEMENT,"ConfigPar__14NPARParmOilBubCFP8NPARData11en_nparmodePC5xVec3PC5xVec3",0x3bc8a0,304),
 (HAZARD,"StreakUpdate__9NPCHazardFUif",0x3c05d0,544),
 (HAZARD,"ColResp_Default__9NPCHazardFP12xSweptSpheref",0x3c8f00,280),
 (HAZARD,"ColTestCyl__9NPCHazardFPC6xBoundff",0x3c9840,240),
 (HAZARD,"PosSet__9NPCHazardFPC5xVec3",0x3cb840,80),
 (HAZARD,"HAZ_ord_sorttest__FPvPv",0x3cd690,88)]
INDEPENDENT={0x1ee4e0:("SB/Core/x/xMath.cpp","xurand",88),
             0x1e8170:("SB/Core/x/xFX.cpp","xFXStreakUpdate",208)}
CALLS={0x3b80f0:[(44,0x1ee4e0)],0x3b89d0:[(44,0x1ee4e0)],
       0x3baf00:[(48,0x1ee4e0),(196,0x1ee4e0),(300,0x1ee4e0),(404,0x1ee4e0)],
       0x3bc8a0:[(44,0x1ee4e0)],0x3c05d0:[(524,0x1e8170)],
       0x3c8f00:[],0x3c9840:[],0x3cb840:[],0x3cd690:[]}


def zero_vector(original,target):
    data=OriginalData(original)
    layout=aggregate_layouts(data.debug,SUPPLEMENT,{"xVec3"})["xVec3"]
    require(layout["size"]==12 and [(m["name"],m["offset"],m["type_attributes"]) for m in layout["members"]]==
            [("x",0,{"5":14}),("y",4,{"5":14}),("z",8,{"5":14})],"Particle zero vector component layout differs")
    off,decl,address=data.declaration(SUPPLEMENT,"g_O3")
    require(decl.get(7)==layout["die_offset"],"Particle zero vector declaration type differs")
    for binary,a in ((original,address),(target,0x4f89a0)):
        storage(binary,a,12,"initialized_data")
        require(binary.read(a,12)==bytes(12),"Complete particle zero vector payload differs")
    return {"name":"g_O3","reference_address":address,"target_address":0x4f89a0,"size":12,
            "declaration_die":off,"layout":layout,"payload":"00"*12,"data_extent_promoted":False}


def generate_unit(originals,registry_dir):
    target=originals[TARGET];known={}
    # Fixed original-only dependency fence: new registry entries cannot affect evidence.
    for path in sorted(registry_dir.glob("*functions.json")):
        for f in json.loads(path.read_text())["functions"]:
            if f["address"] in INDEPENDENT:
                old=known.setdefault(f["address"],f)
                require(all(old[k]==f[k] for k in ("source","name","size","sha256")),"Conflicting particle kernel dependency")
    require(set(known)==set(INDEPENDENT),"Independent particle kernel dependency missing")
    for a,identity in INDEPENDENT.items():
        require(tuple(known[a][k] for k in ("source","name","size"))==identity,"Particle kernel dependency identity differs")
    records={};sequences=[];data_proofs=[];call_proofs=[]
    for version in REFERENCES:
        original=originals[version];links=canonical_linkages(original.data,original.metadata)
        vector=zero_vector(original,target);identities=[];operands=[];transfers=[]
        for source,linkage,b,size in MEMBERS:
            refs=[f for f in original.functions if f["source"]==source and links.get(f["low"])==linkage]
            require(len(refs)==1 and refs[0]["high"]-refs[0]["low"]==size,"Complete particle kernel original identity differs")
            f=refs[0];a=f["low"];require(a%16==b%16==0,"Particle kernel entry alignment differs")
            padding=(-size)%16
            require(not any(original.read(a+size,padding)) and not any(target.read(b+size,padding)),"Particle kernel padding differs")
            pairs,calls,masks=compare(original,target,a,b,size,address_resolver=hangable_pair)
            inventory=[(128,132),(156,160),(184,188)] if b==0x3bc8a0 else []
            require([(p["hi_offset"],p["lo_offset"]) for p in pairs]==inventory,"Particle kernel data inventory differs")
            expected={}
            for p in pairs:
                require(p["reference_address"]==vector["reference_address"] and p["target_address"]==vector["target_address"] and
                        p["opcode"]==9 and p["storage"]=="file_backed","Particle kernel operand lost its complete typed zero vector")
                expected[p["lo_offset"]]=0xffff0000
                if original.read(a+p["hi_offset"],4)!=target.read(b+p["hi_offset"],4):expected[p["hi_offset"]]=0xffff0000
                operands.append({"caller":linkage,"caller_address":b,**p})
            require([(c["offset"],c["target_address"]) for c in calls]==CALLS[b] and all(c["opcode"]==3 for c in calls),
                    "Particle kernel complete callee inventory differs")
            for c in calls:
                evidence=checked_identity(original,target,c["reference_address"],c["target_address"],known)
                expected[c["offset"]]=0xfc000000
                transfers.append({"caller":linkage,"caller_address":b,**c,"callee":evidence})
            require(masks==expected,"Particle kernel masks exceed reviewed typed operands and complete calls")
            boundary=closed(original,target,a,b,size);unique=unique_template(target,original.read(a,size),masks,b)
            identities.append({"source":source,"linkage_name":linkage,"source_address":a,"target_address":b,"size":size,
                               "uniqueness":unique,"terminal_alignment_bytes":padding})
            if b not in records:
                records[b]={"name":f["name"],"source":source,"address":b,"size":size,"sha256":digest(target.read(b,size)),
                    "boundary_confirmation":True,"confirmation_kind":CLUSTER_KIND,"provenance":[],"corroboration":{
                    "proof_scope":"complete_original_particle_builders_and_hazard_kernels",
                    "local_control_flow":boundary,"whole_translation_unit_claimed":False}}
            records[b]["provenance"].append({"version":version,"executable_sha1":original.sha1,"source_address":a,
                "source":source,"name":f["name"],"linkage_name":linkage,"reference_sha256":digest(original.read(a,size)),
                "data_address_operands":pairs,"direct_transfers":calls})
        sequences.append({"version":version,"complete_bodies":identities,"whole_translation_unit_claimed":False})
        data_proofs.append({"version":version,"typed_zero_vector":vector,"operands":operands})
        call_proofs.append({"version":version,"complete_callees":transfers})
    return {"functions":sorted(records.values(),key=lambda f:f["address"]),"sequence_proofs":sequences,
            "data_proofs":data_proofs,"call_proofs":call_proofs,
            "counts":{"functions":9,"code_bytes":3504,"source_units":2,"closed_return_bodies":9,
                      "reviewed_complete_caller_callee_clusters":9}}
