"""Complete original French Sandy damage-effect leaf and typed record array."""
from __future__ import annotations
from platforms.verify_reviewed import REFERENCES,TARGET,require
from platforms.france_tu_sequences import compare,digest,unique_template
from platforms.france_hangable_sequence import hangable_pair
from platforms.france_player_animation_context import OriginalData,closed,storage
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages
from platforms.ps2_type_layouts import aggregate_layouts

SOURCE="SB/Game/zNPCTypeBossSandy.cpp"
LINKAGE="zNPCBSandy_BossDamageEffect__FP14xModelInstanceUi"
ADDRESS=0x335000
SIZE=432
BASE=0x5e1bc0
OPERANDS=[(8,16,0),(68,76,260),(84,92,256),(116,120,0),
          (208,216,0),(268,276,260),(288,296,256),(320,324,0)]


def original_array(original,target):
    data=OriginalData(original)
    layouts=aggregate_layouts(data.debug,SOURCE,{"BossDamageEffectRecord","xModelInstance"})
    record=layouts["BossDamageEffectRecord"];model=layouts["xModelInstance"]
    members={m["name"]:m for m in record["members"]}
    require(record["size"]==264 and model["size"]==108 and set(members)=={"save_F32","BDEtimer","BDEminst"},
            "Original Sandy damage record extent or members differ")
    for n,offset,typ in (("BDEtimer",256,{"5":14}),
                        ("BDEminst",260,{"8":(b"\1"+model["die_offset"].to_bytes(4,"little")).hex()})):
        require(members[n]["offset"]==offset and members[n]["type_attributes"]==typ,"Original Sandy damage record member differs")
    saved=members["save_F32"];tag,_,typ=data.by[saved["type_attributes"]["7"]]
    require(saved["offset"]==0 and tag==1 and typ.get(9)==0 and
            typ.get(10)==bytes.fromhex("000a00000000003f0000000855000e00"),"Sandy damage saved colors are not float[64]")
    fields={m["name"]:m for m in model["members"]}
    for n,offset,typ in (("Next",0,{"8":(b"\1"+model["die_offset"].to_bytes(4,"little")).hex()}),
                        ("RedMultiplier",24,{"5":14}),("GreenMultiplier",28,{"5":14}),("BlueMultiplier",32,{"5":14})):
        require(fields[n]["offset"]==offset and fields[n]["type_attributes"]==typ,"Original Sandy damage model path differs")
    off,decl,address=data.declaration(SOURCE,"BDErecord")
    tag,_,typ=data.by[decl[7]]
    descriptor=bytes.fromhex("000a000000000003000000087200")+record["die_offset"].to_bytes(4,"little")
    require(tag==1 and typ.get(9)==0 and typ.get(10)==descriptor,"Original Sandy damage array shape/type differs")
    for binary,a in ((original,address),(target,BASE)):
        storage(binary,a,4*264,"runtime_bss")
    return {"name":"BDErecord","reference_address":address,"target_address":BASE,"size":1056,
            "declaration_die":off,"array_type_die":decl[7],"array_descriptor":descriptor.hex(),
            "count":4,"stride":264,"record_layout":record,"model_layout":model,
            "saved_color_array_type_die":saved["type_attributes"]["7"],
            "saved_color_descriptor":"000a00000000003f0000000855000e00","data_extent_promoted":False}


def generate_unit(originals,registry_dir):
    # This complete leaf has no calls or registry dependencies.
    target=originals[TARGET];record=None;sequences=[];data_proofs=[]
    for version in REFERENCES:
        original=originals[version];links=canonical_linkages(original.data,original.metadata)
        refs=[f for f in original.functions if f["source"]==SOURCE and links.get(f["low"])==LINKAGE]
        require(len(refs)==1 and refs[0]["high"]-refs[0]["low"]==SIZE,"Complete original Sandy damage identity differs")
        f=refs[0];a=f["low"];require(a%16==0,"Sandy damage original entry alignment differs")
        obj=original_array(original,target)
        pairs,calls,masks=compare(original,target,a,ADDRESS,SIZE,address_resolver=hangable_pair)
        require(not calls and [(p["hi_offset"],p["lo_offset"],p["target_address"]-BASE) for p in pairs]==OPERANDS,
                "Sandy damage complete operand inventory differs")
        expected={};operands=[]
        for p in pairs:
            offset=p["target_address"]-BASE
            require(p["reference_address"]==obj["reference_address"]+offset and p["opcode"]==9 and p["storage"]=="zero_fill",
                    "Sandy damage operand lost its original typed array path")
            expected[p["lo_offset"]]=0xffff0000
            if original.read(a+p["hi_offset"],4)!=target.read(ADDRESS+p["hi_offset"],4):expected[p["hi_offset"]]=0xffff0000
            operands.append({"array_index":0,"field":{0:"record_base_or_save_F32",256:"BDEtimer",260:"BDEminst"}[offset],**p})
        require(masks==expected,"Sandy damage masks exceed typed array operands")
        bounds=closed(original,target,a,ADDRESS,SIZE)
        unique=unique_template(target,original.read(a,SIZE),masks,ADDRESS)
        if record is None:
            record={"name":f["name"],"source":SOURCE,"address":ADDRESS,"size":SIZE,"sha256":digest(target.read(ADDRESS,SIZE)),
                    "boundary_confirmation":True,"confirmation_kind":CLUSTER_KIND,"provenance":[],"corroboration":{
                    "proof_scope":"complete_original_sandy_damage_effect_leaf","local_control_flow":bounds,"whole_translation_unit_claimed":False}}
        record["provenance"].append({"version":version,"executable_sha1":original.sha1,"source_address":a,
                "source":SOURCE,"name":f["name"],"linkage_name":LINKAGE,"reference_sha256":digest(original.read(a,SIZE)),
                "data_address_operands":pairs,"direct_transfers":[]})
        sequences.append({"version":version,"source_address":a,"target_address":ADDRESS,"size":SIZE,
                          "unique_full_body_template":unique,"whole_translation_unit_claimed":False})
        data_proofs.append({"version":version,"complete_typed_record_array":obj,"operands":operands})
    return {"functions":[record],"sequence_proofs":sequences,"data_proofs":data_proofs,
            "counts":{"functions":1,"code_bytes":432,"source_units":1,"closed_return_bodies":1,
                      "reviewed_complete_caller_callee_clusters":1}}
