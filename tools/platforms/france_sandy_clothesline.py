"""Complete original French Sandy clothesline goal with typed static data."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES,TARGET,require
from platforms.france_tu_sequences import compare,digest,unique_template,cstring
from platforms.france_hangable_sequence import hangable_pair
from platforms.france_player_animation_context import OriginalData,closed,storage
from platforms.ps2_type_layouts import aggregate_layouts
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages

SOURCE="SB/Game/zNPCTypeBossSandy.cpp"
MEMBERS=[("Process__28zNPCGoalBossSandyClotheslineFP11en_trantypefPvP6xScene",0x32c7c0,2832)]
INDEPENDENT = {2953696: ('SB/Game/zNPCTypeCommon.cpp', 'AnimTimeRemain', 104),
 2024768: ('SB/Core/x/xMath.cpp', 'xrand', 32),
 2159200: ('SB/Core/x/xString.cpp', 'xStrHash', 88),
 2138896: ('SB/Core/x/xSnd.cpp', 'xSndPlay3D', 24),
 2929168: ('SB/Game/zNPCGoalCommon.cpp', 'DoAutoAnim', 180),
 2166032: ('SB/Core/x/xVec3.cpp', 'xVec3Normalize', 224),
 2024672: ('SB/Core/x/xMath.cpp', 'xurand', 88),
 2137280: ('SB/Core/x/xSnd.cpp', 'xSndStop', 8),
 2636352: ('SB/Core/x/xEvent.cpp', 'zEntEvent', 32),
 3088976: ('SB/Core/x/xBehaviour.cpp', 'Process', 56)}
CALLS = {3327936: [(96, 2953696),
           (156, 2024768),
           (228, 2159200),
           (288, 2138896),
           (356, 2929168),
           (472, 2166032),
           (636, 2024672),
           (648, 2024672),
           (660, 2024672),
           (672, 2024672),
           (684, 2024768),
           (940, 2024672),
           (952, 2024672),
           (964, 2024672),
           (976, 2024672),
           (1292, 2166032),
           (1452, 2166032),
           (1808, 2159200),
           (1872, 2138896),
           (1892, 2137280),
           (1940, 2929168),
           (1996, 2636352),
           (2228, 2636352),
           (2316, 2929168),
           (2776, 3088976)]}
OPERANDS = {3327936: [(180, 188, 35, 6167436),
           (224, 232, 9, 5246820),
           (492, 616, 43, 6167312),
           (624, 640, 43, 6167344),
           (644, 652, 57, 6167296),
           (656, 664, 57, 6167304),
           (668, 676, 57, 6167328),
           (680, 688, 57, 6167336),
           (712, 724, 43, 6167300),
           (736, 740, 43, 6167300),
           (756, 768, 43, 6167308),
           (784, 788, 43, 6167308),
           (804, 816, 43, 6167332),
           (832, 836, 43, 6167332),
           (852, 864, 43, 6167340),
           (880, 884, 43, 6167340),
           (908, 920, 43, 6167248),
           (928, 944, 43, 6167280),
           (948, 956, 57, 6167232),
           (960, 968, 57, 6167240),
           (972, 980, 57, 6167264),
           (984, 996, 57, 6167272),
           (1004, 1016, 43, 6167236),
           (1032, 1036, 43, 6167236),
           (1052, 1064, 43, 6167244),
           (1080, 1084, 43, 6167244),
           (1100, 1112, 43, 6167268),
           (1128, 1132, 43, 6167268),
           (1148, 1160, 43, 6167276),
           (1176, 1184, 43, 6167276),
           (1800, 1804, 9, 5246838),
           (2084, 2092, 43, 6167312),
           (2096, 2100, 43, 6167344),
           (2104, 2108, 43, 6167248),
           (2112, 2116, 43, 6167280)]}
SPRINGS={"sLeftArmSpring":0x5e1ac0,"sRightArmSpring":0x5e1ae0,
         "sLeftLegSpring":0x5e1b00,"sRightLegSpring":0x5e1b20}
STRINGS={0x500f64:b"B101_SC_feet_loop\0",0x500f76:b"B101_ring4\0"}


def original_objects(original,target):
    data=OriginalData(original)
    layouts=aggregate_layouts(data.debug,SOURCE,{"SandyLimbSpring","xBound"})
    layout=layouts["SandyLimbSpring"]
    expected=[("node1",0,{"5":14}),("vel1",4,{"5":14}),("node2",8,{"5":14}),
              ("vel2",12,{"5":14}),("bound",16,{"8":(b"\1"+layouts["xBound"]["die_offset"].to_bytes(4,"little")).hex()})]
    require(layout["size"]==20 and layouts["xBound"]["size"]==76 and
            [(m["name"],m["offset"],m["type_attributes"]) for m in layout["members"]]==expected,
            "Original Sandy spring float/pointer layout differs")
    objects=[]
    for name,b in SPRINGS.items():
        off,decl,a=data.declaration(SOURCE,name)
        require(decl.get(7)==layout["die_offset"],"Sandy spring original declaration type differs")
        objects.append({"name":name,"reference_address":a,"target_address":b,"size":20,
                        "declaration_die":off,"layout":layout,"data_extent_promoted":False})
    off,decl,a=data.declaration(SOURCE,"sNFSoundValue")
    tag,_,typ=data.by[decl[7]]
    require(tag==1 and typ.get(9)==0 and typ.get(10)==bytes.fromhex("000a00000000001d0000000855000900"),
            "Original Sandy sound IDs are not uint32[30]")
    objects.append({"name":"sNFSoundValue","reference_address":a,"target_address":0x5e1b40,"size":120,
                    "declaration_die":off,"array_type_die":decl[7],"count":30,"stride":4,
                    "descriptor":typ[10].hex(),"data_extent_promoted":False})
    for obj in objects:
        for binary,key in ((original,"reference_address"),(target,"target_address")):
            storage(binary,obj[key],obj["size"],"runtime_bss")
    for key in ("reference_address","target_address"):
        ordered=sorted(objects,key=lambda obj:obj[key])
        require(all(x[key]+x["size"]<=y[key] for x,y in zip(ordered,ordered[1:])),"Sandy typed objects overlap")
    return objects


def generate_unit(originals,registry_dir):
    target=originals[TARGET];known={}
    # Fixed dependency fence prevents later identities from changing this proof.
    for path in sorted(registry_dir.glob("*functions.json")):
        for f in json.loads(path.read_text())["functions"]:
            if f["address"] in INDEPENDENT:
                old=known.setdefault(f["address"],f)
                require(all(old[k]==f[k] for k in ("source","name","size","sha256")),"Conflicting Sandy clothesline dependency")
    require(set(known)==set(INDEPENDENT),"Independent Sandy clothesline dependency missing")
    for address,identity in INDEPENDENT.items():
        require(tuple(known[address][k] for k in ("source","name","size"))==identity,"Sandy clothesline dependency identity differs")
    records={};sequences=[];data_proofs=[];call_proofs=[]
    for version in REFERENCES:
        original=originals[version];links=canonical_linkages(original.data,original.metadata)
        objects=original_objects(original,target);identities=[];operands=[];transfers=[]
        for linkage,b,size in MEMBERS:
            refs=[f for f in original.functions if f["source"]==SOURCE and links.get(f["low"])==linkage]
            require(len(refs)==1 and refs[0]["high"]-refs[0]["low"]==size,"Complete Sandy clothesline original identity differs")
            f=refs[0];a=f["low"];require(a%16==b%16==0,"Sandy clothesline entry alignment differs")
            padding=(-size)%16
            require(not any(original.read(a+size,padding)) and not any(target.read(b+size,padding)),"Sandy clothesline padding differs")
            pairs,calls,masks=compare(original,target,a,b,size,address_resolver=hangable_pair)
            require([(p["hi_offset"],p["lo_offset"],p["opcode"],p["target_address"]) for p in pairs]==OPERANDS[b],
                    "Sandy clothesline data inventory differs")
            expected={}
            for p in pairs:
                address=p["target_address"]
                if address in STRINGS:
                    literal=STRINGS[address]
                    require(p["opcode"]==9 and p["storage"]=="file_backed" and
                            cstring(original,p["reference_address"])==cstring(target,address)==literal,
                            "Sandy clothesline complete string differs")
                    path={"string":literal.decode("ascii")}
                else:
                    candidates=[o for o in objects if o["target_address"]<=address<o["target_address"]+o["size"]]
                    require(len(candidates)==1,"Sandy clothesline typed object missing or ambiguous")
                    obj=candidates[0];offset=address-obj["target_address"]
                    require(p["reference_address"]==obj["reference_address"]+offset and p["storage"]=="zero_fill",
                            "Sandy clothesline original typed path differs")
                    if obj["name"]=="sNFSoundValue":
                        require(offset==19*4 and p["opcode"]==35,"Sandy clothesline sound index/type differs")
                        path={"object":obj["name"],"index":19}
                    else:
                        member=next((m for m in obj["layout"]["members"] if m["offset"]==offset),None)
                        require(member and p["opcode"] in ({57} if offset in (0,8) else {43}),
                                "Sandy clothesline spring operand type differs")
                        path={"object":obj["name"],"member":member["name"],"offset":offset}
                expected[p["lo_offset"]]=0xffff0000
                if original.read(a+p["hi_offset"],4)!=target.read(b+p["hi_offset"],4):expected[p["hi_offset"]]=0xffff0000
                operands.append({"caller":linkage,"caller_address":b,"path":path,**p})
            require([(c["offset"],c["target_address"]) for c in calls]==CALLS[b] and all(c["opcode"]==3 for c in calls),
                    "Sandy clothesline complete callee inventory differs")
            for c in calls:
                evidence=checked_identity(original,target,c["reference_address"],c["target_address"],known)
                expected[c["offset"]]=0xfc000000
                transfers.append({"caller":linkage,"caller_address":b,**c,
                    "callee":{k:evidence[k] for k in ("source","name","address","size","sha256")}})
            require(masks==expected,"Sandy clothesline masks exceed typed operands and complete calls")
            boundary=closed(original,target,a,b,size);unique=unique_template(target,original.read(a,size),masks,b)
            identities.append({"source":SOURCE,"linkage_name":linkage,"source_address":a,"target_address":b,"size":size,
                               "uniqueness":unique,"terminal_alignment_bytes":padding})
            if b not in records:
                records[b]={"name":f["name"],"source":SOURCE,"address":b,"size":size,"sha256":digest(target.read(b,size)),
                    "boundary_confirmation":True,"confirmation_kind":CLUSTER_KIND,"provenance":[],"corroboration":{
                    "proof_scope":"complete_original_sandy_clothesline_goal",
                    "local_control_flow":boundary,"whole_translation_unit_claimed":False}}
            records[b]["provenance"].append({"version":version,"executable_sha1":original.sha1,"source_address":a,
                "source":SOURCE,"name":f["name"],"linkage_name":linkage,"reference_sha256":digest(original.read(a,size)),
                "data_address_operands":pairs,"direct_transfers":calls})
        sequences.append({"version":version,"complete_bodies":identities,"whole_translation_unit_claimed":False})
        data_proofs.append({"version":version,"typed_static_objects":objects,"operands":operands})
        call_proofs.append({"version":version,"complete_callees":transfers})
    return {"functions":sorted(records.values(),key=lambda f:f["address"]),"sequence_proofs":sequences,
            "data_proofs":data_proofs,"call_proofs":call_proofs,
            "counts":{"functions":1,"code_bytes":2832,"source_units":1,"closed_return_bodies":1,
                      "reviewed_complete_caller_callee_clusters":1}}

