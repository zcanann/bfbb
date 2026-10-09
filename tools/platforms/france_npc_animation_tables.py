"""Original-only proof of the contiguous French Robot animation-builder cluster."""
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

SOURCE="SB/Game/zNPCTypeRobot.cpp"
START=0x2e3980
SPAN=9884


def array_type(by,die,count,pointer):
    tag,attrs=by[die]
    suffix="0863000300010100" if pointer else "0855000800"
    descriptor=bytes.fromhex("000a0000000000")+(count-1).to_bytes(4,"little")+bytes.fromhex(suffix)
    require(tag==1 and attrs.get(9)==0 and attrs.get(10)==descriptor,
            "Original NPC array type, extent or element type differs")


class OriginalArrays:
    def __init__(self,original,target):
        self.original,self.target=original,target
        section=next(s for s in original.metadata["sections"] if s["name"]==".debug")
        self.rows=list(iter_dies(original.data[section["offset"]:section["offset"]+section["size"]]))
        self.by={off:(tag,attrs) for off,tag,owner,attrs in self.rows}
        self.tables={}
        for name,address,count in (("g_strz_roboanim",0x4deaa0,41),("g_strz_cloudanim",0x4deb48,3)):
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

    def initializer(self,function,pair,count):
        original,target=self.original,self.target;a=function["low"];size=function["high"]-a
        roots=[(off,attrs) for off,tag,owner,attrs in self.rows if tag in (6,14) and
               owner.replace("\\","/").endswith(SOURCE) and attrs.get(17)==a and attrs.get(18)==a+size]
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
        end=next(i+2 for i,w in enumerate(code) if w>>26==3)
        flow=ControlFlow({a+4*i:w for i,w in enumerate(code)})
        for index,w in enumerate(code[:end]):
            offset=index*4;op=w>>26;rs=w>>21&31;rt=w>>16&31;imm=w&65535
            signed=imm-(65536 if imm&32768 else 0)
            kind=flow.instruction(a+offset)["kind"]
            require(kind=="normal" or kind=="call" and index==end-2,
                    "NPC initializer copy prefix is not straight-line code through the first call delay")
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
                "stack_offset":stack,"sha256":digest(body),"copy_loads":loads,"copy_stores":stores}

MEMBERS = [('ZNPC_AnimTable_Slick__Fv', 3029376, 408),
 ('ZNPC_AnimTable_SlickShield__Fv', 3029792, 124),
 ('ZNPC_AnimTable_FloatDevice__Fv', 3029920, 124),
 ('ZNPC_AnimTable_Tubelet__Fv', 3030048, 184),
 ('ZNPC_AnimTable_Chuck__Fv', 3030240, 552),
 ('ZNPC_AnimTable_ArfArf__Fv', 3030800, 640),
 ('ZNPC_AnimTable_ArfDog__Fv', 3031440, 1296),
 ('ZNPC_AnimTable_SleepyTime__Fv', 3032736, 272),
 ('ZNPC_AnimTable_NightLight__Fv', 3033008, 168),
 ('ZNPC_AnimTable_ThunderCloud__Fv', 3033184, 264),
 ('ZNPC_AnimTable_Monsoon__Fv', 3033456, 488),
 ('ZNPC_AnimTable_GLove__Fv', 3033952, 848),
 ('ZNPC_AnimTable_TTSauce__Fv', 3034800, 124),
 ('ZNPC_AnimTable_TarTar__Fv', 3034928, 552),
 ('ZNPC_AnimTable_Hammer__Fv', 3035488, 632),
 ('ZNPC_AnimTable_Fodder__Fv', 3036128, 1504),
 ('ZNPC_AnimTable_RobotBase__FP10xAnimTable', 3037632, 1628)]

INDEPENDENT = {2175472: ('SB/Core/x/xAnim.cpp', 'xAnimTableNewTransition', 824),
 2178464: ('SB/Core/x/xAnim.cpp', 'xAnimTableNewState', 612),
 2179088: ('SB/Core/x/xAnim.cpp', 'xAnimDefaultBeforeEnter', 92),
 2179184: ('SB/Core/x/xAnim.cpp', 'xAnimTableNew', 128),
 2947920: ('SB/Game/zNPCTypeCommon.cpp', 'NPCC_BuildStandardAnimTran', 516)}

