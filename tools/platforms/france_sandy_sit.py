"""Complete original French Sandy sit goal with typed static data."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES,TARGET,require
from platforms.france_tu_sequences import compare,digest,unique_template,cstring
from platforms.france_hangable_sequence import hangable_pair
from platforms.france_player_animation_context import OriginalData,closed,storage
from platforms.ps2_type_layouts import aggregate_layouts
from platforms.france_boss_goal_kernels import model_field
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages

SOURCE="SB/Game/zNPCTypeBossSandy.cpp"
PLAYER="SB/Game/zEntPlayer.cpp"
MEMBERS=[("Process__20zNPCGoalBossSandySitFP11en_trantypefPvP6xScene",0x32db20,2336)]
INDEPENDENT = {2159200: ('SB/Core/x/xString.cpp', 'xStrHash', 88),
 2138896: ('SB/Core/x/xSnd.cpp', 'xSndPlay3D', 24),
 2636464: ('SB/Core/x/xEvent.cpp', 'zEntEvent', 32),
 2027024: ('SB/Core/x/xMath3.cpp', 'xMat4x3Mul', 192),
 2024672: ('SB/Core/x/xMath.cpp', 'xurand', 88),
 2166032: ('SB/Core/x/xVec3.cpp', 'xVec3Normalize', 224),
 2636288: ('SB/Core/x/xEvent.cpp', 'zEntEvent', 32),
 3362816: ('SB/Game/zNPCTypeBossSandy.cpp', 'zNPCBSandy_BossDamageEffect', 432),
 2929168: ('SB/Game/zNPCGoalCommon.cpp', 'DoAutoAnim', 180),
 3088976: ('SB/Core/x/xBehaviour.cpp', 'Process', 56),
 2912416: ('SB/Game/zNPCGoalStd.cpp', 'CalcNewDir', 624)}
CALLS = {3332896: [(320, 2159200),
           (380, 2138896),
           (396, 2636464),
           (472, 2027024),
           (916, 2024672),
           (1184, 2166032),
           (1308, 2024672),
           (1564, 2159200),
           (1624, 2138896),
           (1640, 2636288),
           (2220, 3362816),
           (2236, 3362816),
           (2252, 2929168),
           (2280, 3088976)]}
OPERANDS = {3332896: [(4, 88, 35, 5430544),
           (124, 132, 35, 5426196),
           (168, 192, 35, 5426196),
           (200, 216, 35, 5426196),
           (312, 324, 9, 5246871),
           (412, 452, 35, 5112852),
           (1492, 1500, 9, 6167384),
           (1540, 1544, 9, 5246888)]}
STRINGS={0x500f97:b"B101_SC_headoff1\0",0x500fa8:b"B101_SC_damage3\0"}


def original_objects(original,target,known):
    data=OriginalData(original)
    model=model_field(original,target,known)
    require(data.globals[2]==model["globals"]["reference_address"],"Sandy sit global owners disagree")
    carry=next(m for m in data.layouts["zPlayerGlobals"]["members"] if m["name"]=="carry")
    layouts=aggregate_layouts(data.debug,PLAYER,{"zPlayerCarryInfo","xEnt"})
    layout=layouts["zPlayerCarryInfo"];ent=layouts["xEnt"]
    require(carry["offset"]==4384 and carry["type_attributes"]=={"7":layout["die_offset"]} and
            layout["size"]==224 and ent["size"]==208,"Sandy sit carry aggregate path differs")
    grabbed=next(m for m in layout["members"] if m["name"]=="grabbed")
    require(grabbed["offset"]==0 and grabbed["type_attributes"]==
            {"8":(b"\1"+ent["die_offset"].to_bytes(4,"little")).hex()},"Sandy sit grabbed entity pointer differs")
    fields=[{"reference_address":model["reference_address"],"target_address":0x52cc14,
             "opcode":35,"storage":"zero_fill","path":["globals","player","ent","model"]},
            {"reference_address":data.globals[2]+1792+4384,"target_address":0x52dd10,
             "opcode":35,"storage":"zero_fill","path":["globals","player","carry","grabbed"]}]
    arrays=[]
    for name,b,count,descriptor,offset,opcode,region in (
            ("sBone",0x4e0410,13,"000a00000000000c0000000855000800",4,35,"initialized_data"),
            ("sNFSoundValue",0x5e1b40,30,"000a00000000001d0000000855000900",24,9,"runtime_bss")):
        off,decl,a=data.declaration(SOURCE,name);tag,_,typ=data.by[decl[7]]
        require(tag==1 and typ.get(9)==0 and typ.get(10)==bytes.fromhex(descriptor),"Sandy sit array bounds or type differ")
        for binary,address in ((original,a),(target,b)):storage(binary,address,count*4,region)
        obj={"name":name,"reference_address":a,"target_address":b,"count":count,"stride":4,"size":count*4,
             "declaration_die":off,"array_type_die":decl[7],"descriptor":descriptor,"data_extent_promoted":False}
        if name=="sBone":
            require(original.read(a,count*4)==target.read(b,count*4),"Sandy sit complete bone index payload differs")
            obj["complete_payload"]=original.read(a,count*4).hex()
        arrays.append(obj)
        fields.append({"reference_address":a+offset,"target_address":b+offset,"opcode":opcode,
                       "storage":"file_backed" if region=="initialized_data" else "zero_fill","path":[name,offset//4]})
    return {"model_field":model,"carry_member":carry,"carry_layout":layout,"grabbed_member":grabbed,
            "entity_layout":ent,"typed_arrays":arrays,"fields":fields}


def generate_unit(originals,registry_dir):
    target=originals[TARGET];known={}
    # Fixed dependency fence prevents later identities from changing this proof.
    for path in sorted(registry_dir.glob("*functions.json")):
        for f in json.loads(path.read_text())["functions"]:
            if f["address"] in INDEPENDENT:
                old=known.setdefault(f["address"],f)
                require(all(old[k]==f[k] for k in ("source","name","size","sha256")),"Conflicting Sandy sit dependency")
    require(set(known)==set(INDEPENDENT),"Independent Sandy sit dependency missing")
    for address,identity in INDEPENDENT.items():
        require(tuple(known[address][k] for k in ("source","name","size"))==identity,"Sandy sit dependency identity differs")
    records={};sequences=[];data_proofs=[];call_proofs=[]
    for version in REFERENCES:
        original=originals[version];links=canonical_linkages(original.data,original.metadata)
        objects=original_objects(original,target,known);identities=[];operands=[];transfers=[]
        for linkage,b,size in MEMBERS:
            refs=[f for f in original.functions if f["source"]==SOURCE and links.get(f["low"])==linkage]
            require(len(refs)==1 and refs[0]["high"]-refs[0]["low"]==size,"Complete Sandy sit original identity differs")
            f=refs[0];a=f["low"];require(a%16==b%16==0,"Sandy sit entry alignment differs")
            padding=(-size)%16
            require(not any(original.read(a+size,padding)) and not any(target.read(b+size,padding)),"Sandy sit padding differs")
            pairs,calls,masks=compare(original,target,a,b,size,address_resolver=hangable_pair)
            require([(p["hi_offset"],p["lo_offset"],p["opcode"],p["target_address"]) for p in pairs]==OPERANDS[b],
                    "Sandy sit data inventory differs")
            expected={}
            for p in pairs:
                address=p["target_address"]
                if address in STRINGS:
                    literal=STRINGS[address]
                    require(p["opcode"]==9 and p["storage"]=="file_backed" and
                            cstring(original,p["reference_address"])==cstring(target,address)==literal,
                            "Sandy sit complete string differs")
                    path={"string":literal.decode("ascii")}
                else:
                    candidates=[field for field in objects["fields"] if field["target_address"]==address]
                    require(len(candidates)==1,"Sandy sit original typed field missing or ambiguous")
                    field=candidates[0]
                    require(all(p[k]==field[k] for k in ("reference_address","target_address","opcode","storage")),
                            "Sandy sit operand lost its original typed field")
                    path=field["path"]
                expected[p["lo_offset"]]=0xffff0000
                if original.read(a+p["hi_offset"],4)!=target.read(b+p["hi_offset"],4):expected[p["hi_offset"]]=0xffff0000
                operands.append({"caller":linkage,"caller_address":b,"path":path,**p})
            require([(c["offset"],c["target_address"]) for c in calls]==CALLS[b] and all(c["opcode"]==3 for c in calls),
                    "Sandy sit complete callee inventory differs")
            for c in calls:
                evidence=checked_identity(original,target,c["reference_address"],c["target_address"],known)
                expected[c["offset"]]=0xfc000000
                transfers.append({"caller":linkage,"caller_address":b,**c,
                    "callee":{k:evidence[k] for k in ("source","name","address","size","sha256")}})
            require(masks==expected,"Sandy sit masks exceed typed operands and complete calls")
            boundary=closed(original,target,a,b,size);unique=unique_template(target,original.read(a,size),masks,b)
            identities.append({"source":SOURCE,"linkage_name":linkage,"source_address":a,"target_address":b,"size":size,
                               "uniqueness":unique,"terminal_alignment_bytes":padding})
            if b not in records:
                records[b]={"name":f["name"],"source":SOURCE,"address":b,"size":size,"sha256":digest(target.read(b,size)),
                    "boundary_confirmation":True,"confirmation_kind":CLUSTER_KIND,"provenance":[],"corroboration":{
                    "proof_scope":"complete_original_sandy_sit_goal",
                    "local_control_flow":boundary,"whole_translation_unit_claimed":False}}
            records[b]["provenance"].append({"version":version,"executable_sha1":original.sha1,"source_address":a,
                "source":SOURCE,"name":f["name"],"linkage_name":linkage,"reference_sha256":digest(original.read(a,size)),
                "data_address_operands":pairs,"direct_transfers":calls})
        sequences.append({"version":version,"complete_bodies":identities,"whole_translation_unit_claimed":False})
        data_proofs.append({"version":version,"typed_static_objects":objects,"operands":operands})
        call_proofs.append({"version":version,"complete_callees":transfers})
    return {"functions":sorted(records.values(),key=lambda f:f["address"]),"sequence_proofs":sequences,
            "data_proofs":data_proofs,"call_proofs":call_proofs,
            "counts":{"functions":1,"code_bytes":2336,"source_units":1,"closed_return_bodies":1,
                      "reviewed_complete_caller_callee_clusters":1}}

