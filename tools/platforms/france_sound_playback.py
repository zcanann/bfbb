"""Original-only playback cluster with explicit, nonpromoted runtime contexts."""
from __future__ import annotations

import json
from platforms.verify_reviewed import REFERENCES,TARGET,require
from platforms.france_tu_sequences import compare,digest,unique_template,words,cstring
from platforms.france_hangable_sequence import hangable_pair
from platforms.france_player_animation_context import OriginalData,closed,storage
from platforms.france_sound_listeners import original_data as sound_data
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.france_corroborated import ControlFlow
from platforms.ps2_type_layouts import aggregate_layouts
from platforms.ps2_source import canonical_linkages

CORE="SB/Core/x/xSnd.cpp"
PLATFORM="SB/Core/p2/iSnd.cpp"
API="SB/Core/p2/his/HISAPI.cpp"
STUB="SB/Core/p2/his/HisStubs.cpp"
VECTOR="SB/Core/x/xVec3.cpp"
MEMBERS=[(CORE,"xSndPlayInternal",0x209d60,1376),
         (PLATFORM,"iSndLookup",0x1ba100,96),
         (PLATFORM,"iSndFindFreeVoice",0x1b8cc0,900),
         (PLATFORM,"iSndPlay",0x1b92a0,1120),
         (PLATFORM,"iSndCalcVol",0x1b9af0,776),
         (API,"HISFlushAsyncRequestsNoWait",0x34bb00,84),
         (STUB,"_HISBatchSoundCommandsNoWait",0x34b050,124),
         (VECTOR,"xVec3NormalizeFast",0x210c30,224),
         (VECTOR,"xVec3Normalize",0x210d10,224)]
INDEPENDENT={0x20aa60:(CORE,"xSndAddDelayed",128),
             0x20a530:(CORE,"xSndInternalUpdateVoicePos",284),
             0x20d8d0:("SB/Core/x/xstransvc.cpp","xSTAssetName",288),
             0x34bcb0:(API,"HISStopVoiceAsync",56),
             0x34bdf0:(API,"HISPlaySoundAsync",84),
             0x34bb90:(API,"HISPlayExternalStreamAsync",84),
             0x34bd70:(API,"HISPlayStreamAsync",116)}
OPAQUE={0x114b50,0x114b20,0x119e70,0x114690,0x114930,0x1073e8,0x1071f8}
CALLS={0x209d60:[(264,0x20aa60),(604,0x1ba100),(648,0x1ba100),(752,0x1b8cc0),(1256,0x20a530),(1264,0x1b92a0)],
       0x1ba100:[],0x1b8cc0:[(188,0x34bcb0),(504,0x114b50),(516,0x114b50),(656,0x20d8d0),(724,0x34bcb0)],
       0x1b92a0:[(44,0x1b9af0),(128,0x114b20),(344,0x20d8d0),(468,0x119e70),(516,0x34bdf0),
                 (592,0x34bb90),(604,0x34bb00),(624,0x20d8d0),(748,0x119e70),(900,0x34bd70),(1040,0x34bd70)],
       0x1b9af0:[(156,0x210d10),(296,0x114690),(316,0x114930)],0x34bb00:[(20,0x34b050)],
       0x34b050:[(36,0x1073e8),(96,0x1071f8)],0x210c30:[],0x210d10:[]}


def small_runtime_context(original,target):
    """Fixed 20-byte closed body; the next routine is outside this witness."""
    address=0x114b50
    expected=(0x04810002,0x0080102d,0x00021023,0x03e00008,0)
    for binary in (original,target):
        require(words(binary.read(address,20))==list(expected) and binary.read(address+20,4)==bytes(4),
                "Scoped opaque arithmetic context or zero padding differs")
        require(address not in binary.by_address,"Opaque context unexpectedly has a debug identity")
    flows=[]
    for binary in (original,target):
        bounds=ControlFlow({address+4*i:w for i,w in enumerate(words(binary.read(address,36)))}).bounds(address,20)
        # This fixed SDK context is followed by four zero bytes and the next
        # eight-byte-aligned routine. Game-function promotion still requires
        # the generic sixteen-byte alignment rule, unchanged.
        require(bounds["reasons"]==["following_alignment_not_zero"] and
                bounds["returns"]==[address+12] and bounds["branches"]==[[address,address+12]] and
                not bounds["stack_adjustments"] and not bounds["ra_saves"] and not bounds["ra_loads"] and
                not bounds["direct_or_indirect_calls"] and not bounds["unreachable_zero_words"],
                "Opaque arithmetic context is not the exact closed branch/return leaf")
        flows.append(bounds)
    hits=[]
    needle=target.read(address,20)
    for segment in target.loaded:
        data=target.read(segment["address"],segment["file_size"])
        pos=data.find(needle)
        while pos>=0:
            if pos%4==0:hits.append(segment["address"]+pos)
            pos=data.find(needle,pos+1)
    require(hits==[address],"Scoped opaque arithmetic context is not unique")
    return {"address":address,"context_bytes":20,"sha256":digest(needle),"zero_padding_bytes":4,
            "local_control_flow":{"passes":True,"scope":"exact_five_word_leaf_with_four_zero_padding_bytes",
                                  "generic_sixteen_byte_alignment_diagnostics":flows},
            "matching_addresses":hits,"all_words_literal":True,
            "scope":"complete_exact_closed_opaque_context","no_identity_or_extent_claim":True,
            "promoted_as_named_anchor":False}