CALLS = {3029376: [(44, 2179184),
           (56, 3037632),
           (128, 2178464),
           (200, 2178464),
           (272, 2178464),
           (308, 2947920),
           (380, 2175472)],
 3029792: [(24, 2179184), (96, 2178464)],
 3029920: [(24, 2179184), (96, 2178464)],
 3030048: [(36, 2179184), (48, 3037632), (120, 2178464), (156, 2947920)],
 3030240: [(44, 2179184),
           (56, 3037632),
           (128, 2178464),
           (200, 2178464),
           (272, 2178464),
           (308, 2947920),
           (380, 2175472),
           (452, 2175472),
           (524, 2175472)],
 3030800: [(60, 2179184),
           (72, 3037632),
           (144, 2178464),
           (216, 2178464),
           (288, 2178464),
           (360, 2178464),
           (432, 2178464),
           (504, 2178464),
           (540, 2947920),
           (612, 2175472)],
 3031440: [(76, 2179184),
           (152, 2178464),
           (224, 2178464),
           (296, 2178464),
           (368, 2178464),
           (440, 2178464),
           (512, 2178464),
           (584, 2178464),
           (656, 2178464),
           (728, 2178464),
           (800, 2178464),
           (872, 2178464),
           (944, 2178464),
           (1016, 2178464),
           (1088, 2178464),
           (1160, 2178464),
           (1232, 2178464),
           (1268, 2947920)],
 3032736: [(52, 2179184), (64, 3037632), (136, 2178464), (208, 2178464), (244, 2947920)],
 3033008: [(36, 2179184), (108, 2178464), (140, 2947920)],
 3033184: [(52, 2179184), (128, 2178464), (200, 2178464), (236, 2947920)],
 3033456: [(52, 2179184),
           (64, 3037632),
           (136, 2178464),
           (208, 2178464),
           (280, 2178464),
           (352, 2178464),
           (388, 2947920),
           (460, 2175472)],
 3033952: [(52, 2179184),
           (64, 3037632),
           (136, 2178464),
           (208, 2178464),
           (280, 2178464),
           (352, 2178464),
           (424, 2178464),
           (496, 2178464),
           (568, 2178464),
           (604, 2947920),
           (676, 2175472),
           (748, 2175472),
           (820, 2175472)],
 3034800: [(24, 2179184), (96, 2178464)],
 3034928: [(44, 2179184),
           (56, 3037632),
           (128, 2178464),
           (200, 2178464),
           (272, 2178464),
           (308, 2947920),
           (380, 2175472),
           (452, 2175472),
           (524, 2175472)],
 3035488: [(52, 2179184),
           (64, 3037632),
           (136, 2178464),
           (208, 2178464),
           (280, 2178464),
           (352, 2178464),
           (424, 2178464),
           (460, 2947920),
           (532, 2175472),
           (604, 2175472)],
 3036128: [(68, 2179184),
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
           (864, 2178464),
           (936, 2178464),
           (1008, 2178464),
           (1080, 2178464),
           (1152, 2178464),
           (1188, 2947920),
           (1260, 2175472),
           (1332, 2175472),
           (1404, 2175472),
           (1476, 2175472)],
 3037632: [(128, 2178464),
           (200, 2178464),
           (272, 2178464),
           (344, 2178464),
           (416, 2178464),
           (488, 2178464),
           (560, 2178464),
           (632, 2178464),
           (704, 2178464),
           (776, 2178464),
           (848, 2178464),
           (920, 2178464),
           (992, 2178464),
           (1064, 2178464),
           (1136, 2178464),
           (1208, 2178464),
           (1244, 2947920),
           (1316, 2175472),
           (1388, 2175472),
           (1460, 2175472),
           (1532, 2175472),
           (1604, 2175472)]}

