"""Original French hazard splash callers and their complete perpendicular tail helper."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES,TARGET,require
from platforms.france_tu_sequences import compare,digest,unique_template,words
from platforms.france_corroborated import ControlFlow
from platforms.france_hangable_sequence import hangable_pair
from platforms.france_player_animation_context import closed
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages

MEMBERS=[('SB/Game/zNPCSupport.cpp', 'NPCC_MakePerp__FP5xVec3PC5xVec3', 3256224, 56),
 ('SB/Game/zNPCHazard.cpp', 'WavesOfEvil__9NPCHazardFv', 3949920, 908),
 ('SB/Game/zNPCHazard.cpp', 'WaterSplash__9NPCHazardFPC5xVec3', 3950832, 2172)]
CALLS={3256224: [(48, 2, 2166032)],
 3949920: [(88, 3, 2024672), (240, 3, 2024672), (832, 3, 3885440)],
 3950832: [(104, 3, 3256224),
           (304, 3, 2024672),
           (448, 3, 2024672),
           (496, 3, 2024768),
           (732, 3, 2024672),
           (780, 3, 2024768),
           (1164, 3, 3885888),
           (1200, 3, 2024672),
           (1324, 3, 2024672),
           (1376, 3, 2024768),
           (1612, 3, 2024672),
           (1664, 3, 2024768),
           (2044, 3, 3885664)]}
INDEPENDENT={2024672: ('SB/Core/x/xMath.cpp', 'xurand', 88),
 2024768: ('SB/Core/x/xMath.cpp', 'xrand', 32),
 2166032: ('SB/Core/x/xVec3.cpp', 'xVec3Normalize', 224),
 3256016: ('SB/Game/zNPCSupport.cpp', 'NPCC_MakeArbPlane', 196),
 3885440: ('SB/Game/zNPCSupplement.cpp', 'NPAR_EmitH2OSpray', 216),
 3885664: ('SB/Game/zNPCSupplement.cpp', 'NPAR_EmitH2ODrops', 216),
 3885888: ('SB/Game/zNPCSupplement.cpp', 'NPAR_EmitH2ODrips', 216)}


def perp_tail(binary,address,callee):
    body=words(binary.read(address,56))
    # Complete literal arithmetic and argument setup. LWC1/SUB.S/SWC1 touch
    # only FPRs/memory; DADDU a1,a0,zero forwards the destination as source.
    # This exact inventory preserves SP/RA and introduces no generic decoder.
    require(body[:12]==[0xc4a10004,0xc4a00008,0x46000801,0xe4800000,
                        0xc4a10008,0xc4a00000,0x46000801,0xe4800004,
                        0xc4a10000,0xc4a00004,0x46000801,0x0080282d] and
            body[12]==0x08000000|(callee>>2) and body[13]==0xe4800008,
            'Perpendicular helper literal arithmetic/tail inventory differs')
    flow=ControlFlow({address+i*4:w for i,w in enumerate(words(binary.read(address,64)))})
    bounds=flow.bounds(address,56)
    require(bounds['reasons']==['extent_not_terminal_return_delay','local_edge_outside_extent','no_return'] and
            not any(bounds[k] for k in ('returns','direct_or_indirect_calls','branches','stack_adjustments',
                                       'ra_saves','ra_loads','unreachable_zero_words')) and flow.valid_delay(address+48),
            'Perpendicular helper is not its complete frame-free tail')
    callee_flow=ControlFlow({callee+i*4:w for i,w in enumerate(words(binary.read(callee,224)))})
    require(callee_flow.bounds(callee,224)['passes'],'Perpendicular normalization callee is not closed')
    return {**bounds,'passes':True,'reasons':[],'boundary_kind':'reviewed-complete-perpendicular-leaf-tail',
            'tail_target_address':callee,'tail_target_size':224,'terminal_instruction_offset':48}


def perp_cluster(original,target,address,known):
    a=address-208;b=0x31aed0
    anchor=checked_identity(original,target,a,b,known)
    require((anchor['source'],anchor['name'],anchor['size'])==('SB/Game/zNPCSupport.cpp','NPCC_MakeArbPlane',196),
            'Perpendicular helper preceding anchor differs')
    members=sorted((f['low'],f['high']-f['low'],f['name'],f['source']) for f in original.functions if a<=f['low']<a+264)
    require(members==[(a,196,'NPCC_MakeArbPlane','SB/Game/zNPCSupport.cpp'),
                     (address,56,'NPCC_MakePerp','SB/Game/zNPCSupport.cpp')],
            'Perpendicular complete original cluster membership/order differs')
    require(not any(original.read(a+196,12)) and not any(target.read(b+196,12)),
            'Perpendicular anchor inter-body padding differs')
    pairs,calls,masks=compare(original,target,a,b,196,address_resolver=hangable_pair)
    require(not pairs and len(calls)==1 and calls[0]['offset']==80 and calls[0]['opcode']==3 and
            calls[0]['target_address']==0x210d10 and masks=={80:0xfc000000},
            'Perpendicular anchor call/data inventory differs')
    checked_identity(original,target,calls[0]['reference_address'],0x210d10,known)
    closed(original,target,a,b,196)
    unique=unique_template(target,original.read(a,264),{80:0xfc000000,256:0xfc000000},b)
    return {'source_address':a,'target_address':b,'size':264,'member_sizes':[196,56],
            'known_anchor':{k:anchor[k] for k in ('source','name','address','size','sha256')},
            'uniqueness':unique,'terminal_alignment_bytes':8}


def generate_unit(originals,registry_dir):
    target=originals[TARGET];known={}
    for path in sorted(registry_dir.glob('*functions.json')):
        for f in json.loads(path.read_text())['functions']:
            if f['address'] in INDEPENDENT:
                old=known.setdefault(f['address'],f)
                require(all(old[k]==f[k] for k in ('source','name','size','sha256')),'Conflicting splash dependency')
    require(set(known)==set(INDEPENDENT),'Independent splash dependency missing')
    for address,identity in INDEPENDENT.items():
        require(tuple(known[address][k] for k in ('source','name','size'))==identity,'Splash dependency identity differs')
    records={};sequences=[];call_proofs=[]
    for version in REFERENCES:
        original=originals[version];links=canonical_linkages(original.data,original.metadata)
        identities=[];transfers=[]
        for source,linkage,b,size in MEMBERS:
            refs=[f for f in original.functions if f['source']==source and links.get(f['low'])==linkage]
            require(len(refs)==1 and refs[0]['name']==linkage.split('__')[0] and refs[0]['high']-refs[0]['low']==size,
                    'Complete splash original identity differs')
            f=refs[0];a=f['low'];require(a%16==b%16==0,'Splash entry alignment differs')
            padding=(-size)%16
            require(not any(original.read(a+size,padding)) and not any(target.read(b+size,padding)), 'Splash zero padding differs')
            pairs,calls,masks=compare(original,target,a,b,size,address_resolver=hangable_pair)
            require(not pairs and [(c['offset'],c['opcode'],c['target_address']) for c in calls]==CALLS[b] and
                    masks=={c['offset']:0xfc000000 for c in calls},'Splash complete literal/call inventory differs')
            for c in calls:
                tb=c['target_address']
                evidence=checked_identity(original,target,c['reference_address'],tb,{**known,**records})
                transfers.append({'caller':linkage,'caller_address':b,**c,
                                  'callee':{k:evidence[k] for k in ('source','name','address','size','sha256')}})
            if b==0x31afa0:
                perp_tail(original,a,calls[0]['reference_address']);boundary=perp_tail(target,b,0x210d10)
                unique=perp_cluster(original,target,a,known)
            else:
                boundary=closed(original,target,a,b,size)
                unique=unique_template(target,original.read(a,size),masks,b)
            identities.append({'source':source,'linkage_name':linkage,'source_address':a,'target_address':b,
                               'size':size,'placement_evidence':unique,'terminal_alignment_bytes':padding})
            if b not in records:
                records[b]={'name':f['name'],'source':source,'address':b,'size':size,'sha256':digest(target.read(b,size)),
                            'boundary_confirmation':True,'confirmation_kind':CLUSTER_KIND,'provenance':[],
                            'corroboration':{'proof_scope':'complete_original_hazard_splashes_and_perpendicular_helper',
                                            'local_control_flow':boundary,'whole_translation_unit_claimed':False}}
            records[b]['provenance'].append({'version':version,'executable_sha1':original.sha1,'source_address':a,
                                            'source':source,'name':f['name'],'linkage_name':linkage,
                                            'reference_sha256':digest(original.read(a,size)),
                                            'data_address_operands':pairs,'direct_transfers':calls})
        sequences.append({'version':version,'complete_bodies':identities,'whole_translation_unit_claimed':False})
        call_proofs.append({'version':version,'complete_callees':transfers,'changed_data_operands':0})
    return {'functions':sorted(records.values(),key=lambda f:f['address']),'sequence_proofs':sequences,
            'data_proofs':[],'call_proofs':call_proofs,
            'counts':{'functions':3,'code_bytes':3136,'source_units':2,'closed_return_bodies':2,
                      'reviewed_complete_tail_bodies':1,'reviewed_complete_caller_callee_clusters':1}}
