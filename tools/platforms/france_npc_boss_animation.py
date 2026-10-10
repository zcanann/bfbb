"""Original-only proof of the five independent complete French boss animation builders."""
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


MEMBERS=[('SB/Game/zNPCTypeKingJelly.cpp', 'ZNPC_AnimTable_KingJelly__Fv', 3446800, 2296),
 ('SB/Game/zNPCTypeBossSB2.cpp', 'ZNPC_AnimTable_BossSB2__Fv', 3509024, 3096),
 ('SB/Game/zNPCTypePrawn.cpp', 'ZNPC_AnimTable_Prawn__Fv', 3550528, 992),
 ('SB/Game/zNPCTypeBossPlankton.cpp', 'ZNPC_AnimTable_BossPlankton__Fv', 3595392, 2440),
 ('SB/Game/zNPCTypeDutchman.cpp', 'ZNPC_AnimTable_Dutchman__Fv', 3845232, 1648)]
TABLES={'SB/Game/zNPCTypeBossPlankton.cpp': ('g_strz_bossanim', 5112192, 78),
 'SB/Game/zNPCTypeBossSB2.cpp': ('g_strz_bossanim', 5112192, 78),
 'SB/Game/zNPCTypeDutchman.cpp': ('g_strz_subbanim', 5112512, 23),
 'SB/Game/zNPCTypeKingJelly.cpp': ('g_strz_subbanim', 5112512, 23),
 'SB/Game/zNPCTypePrawn.cpp': ('g_strz_subbanim', 5112512, 23)}
INDEPENDENT={2175472: ('SB/Core/x/xAnim.cpp', 'xAnimTableNewTransition', 824),
 2178464: ('SB/Core/x/xAnim.cpp', 'xAnimTableNewState', 612),
 2179088: ('SB/Core/x/xAnim.cpp', 'xAnimDefaultBeforeEnter', 92),
 2179184: ('SB/Core/x/xAnim.cpp', 'xAnimTableNew', 128),
 2947920: ('SB/Game/zNPCTypeCommon.cpp', 'NPCC_BuildStandardAnimTran', 516)}
CALLS={3446800: [(68, 2179184),
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
           (900, 2175472),
           (972, 2175472),
           (1044, 2175472),
           (1116, 2175472),
           (1188, 2175472),
           (1260, 2175472),
           (1332, 2175472),
           (1404, 2175472),
           (1476, 2175472),
           (1548, 2175472),
           (1620, 2175472),
           (1692, 2175472),
           (1764, 2175472),
           (1836, 2175472),
           (1908, 2175472),
           (1980, 2175472),
           (2052, 2175472),
           (2124, 2175472),
           (2196, 2175472),
           (2268, 2175472)],
 3509024: [(24, 2179184),
           (108, 2178464),
           (188, 2178464),
           (268, 2178464),
           (348, 2178464),
           (428, 2178464),
           (508, 2178464),
           (588, 2178464),
           (668, 2178464),
           (748, 2178464),
           (828, 2178464),
           (908, 2178464),
           (988, 2178464),
           (1068, 2178464),
           (1148, 2178464),
           (1228, 2178464),
           (1308, 2178464),
           (1388, 2178464),
           (1468, 2178464),
           (1548, 2178464),
           (1628, 2178464),
           (1708, 2178464),
           (1788, 2178464),
           (1868, 2178464),
           (1948, 2178464),
           (1988, 2947920),
           (2060, 2175472),
           (2132, 2175472),
           (2204, 2175472),
           (2276, 2175472),
           (2348, 2175472),
           (2420, 2175472),
           (2492, 2175472),
           (2564, 2175472),
           (2636, 2175472),
           (2708, 2175472),
           (2780, 2175472),
           (2852, 2175472),
           (2924, 2175472),
           (2996, 2175472),
           (3068, 2175472)],
 3550528: [(60, 2179184),
           (136, 2178464),
           (208, 2178464),
           (280, 2178464),
           (352, 2178464),
           (424, 2178464),
           (496, 2178464),
           (568, 2178464),
           (640, 2178464),
           (712, 2178464),
           (748, 2947920),
           (820, 2175472),
           (892, 2175472),
           (964, 2175472)],
 3595392: [(24, 2179184),
           (108, 2178464),
           (188, 2178464),
           (268, 2178464),
           (348, 2178464),
           (428, 2178464),
           (508, 2178464),
           (588, 2178464),
           (668, 2178464),
           (748, 2178464),
           (828, 2178464),
           (908, 2178464),
           (988, 2178464),
           (1068, 2178464),
           (1148, 2178464),
           (1188, 2947920),
           (1260, 2175472),
           (1332, 2175472),
           (1404, 2175472),
           (1476, 2175472),
           (1548, 2175472),
           (1620, 2175472),
           (1692, 2175472),
           (1764, 2175472),
           (1836, 2175472),
           (1908, 2175472),
           (1980, 2175472),
           (2052, 2175472),
           (2124, 2175472),
           (2196, 2175472),
           (2268, 2175472),
           (2340, 2175472),
           (2412, 2175472)],
 3845232: [(68, 2179184),
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
           (1044, 2947920),
           (1116, 2175472),
           (1188, 2175472),
           (1260, 2175472),
           (1332, 2175472),
           (1404, 2175472),
           (1476, 2175472),
           (1548, 2175472),
           (1620, 2175472)]}
