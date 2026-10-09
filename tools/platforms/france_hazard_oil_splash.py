"""Complete French OilSplash distinguished by independently bound original callees."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES,TARGET,require
from platforms.france_tu_sequences import compare,digest,unique_template
from platforms.france_hangable_sequence import hangable_pair
from platforms.france_player_animation_context import closed
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages

SOURCE='SB/Game/zNPCHazard.cpp'
LINKAGE='OilSplash__9NPCHazardFPC5xVec3'
START=0x3c2910
SIZE=1084
CALLS=[(88,0x31aed0),(256,0x1ee4e0),(320,0x1ee540),
       (556,0x1ee4e0),(620,0x1ee540),(996,0x3b52c0)]
INDEPENDENT={0x31aed0:('SB/Game/zNPCSupport.cpp','NPCC_MakeArbPlane',196),
             0x1ee4e0:('SB/Core/x/xMath.cpp','xurand',88),
             0x1ee540:('SB/Core/x/xMath.cpp','xrand',32),
             0x3b52c0:('SB/Game/zNPCSupplement.cpp','NPAR_EmitOilSplash',216)}


def bound_template(original,target,address,known):
    pairs,calls,masks=compare(original,target,address,START,SIZE,address_resolver=hangable_pair)
    require(not pairs and [(c['offset'],c['target_address']) for c in calls]==CALLS and
            all(c['opcode']==3 for c in calls) and masks=={off:0xfc000000 for off,_ in CALLS},
            'OilSplash complete instruction/call inventory differs')
    template=bytearray(original.read(address,SIZE));bindings=[]
    for call in calls:
        off=call['offset'];evidence=checked_identity(original,target,call['reference_address'],call['target_address'],known)
        require(evidence['address']>>28==(START+off+4)>>28 and evidence['address']%4==0,
                'OilSplash bound callee does not fit its direct JAL region')
        original_word=int.from_bytes(template[off:off+4],'little')
        word=(original_word&0xfc000000)|((evidence['address']>>2)&0x03ffffff)
        template[off:off+4]=word.to_bytes(4,'little')
        bindings.append({**call,'reference_instruction':original_word,'bound_instruction':word,
                         'callee':{k:evidence[k] for k in ('source','name','address','size','sha256')}})
    # This byte string is built only from the complete named original and its
    # independently authenticated callee addresses. No target bytes are copied.
    require(bytes(template)==target.read(START,SIZE),'OilSplash independently bound template differs')
    unique=unique_template(target,bytes(template),{},START)
    return calls,bindings,{'method':'complete-original-template-with-independently-bound-direct-callees',
                          'template_sha256':digest(template),'direct_call_offsets':[o for o,_ in CALLS],
                          'search_masks':{},'uniqueness':unique}


def generate_unit(originals,registry_dir):
    target=originals[TARGET];known={}
    for path in sorted(registry_dir.glob('*functions.json')):
        for f in json.loads(path.read_text())['functions']:
            if f['address'] in INDEPENDENT:
                old=known.setdefault(f['address'],f)
                require(all(old[k]==f[k] for k in ('source','name','size','sha256')),'Conflicting OilSplash dependency')
    require(set(known)==set(INDEPENDENT),'Independent OilSplash dependency missing')
    for address,identity in INDEPENDENT.items():
        require(tuple(known[address][k] for k in ('source','name','size'))==identity,'OilSplash dependency identity differs')
    record=None;sequences=[];call_proofs=[]
    for version in REFERENCES:
        original=originals[version];links=canonical_linkages(original.data,original.metadata)
        refs=[f for f in original.functions if f['source']==SOURCE and links.get(f['low'])==LINKAGE]
        require(len(refs)==1 and refs[0]['name']=='OilSplash' and refs[0]['high']-refs[0]['low']==SIZE,
                'Complete OilSplash original identity differs')
        f=refs[0];a=f['low'];require(a%16==0,'OilSplash entry alignment differs')
        require(not any(original.read(a+SIZE,4)) and not any(target.read(START+SIZE,4)),'OilSplash zero padding differs')
        calls,bindings,placement=bound_template(original,target,a,known)
        boundary=closed(original,target,a,START,SIZE)
        sequences.append({'version':version,'source':SOURCE,'linkage_name':LINKAGE,'source_address':a,
                          'target_address':START,'size':SIZE,'placement_evidence':placement,
                          'terminal_alignment_bytes':4,'whole_translation_unit_claimed':False})
        call_proofs.append({'version':version,'independently_bound_complete_callees':bindings,'changed_data_operands':0})
        if record is None:
            record={'name':f['name'],'source':SOURCE,'address':START,'size':SIZE,'sha256':digest(target.read(START,SIZE)),
                    'boundary_confirmation':True,'confirmation_kind':CLUSTER_KIND,'provenance':[],
                    'corroboration':{'proof_scope':'complete_original_oil_splash_with_independent_callee_bindings',
                                    'local_control_flow':boundary,'whole_translation_unit_claimed':False}}
        record['provenance'].append({'version':version,'executable_sha1':original.sha1,'source_address':a,
                                    'source':SOURCE,'name':f['name'],'linkage_name':LINKAGE,
                                    'reference_sha256':digest(original.read(a,SIZE)),
                                    'data_address_operands':[],'direct_transfers':calls})
    return {'functions':[record],'sequence_proofs':sequences,'data_proofs':[],'call_proofs':call_proofs,
            'counts':{'functions':1,'code_bytes':SIZE,'source_units':1,'closed_return_bodies':1,
                      'reviewed_complete_caller_callee_clusters':1}}
