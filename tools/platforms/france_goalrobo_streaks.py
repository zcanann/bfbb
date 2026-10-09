"""Complete original robotic-goal streak callers and their typed effects helper."""
from __future__ import annotations

from platforms.verify_reviewed import REFERENCES,TARGET,require
from platforms.france_tu_sequences import compare,digest,unique_template
from platforms.france_player_animation_context import OriginalData,closed,storage
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_type_layouts import aggregate_layouts
from platforms.ps2_source import canonical_linkages

GOAL="SB/Game/zNPCGoalRobo.cpp"
EFFECTS="SB/Core/x/xFX.cpp"
MEMBERS=[(EFFECTS,"xFXStreakUpdate__FUiPC5xVec3PC5xVec3",0x1e8170,208),
         (EFFECTS,"xFXStreakStop__FUi",0x1e8240,96),
         (GOAL,"FXStreakUpdate__22zNPCGoalAttackArfMeleeFv",0x2b7590,1636),
         (GOAL,"FXStreakUpdate__20zNPCGoalAttackHammerFP5xVec3",0x2b8a50,868)]


def original_streaks(original):
    data=OriginalData(original)
    layouts=aggregate_layouts(data.debug,EFFECTS,{"xFXStreak","xFXStreakElem","xVec3"})
    require({n:v["size"] for n,v in layouts.items()}=={"xFXStreak":1644,"xFXStreakElem":32,"xVec3":12},
            "Original complete streak component sizes differ")
    fields={n:{m["name"]:m for m in v["members"]} for n,v in layouts.items()}

    def field(owner,name,offset,typ):
        m=fields[owner].get(name)
        require(m is not None and m["offset"]==offset and m["type_attributes"]==typ,
                "Original streak member offset or type differs")
        return m

    def array(die,count,element):
        tag,_,a=data.by[die]
        descriptor=bytes.fromhex("000a0000000000")+(count-1).to_bytes(4,"little")+bytes.fromhex("087200")+element.to_bytes(4,"little")
        require(tag==1 and a.get(9)==0 and a.get(10)==descriptor,"Original streak array bound or element differs")
        return {"type_die":die,"count":count,"element_die":element,"descriptor":descriptor.hex()}

    for name,offset in (("frequency",4),("alphaStart",12),("elapsed",16)):
        field("xFXStreak",name,offset,{"5":14})
    field("xFXStreak","head",24,{"5":9})
    field("xFXStreakElem","flag",0,{"5":9})
    field("xFXStreakElem","a",28,{"5":14})
    for i,name in enumerate(("x","y","z")):
        field("xVec3",name,i*4,{"5":14})
    elem=fields["xFXStreak"]["elem"]
    points=fields["xFXStreakElem"]["p"]
    require(elem["offset"]==44 and points["offset"]==4,"Original streak nested arrays moved")
    off,decl,address=data.declaration(EFFECTS,"sStreakList")
    arrays=[array(decl[7],10,layouts["xFXStreak"]["die_offset"]),
            array(elem["type_attributes"]["7"],50,layouts["xFXStreakElem"]["die_offset"]),
            array(points["type_attributes"]["7"],2,layouts["xVec3"]["die_offset"])]
    return {"name":"sStreakList","source":EFFECTS,"reference_address":address,"size":16440,
            "declaration_die":off,"layouts":layouts,"arrays":arrays}


def generate_unit(originals,registry_dir):
    # The whole four-member cluster is proved directly. No changing registry
    # inventory contributes an identity or conditional context to these records.
    target=originals[TARGET]
    records,sequences,data_proofs={},[],[]
    for version in REFERENCES:
        original=originals[version]
        links=canonical_linkages(original.data,original.metadata)
        data=original_streaks(original)
        functions={}
        for source,linkage,b,size in MEMBERS:
            refs=[f for f in original.functions if f["source"]==source and links.get(f["low"])==linkage]
            require(len(refs)==1 and refs[0]["high"]-refs[0]["low"]==size,"Complete streak caller/helper identity differs")
            f=refs[0];a=f["low"]
            require(a%16==b%16==0,"Streak caller/helper entry alignment differs")
            pairs,calls,masks=compare(original,target,a,b,size)
            functions[b]=dict(source=source,name=f["name"],linkage=linkage,a=a,b=b,size=size,
                              pairs=pairs,calls=calls,masks=masks,bounds=closed(original,target,a,b,size))
        helper=functions[0x1e8170]
        stop=functions[0x1e8240]
        require(len(helper["pairs"])==1 and not helper["calls"],"Complete effects helper inventory differs")
        require(len(stop["pairs"])==1 and not stop["calls"],"Complete effects stop inventory differs")
        p=helper["pairs"][0]
        require((p["hi_offset"],p["lo_offset"],p["opcode"],p["storage"],p["reference_address"])==
                (12,20,9,"zero_fill",data["reference_address"]),"Streak helper operand loses its typed array base")
        require(stop["pairs"][0]==p and p["reference_address"]%4==p["target_address"]%4==0,
                "Independent complete streak consumers disagree on the naturally aligned array base")
        for binary,address in ((original,data["reference_address"]),(target,p["target_address"])):
            storage(binary,address,data["size"],"runtime_bss")
        unique=[]
        for b,f in functions.items():
            offsets={0x1e8170:[],0x1e8240:[],0x2b7590:[964,1560],0x2b8a50:[580,812]}[b]
            require([(c["offset"],c["opcode"],c["reference_address"],c["target_address"]) for c in f["calls"]]==
                    [(off,3,helper["a"],helper["b"]) for off in offsets],"Streak caller does not reach the complete original helper")
            require(f["source"]==EFFECTS or not f["pairs"],"Robotic streak caller has unreviewed data operands")
            u=unique_template(target,original.read(f["a"],f["size"]),f["masks"],b)
            unique.append({"source":f["source"],"linkage_name":f["linkage"],"address":b,"size":f["size"],"uniqueness":u})
            if b not in records:
                records[b]={"name":f["name"],"source":f["source"],"address":b,"size":f["size"],
                    "sha256":digest(target.read(b,f["size"])),"boundary_confirmation":True,
                    "confirmation_kind":CLUSTER_KIND,"provenance":[],"corroboration":{
                    "proof_scope":"complete_robotic_goal_streak_callers_and_typed_effects_helper",
                    "local_control_flow":f["bounds"],"whole_translation_unit_claimed":False}}
            records[b]["provenance"].append({"version":version,"executable_sha1":original.sha1,
                "source_address":f["a"],"source":f["source"],"name":f["name"],"linkage_name":f["linkage"],
                "reference_sha256":digest(original.read(f["a"],f["size"])),
                "data_address_operands":f["pairs"],"direct_transfers":f["calls"]})
        sequences.append({"version":version,"complete_bodies":unique,"whole_translation_units_claimed":False})
        data_proofs.append({"version":version,"typed_streak_array":data,"data_operand":p,
                            "independent_complete_consumer_addresses":[helper["b"],stop["b"]],
                            "target_base":p["target_address"],"data_extents_promoted":False})
    return {"functions":sorted(records.values(),key=lambda f:f["address"]),"sequence_proofs":sequences,
            "data_proofs":data_proofs,"counts":{"functions":4,"code_bytes":2808,"source_units":2,
            "closed_return_bodies":4,"reviewed_complete_caller_callee_clusters":1}}
