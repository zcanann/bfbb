"""Complete original French Sandy Leap Enter with its reviewed common-goal tail."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES,TARGET,require
from platforms.france_tu_sequences import compare,digest,unique_template,words,gpr_writes
from platforms.france_corroborated import ControlFlow
from platforms.france_hangable_sequence import hangable_pair
from platforms.france_boss_goal_kernels import model_field
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages

SOURCE='SB/Game/zNPCTypeBossSandy.cpp'
LINKAGE='Enter__21zNPCGoalBossSandyLeapFfPv'
START=0x32ea40
SIZE=484
CALLEE=0x2cb490
INDEPENDENT={0x2c70a0:('SB/Game/zNPCGoalStd.cpp','CalcNewDir',624),
             CALLEE:('SB/Game/zNPCGoalCommon.cpp','Enter',216)}


def leap_gpr_writes(word):
    # Reviewed EE scalar COP1 forms actually present in this leaf. Arithmetic
    # writes only FPRs, MULA.S the accumulator, and C.LT.S/C.LE.S the condition flag.
    # EE SQRT.S takes ft; its fs field is reserved zero. Compare fd is zero.
    if word >> 26 == 17 and word >> 21 & 31 == 16:
        fn=word & 63
        if fn in (0,1,2,3,28):return set()
        if fn==4 and word >> 11 & 31 == 0:return set()
        if fn in (26,52,54) and word >> 6 & 31 == 0:return set()
    return gpr_writes(word)


def tail_bounds(binary,address,callee):
    flow=ControlFlow({address+i*4:w for i,w in enumerate(words(binary.read(address,SIZE+12)))})
    bounds=flow.bounds(address,SIZE)
    require(bounds['reasons']==['extent_not_terminal_return_delay','local_edge_outside_extent','no_return'],
            'Sandy Leap has an unreviewed boundary failure')
    require(not any(bounds[k] for k in ('returns','direct_or_indirect_calls','stack_adjustments',
                'ra_saves','ra_loads','unreachable_zero_words')), 'Sandy Leap is not a covered frame-free leaf')
    terminal=flow.instruction(address+SIZE-8)
    require(terminal['kind']=='jump' and terminal['target']==callee and flow.valid_delay(address+SIZE-8),
            'Sandy Leap tail does not reach its complete common-goal callee')
    for pc in range(address,address+SIZE,4):
        require(not leap_gpr_writes(flow.words[pc]) & {29,31},'Sandy Leap clobbers SP or RA')
        inst=flow.instruction(pc)
        if inst['kind']=='jump' and not address<=inst['target']<address+SIZE:
            require(pc==address+SIZE-8,'Another transfer leaves Sandy Leap')
    callee_flow=ControlFlow({callee+i*4:w for i,w in enumerate(words(binary.read(callee,224)))})
    require(callee_flow.bounds(callee,216)['passes'],'Sandy Leap common-goal callee is not closed')
    return {**bounds,'passes':True,'reasons':[],'boundary_kind':'reviewed-sandy-leap-frame-free-tail',
            'tail_target_address':callee,'tail_target_size':216,'terminal_instruction_offset':476}


def generate_unit(originals,registry_dir):
    target=originals[TARGET];known={}
    for path in sorted(registry_dir.glob('*functions.json')):
        for f in json.loads(path.read_text())['functions']:
            if f['address'] in INDEPENDENT:
                old=known.setdefault(f['address'],f)
                require(all(old[k]==f[k] for k in ('source','name','size','sha256')),'Conflicting Sandy Leap dependency')
    require(set(known)==set(INDEPENDENT),'Independent Sandy Leap dependency missing')
    for address,identity in INDEPENDENT.items():
        require(tuple(known[address][k] for k in ('source','name','size'))==identity,'Sandy Leap dependency identity differs')
    record=None;sequences=[];data_proofs=[];call_proofs=[]
    for version in REFERENCES:
        original=originals[version];links=canonical_linkages(original.data,original.metadata)
        refs=[f for f in original.functions if f['source']==SOURCE and links.get(f['low'])==LINKAGE]
        require(len(refs)==1 and refs[0]['high']-refs[0]['low']==SIZE,'Complete Sandy Leap original identity differs')
        f=refs[0];a=f['low'];require(a%16==0,'Sandy Leap entry alignment differs')
        require(not any(original.read(a+SIZE,12)) and not any(target.read(START+SIZE,12)),'Sandy Leap padding differs')
        field=model_field(original,target,known)
        pairs,calls,masks=compare(original,target,a,START,SIZE,address_resolver=hangable_pair)
        require([(p['hi_offset'],p['lo_offset'],p['target_address']) for p in pairs]==
                [(12,60,0x52cc14),(68,80,0x52cc14)],'Sandy Leap data inventory differs')
        expected={}
        for p in pairs:
            require(p['target_address']==field['target_address'] and p['reference_address']==field['reference_address'] and
                    p['opcode']==35 and p['storage']=='zero_fill','Sandy Leap operand lost its typed model path')
            expected[p['lo_offset']]=0xffff0000
            if original.read(a+p['hi_offset'],4)!=target.read(START+p['hi_offset'],4):expected[p['hi_offset']]=0xffff0000
        require(len(calls)==1 and (calls[0]['offset'],calls[0]['opcode'],calls[0]['target_address'])==(476,2,CALLEE),
                'Sandy Leap complete tail inventory differs')
        call=calls[0];evidence=checked_identity(original,target,call['reference_address'],CALLEE,known)
        expected[476]=0xfc000000
        require(masks==expected,'Sandy Leap masks exceed typed operands and complete tail')
        tail_bounds(original,a,call['reference_address']);boundary=tail_bounds(target,START,CALLEE)
        unique=unique_template(target,original.read(a,SIZE),masks,START)
        sequences.append({'version':version,'source':SOURCE,'linkage_name':LINKAGE,'source_address':a,
                          'target_address':START,'size':SIZE,'uniqueness':unique,'terminal_alignment_bytes':12,
                          'whole_translation_unit_claimed':False})
        data_proofs.append({'version':version,'typed_model_field':field,'operands':pairs})
        call_proofs.append({'version':version,'tail':call,
                           'complete_callee':{k:evidence[k] for k in ('source','name','address','size','sha256')}})
        if record is None:
            record={'name':f['name'],'source':SOURCE,'address':START,'size':SIZE,'sha256':digest(target.read(START,SIZE)),
                    'boundary_confirmation':True,'confirmation_kind':CLUSTER_KIND,'provenance':[],
                    'corroboration':{'proof_scope':'complete_original_sandy_leap_tail',
                                    'local_control_flow':boundary,'whole_translation_unit_claimed':False}}
        record['provenance'].append({'version':version,'executable_sha1':original.sha1,'source_address':a,
                                    'source':SOURCE,'name':f['name'],'linkage_name':LINKAGE,
                                    'reference_sha256':digest(original.read(a,SIZE)),
                                    'data_address_operands':pairs,'direct_transfers':calls})
    return {'functions':[record],'sequence_proofs':sequences,'data_proofs':data_proofs,'call_proofs':call_proofs,
            'counts':{'functions':1,'code_bytes':SIZE,'source_units':1,'reviewed_complete_caller_callee_clusters':1,
                      'reviewed_complete_tail_bodies':1}}
