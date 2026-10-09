"""Complete original French Sandy and Patrick goal/behavior kernels."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES,TARGET,require
from platforms.france_tu_sequences import compare,digest,unique_template
from platforms.france_hangable_sequence import hangable_pair
from platforms.france_player_animation_context import closed,storage
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages

MEMBERS=[('SB/Game/zNPCTypeBossSandy.cpp',
  'Process__26zNPCGoalBossSandyRunToRopeFP11en_trantypefPvP6xScene',
  3331248,
  544),
 ('SB/Game/zNPCTypeBossSandy.cpp', 'Enter__26zNPCGoalBossSandyRunToRopeFfPv', 3331792, 704),
 ('SB/Game/zNPCTypeBossSandy.cpp', 'Exit__20zNPCGoalBossSandySitFfPv', 3332800, 88),
 ('SB/Game/zNPCTypeBossSandy.cpp',
  'Process__26zNPCGoalBossSandyElbowDropFP11en_trantypefPvP6xScene',
  3337296,
  812),
 ('SB/Game/zNPCTypeBossSandy.cpp',
  'Process__22zNPCGoalBossSandyMeleeFP11en_trantypefPvP6xScene',
  3341728,
  632),
 ('SB/Game/zNPCTypeBossSandy.cpp', 'Exit__22zNPCGoalBossSandyMeleeFfPv', 3342368, 104),
 ('SB/Game/zNPCTypeBossSandy.cpp',
  'Process__22zNPCGoalBossSandyChaseFP11en_trantypefPvP6xScene',
  3342848,
  536),
 ('SB/Game/zNPCTypeBossSandy.cpp',
  'Process__22zNPCGoalBossSandyTauntFP11en_trantypefPvP6xScene',
  3343456,
  476),
 ('SB/Game/zNPCTypeBossSandy.cpp',
  'Process__21zNPCGoalBossSandyIdleFP11en_trantypefPvP6xScene',
  3344208,
  592),
 ('SB/Game/zNPCTypeBossSandy.cpp', 'leapCB__FP5xGoalPvP11en_trantypefPv', 3346112, 184),
 ('SB/Game/zNPCTypeBossSandy.cpp', 'noHeadCB__FP5xGoalPvP11en_trantypefPv', 3346688, 148),
 ('SB/Game/zNPCTypeBossSandy.cpp', 'hiddenByCutscene__10zNPCBSandyFv', 3352208, 520),
 ('SB/Game/zNPCTypeBossSandy.cpp', 'BoundEventCB__FP5xBaseP5xBaseUiPCfP5xBase', 3357424, 128),
 ('SB/Game/zNPCTypeBossPatrick.cpp', 'Enter__20zNPCGoalBossPatFudgeFfPv', 3602288, 120),
 ('SB/Game/zNPCTypeBossPatrick.cpp', 'Enter__20zNPCGoalBossPatSpawnFfPv', 3609248, 100),
 ('SB/Game/zNPCTypeBossPatrick.cpp', 'Enter__20zNPCGoalBossPatSmackFfPv', 3612080, 108),
 ('SB/Game/zNPCTypeBossPatrick.cpp', 'Process__18zNPCGoalBossPatRunFP11en_trantypefPvP6xScene', 3612240, 120),
 ('SB/Game/zNPCTypeBossPatrick.cpp', 'Enter__19zNPCGoalBossPatSpitFfPv', 3614240, 116),
 ('SB/Game/zNPCTypeBossPatrick.cpp', 'Exit__18zNPCGoalBossPatHitFfPv', 3614368, 128),
 ('SB/Game/zNPCTypeBossPatrick.cpp', 'Enter__19zNPCGoalBossPatIdleFfPv', 3616048, 108),
 ('SB/Game/zNPCTypeBossPatrick.cpp',
  'ParabolaHitsConveyors__12zNPCBPatrickFP9xParabolaP7xCollis',
  3620112,
  584),
 ('SB/Game/zNPCTypeBossPatrick.cpp', 'gotoRound__12zNPCBPatrickFi', 3621264, 448),
 ('SB/Game/zNPCTypeBossPatrick.cpp', 'canSpawnChucks__12zNPCBPatrickFv', 3621712, 64),
 ('SB/Game/zNPCTypeBossPatrick.cpp', 'DuploNotice__12zNPCBPatrickF13en_SM_NOTICESPv', 3626560, 64)]
INDEPENDENT={1580160: ('SB/Game/zLightning.cpp', 'zLightningShow', 48),
 2166032: ('SB/Core/x/xVec3.cpp', 'xVec3Normalize', 224),
 2636464: ('SB/Core/x/xEvent.cpp', 'zEntEvent', 32),
 2912416: ('SB/Game/zNPCGoalStd.cpp', 'CalcNewDir', 624),
 2929808: ('SB/Game/zNPCGoalCommon.cpp', 'Enter', 216),
 2953696: ('SB/Game/zNPCTypeCommon.cpp', 'AnimTimeRemain', 104),
 3088976: ('SB/Core/x/xBehaviour.cpp', 'Process', 56),
 3089184: ('SB/Core/x/xBehaviour.cpp', 'GetOwner', 12)}
CALLS={3331248: [(160, 2166032), (320, 2166032), (500, 3088976)],
 3331792: [(176, 2166032), (648, 2929808)],
 3332800: [],
 3337296: [(484, 2166032), (644, 2166032), (768, 3088976)],
 3341728: [(192, 2166032), (352, 2166032), (588, 3088976)],
 3342368: [],
 3342848: [(164, 2166032), (324, 2166032), (492, 3088976)],
 3343456: [(152, 2166032), (312, 2166032), (432, 3088976)],
 3344208: [(160, 2166032), (328, 2166032), (540, 3088976)],
 3346112: [(60, 2953696)],
 3346688: [(64, 2953696)],
 3352208: [(60, 2636464), (280, 1580160), (292, 1580160), (452, 1580160), (464, 1580160)],
 3357424: [],
 3602288: [(28, 3089184), (88, 2929808)],
 3609248: [(28, 3089184), (68, 2929808)],
 3612080: [(28, 3089184), (76, 2929808)],
 3612240: [(44, 3089184), (80, 3088976)],
 3614240: [(28, 3089184), (84, 2929808)],
 3614368: [(8, 3089184)],
 3616048: [(28, 3089184), (76, 2929808)],
 3620112: [],
 3621264: [(384, 2636464)],
 3621712: [],
 3626560: []}
PAIRS={3331248: [],
 3331792: [(20, 116)],
 3332800: [(36, 44)],
 3337296: [(404, 412)],
 3341728: [(112, 120)],
 3342368: [],
 3342848: [(4, 100)],
 3343456: [(4, 88)],
 3344208: [(4, 96)],
 3346112: [],
 3346688: [],
 3352208: [],
 3357424: [],
 3602288: [],
 3609248: [],
 3612080: [],
 3612240: [],
 3614240: [],
 3614368: [],
 3616048: [],
 3620112: [],
 3621264: [],
 3621712: [],
 3626560: []}


def model_field(original,target,known):
    from platforms.france_goal_sequence import original_data,goal_pair
    objects,_,paths,layouts=original_data(original)
    glob=objects["globals"]
    require([p["offset"] for p in paths[0]]==[1792,0,0,36],"Boss player model component path differs")
    field=glob["reference_address"]+1828
    anchor=known[0x2c70a0];ref=next(p for p in anchor["provenance"] if p["version"]==original.version)
    checked_identity(original,target,ref["source_address"],0x2c70a0,known)
    pairs,_,_=compare(original,target,ref["source_address"],0x2c70a0,624,address_resolver=goal_pair)
    require(any(p["reference_address"]==field and p["target_address"]==0x52cc14 and p["opcode"]==35 for p in pairs),
            "Independent complete goal lost its typed player model anchor")
    storage(original,glob["reference_address"],glob["size"],"runtime_bss")
    storage(target,0x52cc14-1828,glob["size"],"runtime_bss")
    return {"reference_address":field,"target_address":0x52cc14,"globals":glob,
            "path":paths[0],"component_offset":1828,"field_size":4,"data_extent_promoted":False,
            "anchor":{k:anchor[k] for k in ("source","name","address","size","sha256")}}


def generate_unit(originals,registry_dir):
    target=originals[TARGET];known={}
    # Fixed original-only dependency fence: new registry entries cannot affect evidence.
    for path in sorted(registry_dir.glob("*functions.json")):
        for f in json.loads(path.read_text())["functions"]:
            if f["address"] in INDEPENDENT:
                old=known.setdefault(f["address"],f)
                require(all(old[k]==f[k] for k in ("source","name","size","sha256")),"Conflicting boss goal kernel dependency")
    require(set(known)==set(INDEPENDENT),"Independent boss goal kernel dependency missing")
    for a,identity in INDEPENDENT.items():
        require(tuple(known[a][k] for k in ("source","name","size"))==identity,"Boss goal kernel dependency identity differs")
    records={};sequences=[];data_proofs=[];call_proofs=[]
    for version in REFERENCES:
        original=originals[version];links=canonical_linkages(original.data,original.metadata)
        field=model_field(original,target,known);identities=[];operands=[];transfers=[]
        for source,linkage,b,size in MEMBERS:
            refs=[f for f in original.functions if f["source"]==source and links.get(f["low"])==linkage]
            require(len(refs)==1 and refs[0]["high"]-refs[0]["low"]==size,"Complete boss goal kernel original identity differs")
            f=refs[0];a=f["low"];require(a%16==b%16==0,"Boss goal kernel entry alignment differs")
            padding=(-size)%16
            require(not any(original.read(a+size,padding)) and not any(target.read(b+size,padding)),"Boss goal kernel padding differs")
            pairs,calls,masks=compare(original,target,a,b,size,address_resolver=hangable_pair)
            inventory=PAIRS[b]
            require([(p["hi_offset"],p["lo_offset"]) for p in pairs]==inventory,"Boss goal kernel data inventory differs")
            expected={}
            for p in pairs:
                require(p["reference_address"]==field["reference_address"] and p["target_address"]==field["target_address"] and
                        p["opcode"]==35 and p["storage"]=="zero_fill","Boss goal kernel operand lost its complete typed player model field")
                expected[p["lo_offset"]]=0xffff0000
                if original.read(a+p["hi_offset"],4)!=target.read(b+p["hi_offset"],4):expected[p["hi_offset"]]=0xffff0000
                operands.append({"caller":linkage,"caller_address":b,**p})
            require([(c["offset"],c["target_address"]) for c in calls]==CALLS[b] and all(c["opcode"]==3 for c in calls),
                    "Boss goal kernel complete callee inventory differs")
            for c in calls:
                evidence=checked_identity(original,target,c["reference_address"],c["target_address"],known)
                expected[c["offset"]]=0xfc000000
                transfers.append({"caller":linkage,"caller_address":b,**c,"callee":{k:evidence[k] for k in ("source","name","address","size","sha256")}})
            require(masks==expected,"Boss goal kernel masks exceed reviewed typed operands and complete calls")
            boundary=closed(original,target,a,b,size);unique=unique_template(target,original.read(a,size),masks,b)
            identities.append({"source":source,"linkage_name":linkage,"source_address":a,"target_address":b,"size":size,
                               "uniqueness":unique,"terminal_alignment_bytes":padding})
            if b not in records:
                records[b]={"name":f["name"],"source":source,"address":b,"size":size,"sha256":digest(target.read(b,size)),
                    "boundary_confirmation":True,"confirmation_kind":CLUSTER_KIND,"provenance":[],"corroboration":{
                    "proof_scope":"complete_original_sandy_patrick_goal_and_behavior_kernels",
                    "local_control_flow":boundary,"whole_translation_unit_claimed":False}}
            records[b]["provenance"].append({"version":version,"executable_sha1":original.sha1,"source_address":a,
                "source":source,"name":f["name"],"linkage_name":linkage,"reference_sha256":digest(original.read(a,size)),
                "data_address_operands":pairs,"direct_transfers":calls})
        sequences.append({"version":version,"complete_bodies":identities,"whole_translation_unit_claimed":False})
        data_proofs.append({"version":version,"typed_player_model":field,"operands":operands})
        call_proofs.append({"version":version,"complete_callees":transfers})
    return {"functions":sorted(records.values(),key=lambda f:f["address"]),"sequence_proofs":sequences,
            "data_proofs":data_proofs,"call_proofs":call_proofs,
            "counts":{"functions":24,"code_bytes":7428,"source_units":2,"closed_return_bodies":24,
                      "reviewed_complete_caller_callee_clusters":24}}
