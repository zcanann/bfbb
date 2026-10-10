"""Complete original French NPC smoothing leaf and its typed local data."""
from __future__ import annotations

from platforms.verify_reviewed import REFERENCES,TARGET,require,original_gp,pair_address
from platforms.france_tu_sequences import compare,digest,unique_template,words
from platforms.france_hangable_sequence import hangable_pair
from platforms.france_player_animation_context import closed,storage
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages
from platforms.dwarf1 import iter_dies

SOURCE='SB/Game/zNPCSupport.cpp'
NAME='NPCC_GenSmooth'
LINKAGE='NPCC_GenSmooth__FPP5xVec3PP5xVec3'
START=0x31ac30
SIZE=508
TARGETS={'yews':0x5004e0,'prepute':0x5d54e0,'init':0x50fc90}
FLOAT_ARRAY=bytes.fromhex('000a0000000000030000000855000e00')


def gp_setup(original):
    # The same original startup sequence is independently used by the reviewed
    # data-anchor verifier. Its real address construction must agree with ELF.
    gp=original_gp(original)
    require(original.word(0x100148)>>16==0x3c04 and
            original.word(0x10015c)>>16==0x2484 and
            pair_address(original,0,{'high_offset':0x100148,'low_offset':0x10015c,'low_opcode':9})==gp and
            original.word(0x100170)==0x0080e025,
            'NPC smoothing original startup GP differs from reginfo')
    code=words(original.read(0x100148,44))
    require([w>>16 for w in code[:10]]==[0x3c04,0x3c05,0x3c06,0x3c07,0x3c08,
                                       0x2484,0x24a5,0x24c6,0x24e7,0x2508],
            'NPC smoothing GP setup lifetime contains a clobber or control edge')
    return {'gp':gp,'high_address':0x100148,'low_address':0x10015c,'move_address':0x100170,
            'complete_setup_words':code,'gp_input_preserved_until_move':True}


def local_data(original,target,function):
    sec=next(s for s in original.metadata['sections'] if s['name']=='.debug')
    rows=list(iter_dies(original.data[sec['offset']:sec['offset']+sec['size']]))
    by={off:(tag,owner,attrs) for off,tag,owner,attrs in rows}
    owners=[(off,attrs) for off,tag,owner,attrs in rows if tag==6 and
            owner.replace('\\','/').endswith(SOURCE) and attrs.get(3)==NAME and attrs.get(17)==function['low']]
    require(len(owners)==1 and owners[0][1].get(18)==function['high'],
            'NPC smoothing original lexical owner differs')
    low,owner=owners[0];declarations={}
    for off,tag,source,attrs in rows:
        name=attrs.get(3)
        if not low<off<owner[1] or name not in TARGETS:continue
        require(name not in declarations and tag==12 and source.replace('\\','/').endswith(SOURCE),
                'NPC smoothing local declaration owner differs')
        loc=attrs.get(2)
        require(isinstance(loc,bytes) and len(loc)==5 and loc[0]==3 and
                attrs.get(512)==name+'$'+{'prepute':'6721','yews':'6722','init':'6723'}[name],
                'NPC smoothing local lacks reviewed absolute linkage')
        a=int.from_bytes(loc[1:],'little');b=TARGETS[name]
        if name=='init':
            require(attrs.get(5)==8 and 7 not in attrs,'NPC smoothing init is not signed 32-bit')
            size=4;shape=[];kind='runtime_bss';descriptor=None
        else:
            typ,source_type,array=by.get(attrs.get(7),(None,'',{}));desc=array.get(10)
            require(typ==1 and source_type.replace('\\','/').endswith(SOURCE) and array.get(9)==0,
                    'NPC smoothing array owner/type differs')
            if name=='yews':
                require(desc==FLOAT_ARRAY,'NPC smoothing sample array type or count differs')
                size=16;shape=[4];kind='initialized_data'
            else:
                require(isinstance(desc,bytes) and len(desc)==18 and
                        desc[:14]==bytes.fromhex('000a000000000003000000087200'),
                        'NPC smoothing coefficient outer array differs')
                element=int.from_bytes(desc[14:],'little');etag,esource,inner=by.get(element,(None,'',{}))
                require(etag==1 and esource.replace('\\','/').endswith(SOURCE) and
                        inner.get(9)==0 and inner.get(10)==FLOAT_ARRAY,
                        'NPC smoothing coefficient row type or count differs')
                size=64;shape=[4,4];kind='runtime_bss'
            descriptor=desc.hex()
        for binary,address in ((original,a),(target,b)):storage(binary,address,size,kind)
        payload=None
        if name=='yews':
            payload=bytes.fromhex('0000803e0000003f0000403f0000803f')
            require(original.read(a,size)==payload and target.read(b,size)==payload,
                    'NPC smoothing complete sample payload differs')
        declarations[name]={'name':name,'reference_address':a,'target_address':b,'size':size,
                            'shape':shape,'storage':kind,'declaration_die':off,
                            'subscript_descriptor':descriptor,'payload':payload.hex() if payload else None,
                            'data_extent_promoted':False}
    require(set(declarations)==set(TARGETS),'NPC smoothing typed local is missing')
    return declarations