PAIRS = {3029376: [(4, 20, 5106944, 9, 'initializer', 4),
           (12, 32, 5237341, 9, 'string', None),
           (64, 72, 2179088, 9, 'callback', None),
           (76, 104, 5106404, 35, 'table:g_strz_roboanim', None),
           (136, 144, 2179088, 9, 'callback', None),
           (148, 176, 5106408, 35, 'table:g_strz_roboanim', None),
           (208, 216, 2179088, 9, 'callback', None),
           (220, 248, 5106412, 35, 'table:g_strz_roboanim', None),
           (284, 300, 5106336, 9, 'table:g_strz_roboanim', None),
           (328, 332, 5106404, 35, 'table:g_strz_roboanim', None),
           (372, 376, 5106408, 35, 'table:g_strz_roboanim', None)],
 3029792: [(4, 12, 5237351, 9, 'string', None), (40, 48, 2179088, 9, 'callback', None)],
 3029920: [(4, 12, 5237369, 9, 'string', None), (40, 48, 2179088, 9, 'callback', None)],
 3030048: [(4, 20, 5237388, 9, 'string', None),
           (56, 64, 2179088, 9, 'callback', None),
           (68, 96, 5106392, 35, 'table:g_strz_roboanim', None),
           (132, 148, 5106336, 9, 'table:g_strz_roboanim', None)],
 3030240: [(4, 20, 5106928, 9, 'initializer', 4),
           (12, 32, 5237400, 9, 'string', None),
           (64, 72, 2179088, 9, 'callback', None),
           (76, 104, 5106384, 35, 'table:g_strz_roboanim', None),
           (136, 144, 2179088, 9, 'callback', None),
           (148, 176, 5106388, 35, 'table:g_strz_roboanim', None),
           (208, 216, 2179088, 9, 'callback', None),
           (220, 248, 5106392, 35, 'table:g_strz_roboanim', None),
           (284, 300, 5106336, 9, 'table:g_strz_roboanim', None),
           (328, 332, 5106392, 35, 'table:g_strz_roboanim', None),
           (372, 376, 5106384, 35, 'table:g_strz_roboanim', None),
           (400, 404, 5106352, 35, 'table:g_strz_roboanim', None),
           (444, 448, 5106384, 35, 'table:g_strz_roboanim', None),
           (472, 476, 5106352, 35, 'table:g_strz_roboanim', None),
           (516, 520, 5106384, 35, 'table:g_strz_roboanim', None)],
 3030800: [(4, 20, 5106896, 9, 'initializer', 7),
           (12, 40, 5237410, 9, 'string', None),
           (80, 88, 2179088, 9, 'callback', None),
           (92, 120, 5106392, 35, 'table:g_strz_roboanim', None),
           (152, 160, 2179088, 9, 'callback', None),
           (164, 192, 5106396, 35, 'table:g_strz_roboanim', None),
           (224, 232, 2179088, 9, 'callback', None),
           (236, 264, 5106400, 35, 'table:g_strz_roboanim', None),
           (296, 304, 2179088, 9, 'callback', None),
           (308, 336, 5106452, 35, 'table:g_strz_roboanim', None),
           (368, 376, 2179088, 9, 'callback', None),
           (380, 408, 5106456, 35, 'table:g_strz_roboanim', None),
           (440, 448, 2179088, 9, 'callback', None),
           (452, 480, 5106460, 35, 'table:g_strz_roboanim', None),
           (516, 532, 5106336, 9, 'table:g_strz_roboanim', None),
           (560, 564, 5106452, 35, 'table:g_strz_roboanim', None),
           (604, 608, 5106456, 35, 'table:g_strz_roboanim', None)],
 3031440: [(4, 20, 5106816, 9, 'initializer', 17),
           (12, 36, 5237421, 9, 'string', None),
           (92, 100, 2179088, 9, 'callback', None),
           (88, 132, 5106340, 35, 'table:g_strz_roboanim', None),
           (160, 168, 2179088, 9, 'callback', None),
           (172, 200, 5106348, 35, 'table:g_strz_roboanim', None),
           (232, 240, 2179088, 9, 'callback', None),
           (244, 272, 5106344, 35, 'table:g_strz_roboanim', None),
           (304, 312, 2179088, 9, 'callback', None),
           (316, 344, 5106352, 35, 'table:g_strz_roboanim', None),
           (376, 384, 2179088, 9, 'callback', None),
           (388, 416, 5106356, 35, 'table:g_strz_roboanim', None),
           (448, 456, 2179088, 9, 'callback', None),
           (460, 488, 5106464, 35, 'table:g_strz_roboanim', None),
           (520, 528, 2179088, 9, 'callback', None),
           (532, 560, 5106468, 35, 'table:g_strz_roboanim', None),
           (592, 600, 2179088, 9, 'callback', None),
           (604, 632, 5106384, 35, 'table:g_strz_roboanim', None),
           (664, 672, 2179088, 9, 'callback', None),
           (676, 704, 5106388, 35, 'table:g_strz_roboanim', None),
           (736, 744, 2179088, 9, 'callback', None),
           (748, 776, 5106408, 35, 'table:g_strz_roboanim', None),
           (808, 816, 2179088, 9, 'callback', None),
           (820, 848, 5106416, 35, 'table:g_strz_roboanim', None),
           (880, 888, 2179088, 9, 'callback', None),
           (892, 920, 5106424, 35, 'table:g_strz_roboanim', None),
           (952, 960, 2179088, 9, 'callback', None),
           (964, 992, 5106420, 35, 'table:g_strz_roboanim', None),
           (1024, 1032, 2179088, 9, 'callback', None),
           (1036, 1064, 5106484, 35, 'table:g_strz_roboanim', None),
           (1096, 1104, 2179088, 9, 'callback', None),
           (1108, 1136, 5106488, 35, 'table:g_strz_roboanim', None),
           (1168, 1176, 2179088, 9, 'callback', None),
           (1180, 1208, 5106360, 35, 'table:g_strz_roboanim', None),
           (1244, 1260, 5106336, 9, 'table:g_strz_roboanim', None)],
 3032736: [(4, 12, 5106792, 9, 'initializer', 3),
           (40, 48, 5237432, 9, 'string', None),
           (72, 80, 2179088, 9, 'callback', None),
           (84, 112, 5106384, 35, 'table:g_strz_roboanim', None),
           (144, 152, 2179088, 9, 'callback', None),
           (156, 184, 5106392, 35, 'table:g_strz_roboanim', None),
           (220, 236, 5106336, 9, 'table:g_strz_roboanim', None)],
 3033008: [(4, 20, 5237443, 9, 'string', None), (52, 60, 2179088, 9, 'callback', None)],
 3033184: [(4, 12, 5106776, 9, 'initializer', 3),
           (40, 48, 5237454, 9, 'string', None),
           (68, 76, 2179088, 9, 'callback', None),
           (64, 108, 5106508, 35, 'table:g_strz_cloudanim', None),
           (136, 144, 2179088, 9, 'callback', None),
           (148, 176, 5106512, 35, 'table:g_strz_cloudanim', None),
           (212, 228, 5106504, 9, 'table:g_strz_cloudanim', None)],
 3033456: [(4, 12, 5106752, 9, 'initializer', 5),
           (40, 48, 5237467, 9, 'string', None),
           (72, 80, 2179088, 9, 'callback', None),
           (84, 112, 5106384, 35, 'table:g_strz_roboanim', None),
           (144, 152, 2179088, 9, 'callback', None),
           (156, 184, 5106388, 35, 'table:g_strz_roboanim', None),
           (216, 224, 2179088, 9, 'callback', None),
           (228, 256, 5106404, 35, 'table:g_strz_roboanim', None),
           (288, 296, 2179088, 9, 'callback', None),
           (300, 328, 5106408, 35, 'table:g_strz_roboanim', None),
           (364, 380, 5106336, 9, 'table:g_strz_roboanim', None),
           (408, 412, 5106404, 35, 'table:g_strz_roboanim', None),
           (452, 456, 5106408, 35, 'table:g_strz_roboanim', None)],
 3033952: [(4, 20, 5106720, 9, 'initializer', 8),
           (12, 32, 5237479, 9, 'string', None),
           (72, 80, 2179088, 9, 'callback', None),
           (84, 112, 5106404, 35, 'table:g_strz_roboanim', None),
           (144, 152, 2179088, 9, 'callback', None),
           (156, 184, 5106408, 35, 'table:g_strz_roboanim', None),
           (216, 224, 2179088, 9, 'callback', None),
           (228, 256, 5106412, 35, 'table:g_strz_roboanim', None),
           (288, 296, 2179088, 9, 'callback', None),
           (300, 328, 5106432, 35, 'table:g_strz_roboanim', None),
           (360, 368, 2179088, 9, 'callback', None),
           (372, 400, 5106436, 35, 'table:g_strz_roboanim', None),
           (432, 440, 2179088, 9, 'callback', None),
           (444, 472, 5106440, 35, 'table:g_strz_roboanim', None),
           (504, 512, 2179088, 9, 'callback', None),
           (516, 544, 5106444, 35, 'table:g_strz_roboanim', None),
           (580, 596, 5106336, 9, 'table:g_strz_roboanim', None),
           (624, 628, 5106440, 35, 'table:g_strz_roboanim', None),
           (668, 672, 5106444, 35, 'table:g_strz_roboanim', None),
           (696, 700, 5106404, 35, 'table:g_strz_roboanim', None),
           (740, 744, 5106408, 35, 'table:g_strz_roboanim', None),
           (768, 772, 5106432, 35, 'table:g_strz_roboanim', None),
           (812, 816, 5106436, 35, 'table:g_strz_roboanim', None)],
 3034800: [(4, 12, 5237489, 9, 'string', None), (40, 48, 2179088, 9, 'callback', None)],
 3034928: [(4, 20, 5106704, 9, 'initializer', 4),
           (12, 32, 5237501, 9, 'string', None),
           (64, 72, 2179088, 9, 'callback', None),
           (76, 104, 5106384, 35, 'table:g_strz_roboanim', None),
           (136, 144, 2179088, 9, 'callback', None),
           (148, 176, 5106404, 35, 'table:g_strz_roboanim', None),
           (208, 216, 2179088, 9, 'callback', None),
           (220, 248, 5106392, 35, 'table:g_strz_roboanim', None),
           (284, 300, 5106336, 9, 'table:g_strz_roboanim', None),
           (328, 332, 5106404, 35, 'table:g_strz_roboanim', None),
           (372, 376, 5106392, 35, 'table:g_strz_roboanim', None),
           (400, 404, 5106392, 35, 'table:g_strz_roboanim', None),
           (444, 448, 5106384, 35, 'table:g_strz_roboanim', None),
           (472, 476, 5106352, 35, 'table:g_strz_roboanim', None),
           (516, 520, 5106384, 35, 'table:g_strz_roboanim', None)],
 3035488: [(4, 20, 5106672, 9, 'initializer', 6),
           (12, 32, 5237512, 9, 'string', None),
           (72, 80, 2179088, 9, 'callback', None),
           (84, 112, 5106392, 35, 'table:g_strz_roboanim', None),
           (144, 152, 2179088, 9, 'callback', None),
           (156, 184, 5106432, 35, 'table:g_strz_roboanim', None),
           (216, 224, 2179088, 9, 'callback', None),
           (228, 256, 5106436, 35, 'table:g_strz_roboanim', None),
           (288, 296, 2179088, 9, 'callback', None),
           (300, 328, 5106440, 35, 'table:g_strz_roboanim', None),
           (360, 368, 2179088, 9, 'callback', None),
           (372, 400, 5106444, 35, 'table:g_strz_roboanim', None),
           (436, 452, 5106336, 9, 'table:g_strz_roboanim', None),
           (480, 484, 5106432, 35, 'table:g_strz_roboanim', None),
           (524, 528, 5106436, 35, 'table:g_strz_roboanim', None),
           (552, 556, 5106440, 35, 'table:g_strz_roboanim', None),
           (596, 600, 5106444, 35, 'table:g_strz_roboanim', None)],
 3036128: [(4, 20, 5106608, 9, 'initializer', 16),
           (12, 36, 5237523, 9, 'string', None),
           (84, 92, 2179088, 9, 'callback', None),
           (80, 124, 5106340, 35, 'table:g_strz_roboanim', None),
           (152, 160, 2179088, 9, 'callback', None),
           (164, 192, 5106348, 35, 'table:g_strz_roboanim', None),
           (224, 232, 2179088, 9, 'callback', None),
           (236, 264, 5106344, 35, 'table:g_strz_roboanim', None),
           (296, 304, 2179088, 9, 'callback', None),
           (308, 336, 5106352, 35, 'table:g_strz_roboanim', None),
           (368, 376, 2179088, 9, 'callback', None),
           (380, 408, 5106356, 35, 'table:g_strz_roboanim', None),
           (440, 448, 2179088, 9, 'callback', None),
           (452, 480, 5106472, 35, 'table:g_strz_roboanim', None),
           (512, 520, 2179088, 9, 'callback', None),
           (524, 552, 5106476, 35, 'table:g_strz_roboanim', None),
           (584, 592, 2179088, 9, 'callback', None),
           (596, 624, 5106480, 35, 'table:g_strz_roboanim', None),
           (656, 664, 2179088, 9, 'callback', None),
           (668, 696, 5106392, 35, 'table:g_strz_roboanim', None),
           (728, 736, 2179088, 9, 'callback', None),
           (740, 768, 5106416, 35, 'table:g_strz_roboanim', None),
           (800, 808, 2179088, 9, 'callback', None),
           (812, 840, 5106424, 35, 'table:g_strz_roboanim', None),
           (872, 880, 2179088, 9, 'callback', None),
           (884, 912, 5106420, 35, 'table:g_strz_roboanim', None),
           (944, 952, 2179088, 9, 'callback', None),
           (956, 984, 5106484, 35, 'table:g_strz_roboanim', None),
           (1016, 1024, 2179088, 9, 'callback', None),
           (1028, 1056, 5106488, 35, 'table:g_strz_roboanim', None),
           (1088, 1096, 2179088, 9, 'callback', None),
           (1100, 1128, 5106360, 35, 'table:g_strz_roboanim', None),
           (1164, 1180, 5106336, 9, 'table:g_strz_roboanim', None),
           (1208, 1212, 5106416, 35, 'table:g_strz_roboanim', None),
           (1252, 1256, 5106484, 35, 'table:g_strz_roboanim', None),
           (1280, 1284, 5106424, 35, 'table:g_strz_roboanim', None),
           (1324, 1328, 5106484, 35, 'table:g_strz_roboanim', None),
           (1352, 1356, 5106420, 35, 'table:g_strz_roboanim', None),
           (1396, 1400, 5106484, 35, 'table:g_strz_roboanim', None),
           (1424, 1428, 5106472, 35, 'table:g_strz_roboanim', None),
           (1468, 1472, 5106476, 35, 'table:g_strz_roboanim', None)],
 3037632: [(12, 20, 5106528, 9, 'initializer', 17),
           (36, 52, 2179088, 9, 'callback', None),
           (56, 124, 5106340, 35, 'table:g_strz_roboanim', None),
           (136, 144, 2179088, 9, 'callback', None),
           (148, 176, 5106348, 35, 'table:g_strz_roboanim', None),
           (208, 216, 2179088, 9, 'callback', None),
           (220, 248, 5106344, 35, 'table:g_strz_roboanim', None),
           (280, 288, 2179088, 9, 'callback', None),
           (292, 320, 5106352, 35, 'table:g_strz_roboanim', None),
           (352, 360, 2179088, 9, 'callback', None),
           (364, 392, 5106356, 35, 'table:g_strz_roboanim', None),
           (424, 432, 2179088, 9, 'callback', None),
           (436, 464, 5106416, 35, 'table:g_strz_roboanim', None),
           (496, 504, 2179088, 9, 'callback', None),
           (508, 536, 5106424, 35, 'table:g_strz_roboanim', None),
           (568, 576, 2179088, 9, 'callback', None),
           (580, 608, 5106420, 35, 'table:g_strz_roboanim', None),
           (640, 648, 2179088, 9, 'callback', None),
           (652, 680, 5106484, 35, 'table:g_strz_roboanim', None),
           (712, 720, 2179088, 9, 'callback', None),
           (724, 752, 5106488, 35, 'table:g_strz_roboanim', None),
           (784, 792, 2179088, 9, 'callback', None),
           (796, 824, 5106360, 35, 'table:g_strz_roboanim', None),
           (856, 864, 2179088, 9, 'callback', None),
           (868, 896, 5106364, 35, 'table:g_strz_roboanim', None),
           (928, 936, 2179088, 9, 'callback', None),
           (940, 968, 5106368, 35, 'table:g_strz_roboanim', None),
           (1000, 1008, 2179088, 9, 'callback', None),
           (1012, 1040, 5106428, 35, 'table:g_strz_roboanim', None),
           (1072, 1080, 2179088, 9, 'callback', None),
           (1084, 1112, 5106372, 35, 'table:g_strz_roboanim', None),
           (1144, 1152, 2179088, 9, 'callback', None),
           (1156, 1184, 5106376, 35, 'table:g_strz_roboanim', None),
           (1220, 1236, 5106336, 9, 'table:g_strz_roboanim', None),
           (1264, 1268, 5106364, 35, 'table:g_strz_roboanim', None),
           (1308, 1312, 5106368, 35, 'table:g_strz_roboanim', None),
           (1336, 1340, 5106372, 35, 'table:g_strz_roboanim', None),
           (1380, 1384, 5106376, 35, 'table:g_strz_roboanim', None),
           (1408, 1412, 5106416, 35, 'table:g_strz_roboanim', None),
           (1452, 1456, 5106484, 35, 'table:g_strz_roboanim', None),
           (1480, 1484, 5106424, 35, 'table:g_strz_roboanim', None),
           (1524, 1528, 5106484, 35, 'table:g_strz_roboanim', None),
           (1552, 1556, 5106420, 35, 'table:g_strz_roboanim', None),
           (1596, 1600, 5106484, 35, 'table:g_strz_roboanim', None)]}

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
        require(sum(f["high"]-f["low"] for f in owned)==9808 and owned[-1]["high"]==begin+SPAN,
                "Original NPC builder cluster extent differs")
        require(not any(original.read(begin+SPAN,4)) and not any(target.read(START+SPAN,4)),"NPC builder terminal alignment differs")
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
                    require(kind=="initializer" and pair["opcode"]==9 and count is not None,"Unreviewed NPC builder data kind")
                    extra=arrays.initializer(f,pair,count)
                data_rows.append({"caller":linkage,"caller_address":b,"kind":kind,**pair,**extra})
            require(masks==expected,"NPC builder mask inventory differs")
            bounds[b]=closed(original,target,a,b,size)
            masks_all.update({b-START+offset:mask for offset,mask in masks.items()})
            for c in calls:
                ra,tb=c["reference_address"],c["target_address"]
                if tb in functions:
                    require(tb==0x2e59c0 and b!=tb and functions[tb]["low"]==ra,"Unreviewed or cyclic NPC builder internal call")
                    identity={"source":SOURCE,"name":functions[tb]["name"],"size":functions[tb]["high"]-ra,
                              "scope":"complete_member_of_same_unique_contiguous_cluster"}
                else:
                    require(tb in known,"NPC animation call lacks an independent complete identity")
                    checked_identity(original,target,ra,tb,known);used.add(tb)
                    identity={k:known[tb][k] for k in ("source","name","size")}
                transfers.append({"caller":linkage,"caller_address":b,**c,"complete_callee_identity":identity})
        require(used==set(INDEPENDENT) and len(data_rows)==257 and len(transfers)==149 and
                len(mapping)==len(set(mapping.values())),"NPC builder complete dependency/data inventory differs")
        unique=unique_template(target,original.read(begin,SPAN),masks_all,START)
        sequences.append({"version":version,"source_start":begin,"target_start":START,"span_bytes":SPAN,
                          "code_bytes":9808,"complete_original_members":17,"inter_body_alignment":gaps,
                          "terminal_alignment_bytes":4,"uniqueness":unique,"whole_translation_unit_claimed":False})
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
            "data_proofs":data_proofs,"counts":{"functions":17,"code_bytes":9808,"source_units":1,
            "closed_return_bodies":17,"reviewed_complete_caller_callee_clusters":1}}
