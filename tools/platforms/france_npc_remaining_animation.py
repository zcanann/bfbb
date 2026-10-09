"""Original-only proof of the five further complete French NPC animation builders."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES,TARGET,require
from platforms.france_tu_sequences import compare,digest,unique_template,words
from platforms.france_hangable_sequence import hangable_pair
from platforms.france_player_animation_context import closed,storage
from platforms.france_player_animation_tables import animation_literal
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages
from platforms.dwarf1 import iter_dies




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
        if function["source"]!="SB/Game/zNPCTypeBossSandy.cpp":
            return super().initializer(function,pair,count)
        require(function["name"]=="ZNPC_AnimTable_BossSandy" and function["high"]-function["low"]==1852 and count==25,
                "Unreviewed Sandy copy-loop owner or extent")
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
        require(stack==64 and pair["hi_offset"]==4 and pair["lo_offset"]==12,
                "Sandy initializer stack or source operand differs")
        # This closed, literal loop is specific to the authenticated 100-byte
        # Sandy initializer. It does not extend the generic straight-line rule.
        expected=[0x27bdff50,0x3c050000,0xffbf0030,0x24a50000,0x7fb00020,0x27a70040,
                  0x24040003,0,0x78a30000,0x2484ffff,0x78a20010,0x7ce30000,
                  0x24a50020,0x7ce20010,0x1c80fff9,0x24e70020,0xc4a00000,
                  0x3c040000,0x24840000,0x302d,0x282d,0x0c000000,0xe4e00000]
        code=words(original.read(a,len(expected)*4))
        masks={4:0xffff0000,12:0xffff0000,68:0xffff0000,72:0xffff0000,84:0xfc000000}
        require(all((word&masks.get(i*4,0xffffffff))==expected[i] for i,word in enumerate(code)),
                "Sandy complete bounded-copy prefix differs")
        lo=code[3]&65535
        require(((code[1]&65535)<<16)+(lo-(65536 if lo&32768 else 0))==ra,
                "Sandy initializer source address differs")
        loads=[];stores=[]
        for iteration in range(3):
            for source_offset,load,store in ((0,32,44),(16,40,52)):
                offset=iteration*32+source_offset
                loads.append({"instruction_offset":load,"iteration":iteration,"data_offset":offset,"size":16})
                stores.append({"instruction_offset":store,"iteration":iteration,"data_offset":offset,
                               "size":16,"stack_offset":stack+offset})
        loads.append({"instruction_offset":64,"data_offset":96,"size":4})
        stores.append({"instruction_offset":88,"data_offset":96,"size":4,"stack_offset":stack+96})
        require(sorted(i for row in loads for i in range(row["data_offset"],row["data_offset"]+row["size"]))==list(range(100)) and
                sorted(i for row in stores for i in range(row["data_offset"],row["data_offset"]+row["size"]))==list(range(100)),
                "Sandy typed copy coverage differs")
        return {"declaration_die":off,"type_die":attrs[7],"count":count,"size":4*count,
                "stack_offset":stack,"sha256":digest(body),"copy_loads":loads,"copy_stores":stores,
                "bounded_loop":{"start_offset":32,"branch_offset":56,"delay_offset":60,"iterations":3,
                                "bytes_per_iteration":32,"counter_register":4,"terminal_copy_bytes":4}}

MEMBERS=[('SB/Game/zNPCTypeTiki.cpp', 'ZNPC_AnimTable_Tiki__Fv', 3057152, 124),
 ('SB/Game/zNPCTypeTest.cpp', 'ZNPC_AnimTable_Test__Fv', 3295504, 368),
 ('SB/Game/zNPCTypeBossSandy.cpp', 'ZNPC_AnimTable_BossSandy__Fv', 3370864, 1852),
 ('SB/Game/zNPCTypeBossSB1.cpp', 'ZNPC_AnimTable_BossSB1__Fv', 3523648, 1028),
 ('SB/Game/zNPCTypeBossPatrick.cpp', 'ZNPC_AnimTable_BossPatrick__Fv', 3638976, 1744)]
TABLES={'SB/Game/zNPCTypeBossPatrick.cpp': ('g_strz_bossanim', 5112192, 78),
 'SB/Game/zNPCTypeBossSB1.cpp': ('g_strz_bossanim', 5112192, 78),
 'SB/Game/zNPCTypeBossSandy.cpp': ('g_strz_bossanim', 5112192, 78),
 'SB/Game/zNPCTypeTest.cpp': ('g_strz_testanim', 5112144, 11),
 'SB/Game/zNPCTypeTiki.cpp': ('g_strz_tikianim', 5303320, 2)}
INDEPENDENT={2175472: ('SB/Core/x/xAnim.cpp', 'xAnimTableNewTransition', 824),
 2178464: ('SB/Core/x/xAnim.cpp', 'xAnimTableNewState', 612),
 2179088: ('SB/Core/x/xAnim.cpp', 'xAnimDefaultBeforeEnter', 92),
 2179184: ('SB/Core/x/xAnim.cpp', 'xAnimTableNew', 128),
 2947920: ('SB/Game/zNPCTypeCommon.cpp', 'NPCC_BuildStandardAnimTran', 516)}
CALLS={3057152: [(24, 2179184), (96, 2178464)],
 3295504: [(44, 2179184), (124, 2178464), (228, 2175472), (308, 2175472)],
 3370864: [(84, 2179184),
           (160, 2178464),
           (232, 2178464),
           (304, 2178464),
           (376, 2178464),
           (448, 2178464),
           (520, 2178464),
           (592, 2178464),
           (664, 2178464),
           (736, 2178464),
           (808, 2178464),
           (880, 2178464),
           (952, 2178464),
           (1024, 2178464),
           (1096, 2178464),
           (1168, 2178464),
           (1240, 2178464),
           (1312, 2178464),
           (1384, 2178464),
           (1456, 2178464),
           (1528, 2178464),
           (1600, 2178464),
           (1672, 2178464),
           (1744, 2178464),
           (1780, 2947920)],
 3523648: [(68, 2179184),
           (144, 2178464),
           (216, 2178464),
           (288, 2178464),
           (360, 2178464),
           (432, 2178464),
           (504, 2178464),
           (576, 2178464),
           (648, 2178464),
           (720, 2178464),
           (792, 2178464),
           (828, 2947920),
           (892, 2175472),
           (956, 2175472)],
 3638976: [(92, 2179184),
           (168, 2178464),
           (240, 2178464),
           (312, 2178464),
           (384, 2178464),
           (456, 2178464),
           (528, 2178464),
           (600, 2178464),
           (672, 2178464),
           (744, 2178464),
           (816, 2178464),
           (888, 2178464),
           (960, 2178464),
           (1032, 2178464),
           (1104, 2178464),
           (1176, 2178464),
           (1248, 2178464),
           (1320, 2178464),
           (1392, 2178464),
           (1464, 2178464),
           (1536, 2178464),
           (1608, 2178464),
           (1680, 2178464),
           (1716, 2947920)]}
PAIRS={3057152: [(4, 12, 5237581, 9, 'string', None), (40, 48, 2179088, 9, 'callback', None)],
 3295504: [(4, 12, 5244862, 9, 'string', None),
           (36, 48, 5112144, 9, 'table:g_strz_testanim', None),
           (64, 68, 2179088, 9, 'callback', None)],
 3370864: [(4, 12, 5113296, 9, 'initializer', 25),
           (68, 72, 5247672, 9, 'string', None),
           (100, 108, 2179088, 9, 'callback', None),
           (96, 140, 5112196, 35, 'table:g_strz_bossanim', None),
           (168, 176, 2179088, 9, 'callback', None),
           (180, 208, 5112204, 35, 'table:g_strz_bossanim', None),
           (240, 248, 2179088, 9, 'callback', None),
           (252, 280, 5112208, 35, 'table:g_strz_bossanim', None),
           (312, 320, 2179088, 9, 'callback', None),
           (324, 352, 5112212, 35, 'table:g_strz_bossanim', None),
           (384, 392, 2179088, 9, 'callback', None),
           (396, 424, 5112216, 35, 'table:g_strz_bossanim', None),
           (456, 464, 2179088, 9, 'callback', None),
           (468, 496, 5112236, 35, 'table:g_strz_bossanim', None),
           (528, 536, 2179088, 9, 'callback', None),
           (540, 568, 5112240, 35, 'table:g_strz_bossanim', None),
           (600, 608, 2179088, 9, 'callback', None),
           (612, 640, 5112244, 35, 'table:g_strz_bossanim', None),
           (672, 680, 2179088, 9, 'callback', None),
           (684, 712, 5112248, 35, 'table:g_strz_bossanim', None),
           (744, 752, 2179088, 9, 'callback', None),
           (756, 784, 5112252, 35, 'table:g_strz_bossanim', None),
           (816, 824, 2179088, 9, 'callback', None),
           (828, 856, 5112256, 35, 'table:g_strz_bossanim', None),
           (888, 896, 2179088, 9, 'callback', None),
           (900, 928, 5112260, 35, 'table:g_strz_bossanim', None),
           (960, 968, 2179088, 9, 'callback', None),
           (972, 1000, 5112228, 35, 'table:g_strz_bossanim', None),
           (1032, 1040, 2179088, 9, 'callback', None),
           (1044, 1072, 5112264, 35, 'table:g_strz_bossanim', None),
           (1104, 1112, 2179088, 9, 'callback', None),
           (1116, 1144, 5112268, 35, 'table:g_strz_bossanim', None),
           (1176, 1184, 2179088, 9, 'callback', None),
           (1188, 1216, 5112272, 35, 'table:g_strz_bossanim', None),
           (1248, 1256, 2179088, 9, 'callback', None),
           (1260, 1288, 5112276, 35, 'table:g_strz_bossanim', None),
           (1320, 1328, 2179088, 9, 'callback', None),
           (1332, 1360, 5112280, 35, 'table:g_strz_bossanim', None),
           (1392, 1400, 2179088, 9, 'callback', None),
           (1404, 1432, 5112284, 35, 'table:g_strz_bossanim', None),
           (1464, 1472, 2179088, 9, 'callback', None),
           (1476, 1504, 5112288, 35, 'table:g_strz_bossanim', None),
           (1536, 1544, 2179088, 9, 'callback', None),
           (1548, 1576, 5112292, 35, 'table:g_strz_bossanim', None),
           (1608, 1616, 2179088, 9, 'callback', None),
           (1620, 1648, 5112296, 35, 'table:g_strz_bossanim', None),
           (1680, 1688, 2179088, 9, 'callback', None),
           (1692, 1720, 5112300, 35, 'table:g_strz_bossanim', None),
           (1756, 1772, 5112192, 9, 'table:g_strz_bossanim', None)],
 3523648: [(4, 20, 5114224, 9, 'initializer', 11),
           (12, 44, 5257642, 9, 'string', None),
           (84, 92, 2179088, 9, 'callback', None),
           (80, 124, 5112196, 35, 'table:g_strz_bossanim', None),
           (152, 160, 2179088, 9, 'callback', None),
           (164, 192, 5112200, 35, 'table:g_strz_bossanim', None),
           (224, 232, 2179088, 9, 'callback', None),
           (236, 264, 5112204, 35, 'table:g_strz_bossanim', None),
           (296, 304, 2179088, 9, 'callback', None),
           (308, 336, 5112356, 35, 'table:g_strz_bossanim', None),
           (368, 376, 2179088, 9, 'callback', None),
           (380, 408, 5112360, 35, 'table:g_strz_bossanim', None),
           (440, 448, 2179088, 9, 'callback', None),
           (452, 480, 5112364, 35, 'table:g_strz_bossanim', None),
           (512, 520, 2179088, 9, 'callback', None),
           (524, 552, 5112368, 35, 'table:g_strz_bossanim', None),
           (584, 592, 2179088, 9, 'callback', None),
           (596, 624, 5112372, 35, 'table:g_strz_bossanim', None),
           (656, 664, 2179088, 9, 'callback', None),
           (668, 696, 5112376, 35, 'table:g_strz_bossanim', None),
           (728, 736, 2179088, 9, 'callback', None),
           (740, 768, 5112380, 35, 'table:g_strz_bossanim', None),
           (804, 820, 5112192, 9, 'table:g_strz_bossanim', None),
           (848, 852, 5112356, 35, 'table:g_strz_bossanim', None),
           (884, 888, 5112360, 35, 'table:g_strz_bossanim', None),
           (912, 916, 5112364, 35, 'table:g_strz_bossanim', None),
           (948, 952, 5112196, 35, 'table:g_strz_bossanim', None)],
 3638976: [(4, 20, 5115104, 9, 'initializer', 23),
           (12, 36, 5264980, 9, 'string', None),
           (108, 116, 2179088, 9, 'callback', None),
           (104, 148, 5112196, 35, 'table:g_strz_bossanim', None),
           (176, 184, 2179088, 9, 'callback', None),
           (188, 216, 5112200, 35, 'table:g_strz_bossanim', None),
           (248, 256, 2179088, 9, 'callback', None),
           (260, 288, 5112204, 35, 'table:g_strz_bossanim', None),
           (320, 328, 2179088, 9, 'callback', None),
           (332, 360, 5112208, 35, 'table:g_strz_bossanim', None),
           (392, 400, 2179088, 9, 'callback', None),
           (404, 432, 5112216, 35, 'table:g_strz_bossanim', None),
           (464, 472, 2179088, 9, 'callback', None),
           (476, 504, 5112220, 35, 'table:g_strz_bossanim', None),
           (536, 544, 2179088, 9, 'callback', None),
           (548, 576, 5112224, 35, 'table:g_strz_bossanim', None),
           (608, 616, 2179088, 9, 'callback', None),
           (620, 648, 5112228, 35, 'table:g_strz_bossanim', None),
           (680, 688, 2179088, 9, 'callback', None),
           (692, 720, 5112304, 35, 'table:g_strz_bossanim', None),
           (752, 760, 2179088, 9, 'callback', None),
           (764, 792, 5112308, 35, 'table:g_strz_bossanim', None),
           (824, 832, 2179088, 9, 'callback', None),
           (836, 864, 5112312, 35, 'table:g_strz_bossanim', None),
           (896, 904, 2179088, 9, 'callback', None),
           (908, 936, 5112316, 35, 'table:g_strz_bossanim', None),
           (968, 976, 2179088, 9, 'callback', None),
           (980, 1008, 5112320, 35, 'table:g_strz_bossanim', None),
           (1040, 1048, 2179088, 9, 'callback', None),
           (1052, 1080, 5112324, 35, 'table:g_strz_bossanim', None),
           (1112, 1120, 2179088, 9, 'callback', None),
           (1124, 1152, 5112328, 35, 'table:g_strz_bossanim', None),
           (1184, 1192, 2179088, 9, 'callback', None),
           (1196, 1224, 5112332, 35, 'table:g_strz_bossanim', None),
           (1256, 1264, 2179088, 9, 'callback', None),
           (1268, 1296, 5112336, 35, 'table:g_strz_bossanim', None),
           (1328, 1336, 2179088, 9, 'callback', None),
           (1340, 1368, 5112340, 35, 'table:g_strz_bossanim', None),
           (1400, 1408, 2179088, 9, 'callback', None),
           (1412, 1440, 5112344, 35, 'table:g_strz_bossanim', None),
           (1472, 1480, 2179088, 9, 'callback', None),
           (1484, 1512, 5112232, 35, 'table:g_strz_bossanim', None),
           (1544, 1552, 2179088, 9, 'callback', None),
           (1556, 1584, 5112348, 35, 'table:g_strz_bossanim', None),
           (1616, 1624, 2179088, 9, 'callback', None),
           (1628, 1656, 5112352, 35, 'table:g_strz_bossanim', None),
           (1692, 1708, 5112192, 9, 'table:g_strz_bossanim', None)]}

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
        data_rows=[];transfers=[];bounds={};mapping={};used=set();uniques={}
        for source,linkage,b,size in MEMBERS:
            arrays=arrays_by_source[source];f=functions[b];a=f["low"]
            require(a%16==b%16==0,"Boss builder entry alignment differs")
            padding=(-size)%16
            require(not any(original.read(a+size,padding)) and not any(target.read(b+size,padding)),
                    "Boss builder terminal alignment differs")
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
                    require(kind=="initializer" and pair["opcode"]==9 and count is not None,"Unreviewed boss initializer kind")
                    extra=arrays.initializer(f,pair,count)
                data_rows.append({"caller":linkage,"caller_address":b,"kind":kind,**pair,**extra})
            require(masks==expected,"NPC builder mask inventory differs")
            bounds[b]=closed(original,target,a,b,size)
            uniques[b]=unique_template(target,original.read(a,size),masks,b)
            sequences.append({"version":version,"source":source,"source_start":a,"target_start":b,
                              "code_bytes":size,"terminal_alignment_bytes":padding,"uniqueness":uniques[b],
                              "whole_translation_unit_claimed":False})
            for c in calls:
                ra,tb=c["reference_address"],c["target_address"]
                require(tb in known,"Boss animation call lacks an independent complete identity")
                checked_identity(original,target,ra,tb,known);used.add(tb)
                identity={k:known[tb][k] for k in ("source","name","size")}
                transfers.append({"caller":linkage,"caller_address":b,**c,"complete_callee_identity":identity})
        require(used==set(INDEPENDENT) and len(data_rows)==128 and len(transfers)==69 and
                len(mapping)==len(set(mapping.values())),"NPC builder complete dependency/data inventory differs")
        data_proofs.append({"version":version,"typed_string_tables_by_source":{source:a.tables for source,a in arrays_by_source.items()},
                            "operands":data_rows,"complete_direct_transfers":transfers,"data_extents_promoted":False})
        for source,linkage,b,size in MEMBERS:
            f=functions[b];a=f["low"]
            if b not in records:
                records[b]={"name":f["name"],"source":source,"address":b,"size":size,
                            "sha256":digest(target.read(b,size)),"boundary_confirmation":True,
                            "confirmation_kind":CLUSTER_KIND,"provenance":[],"corroboration":{
                            "proof_scope":"unique_complete_original_animation_builder_with_typed_data_and_independent_callees",
                            "uniqueness":uniques[b],
                            "local_control_flow":bounds[b],"whole_translation_unit_claimed":False}}
            rows=[p for p in data_rows if p["caller_address"]==b]
            records[b]["provenance"].append({"version":version,"executable_sha1":original.sha1,
                "source_address":a,"source":source,"name":f["name"],"linkage_name":linkage,
                "reference_sha256":digest(original.read(a,size)),
                "data_address_operands":[{k:p[k] for k in ("hi_offset","lo_offset","reference_address","target_address","opcode","storage")} for p in rows],
                "direct_transfers":[{k:c[k] for k in ("offset","reference_address","target_address","opcode")} for c in transfers if c["caller_address"]==b]})
    return {"functions":sorted(records.values(),key=lambda f:f["address"]),"sequence_proofs":sequences,
            "data_proofs":data_proofs,"counts":{"functions":5,"code_bytes":5116,"source_units":5,
            "closed_return_bodies":5,"reviewed_complete_caller_callee_clusters":5}}