def playback_data(original):
    snd,_=sound_data(original)
    data=OriginalData(original)
    globals_fields={m["name"]:m for m in snd["layouts"]["xSndGlobals"]["members"]}
    for n,off in (("stereo",0),("SndCount",4)):
        require(globals_fields[n]["offset"]==off and globals_fields[n]["type_attributes"]=={"5":9},
                "Original playback unsigned global differs")
    snd["paths"].update({"0":{"path":["gSnd_base_or_stereo"],"opcodes":[9,35]},
                          "4":{"path":["SndCount"],"opcodes":[35,43]}})
    category=globals_fields["categoryVolFader"]
    tag,_,a=data.by[category["type_attributes"]["7"]]
    require(category["offset"]==8 and tag==1 and a.get(9)==0 and
            a.get(10)==bytes.fromhex("000a0000000000040000000855000e00"),
            "Original category volume array differs")
    snd["paths"]["8"]={"path":["categoryVolFader"],"opcodes":[9]}
    vf={m["name"]:m for m in snd["layouts"]["xSndVoiceInfo"]["members"]}
    for n,off in (("sndID",4),("flags",20)):
        require(vf[n]["offset"]==off and vf[n]["type_attributes"]=={"5":9},
                "Original voice playback field differs")
        snd["paths"][str(28+off)]={"path":["voice",0,n],"opcodes":[9]}
    objects=[snd]
    for source,name,count,size,element,descriptor in (
            (PLATFORM,"eeFiles",512,14336,"iSndFileInfo",None),
            (API,"asyncRequestBuffers",4096,4096,None,"000a0000000000ff0f00000855000100"),
            (STUB,"clientData",None,40,"_sif_client_data",None)):
        off,a,base=data.declaration(source,name)
        die=a[7];tag,_,typ=data.by[die]
        obj={"name":name,"source":source,"reference_address":base,"size":size,"declaration_die":off,"type_die":die}
        if count:
            require(tag==1 and typ.get(9)==0,"Playback storage is not an original array")
            desc=typ.get(10)
            if element:
                layouts=aggregate_layouts(data.debug,source,{element})
                require(layouts[element]["size"]==28,"Original sound file record stride differs")
                expected=bytes.fromhex("000a0000000000")+(count-1).to_bytes(4,"little")+bytes.fromhex("087200")+layouts[element]["die_offset"].to_bytes(4,"little")
                obj["element_layout"]=layouts[element]
            else:expected=bytes.fromhex(descriptor)
            require(desc==expected,"Original playback array bound or type differs")
            obj.update(count=count,descriptor=desc.hex())
        else:
            require(tag==2 and typ.get(3)==element and typ.get(11)==size,"Original RPC client object differs")
            obj["layout"]=aggregate_layouts(data.debug,source,{element})[element]
        objects.append(obj)
    return objects