PAIRS={3446800: [(4, 20, 5113792, 9, 'initializer', 11),
           (12, 44, 5251907, 9, 'string', None),
           (84, 92, 2179088, 9, 'callback', None),
           (80, 124, 5112516, 35, 'table:g_strz_subbanim', None),
           (152, 160, 2179088, 9, 'callback', None),
           (164, 192, 5112520, 35, 'table:g_strz_subbanim', None),
           (224, 232, 2179088, 9, 'callback', None),
           (236, 264, 5112524, 35, 'table:g_strz_subbanim', None),
           (296, 304, 2179088, 9, 'callback', None),
           (308, 336, 5112540, 35, 'table:g_strz_subbanim', None),
           (368, 376, 2179088, 9, 'callback', None),
           (380, 408, 5112544, 35, 'table:g_strz_subbanim', None),
           (440, 448, 2179088, 9, 'callback', None),
           (452, 480, 5112560, 35, 'table:g_strz_subbanim', None),
           (512, 520, 2179088, 9, 'callback', None),
           (524, 552, 5112564, 35, 'table:g_strz_subbanim', None),
           (584, 592, 2179088, 9, 'callback', None),
           (596, 624, 5112568, 35, 'table:g_strz_subbanim', None),
           (656, 664, 2179088, 9, 'callback', None),
           (668, 696, 5112548, 35, 'table:g_strz_subbanim', None),
           (728, 736, 2179088, 9, 'callback', None),
           (740, 768, 5112572, 35, 'table:g_strz_subbanim', None),
           (804, 820, 5112512, 9, 'table:g_strz_subbanim', None),
           (848, 852, 5112560, 35, 'table:g_strz_subbanim', None),
           (892, 896, 5112544, 35, 'table:g_strz_subbanim', None),
           (920, 924, 5112564, 35, 'table:g_strz_subbanim', None),
           (964, 968, 5112544, 35, 'table:g_strz_subbanim', None),
           (992, 996, 5112544, 35, 'table:g_strz_subbanim', None),
           (1036, 1040, 5112564, 35, 'table:g_strz_subbanim', None),
           (1064, 1068, 5112564, 35, 'table:g_strz_subbanim', None),
           (1108, 1112, 5112568, 35, 'table:g_strz_subbanim', None),
           (1136, 1140, 5112520, 35, 'table:g_strz_subbanim', None),
           (1180, 1184, 5112548, 35, 'table:g_strz_subbanim', None),
           (1208, 1212, 5112524, 35, 'table:g_strz_subbanim', None),
           (1252, 1256, 5112548, 35, 'table:g_strz_subbanim', None),
           (1280, 1284, 5112540, 35, 'table:g_strz_subbanim', None),
           (1324, 1328, 5112548, 35, 'table:g_strz_subbanim', None),
           (1352, 1356, 5112560, 35, 'table:g_strz_subbanim', None),
           (1396, 1400, 5112548, 35, 'table:g_strz_subbanim', None),
           (1424, 1428, 5112564, 35, 'table:g_strz_subbanim', None),
           (1468, 1472, 5112548, 35, 'table:g_strz_subbanim', None),
           (1496, 1500, 5112544, 35, 'table:g_strz_subbanim', None),
           (1540, 1544, 5112548, 35, 'table:g_strz_subbanim', None),
           (1568, 1572, 5112568, 35, 'table:g_strz_subbanim', None),
           (1612, 1616, 5112548, 35, 'table:g_strz_subbanim', None),
           (1640, 1644, 5112572, 35, 'table:g_strz_subbanim', None),
           (1684, 1688, 5112548, 35, 'table:g_strz_subbanim', None),
           (1712, 1716, 5112520, 35, 'table:g_strz_subbanim', None),
           (1756, 1760, 5112540, 35, 'table:g_strz_subbanim', None),
           (1784, 1788, 5112524, 35, 'table:g_strz_subbanim', None),
           (1828, 1832, 5112540, 35, 'table:g_strz_subbanim', None),
           (1856, 1860, 5112560, 35, 'table:g_strz_subbanim', None),
           (1900, 1904, 5112540, 35, 'table:g_strz_subbanim', None),
           (1928, 1932, 5112564, 35, 'table:g_strz_subbanim', None),
           (1972, 1976, 5112540, 35, 'table:g_strz_subbanim', None),
           (2000, 2004, 5112544, 35, 'table:g_strz_subbanim', None),
           (2044, 2048, 5112540, 35, 'table:g_strz_subbanim', None),
           (2072, 2076, 5112568, 35, 'table:g_strz_subbanim', None),
           (2116, 2120, 5112540, 35, 'table:g_strz_subbanim', None),
           (2144, 2148, 5112572, 35, 'table:g_strz_subbanim', None),
           (2188, 2192, 5112540, 35, 'table:g_strz_subbanim', None),
           (2216, 2220, 5112548, 35, 'table:g_strz_subbanim', None),
           (2260, 2264, 5112572, 35, 'table:g_strz_subbanim', None)],
 3509024: [(4, 12, 5257518, 9, 'string', None),
           (56, 64, 2179088, 9, 'callback', None),
           (36, 96, 5112196, 35, 'table:g_strz_bossanim', None),
           (120, 128, 2179088, 9, 'callback', None),
           (144, 156, 5112200, 35, 'table:g_strz_bossanim', None),
           (200, 208, 2179088, 9, 'callback', None),
           (224, 236, 5112204, 35, 'table:g_strz_bossanim', None),
           (280, 288, 2179088, 9, 'callback', None),
           (304, 316, 5112384, 35, 'table:g_strz_bossanim', None),
           (360, 368, 2179088, 9, 'callback', None),
           (384, 396, 5112388, 35, 'table:g_strz_bossanim', None),
           (440, 448, 2179088, 9, 'callback', None),
           (464, 476, 5112392, 35, 'table:g_strz_bossanim', None),
           (520, 528, 2179088, 9, 'callback', None),
           (544, 556, 5112396, 35, 'table:g_strz_bossanim', None),
           (600, 608, 2179088, 9, 'callback', None),
           (624, 636, 5112400, 35, 'table:g_strz_bossanim', None),
           (680, 688, 2179088, 9, 'callback', None),
           (704, 716, 5112404, 35, 'table:g_strz_bossanim', None),
           (760, 768, 2179088, 9, 'callback', None),
           (784, 796, 5112408, 35, 'table:g_strz_bossanim', None),
           (840, 848, 2179088, 9, 'callback', None),
           (864, 876, 5112412, 35, 'table:g_strz_bossanim', None),
           (920, 928, 2179088, 9, 'callback', None),
           (944, 956, 5112416, 35, 'table:g_strz_bossanim', None),
           (1000, 1008, 2179088, 9, 'callback', None),
           (1024, 1036, 5112420, 35, 'table:g_strz_bossanim', None),
           (1080, 1088, 2179088, 9, 'callback', None),
           (1104, 1116, 5112424, 35, 'table:g_strz_bossanim', None),
           (1160, 1168, 2179088, 9, 'callback', None),
           (1184, 1196, 5112428, 35, 'table:g_strz_bossanim', None),
           (1240, 1248, 2179088, 9, 'callback', None),
           (1264, 1276, 5112432, 35, 'table:g_strz_bossanim', None),
           (1320, 1328, 2179088, 9, 'callback', None),
           (1344, 1356, 5112436, 35, 'table:g_strz_bossanim', None),
           (1400, 1408, 2179088, 9, 'callback', None),
           (1424, 1436, 5112232, 35, 'table:g_strz_bossanim', None),
           (1480, 1488, 2179088, 9, 'callback', None),
           (1504, 1516, 5112220, 35, 'table:g_strz_bossanim', None),
           (1560, 1568, 2179088, 9, 'callback', None),
           (1584, 1596, 5112224, 35, 'table:g_strz_bossanim', None),
           (1640, 1648, 2179088, 9, 'callback', None),
           (1664, 1676, 5112440, 35, 'table:g_strz_bossanim', None),
           (1720, 1728, 2179088, 9, 'callback', None),
           (1744, 1756, 5112444, 35, 'table:g_strz_bossanim', None),
           (1800, 1808, 2179088, 9, 'callback', None),
           (1824, 1836, 5112448, 35, 'table:g_strz_bossanim', None),
           (1880, 1888, 2179088, 9, 'callback', None),
           (1904, 1916, 5112452, 35, 'table:g_strz_bossanim', None),
           (1960, 1976, 5112192, 9, 'table:g_strz_bossanim', None),
           (2008, 2012, 5112384, 35, 'table:g_strz_bossanim', None),
           (2052, 2056, 5112232, 35, 'table:g_strz_bossanim', None),
           (2080, 2084, 5112388, 35, 'table:g_strz_bossanim', None),
           (2124, 2128, 5112232, 35, 'table:g_strz_bossanim', None),
           (2152, 2156, 5112440, 35, 'table:g_strz_bossanim', None),
           (2196, 2200, 5112196, 35, 'table:g_strz_bossanim', None),
           (2224, 2228, 5112444, 35, 'table:g_strz_bossanim', None),
           (2268, 2272, 5112448, 35, 'table:g_strz_bossanim', None),
           (2296, 2300, 5112452, 35, 'table:g_strz_bossanim', None),
           (2340, 2344, 5112196, 35, 'table:g_strz_bossanim', None),
           (2368, 2372, 5112392, 35, 'table:g_strz_bossanim', None),
           (2412, 2416, 5112396, 35, 'table:g_strz_bossanim', None),
           (2440, 2444, 5112396, 35, 'table:g_strz_bossanim', None),
           (2484, 2488, 5112400, 35, 'table:g_strz_bossanim', None),
           (2512, 2516, 5112404, 35, 'table:g_strz_bossanim', None),
           (2556, 2560, 5112408, 35, 'table:g_strz_bossanim', None),
           (2584, 2588, 5112408, 35, 'table:g_strz_bossanim', None),
           (2628, 2632, 5112412, 35, 'table:g_strz_bossanim', None),
           (2656, 2660, 5112416, 35, 'table:g_strz_bossanim', None),
           (2700, 2704, 5112420, 35, 'table:g_strz_bossanim', None),
           (2728, 2732, 5112420, 35, 'table:g_strz_bossanim', None),
           (2772, 2776, 5112424, 35, 'table:g_strz_bossanim', None),
           (2800, 2804, 5112428, 35, 'table:g_strz_bossanim', None),
           (2844, 2848, 5112432, 35, 'table:g_strz_bossanim', None),
           (2872, 2876, 5112432, 35, 'table:g_strz_bossanim', None),
           (2916, 2920, 5112436, 35, 'table:g_strz_bossanim', None),
           (2944, 2948, 5112220, 35, 'table:g_strz_bossanim', None),
           (2988, 2992, 5112196, 35, 'table:g_strz_bossanim', None),
           (3016, 3020, 5112224, 35, 'table:g_strz_bossanim', None),
           (3060, 3064, 5112232, 35, 'table:g_strz_bossanim', None)],
 3550528: [(4, 20, 5114384, 9, 'initializer', 10),
           (12, 36, 5260044, 9, 'string', None),
           (76, 84, 2179088, 9, 'callback', None),
           (72, 116, 5112516, 35, 'table:g_strz_subbanim', None),
           (144, 152, 2179088, 9, 'callback', None),
           (156, 184, 5112528, 35, 'table:g_strz_subbanim', None),
           (216, 224, 2179088, 9, 'callback', None),
           (228, 256, 5112532, 35, 'table:g_strz_subbanim', None),
           (288, 296, 2179088, 9, 'callback', None),
           (300, 328, 5112540, 35, 'table:g_strz_subbanim', None),
           (360, 368, 2179088, 9, 'callback', None),
           (372, 400, 5112560, 35, 'table:g_strz_subbanim', None),
           (432, 440, 2179088, 9, 'callback', None),
           (444, 472, 5112564, 35, 'table:g_strz_subbanim', None),
           (504, 512, 2179088, 9, 'callback', None),
           (516, 544, 5112568, 35, 'table:g_strz_subbanim', None),
           (576, 584, 2179088, 9, 'callback', None),
           (588, 616, 5112548, 35, 'table:g_strz_subbanim', None),
           (648, 656, 2179088, 9, 'callback', None),
           (660, 688, 5112552, 35, 'table:g_strz_subbanim', None),
           (724, 740, 5112512, 9, 'table:g_strz_subbanim', None),
           (768, 772, 5112560, 35, 'table:g_strz_subbanim', None),
           (812, 816, 5112564, 35, 'table:g_strz_subbanim', None),
           (840, 844, 5112564, 35, 'table:g_strz_subbanim', None),
           (884, 888, 5112568, 35, 'table:g_strz_subbanim', None),
           (912, 916, 5112568, 35, 'table:g_strz_subbanim', None),
           (956, 960, 5112516, 35, 'table:g_strz_subbanim', None)],
 3595392: [(4, 12, 5263366, 9, 'string', None),
           (56, 64, 2179088, 9, 'callback', None),
           (36, 96, 5112196, 35, 'table:g_strz_bossanim', None),
           (120, 128, 2179088, 9, 'callback', None),
           (144, 156, 5112204, 35, 'table:g_strz_bossanim', None),
           (200, 208, 2179088, 9, 'callback', None),
           (224, 236, 5112456, 35, 'table:g_strz_bossanim', None),
           (280, 288, 2179088, 9, 'callback', None),
           (304, 316, 5112460, 35, 'table:g_strz_bossanim', None),
           (360, 368, 2179088, 9, 'callback', None),
           (384, 396, 5112464, 35, 'table:g_strz_bossanim', None),
           (440, 448, 2179088, 9, 'callback', None),
           (464, 476, 5112468, 35, 'table:g_strz_bossanim', None),
           (520, 528, 2179088, 9, 'callback', None),
           (544, 556, 5112472, 35, 'table:g_strz_bossanim', None),
           (600, 608, 2179088, 9, 'callback', None),
           (624, 636, 5112476, 35, 'table:g_strz_bossanim', None),
           (680, 688, 2179088, 9, 'callback', None),
           (704, 716, 5112480, 35, 'table:g_strz_bossanim', None),
           (760, 768, 2179088, 9, 'callback', None),
           (784, 796, 5112484, 35, 'table:g_strz_bossanim', None),
           (840, 848, 2179088, 9, 'callback', None),
           (864, 876, 5112488, 35, 'table:g_strz_bossanim', None),
           (920, 928, 2179088, 9, 'callback', None),
           (944, 956, 5112492, 35, 'table:g_strz_bossanim', None),
           (1000, 1008, 2179088, 9, 'callback', None),
           (1024, 1036, 5112496, 35, 'table:g_strz_bossanim', None),
           (1080, 1088, 2179088, 9, 'callback', None),
           (1104, 1116, 5112500, 35, 'table:g_strz_bossanim', None),
           (1160, 1176, 5112192, 9, 'table:g_strz_bossanim', None),
           (1208, 1212, 5112460, 35, 'table:g_strz_bossanim', None),
           (1252, 1256, 5112464, 35, 'table:g_strz_bossanim', None),
           (1280, 1284, 5112464, 35, 'table:g_strz_bossanim', None),
           (1324, 1328, 5112468, 35, 'table:g_strz_bossanim', None),
           (1352, 1356, 5112472, 35, 'table:g_strz_bossanim', None),
           (1396, 1400, 5112476, 35, 'table:g_strz_bossanim', None),
           (1424, 1428, 5112472, 35, 'table:g_strz_bossanim', None),
           (1468, 1472, 5112480, 35, 'table:g_strz_bossanim', None),
           (1496, 1500, 5112476, 35, 'table:g_strz_bossanim', None),
           (1540, 1544, 5112480, 35, 'table:g_strz_bossanim', None),
           (1568, 1572, 5112484, 35, 'table:g_strz_bossanim', None),
           (1612, 1616, 5112488, 35, 'table:g_strz_bossanim', None),
           (1640, 1644, 5112488, 35, 'table:g_strz_bossanim', None),
           (1684, 1688, 5112492, 35, 'table:g_strz_bossanim', None),
           (1712, 1716, 5112204, 35, 'table:g_strz_bossanim', None),
           (1756, 1760, 5112460, 35, 'table:g_strz_bossanim', None),
           (1784, 1788, 5112456, 35, 'table:g_strz_bossanim', None),
           (1828, 1832, 5112460, 35, 'table:g_strz_bossanim', None),
           (1856, 1860, 5112472, 35, 'table:g_strz_bossanim', None),
           (1900, 1904, 5112460, 35, 'table:g_strz_bossanim', None),
           (1928, 1932, 5112476, 35, 'table:g_strz_bossanim', None),
           (1972, 1976, 5112460, 35, 'table:g_strz_bossanim', None),
           (2000, 2004, 5112480, 35, 'table:g_strz_bossanim', None),
           (2044, 2048, 5112460, 35, 'table:g_strz_bossanim', None),
           (2072, 2076, 5112484, 35, 'table:g_strz_bossanim', None),
           (2116, 2120, 5112460, 35, 'table:g_strz_bossanim', None),
           (2144, 2148, 5112488, 35, 'table:g_strz_bossanim', None),
           (2188, 2192, 5112460, 35, 'table:g_strz_bossanim', None),
           (2216, 2220, 5112492, 35, 'table:g_strz_bossanim', None),
           (2260, 2264, 5112460, 35, 'table:g_strz_bossanim', None),
           (2288, 2292, 5112496, 35, 'table:g_strz_bossanim', None),
           (2332, 2336, 5112460, 35, 'table:g_strz_bossanim', None),
           (2360, 2364, 5112500, 35, 'table:g_strz_bossanim', None),
           (2404, 2408, 5112460, 35, 'table:g_strz_bossanim', None)],
 3845232: [(4, 20, 5127792, 9, 'initializer', 13),
           (12, 44, 5269023, 9, 'string', None),
           (84, 92, 2179088, 9, 'callback', None),
           (80, 124, 5112516, 35, 'table:g_strz_subbanim', None),
           (152, 160, 2179088, 9, 'callback', None),
           (164, 192, 5112556, 35, 'table:g_strz_subbanim', None),
           (224, 232, 2179088, 9, 'callback', None),
           (236, 264, 5112528, 35, 'table:g_strz_subbanim', None),
           (296, 304, 2179088, 9, 'callback', None),
           (308, 336, 5112532, 35, 'table:g_strz_subbanim', None),
           (368, 376, 2179088, 9, 'callback', None),
           (380, 408, 5112536, 35, 'table:g_strz_subbanim', None),
           (440, 448, 2179088, 9, 'callback', None),
           (452, 480, 5112540, 35, 'table:g_strz_subbanim', None),
           (512, 520, 2179088, 9, 'callback', None),
           (524, 552, 5112560, 35, 'table:g_strz_subbanim', None),
           (584, 592, 2179088, 9, 'callback', None),
           (596, 624, 5112564, 35, 'table:g_strz_subbanim', None),
           (656, 664, 2179088, 9, 'callback', None),
           (668, 696, 5112568, 35, 'table:g_strz_subbanim', None),
           (728, 736, 2179088, 9, 'callback', None),
           (740, 768, 5112576, 35, 'table:g_strz_subbanim', None),
           (800, 808, 2179088, 9, 'callback', None),
           (812, 840, 5112580, 35, 'table:g_strz_subbanim', None),
           (872, 880, 2179088, 9, 'callback', None),
           (884, 912, 5112584, 35, 'table:g_strz_subbanim', None),
           (944, 952, 2179088, 9, 'callback', None),
           (956, 984, 5112588, 35, 'table:g_strz_subbanim', None),
           (1020, 1036, 5112512, 9, 'table:g_strz_subbanim', None),
           (1064, 1068, 5112560, 35, 'table:g_strz_subbanim', None),
           (1108, 1112, 5112564, 35, 'table:g_strz_subbanim', None),
           (1136, 1140, 5112564, 35, 'table:g_strz_subbanim', None),
           (1180, 1184, 5112568, 35, 'table:g_strz_subbanim', None),
           (1208, 1212, 5112576, 35, 'table:g_strz_subbanim', None),
           (1252, 1256, 5112580, 35, 'table:g_strz_subbanim', None),
           (1280, 1284, 5112580, 35, 'table:g_strz_subbanim', None),
           (1324, 1328, 5112584, 35, 'table:g_strz_subbanim', None),
           (1352, 1356, 5112532, 35, 'table:g_strz_subbanim', None),
           (1396, 1400, 5112516, 35, 'table:g_strz_subbanim', None),
           (1424, 1428, 5112532, 35, 'table:g_strz_subbanim', None),
           (1468, 1472, 5112560, 35, 'table:g_strz_subbanim', None),
           (1496, 1500, 5112532, 35, 'table:g_strz_subbanim', None),
           (1540, 1544, 5112528, 35, 'table:g_strz_subbanim', None),
           (1568, 1572, 5112588, 35, 'table:g_strz_subbanim', None),
           (1612, 1616, 5112556, 35, 'table:g_strz_subbanim', None)]}

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
        require(used==set(INDEPENDENT) and len(data_rows)==279 and len(transfers)==143 and
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
            "data_proofs":data_proofs,"counts":{"functions":5,"code_bytes":10472,"source_units":5,
            "closed_return_bodies":5,"reviewed_complete_caller_callee_clusters":5}}
