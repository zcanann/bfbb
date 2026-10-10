"""Original-only French model leaves with typed material/frustum storage."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES,TARGET,require
from platforms.france_tu_sequences import compare,digest,unique_template,words
from platforms.france_hangable_sequence import hangable_pair
from platforms.france_goal_sequence import goal_pair,original_data
from platforms.france_player_animation_context import closed,storage
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages
from platforms.dwarf1 import iter_dies
from platforms.ps2_type_layouts import aggregate_layouts

SOURCE="SB/Core/p2/iModel.cpp"
MEMBERS=[("iModelMaterialMulCB__FP10RpMaterialPv",0x1ad240,712),
         ("iModelSetMaterialAlpha__FP8RpAtomicUc",0x1ad7f0,224),
         ("iModelCull__FP8RpAtomicP11RwMatrixTag",0x1aee30,288),
         ("NextAtomicCallback__FP8RpAtomicPv",0x1af3d0,48)]
PAIRS={0x1ad240:[(4,12,0x532c40,9)],0x1ad7f0:[(60,68,0x532cc0,9)],
       0x1aee30:[(8,12,0x52c760,9)],0x1af3d0:[]}
ANCHOR=(0x2c70a0,"SB/Game/zNPCGoalStd.cpp","CalcNewDir",624)


def typed_storage(original,target,known):
    section=next(s for s in original.metadata["sections"] if s["name"]==".debug")
    debug=original.data[section["offset"]:section["offset"]+section["size"]]
    rows=list(iter_dies(debug));by={off:(tag,attrs) for off,tag,owner,attrs in rows}
    layouts=aggregate_layouts(debug,SOURCE,{"RwRGBA","zGlobals","xGlobals","xCamera","xVec4"})
    require({k:v["size"] for k,v in layouts.items()}==
            {"RwRGBA":4,"zGlobals":8272,"xGlobals":1792,"xCamera":816,"xVec4":16},
            "Original model aggregate sizes differ")
    def fields(name):
        off=layouts[name]["die_offset"];end=by[off][1][1]
        return [(i,a) for i,tag,owner,a in rows if off<i<end and tag==13]
    for name,expected in (("RwRGBA",[("red",0,3),("green",1,3),("blue",2,3),("alpha",3,3)]),
                          ("xVec4",[("x",0,14),("y",4,14),("z",8,14),("w",12,14)])):
        require([(a.get(3),a.get(2),a.get(5)) for _,a in fields(name)]==
                [(n,b"\x04"+off.to_bytes(4,"little")+b"\x07",t) for n,off,t in expected],
                "Original model scalar component layout differs")
    def array(die,count,suffix):
        tag,a=by[die];descriptor=bytes.fromhex("000a0000000000")+(count-1).to_bytes(4,"little")+suffix
        require(tag==1 and a.get(9)==0 and a.get(10)==descriptor,"Original model array extent/element type differs")
    def declaration(name,type_die):
        candidates=[(off,a) for off,tag,owner,a in rows if tag in (7,12) and
                    owner.replace("\\","/").endswith(SOURCE) and a.get(3)==name]
        require(len(candidates)==1,"Original model data declaration ambiguous")
        off,a=candidates[0];loc=a.get(2)
        require(isinstance(loc,bytes) and len(loc)==5 and loc[0]==3 and a.get(7)==type_die,
                "Original model data type/storage differs")
        return {"declaration_die":off,"type_die":type_die,"reference_address":int.from_bytes(loc[1:],"little")}
    objects={}
    for name,address,count,width,suffix in (
            ("sMaterialColor",0x532c40,16,4,bytes.fromhex("087200")+layouts["RwRGBA"]["die_offset"].to_bytes(4,"little")),
            ("sMaterialAlpha",0x532cc0,16,1,bytes.fromhex("0855000300"))):
        candidates=[a[7] for off,tag,owner,a in rows if tag==12 and owner.replace("\\","/").endswith(SOURCE) and a.get(3)==name]
        require(len(candidates)==1,"Original material array declaration missing")
        die=candidates[0];array(die,count,suffix);record=declaration(name,die)
        size=count*width;storage(original,record["reference_address"],size,"runtime_bss");storage(target,address,size,"runtime_bss")
        objects[name]={**record,"target_address":address,"count":count,"element_size":width,"size":size}
    glob=declaration("globals",layouts["zGlobals"]["die_offset"])
    off=layouts["zGlobals"]["die_offset"];end=by[off][1][1]
    bases=[a for i,tag,owner,a in rows if off<i<end and tag==28]
    require(len(bases)==1 and bases[0].get(7)==layouts["xGlobals"]["die_offset"] and
            bases[0].get(2)==bytes.fromhex("040000000007"),"Model globals base inheritance differs")
    camera=[(i,a) for i,a in fields("xGlobals") if a.get(3)=="camera"]
    require(len(camera)==1 and camera[0][1].get(7)==layouts["xCamera"]["die_offset"] and
            camera[0][1].get(2)==bytes.fromhex("040000000007"),"Model camera path differs")
    plane=[(i,a) for i,a in fields("xCamera") if a.get(3)=="frustplane"]
    require(len(plane)==1 and plane[0][1].get(2)==bytes.fromhex("047002000007"),"Model frustum member offset differs")
    array(plane[0][1][7],12,bytes.fromhex("087200")+layouts["xVec4"]["die_offset"].to_bytes(4,"little"))
    globals_data,_,paths,_=original_data(original)
    require(glob["reference_address"]==globals_data["globals"]["reference_address"] and
            [p["offset"] for p in paths[0]]==[1792,0,0,36],"Cross-unit typed globals identity differs")
    anchor=known[ANCHOR[0]];ref=next(p for p in anchor["provenance"] if p["version"]==original.version)
    checked_identity(original,target,ref["source_address"],ANCHOR[0],known)
    pairs,_,_=compare(original,target,ref["source_address"],ANCHOR[0],624,address_resolver=goal_pair)
    require(any(p["reference_address"]==glob["reference_address"]+1828 and p["target_address"]==0x52cc14 and
                p["opcode"]==35 for p in pairs),"Independent complete goal lost its globals anchor")
    storage(original,glob["reference_address"],8272,"runtime_bss");storage(target,0x52c4f0,8272,"runtime_bss")
    objects["globals.camera.frustplane"]={**glob,"reference_address":glob["reference_address"]+624,
            "target_address":0x52c760,"count":12,"element_size":16,"size":192,
            "member_die":plane[0][0],"array_type_die":plane[0][1][7],"component_offset":624,
            "independent_anchor":{k:anchor[k] for k in ("source","name","address","size","sha256")}}
    return {"objects":objects,"layouts":layouts,"data_extents_promoted":False}


def generate_unit(originals,registry_dir):
    target=originals[TARGET];known={}
    for path in sorted(registry_dir.glob("*functions.json")):
        for f in json.loads(path.read_text())["functions"]:
            if f["address"]==ANCHOR[0]:
                old=known.setdefault(f["address"],f)
                require(all(old[k]==f[k] for k in ("source","name","size","sha256")),"Conflicting model data anchor")
    require(set(known)=={ANCHOR[0]} and tuple(known[ANCHOR[0]][k] for k in ("source","name","size"))==ANCHOR[1:],
            "Independent complete model data anchor missing/different")
    records={};sequences=[];proofs=[]
    for version in REFERENCES:
        original=originals[version];links=canonical_linkages(original.data,original.metadata)
        data=typed_storage(original,target,known);identities=[];operands=[]
        for linkage,b,size in MEMBERS:
            matches=[f for f in original.functions if f["source"]==SOURCE and links.get(f["low"])==linkage]
            require(len(matches)==1 and matches[0]["high"]-matches[0]["low"]==size,"Original model complete leaf identity differs")
            f=matches[0];a=f["low"];require(a%16==b%16==0,"Model leaf alignment differs")
            padding=(-size)%16
            require(not any(original.read(a+size,padding)) and not any(target.read(b+size,padding)),"Model leaf padding differs")
            pairs,calls,masks=compare(original,target,a,b,size,address_resolver=hangable_pair)
            require(not calls and [(p["hi_offset"],p["lo_offset"],p["target_address"],p["opcode"]) for p in pairs]==PAIRS[b],
                    "Model leaf call/data inventory differs")
            expected={}
            for p in pairs:
                candidates=[(name,obj) for name,obj in data["objects"].items() if obj["target_address"]==p["target_address"]]
                require(len(candidates)==1 and candidates[0][1]["reference_address"]==p["reference_address"] and
                        p["opcode"]==9 and p["storage"]=="zero_fill","Model leaf typed operand differs")
                expected[p["lo_offset"]]=0xffff0000
                if original.read(a+p["hi_offset"],4)!=target.read(b+p["hi_offset"],4):expected[p["hi_offset"]]=0xffff0000
                operands.append({"caller":linkage,"caller_address":b,"typed_object":candidates[0][0],**p})
            require(masks==expected,"Model leaf masks exceed typed address halves")
            boundary=closed(original,target,a,b,size);unique=unique_template(target,original.read(a,size),masks,b)
            vector=[]
            if b==0x1aee30:
                vector=[{"offset":i*4,"word":w} for i,w in enumerate(words(original.read(a,size))) if w>>26 in (18,54,62)]
                require(len(vector)==42 and all(target.read(b+r["offset"],4)==r["word"].to_bytes(4,"little") for r in vector),
                        "Complete raw model vector instruction words differ")
            identities.append({"source_address":a,"target_address":b,"size":size,"uniqueness":unique,
                               "terminal_alignment_bytes":padding,"raw_vector_instruction_words":vector})
            if b not in records:
                records[b]={"name":f["name"],"source":SOURCE,"address":b,"size":size,"sha256":digest(target.read(b,size)),
                    "boundary_confirmation":True,"confirmation_kind":CLUSTER_KIND,"provenance":[],"corroboration":{
                    "proof_scope":"complete_original_model_leaves_with_typed_material_storage_and_anchored_frustum",
                    "local_control_flow":boundary,"whole_translation_unit_claimed":False}}
            records[b]["provenance"].append({"version":version,"executable_sha1":original.sha1,"source_address":a,
                "source":SOURCE,"name":f["name"],"linkage_name":linkage,"reference_sha256":digest(original.read(a,size)),
                "data_address_operands":pairs,"direct_transfers":[]})
        sequences.append({"version":version,"complete_bodies":identities,"whole_translation_unit_claimed":False})
        proofs.append({"version":version,"typed_storage":data,"operands":operands})
    return {"functions":sorted(records.values(),key=lambda f:f["address"]),"sequence_proofs":sequences,"data_proofs":proofs,
            "counts":{"functions":4,"code_bytes":1272,"source_units":1,"closed_return_bodies":4,
                      "reviewed_complete_caller_callee_clusters":4}}