def generate_unit(originals,registry_dir):
    # No evolving registry dependencies: later identities cannot alter evidence.
    target=originals[TARGET];record=None;sequences=[];data_proofs=[]
    for version in REFERENCES:
        original=originals[version];links=canonical_linkages(original.data,original.metadata)
        refs=[f for f in original.functions if f['source']==SOURCE and f['name']==NAME and links.get(f['low'])==LINKAGE]
        require(len(refs)==1 and refs[0]['high']-refs[0]['low']==SIZE,'NPC smoothing original identity or extent differs')
        f=refs[0];a=f['low']
        require(a%16==START%16==0 and not any(original.read(a+SIZE,4)) and
                not any(target.read(START+SIZE,4)),'NPC smoothing entry or padding differs')
        objects=local_data(original,target,f)
        pairs,calls,masks=compare(original,target,a,START,SIZE,address_resolver=hangable_pair)
        require(not calls and [(p['hi_offset'],p['lo_offset'],p['opcode'],p['target_address']) for p in pairs]==
                [(12,28,9,TARGETS['yews']),(16,32,9,TARGETS['prepute']),(200,208,9,TARGETS['prepute'])],
                'NPC smoothing complete operand/call inventory differs')
        for p in pairs:
            name='yews' if p['lo_offset']==28 else 'prepute'
            require(p['reference_address']==objects[name]['reference_address'] and
                    p['storage']==('file_backed' if name=='yews' else 'zero_fill'),
                    'NPC smoothing operand changes typed object ownership')
        require(masks=={28:0xffff0000,32:0xffff0000,208:0xffff0000},'NPC smoothing unexpected masked fields')
        setups=[];uses=[]
        for binary,entry,base in ((original,a,objects['init']['reference_address']),(target,START,TARGETS['init'])):
            setup=gp_setup(binary);setups.append(setup)
            instructions=words(binary.read(entry,SIZE))
            gp_uses=[(i*4,w) for i,w in enumerate(instructions) if w>>21&31==28]
            require([off for off,w in gp_uses]==[0,24],'NPC smoothing GP use inventory differs')
            require(gp_uses[0][1]>>16==0x8f83 and gp_uses[1][1]>>16==0xaf83,
                    'NPC smoothing GP uses are not the reviewed LW/SW')
            for off,w in gp_uses:
                imm=(w&65535)-(65536 if w&32768 else 0)
                require(setup['gp']+imm==base,'NPC smoothing GP operand does not name its typed init')
            uses.append([{'offset':off,'word':w,'effective_address':base} for off,w in gp_uses])
        unique=unique_template(target,original.read(a,SIZE),masks,START)
        boundary=closed(original,target,a,START,SIZE)
        require(not boundary['direct_or_indirect_calls'] and not boundary['stack_adjustments'] and
                not boundary['ra_saves'] and not boundary['ra_loads'], 'NPC smoothing is not a closed call-free leaf')
        if record is None:
            record={'name':NAME,'source':SOURCE,'address':START,'size':SIZE,'sha256':digest(target.read(START,SIZE)),
                    'boundary_confirmation':True,'confirmation_kind':CLUSTER_KIND,'provenance':[],
                    'corroboration':{'proof_scope':'complete_original_smoothing_leaf_and_typed_locals',
                                    'whole_translation_unit_claimed':False,'local_control_flow':boundary}}
        record['provenance'].append({'version':version,'executable_sha1':original.sha1,'source_address':a,
                                     'source':SOURCE,'name':NAME,'linkage_name':LINKAGE,
                                     'reference_sha256':digest(original.read(a,SIZE)),
                                     'data_address_operands':pairs,'direct_transfers':[]})
        sequences.append({'version':version,'source_address':a,'target_address':START,'size':SIZE,
                          'uniqueness':unique,'terminal_alignment_bytes':4,'whole_translation_unit_claimed':False})
        data_proofs.append({'version':version,'complete_typed_locals':objects,'gp_setup':setups,
                            'gp_uses':uses,'gp_words_unmasked':True})
    return {'functions':[record],'sequence_proofs':sequences,'data_proofs':data_proofs,'call_proofs':[],
            'counts':{'functions':1,'code_bytes':SIZE,'source_units':1,'closed_return_bodies':1,
                      'reviewed_complete_caller_callee_clusters':1}}
