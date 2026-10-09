"""Original-only proof of the contiguous French Common animation-builder cluster."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES,TARGET,require
from platforms.france_tu_sequences import compare,digest,unique_template
from platforms.france_hangable_sequence import hangable_pair
from platforms.france_player_animation_context import closed,storage
from platforms.france_player_animation_tables import animation_literal
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages
from platforms.dwarf1 import iter_dies

SOURCE="SB/Game/zNPCTypeCommon.cpp"
START=0x2cfd60
SPAN=400


from platforms.france_npc_animation_tables import array_type

class OriginalArrays:
    def __init__(self,original,target):
        self.original,self.target=original,target
        section=next(s for s in original.metadata["sections"] if s["name"]==".debug")
        self.rows=list(iter_dies(original.data[section["offset"]:section["offset"]+section["size"]]))
        self.by={off:(tag,attrs) for off,tag,owner,attrs in self.rows}
        self.tables={}
        for name,address,count in (("g_strz_lassanim",0x4de8e0,3),):
            declarations=[(off,attrs) for off,tag,owner,attrs in self.rows if tag in (7,12) and
                          owner.replace("\\","/").endswith(SOURCE) and attrs.get(3)==name]
            require(len(declarations)==1,"Original NPC string-table declaration ambiguous")
            off,attrs=declarations[0];loc=attrs.get(2)
            require(isinstance(loc,bytes) and len(loc)==5 and loc[0]==3,"NPC string table lacks original absolute storage")
            array_type(self.by,attrs[7],count,True)
            reference=int.from_bytes(loc[1:],"little")
            storage(original,reference,4*count,"initialized_data")
            storage(target,address,4*count,"initialized_data")
            strings=[];mapping={}
            for i in range(count):
                ra=int.from_bytes(original.read(reference+4*i,4),"little")
                tb=int.from_bytes(target.read(address+4*i,4),"little")
                body=animation_literal(original,ra)
                require(body==animation_literal(target,tb),"Complete NPC animation table string differs")
                require(mapping.setdefault(ra,tb)==tb,"NPC table string identity splits")
                strings.append({"index":i,"reference_address":ra,"target_address":tb,
                                "size":len(body),"sha256":digest(body),"text":body[:-1].decode("ascii")})
            require(len(mapping)==len(set(mapping.values())),"NPC table string identities collapse")
            self.tables[name]={"reference_address":reference,"target_address":address,"count":count,
                               "size":4*count,"declaration_die":off,"type_die":attrs[7],"strings":strings}


MEMBERS=[('ZNPC_AnimTable_LassoGuide__Fv',0x2cfd60,264),('ZNPC_AnimTable_Common__Fv',0x2cfe70,128)]
INDEPENDENT={0x2131f0:('SB/Core/x/xAnim.cpp','xAnimTableNewTransition',824),
             0x213da0:('SB/Core/x/xAnim.cpp','xAnimTableNewState',612),
             0x214010:('SB/Core/x/xAnim.cpp','xAnimDefaultBeforeEnter',92),
             0x214070:('SB/Core/x/xAnim.cpp','xAnimTableNew',128)}
CALLS={0x2cfd60:[(24,0x214070),(100,0x213da0),(172,0x213da0),(236,0x2131f0)],
       0x2cfe70:[(24,0x214070),(100,0x213da0)]}
PAIRS={0x2cfd60:[(4,12,0x4fe45b,9,'string',None),(40,48,0x214010,9,'callback',None),
                 (36,80,0x4de8e4,35,'table:g_strz_lassanim',None),(108,116,0x214010,9,'callback',None),
                 (120,148,0x4de8e8,35,'table:g_strz_lassanim',None),(192,196,0x4de8e4,35,'table:g_strz_lassanim',None),
                 (228,232,0x4de8e8,35,'table:g_strz_lassanim',None)],
       0x2cfe70:[(4,12,0x4fe467,9,'string',None),(40,48,0x214010,9,'callback',None),(36,52,0x4fe472,9,'string',None)]}

def generate_unit(originals,registry_dir):
    target=originals[TARGET];known={}
    for path in sorted(registry_dir.glob("*functions.json")):
        for f in json.loads(path.read_text())["functions"]:
            if f["address"] in INDEPENDENT:
                prior=known.setdefault(f["address"],f)
                require(all(prior[k]==f[k] for k in ("source","name","size","sha256")),"Conflicting NPC animation dependency")
    require(set(known)==set(INDEPENDENT),"Independent NPC animation dependencies missing")
    for address,identity in INDEPENDENT.items():
        require(tuple(known[address][k] for k in ("source","name","size"))==identity,"NPC animation dependency identity differs")
    records={};sequences=[];data_proofs=[]
    for version in REFERENCES:
        original=originals[version];links=canonical_linkages(original.data,original.metadata)
        arrays=OriginalArrays(original,target);functions={}
        for linkage,b,size in MEMBERS:
            matches=[f for f in original.functions if f["source"]==SOURCE and links.get(f["low"])==linkage]
            require(len(matches)==1 and matches[0]["high"]-matches[0]["low"]==size,"Complete original NPC builder identity differs")
            functions[b]=matches[0]
        begin=functions[START]["low"]
        owned=sorted([f for f in original.functions if f["source"]==SOURCE and begin<=f["low"]<begin+SPAN],key=lambda f:f["low"])
        require([(links[f["low"]],f["low"]-begin,f["high"]-f["low"]) for f in owned]==
                [(name,b-START,size) for name,b,size in MEMBERS],"Original contiguous NPC builder membership/order differs")
        require(sum(f["high"]-f["low"] for f in owned)==392 and owned[-1]["high"]==begin+SPAN,
                "Original NPC builder cluster extent differs")
        masks_all={};data_rows=[];transfers=[];gaps=[];bounds={};mapping={};used=set()
        for index,(linkage,b,size) in enumerate(MEMBERS):
            f=functions[b];a=f["low"]
            require(a%16==b%16==0 and a-b==begin-START,"NPC builder entry alignment or cluster offset differs")
            if index:
                prev=MEMBERS[index-1];end=prev[1]+prev[2];gap=b-end
                require(0<=gap<16 and not any(target.read(end,gap)) and
                        not any(original.read(begin+end-START,gap)),"Nonzero or unexpected NPC inter-body alignment")
                gaps.append({"address":end,"size":gap})
            pairs,calls,masks=compare(original,target,a,b,size,address_resolver=hangable_pair)
            require([(p["hi_offset"],p["lo_offset"],p["target_address"],p["opcode"]) for p in pairs]==
                    [p[:4] for p in PAIRS[b]],"Complete NPC builder data operand inventory differs")
            require([(c["offset"],c["target_address"]) for c in calls]==CALLS[b] and all(c["opcode"]==3 for c in calls),
                    "Complete NPC builder call inventory differs")
            expected={c["offset"]:0xfc000000 for c in calls}
            for pair,spec in zip(pairs,PAIRS[b]):
                expected[pair["lo_offset"]]=0xffff0000
                if original.read(a+pair["hi_offset"],4)!=target.read(b+pair["hi_offset"],4):expected[pair["hi_offset"]]=0xffff0000
                ra,tb=pair["reference_address"],pair["target_address"];kind,count=spec[4:]
                require(mapping.setdefault(ra,tb)==tb,"NPC builder data mapping inconsistent")
                require(pair["storage"]=="file_backed","NPC builder operand is not initialized data or known code")
                extra={}
                if kind=="callback":
                    require(pair["opcode"]==9 and tb==0x214010,"NPC builder callback role differs")
                    checked_identity(original,target,ra,tb,known);used.add(tb)
                    extra={"complete_callback_identity":{k:known[tb][k] for k in ("source","name","address","size","sha256")}}
                elif kind.startswith("table:"):
                    table=arrays.tables[kind[6:]];offset=tb-table["target_address"]
                    require(0<=offset<table["size"] and offset%4==0 and ra==table["reference_address"]+offset and
                            (pair["opcode"]==35 or pair["opcode"]==9 and offset==0),"NPC typed table component differs")
                    extra={"table_name":kind[6:],"table_offset":offset}
                elif kind=="string":
                    require(pair["opcode"]==9,"NPC string operand is not an address")
                    body=animation_literal(original,ra)
                    require(body==animation_literal(target,tb),"Complete NPC builder literal differs")
                    extra={"size":len(body),"sha256":digest(body),"text":body[:-1].decode("ascii")}
                else:
                    require(False,"Unreviewed Common animation operand kind")
                data_rows.append({"caller":linkage,"caller_address":b,"kind":kind,**pair,**extra})
            require(masks==expected,"NPC builder mask inventory differs")
            bounds[b]=closed(original,target,a,b,size)
            masks_all.update({b-START+offset:mask for offset,mask in masks.items()})
            for c in calls:
                ra,tb=c["reference_address"],c["target_address"]
                require(tb in known,"Common animation call lacks an independent complete identity")
                checked_identity(original,target,ra,tb,known);used.add(tb)
                identity={k:known[tb][k] for k in ("source","name","size")}
                transfers.append({"caller":linkage,"caller_address":b,**c,"complete_callee_identity":identity})
        require(used==set(INDEPENDENT) and len(data_rows)==10 and len(transfers)==6 and
                len(mapping)==len(set(mapping.values())),"NPC builder complete dependency/data inventory differs")
        unique=unique_template(target,original.read(begin,SPAN),masks_all,START)
        sequences.append({"version":version,"source_start":begin,"target_start":START,"span_bytes":SPAN,
                          "code_bytes":392,"complete_original_members":2,"inter_body_alignment":gaps,
                          "terminal_alignment_bytes":0,"uniqueness":unique,"whole_translation_unit_claimed":False})
        data_proofs.append({"version":version,"typed_string_tables":arrays.tables,"operands":data_rows,
                            "complete_direct_transfers":transfers,"data_extents_promoted":False})
        for linkage,b,size in MEMBERS:
            f=functions[b];a=f["low"]
            if b not in records:
                records[b]={"name":f["name"],"source":SOURCE,"address":b,"size":size,
                            "sha256":digest(target.read(b,size)),"boundary_confirmation":True,
                            "confirmation_kind":CLUSTER_KIND,"provenance":[],"corroboration":{
                            "proof_scope":"complete_member_of_unique_contiguous_original_animation_builder_cluster",
                            "cluster_start":START,"cluster_span_bytes":SPAN,"cluster_member_offset":b-START,
                            "local_control_flow":bounds[b],"whole_translation_unit_claimed":False}}
            rows=[p for p in data_rows if p["caller_address"]==b]
            records[b]["provenance"].append({"version":version,"executable_sha1":original.sha1,
                "source_address":a,"source":SOURCE,"name":f["name"],"linkage_name":linkage,
                "reference_sha256":digest(original.read(a,size)),
                "data_address_operands":[{k:p[k] for k in ("hi_offset","lo_offset","reference_address","target_address","opcode","storage")} for p in rows],
                "direct_transfers":[{k:c[k] for k in ("offset","reference_address","target_address","opcode")} for c in transfers if c["caller_address"]==b]})
    return {"functions":sorted(records.values(),key=lambda f:f["address"]),"sequence_proofs":sequences,
            "data_proofs":data_proofs,"counts":{"functions":2,"code_bytes":392,"source_units":1,
            "closed_return_bodies":2,"reviewed_complete_caller_callee_clusters":1}}
