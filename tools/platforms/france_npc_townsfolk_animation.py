"""Original-only proof of the two complete French Ambient/Villager animation clusters."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES,TARGET,require
from platforms.france_tu_sequences import compare,digest,unique_template,words,gpr_writes
from platforms.france_hangable_sequence import hangable_pair
from platforms.france_player_animation_context import closed,storage
from platforms.france_player_animation_tables import animation_literal
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages
from platforms.dwarf1 import iter_dies
from platforms.france_corroborated import ControlFlow




from platforms.france_npc_animation_tables import array_type, OriginalArrays as RobotArrays

class OriginalArrays(RobotArrays):
    def __init__(self,original,target,source):
        self.original,self.target=original,target
        section=next(s for s in original.metadata["sections"] if s["name"]==".debug")
        self.rows=list(iter_dies(original.data[section["offset"]:section["offset"]+section["size"]]))
        self.by={off:(tag,attrs) for off,tag,owner,attrs in self.rows}
        self.tables={}
        for name,address,count in (TABLES[source],):
            declarations=[(off,attrs) for off,tag,owner,attrs in self.rows if tag in (7,12) and
                          owner.replace("\\","/").endswith(source) and attrs.get(3)==name]
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


    def initializer(self,function,pair,count):
        if function["source"]=="SB/Game/zNPCTypeAmbient.cpp" and function["name"]=="ZNPC_AnimTable_Neptune":
            return super().initializer(function,pair,count)
        allowed={"ZNPC_AnimTable_Jelly":(480,6,136),"ZNPC_AnimTable_SuperFriend":(972,10,64),
                 "ZNPC_AnimTable_BalloonBoy":(704,7,64),"ZNPC_AnimTable_Villager":(908,11,72)}
        require(function["name"] in allowed,"Unreviewed NPC scoped copy owner")
        extent,expected_count,prefix=allowed[function["name"]]
        is_jelly=function["name"]=="ZNPC_AnimTable_Jelly"
        require(function["source"]==("SB/Game/zNPCTypeAmbient.cpp" if is_jelly else "SB/Game/zNPCTypeVillager.cpp") and
                function["high"]-function["low"]==extent and count==expected_count,"Scoped NPC copy identity differs")
        original,target=self.original,self.target;a=function["low"];size=function["high"]-a
        roots=[(off,attrs) for off,tag,owner,attrs in self.rows if tag in (6,14) and
               owner.replace("\\","/").endswith(function["source"]) and attrs.get(17)==a and attrs.get(18)==a+size]
        require(len(roots)==1,"NPC original builder DWARF identity ambiguous")
        off,root=roots[0]
        declarations=[(i,attrs) for i,tag,owner,attrs in self.rows if off<i<root[1] and
                      tag==12 and attrs.get(3)=="ourAnims"]
        require(len(declarations)==1,"NPC builder original local array ambiguous")
        off,attrs=declarations[0];array_type(self.by,attrs[7],count,False)
        loc=attrs.get(2)
        require(isinstance(loc,bytes) and len(loc)==11 and loc[:6]==bytes.fromhex("021d00000004") and loc[-1:]==b"\x07",
                "NPC local array lacks its original stack-relative location")
        stack=int.from_bytes(loc[6:10],"little")
        ra,tb=pair["reference_address"],pair["target_address"]
        storage(original,ra,4*count,"initialized_data");storage(target,tb,4*count,"initialized_data")
        body=original.read(ra,4*count)
        require(body==target.read(tb,4*count),"Complete typed NPC local-array initializer differs")
        code=words(original.read(a,size));lo=pair["lo_offset"];base=code[lo//4]>>16&31
        pointers={29:("stack",0)};values={};fvalues={};loads=[];stores=[]
        end=prefix//4
        flow=ControlFlow({a+4*i:w for i,w in enumerate(code)})
        for index,w in enumerate(code[:end]):
            offset=index*4;op=w>>26;rs=w>>21&31;rt=w>>16&31;imm=w&65535
            signed=imm-(65536 if imm&32768 else 0)
            kind=flow.instruction(a+offset)["kind"]
            if is_jelly and index==6:
                require(w>>26==3 and not values and not fvalues and set(pointers)=={29},
                        "Jelly preceding call crosses live initializer data")
                callee=original.by_address.get(((a+offset+4)&0xf0000000)|((w&0x3ffffff)<<2))
                require(callee and callee["source"]=="SB/Core/x/xAnim.cpp" and callee["name"]=="xAnimTableNew" and
                        callee["high"]-callee["low"]==128,"Jelly preceding call identity differs")
            else:
                require(kind=="normal" or is_jelly and kind=="call" and index==end-2 or
                        not is_jelly and index==end-2 and w==0x10800003,
                        "Scoped NPC initializer prefix contains an unreviewed control transfer")
            pointer=pointers.get(rs)
            if offset==lo:
                pointers[base]=("data",0)
                continue
            if op in (30,55,35,49) and pointer and pointer[0]=="data":
                width={30:16,55:8,35:4,49:4}[op];entry=(pointer[1]+signed,width)
                (fvalues if op==49 else values)[rt]=entry
                loads.append({"instruction_offset":offset,"data_offset":entry[0],"size":width})
            elif op in (31,63,43,57) and pointer and pointer[0]=="stack":
                value=(fvalues if op==57 else values).get(rt)
                if value:
                    width={31:16,63:8,43:4,57:4}[op]
                    require(width==value[1] and pointer[1]+signed==stack+value[0],
                            "NPC array copy does not initialize the original typed stack object")
                    stores.append({"instruction_offset":offset,"data_offset":value[0],"size":width,
                                   "stack_offset":pointer[1]+signed})
            else:
                for reg in gpr_writes(w):values.pop(reg,None)
                if op==17 and rs in (4,5,6):fvalues.pop(w>>11&31,None)
            if op==9 and pointer:
                # The first SP adjustment establishes the current frame used by
                # the original DWARF location, rather than an incoming-SP base.
                pointers[rt]=("stack",0) if rs==rt==29 else (pointer[0],pointer[1]+signed)
            else:
                for reg in gpr_writes(w):pointers.pop(reg,None)
        require(sorted(i for row in loads for i in range(row["data_offset"],row["data_offset"]+row["size"]))==list(range(4*count)) and
                sorted(i for row in stores for i in range(row["data_offset"],row["data_offset"]+row["size"]))==list(range(4*count)),
                f"NPC local-array copy does not cover the complete original extent exactly: {function['name']} {loads} {stores}")
        return {"declaration_die":off,"type_die":attrs[7],"count":count,"size":4*count,
                "stack_offset":stack,"sha256":digest(body),"copy_loads":loads,"copy_stores":stores,
                "scoped_copy_control":{"prefix_bytes":prefix,"preceding_complete_call_offset":24 if is_jelly else None,
                    "nonlikely_split_offset":None if is_jelly else prefix-8,"copy_completed_before_successors":True}}

MEMBERS=[('SB/Game/zNPCTypeAmbient.cpp', 'ZNPC_AnimTable_Neptune__Fv', 2945584, 760),
 ('SB/Game/zNPCTypeAmbient.cpp', 'ZNPC_AnimTable_Jelly__Fv', 2946352, 480),
 ('SB/Game/zNPCTypeAmbient.cpp', 'ZNPC_AnimTable_Ambient__Fv', 2946832, 128),
 ('SB/Game/zNPCTypeVillager.cpp', 'ZNPC_AnimTable_SuperFriend__FP10xAnimTable', 3077328, 972),
 ('SB/Game/zNPCTypeVillager.cpp', 'ZNPC_AnimTable_SuperFriend__Fv', 3078304, 8),
 ('SB/Game/zNPCTypeVillager.cpp', 'ZNPC_AnimTable_BalloonBoy__FP10xAnimTable', 3078320, 704),
 ('SB/Game/zNPCTypeVillager.cpp', 'ZNPC_AnimTable_BalloonBoy__Fv', 3079024, 8),
 ('SB/Game/zNPCTypeVillager.cpp', 'ZNPC_AnimTable_Villager__FP10xAnimTable', 3079040, 908),
 ('SB/Game/zNPCTypeVillager.cpp', 'ZNPC_AnimTable_Villager__Fv', 3079952, 8)]
TABLES={'SB/Game/zNPCTypeAmbient.cpp': ('g_strz_ambianim', 5105728, 12),
 'SB/Game/zNPCTypeVillager.cpp': ('g_strz_folkanim', 5108000, 26)}
CLUSTERS={"SB/Game/zNPCTypeAmbient.cpp":(0x2cf230,1376,1368),"SB/Game/zNPCTypeVillager.cpp":(0x2ef4d0,2632,2608)}
INDEPENDENT={2175472: ('SB/Core/x/xAnim.cpp', 'xAnimTableNewTransition', 824),
 2178464: ('SB/Core/x/xAnim.cpp', 'xAnimTableNewState', 612),
 2179088: ('SB/Core/x/xAnim.cpp', 'xAnimDefaultBeforeEnter', 92),
 2179184: ('SB/Core/x/xAnim.cpp', 'xAnimTableNew', 128),
 2947920: ('SB/Game/zNPCTypeCommon.cpp', 'NPCC_BuildStandardAnimTran', 516)}
CALLS={2945584: [(60, 2179184, 3),
           (136, 2178464, 3),
           (208, 2178464, 3),
           (280, 2178464, 3),
           (352, 2178464, 3),
           (424, 2178464, 3),
           (496, 2178464, 3),
           (528, 2947920, 3),
           (596, 2175472, 3),
           (664, 2175472, 3),
           (732, 2175472, 3)],
 2946352: [(24, 2179184, 3),
           (128, 2178464, 3),
           (200, 2178464, 3),
           (272, 2178464, 3),
           (344, 2178464, 3),
           (416, 2178464, 3),
           (452, 2947920, 3)],
 2946832: [(24, 2179184, 3), (100, 2178464, 3)],
 3077328: [(84, 2179184, 3),
           (100, 3079040, 3),
           (168, 2178464, 3),
           (236, 2178464, 3),
           (304, 2178464, 3),
           (372, 2178464, 3),
           (440, 2178464, 3),
           (508, 2178464, 3),
           (576, 2178464, 3),
           (644, 2178464, 3),
           (712, 2178464, 3),
           (748, 2947920, 3),
           (812, 2175472, 3),
           (876, 2175472, 3),
           (940, 2175472, 3)],
 3078304: [(0, 3077328, 2)],
 3078320: [(84, 2179184, 3),
           (100, 3079040, 3),
           (168, 2178464, 3),
           (236, 2178464, 3),
           (304, 2178464, 3),
           (372, 2178464, 3),
           (440, 2178464, 3),
           (508, 2178464, 3),
           (544, 2947920, 3),
           (608, 2175472, 3),
           (672, 2175472, 3)],
 3079024: [(0, 3078320, 2)],
 3079040: [(92, 2179184, 3),
           (164, 2178464, 3),
           (232, 2178464, 3),
           (300, 2178464, 3),
           (368, 2178464, 3),
           (436, 2178464, 3),
           (504, 2178464, 3),
           (572, 2178464, 3),
           (640, 2178464, 3),
           (708, 2178464, 3),
           (776, 2178464, 3),
           (812, 2947920, 3),
           (876, 2175472, 3)],
 3079952: [(0, 3079040, 2)]}
PAIRS={2945584: [(4, 20, 5105808, 9, 'initializer', 7),
           (12, 40, 5233888, 9, 'string', None),
           (76, 84, 2179088, 9, 'callback', None),
           (72, 116, 5105732, 35, 'table:g_strz_ambianim', None),
           (144, 152, 2179088, 9, 'callback', None),
           (156, 184, 5105736, 35, 'table:g_strz_ambianim', None),
           (216, 224, 2179088, 9, 'callback', None),
           (228, 256, 5105740, 35, 'table:g_strz_ambianim', None),
           (288, 296, 2179088, 9, 'callback', None),
           (300, 328, 5105744, 35, 'table:g_strz_ambianim', None),
           (360, 368, 2179088, 9, 'callback', None),
           (372, 400, 5105748, 35, 'table:g_strz_ambianim', None),
           (432, 440, 2179088, 9, 'callback', None),
           (444, 472, 5105752, 35, 'table:g_strz_ambianim', None),
           (508, 520, 5105728, 9, 'table:g_strz_ambianim', None),
           (548, 552, 5105744, 35, 'table:g_strz_ambianim', None),
           (588, 592, 5105732, 35, 'table:g_strz_ambianim', None),
           (616, 620, 5105748, 35, 'table:g_strz_ambianim', None),
           (656, 660, 5105736, 35, 'table:g_strz_ambianim', None),
           (684, 688, 5105752, 35, 'table:g_strz_ambianim', None),
           (724, 728, 5105740, 35, 'table:g_strz_ambianim', None)],
 2946352: [(4, 12, 5233900, 9, 'string', None),
           (40, 48, 5105776, 9, 'initializer', 6),
           (36, 52, 2179088, 9, 'callback', None),
           (60, 124, 5105732, 35, 'table:g_strz_ambianim', None),
           (136, 144, 2179088, 9, 'callback', None),
           (148, 176, 5105756, 35, 'table:g_strz_ambianim', None),
           (208, 216, 2179088, 9, 'callback', None),
           (220, 248, 5105744, 35, 'table:g_strz_ambianim', None),
           (280, 288, 2179088, 9, 'callback', None),
           (292, 320, 5105760, 35, 'table:g_strz_ambianim', None),
           (352, 360, 2179088, 9, 'callback', None),
           (364, 392, 5105772, 35, 'table:g_strz_ambianim', None),
           (428, 444, 5105728, 9, 'table:g_strz_ambianim', None)],
 2946832: [(4, 12, 5233910, 9, 'string', None),
           (40, 48, 2179088, 9, 'callback', None),
           (36, 80, 5105732, 35, 'table:g_strz_ambianim', None)],
 3077328: [(4, 12, 5108192, 9, 'initializer', 10),
           (32, 40, 5108000, 9, 'table:g_strz_folkanim', None),
           (72, 80, 5238678, 9, 'string', None),
           (108, 116, 2179088, 9, 'callback', None),
           (176, 184, 2179088, 9, 'callback', None),
           (244, 252, 2179088, 9, 'callback', None),
           (312, 320, 2179088, 9, 'callback', None),
           (380, 388, 2179088, 9, 'callback', None),
           (448, 456, 2179088, 9, 'callback', None),
           (516, 524, 2179088, 9, 'callback', None),
           (584, 592, 2179088, 9, 'callback', None),
           (652, 660, 2179088, 9, 'callback', None),
           (724, 740, 5108000, 9, 'table:g_strz_folkanim', None)],
 3078304: [],
 3078320: [(4, 12, 5108160, 9, 'initializer', 7),
           (40, 44, 5108000, 9, 'table:g_strz_folkanim', None),
           (72, 80, 5238713, 9, 'string', None),
           (108, 116, 2179088, 9, 'callback', None),
           (176, 184, 2179088, 9, 'callback', None),
           (244, 252, 2179088, 9, 'callback', None),
           (312, 320, 2179088, 9, 'callback', None),
           (380, 388, 2179088, 9, 'callback', None),
           (448, 456, 2179088, 9, 'callback', None),
           (520, 536, 5108000, 9, 'table:g_strz_folkanim', None)],
 3079024: [],
 3079040: [(4, 12, 5108112, 9, 'initializer', 11),
           (40, 48, 5108000, 9, 'table:g_strz_folkanim', None),
           (80, 88, 5238727, 9, 'string', None),
           (104, 112, 2179088, 9, 'callback', None),
           (172, 180, 2179088, 9, 'callback', None),
           (240, 248, 2179088, 9, 'callback', None),
           (308, 316, 2179088, 9, 'callback', None),
           (376, 384, 2179088, 9, 'callback', None),
           (444, 452, 2179088, 9, 'callback', None),
           (512, 520, 2179088, 9, 'callback', None),
           (580, 588, 2179088, 9, 'callback', None),
           (648, 656, 2179088, 9, 'callback', None),
           (716, 724, 2179088, 9, 'callback', None),
           (788, 804, 5108000, 9, 'table:g_strz_folkanim', None)],
 3079952: []}

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
        arrays_by_source={source:OriginalArrays(original,target,source) for source in TABLES};functions={}
        for source,linkage,b,size in MEMBERS:
            matches=[f for f in original.functions if f["source"]==source and links.get(f["low"])==linkage]
            require(len(matches)==1 and matches[0]["high"]-matches[0]["low"]==size,"Complete original NPC builder identity differs")
            functions[b]=matches[0]
        data_rows=[];transfers=[];bounds={};mapping={};used=set();uniques={};cluster_info={}
        for source,(start,span,code_bytes) in CLUSTERS.items():
            arrays=arrays_by_source[source];members=[m for m in MEMBERS if m[0]==source]
            begin=functions[start]["low"]
            owned=sorted([f for f in original.functions if f["source"]==source and begin<=f["low"]<begin+span],key=lambda f:f["low"])
            require(all(f["low"] in links for f in owned),"Original townsfolk cluster member lacks linkage evidence")
            require([(links[f["low"]],f["low"]-begin,f["high"]-f["low"]) for f in owned]==
                    [(link,b-start,size) for _,link,b,size in members],"Original townsfolk cluster membership/order differs")
            require(sum(f["high"]-f["low"] for f in owned)==code_bytes and owned[-1]["high"]==begin+span,
                    "Original townsfolk cluster extent differs")
            masks_all={};gaps=[]
            for index,(_,linkage,b,size) in enumerate(members):
                f=functions[b];a=f["low"]
                require(a%16==b%16==0 and a-b==begin-start,"Townsfolk cluster entry/offset differs")
                padding=(-size)%16
                require(not any(original.read(a+size,padding)) and not any(target.read(b+size,padding)),
                        "Townsfolk builder alignment differs")
                if index:
                    prev=members[index-1];gaps.append({"address":prev[2]+prev[3],"size":b-prev[2]-prev[3]})
                pairs,calls,masks=compare(original,target,a,b,size,address_resolver=hangable_pair)
                require([(p["hi_offset"],p["lo_offset"],p["target_address"],p["opcode"]) for p in pairs]==
                        [p[:4] for p in PAIRS[b]],"Complete NPC builder data operand inventory differs")
                require([(c["offset"],c["target_address"],c["opcode"]) for c in calls]==CALLS[b],
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
                        require(kind=="initializer" and pair["opcode"]==9 and count is not None,"Unreviewed boss initializer kind")
                        extra=arrays.initializer(f,pair,count)
                    data_rows.append({"caller":linkage,"caller_address":b,"kind":kind,**pair,**extra})
                require(masks==expected,"NPC builder mask inventory differs")
                if size==8:
                    require(b in (0x2ef8a0,0x2efb70,0x2eff10) and len(calls)==1 and calls[0]["opcode"]==2 and
                            original.read(a+4,4)==target.read(b+4,4)==(0x202d).to_bytes(4,"little"),
                            "Townsfolk null-argument tail wrapper differs")
                    dest=calls[0]["target_address"];ref=calls[0]["reference_address"]
                    require(dest in functions and functions[dest]["low"]==ref and functions[dest]["name"]==f["name"] and
                            functions[dest]["high"]-ref>8,"Townsfolk wrapper lacks its complete local builder")
                    callee=closed(original,target,ref,dest,functions[dest]["high"]-ref)
                    bounds[b]={"passes":True,"rule":"exact_J_with_daddu_a0_zero_zero_delay_to_complete_local_builder",
                               "target_address":dest,"preserves_stack_and_return_address":True,"complete_callee_boundary":callee}
                else:
                    require(all(c["opcode"]==3 for c in calls),"Unexpected townsfolk non-call transfer")
                    bounds[b]=closed(original,target,a,b,size)
                masks_all.update({b-start+off:mask for off,mask in masks.items()})
                for c in calls:
                    ra,tb=c["reference_address"],c["target_address"]
                    if tb in functions:
                        require(functions[tb]["low"]==ra and (size==8 or tb==0x2efb80 and b!=tb),
                                "Unreviewed or cyclic townsfolk internal transfer")
                        identity={"source":source,"name":functions[tb]["name"],"size":functions[tb]["high"]-ra,
                                  "scope":"complete_member_of_same_unique_original_cluster"}
                    else:
                        require(tb in known,"Townsfolk call lacks independent complete identity")
                        checked_identity(original,target,ra,tb,known);used.add(tb)
                        identity={k:known[tb][k] for k in ("source","name","size")}
                    transfers.append({"caller":linkage,"caller_address":b,**c,"complete_callee_identity":identity})
            unique=unique_template(target,original.read(begin,span),masks_all,start)
            for _,_,b,_ in members:
                uniques[b]=unique;cluster_info[b]={"start":start,"span_bytes":span,"member_offset":b-start}
            sequences.append({"version":version,"source":source,"source_start":begin,"target_start":start,
                              "span_bytes":span,"code_bytes":code_bytes,"complete_original_members":len(members),
                              "inter_body_alignment":gaps,"terminal_alignment_bytes":(-span)%16,
                              "uniqueness":unique,"whole_translation_unit_claimed":False})
        require(used==set(INDEPENDENT) and len(data_rows)==74 and len(transfers)==62 and
                len(mapping)==len(set(mapping.values())),"NPC builder complete dependency/data inventory differs")
        data_proofs.append({"version":version,"typed_string_tables_by_source":{source:a.tables for source,a in arrays_by_source.items()},
                            "operands":data_rows,"complete_direct_transfers":transfers,"data_extents_promoted":False})
        for source,linkage,b,size in MEMBERS:
            f=functions[b];a=f["low"]
            if b not in records:
                records[b]={"name":f["name"],"source":source,"address":b,"size":size,
                            "sha256":digest(target.read(b,size)),"boundary_confirmation":True,
                            "confirmation_kind":CLUSTER_KIND,"provenance":[],"corroboration":{
                            "proof_scope":"complete_member_of_unique_original_townsfolk_animation_cluster",
                            "cluster":cluster_info[b],
                            "uniqueness":uniques[b],
                            "local_control_flow":bounds[b],"whole_translation_unit_claimed":False}}
            rows=[p for p in data_rows if p["caller_address"]==b]
            records[b]["provenance"].append({"version":version,"executable_sha1":original.sha1,
                "source_address":a,"source":source,"name":f["name"],"linkage_name":linkage,
                "reference_sha256":digest(original.read(a,size)),
                "data_address_operands":[{k:p[k] for k in ("hi_offset","lo_offset","reference_address","target_address","opcode","storage")} for p in rows],
                "direct_transfers":[{k:c[k] for k in ("offset","reference_address","target_address","opcode")} for c in transfers if c["caller_address"]==b]})
    return {"functions":sorted(records.values(),key=lambda f:f["address"]),"sequence_proofs":sequences,
            "data_proofs":data_proofs,"counts":{"functions":9,"code_bytes":3976,"source_units":2,
            "closed_return_bodies":6,"exact_null_argument_tail_wrappers":3,"reviewed_complete_caller_callee_clusters":2}}