def flush_uniqueness(original,target,a,b,masks):
    """The complete 84-byte flush has a checked 28-byte exact final run."""
    require(b==0x34bb00 and masks=={20:0xfc000000,36:0xffff0000,52:0xffff0000},
            "Flush helper uniqueness masks differ")
    body=original.read(a,84);code=words(body);hits=[]
    for segment in target.loaded:
        data=target.read(segment["address"],segment["file_size"])
        pos=data.find(body[56:84])
        while pos>=0:
            begin=pos-56
            if begin>=0 and begin%4==0 and begin+84<=len(data):
                other=words(data[begin:begin+84])
                if all((x&masks.get(i*4,0xffffffff))==(y&masks.get(i*4,0xffffffff))
                       for i,(x,y) in enumerate(zip(code,other))):hits.append(segment["address"]+begin)
            pos=data.find(body[56:84],pos+1)
    require(hits==[b],"Complete flush helper is not uniquely located")
    return {"matching_addresses":hits,"unchanged_anchor_offset":56,"unchanged_anchor_bytes":28,
            "masked_instruction_count":3,"scope":"complete_84_byte_flush_with_exact_three_operand_inventory"}


def generate_unit(originals,registry_dir):
    target=originals[TARGET]
    known={}
    # Explicit dependency fence: later SDK and playback recoveries cannot alter
    # these seven independent identities or the literal opaque witness scope.
    for path in sorted(registry_dir.glob("*functions.json")):
        for f in json.loads(path.read_text())["functions"]:
            if f["address"] in INDEPENDENT:
                prior=known.setdefault(f["address"],f)
                require(all(prior[k]==f[k] for k in ("source","name","size","sha256")),"Conflicting playback dependency")
    require(set(known)==set(INDEPENDENT),"Independent sound listener or HIS dependencies missing")
    for address,(source,name,size) in INDEPENDENT.items():
        f=known[address]
        require((f["source"],f["name"])==(source,name) and (not size or f["size"]==size),"Playback dependency identity differs")
    records,sequences,data_proofs={ },[],[]
    for version in REFERENCES:
        original=originals[version]
        links=canonical_linkages(original.data,original.metadata)
        objects=playback_data(original)
        functions={}
        for source,name,b,size in MEMBERS:
            refs=[f for f in original.functions if (f["source"],f["name"],f["high"]-f["low"])==(source,name,size)]
            require(len(refs)==1 and refs[0]["low"] in links,"Complete playback original identity differs")
            a=refs[0]["low"]
            require(a%16==b%16==0,"Playback original entry alignment differs")
            pairs,calls,masks=compare(original,target,a,b,size,address_resolver=hangable_pair)
            require([(c["offset"],c["target_address"]) for c in calls]==CALLS[b] and all(c["opcode"]==3 for c in calls),
                    "Complete playback call inventory differs")
            functions[b]=dict(source=source,name=name,a=a,b=b,size=size,pairs=pairs,calls=calls,masks=masks,
                              bounds=closed(original,target,a,b,size))
        fields,strings,contexts,unique=[],[],[],[]
        mapping,bases={},{}
        arithmetic=small_runtime_context(original,target)
        vectors=sorted((f for f in original.functions if f["source"]==VECTOR),key=lambda f:f["low"])
        fast,normal=functions[0x210c30],functions[0x210d10]
        require([(f["name"],f["low"],f["high"]-f["low"]) for f in vectors]==
                [("xVec3NormalizeFast",fast["a"],224),("xVec3Normalize",normal["a"],224)] and
                normal["a"]==fast["a"]+224 and not fast["masks"] and not normal["masks"],
                "Complete original vector pair order or literal bodies differ")
        vector_unique=unique_template(target,original.read(fast["a"],448),{},fast["b"])
        for b,f in functions.items():
            for p in f["pairs"]:
                ra,tb=p["reference_address"],p["target_address"]
                require(mapping.setdefault(ra,tb)==tb,"Playback address mapping differs")
                if p["storage"]=="file_backed":
                    require(b==0x1b92a0 and p["opcode"]==9,"Unreviewed playback file-backed operand")
                    literal=cstring(original,ra)
                    require(all(x in (0,9,10,13) or 32<=x<=126 for x in literal) and
                            literal==cstring(target,tb),"Complete playback diagnostic literal differs")
                    strings.append({**p,"size":len(literal),"sha256":digest(literal),"text":literal[:-1].decode("ascii")})
                    continue
                candidates=[obj for obj in objects if obj["reference_address"]<=ra<obj["reference_address"]+obj["size"]]
                require(len(candidates)==1 and p["storage"]=="zero_fill","Unreviewed playback object operand")
                obj=candidates[0];offset=ra-obj["reference_address"]
                if obj["name"]=="gSnd":
                    path=obj["paths"].get(str(offset))
                    require(path is not None and p["opcode"] in path["opcodes"],"Unreviewed playback sound member")
                    path=path["path"]
                else:
                    offsets={"eeFiles":(0,),"asyncRequestBuffers":(0,2048),"clientData":(0,)}[obj["name"]]
                    require(offset in offsets and p["opcode"]==9,"Playback array/client address path differs")
                    path=[obj["name"],offset]
                base=tb-offset
                require(bases.setdefault(obj["name"],base)==base,"Playback object base is inconsistent")
                for binary,address in ((original,obj["reference_address"]),(target,base)):
                    storage(binary,address,obj["size"],"runtime_bss")
                fields.append({**p,"caller":f["name"],"object":obj["name"],"path":path,"target_base":base})
            for c in f["calls"]:
                ra,tb=c["reference_address"],c["target_address"]
                if tb in functions:
                    require(ra==functions[tb]["a"],"Playback internal transfer loses complete original identity")
                elif tb in known:
                    checked_identity(original,target,ra,tb,known)
                else:
                    require(tb in OPAQUE and ra==tb and ra not in original.by_address and
                            original.word(f["a"]+c["offset"])==target.word(b+c["offset"]),
                            "Opaque playback caller transfer is not literal")
                    if tb==0x114b50:
                        context=arithmetic
                    else:
                        require(original.read(ra,64)==target.read(tb,64),"Opaque playback 64-byte context differs")
                        context={"address":tb,"context_bytes":64,"sha256":digest(target.read(tb,64)),
                                 "scope":"unchanged_opaque_prefix","no_identity_or_extent_claim":True}
                    f["masks"].pop(c["offset"])
                    c.update(transfer_word_unmasked=True,no_identity_or_extent_claim=True,
                             opaque_context_bytes=context["context_bytes"],opaque_context_sha256=context["sha256"])
                    contexts.append({"caller":f["name"],**c,"context":context,"promoted_as_named_anchor":False})
            if f["source"]==VECTOR:
                u={**vector_unique,"scope":"complete_two_member_literal_vector_sequence",
                   "member_offset":b-fast["b"],"sequence_bytes":448}
            elif b==0x34bb00:
                u=flush_uniqueness(original,target,f["a"],b,f["masks"])
            else:
                u=unique_template(target,original.read(f["a"],f["size"]),f["masks"],b)
            unique.append({"source":f["source"],"name":f["name"],"address":b,"size":f["size"],"uniqueness":u})
            if b not in records:
                records[b]={"name":f["name"],"source":f["source"],"address":b,"size":f["size"],
                    "sha256":digest(target.read(b,f["size"])),"boundary_confirmation":True,
                    "confirmation_kind":CLUSTER_KIND,"provenance":[],"corroboration":{
                    "proof_scope":"complete_sound_playback_cluster_with_typed_data_and_closed_opaque_context",
                    "local_control_flow":f["bounds"],"whole_translation_unit_claimed":False}}
            require(not records[b]["provenance"] or records[b]["provenance"][0]["linkage_name"]==links[f["a"]],
                    "Playback linkage differs across originals")
            records[b]["provenance"].append({"version":version,"executable_sha1":original.sha1,
                "source_address":f["a"],"source":f["source"],"name":f["name"],"linkage_name":links[f["a"]],
                "reference_sha256":digest(original.read(f["a"],f["size"])),"data_address_operands":f["pairs"],"direct_transfers":f["calls"]})
        require(len(fields)==36 and len(strings)==2 and len(contexts)==9 and len(mapping)==len(set(mapping.values())),
                f"Playback operand inventory differs: {len(fields)}/{len(strings)}/{len(contexts)}")
        require(set(bases)=={obj["name"] for obj in objects},"Playback object inventory incomplete")
        spans=sorted((bases[obj["name"]],bases[obj["name"]]+obj["size"]) for obj in objects)
        require(all(a[1]<=b[0] for a,b in zip(spans,spans[1:])),"Distinct playback objects overlap")
        sequences.append({"version":version,"complete_bodies":unique,"whole_translation_units_claimed":False})
        data_proofs.append({"version":version,"typed_objects":objects,"typed_fields":fields,
                            "complete_strings":strings,"opaque_runtime_contexts":contexts,"data_extents_promoted":False})
    return {"functions":sorted(records.values(),key=lambda f:f["address"]),"sequence_proofs":sequences,
            "data_proofs":data_proofs,"counts":{"functions":9,"code_bytes":4924,"source_units":5,
            "closed_return_bodies":9,"reviewed_complete_caller_callee_clusters":1}}
