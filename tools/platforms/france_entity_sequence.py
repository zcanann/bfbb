"""Original-only French entity sequence, callback and typed-array proof."""
from __future__ import annotations
from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow, rooted_graph
from platforms.france_tu_sequences import KIND, compare, digest, gpr_writes, named_data, unique_template, words
from platforms.france_math_sequence import float_pair
from platforms.ps2_source import canonical_linkages

SOURCE = 'SB/Core/x/xEnt.cpp'
START = 0x1d1190
TAILS = {('xEntSetupPipeline',12):('xEntSetupPipeline',500)}
CALLBACKS = {'xEntCollCheckOneEntNoDepen','xEntDefaultTranslate','xEntDefaultBoundUpdate',
             'xEntUpdate','xEntRender','stacked_owner_destroyed'}
GLOBAL_TYPES = {'g_O3':'xVec3','g_I3':'xMat4x3','all_ents_box':'xBox',
                'colls_grid':'xGrid','colls_oso_grid':'xGrid','npcs_grid':'xGrid'}


def entity_pair(body,address,offset,*,allow_return_store=False):
    code=words(body)
    # The observed whole leaf returns one LUI-derived pointer in v0. Its ADDIU
    # is the architectural JR delay slot, with no alternate entry or clobber.
    if len(code)==3 and offset==8 and code[0]>>16==0x3c02 and code[1]==0x03e00008 and code[2]>>16==0x2442:
        low=code[2]&65535;low-=65536 if low&32768 else 0
        return 0,(((code[0]&65535)<<16)+low)&0xffffffff
    return float_pair(body,address,offset,allow_return_store=allow_return_store)


def original_data(original):
    from platforms.dwarf1 import iter_dies
    from platforms.ps2_type_layouts import aggregate_layouts
    section=next(s for s in original.metadata['sections'] if s['name']=='.debug' and s['size'])
    debug=original.data[section['offset']:section['offset']+section['size']]
    layouts=aggregate_layouts(debug,SOURCE,set(GLOBAL_TYPES.values())|{'xMat3x3'})
    rows=list(iter_dies(debug));by={off:(tag,owner,a) for off,tag,owner,a in rows}
    shape=lambda name:{m['name']:m['offset'] for m in layouts[name]['members']}
    require({name:record['size'] for name,record in layouts.items()}==
            {'xVec3':12,'xMat3x3':48,'xMat4x3':64,'xBox':24,'xGrid':52},'Entity original type sizes changed')
    require(shape('xVec3')=={'x':0,'y':4,'z':8} and shape('xBox')=={'upper':0,'lower':12} and
            shape('xMat3x3')=={'right':0,'flags':12,'up':16,'pad1':28,'at':32,'pad2':44} and
            shape('xMat4x3')=={'pos':48,'pad3':60},'Entity original component layout changed')
    matrix=layouts['xMat4x3']['die_offset']
    bases=[a for off,tag,owner,a in rows if matrix<off<by[matrix][2][1] and tag==0x1c]
    require(len(bases)==1 and bases[0].get(7)==layouts['xMat3x3']['die_offset'] and
            bases[0].get(2)==bytes.fromhex('040000000007'),'Entity matrix base is not at offset zero')
    def declaration(name):
        matches=[(off,a) for off,tag,owner,a in rows if tag in (7,12) and
                 owner.replace('\\','/').endswith(SOURCE) and a.get(3)==name]
        require(len(matches)==1,'Entity original data declaration ambiguous')
        off,a=matches[0];loc=a.get(2)
        require(isinstance(loc,bytes) and len(loc)==5 and loc[0]==3,'Entity data declaration has no absolute address')
        return off,a,int.from_bytes(loc[1:],'little')
    globals_by_name={}
    for name,type_name in GLOBAL_TYPES.items():
        off,a,address=declaration(name)
        require(a.get(7)==layouts[type_name]['die_offset'],'Entity named global has the wrong original type')
        globals_by_name[name]={'reference_address':address,'type':type_name,'size':layouts[type_name]['size'],'declaration_die':off}
    arrays={}
    for name,dimensions,element_type in [('offs',(4,3,2),8),('receive_models',(15,),9)]:
        off,a,address=declaration(name);die=a.get(7);chain=[]
        for index,count in enumerate(dimensions):
            tag,owner,attrs=by[die];desc=attrs.get(10)
            prefix=bytes.fromhex('000a0000000000')+(count-1).to_bytes(4,'little')+b'\x08'
            require(tag==1 and attrs.get(9)==0 and isinstance(desc,bytes) and desc.startswith(prefix),
                    'Entity array has a different original zero-based constant bound')
            chain.append({'die':die,'subscript_descriptor':desc.hex()})
            if index+1<len(dimensions):
                require(len(desc)==18 and desc[12:14]==b'\x72\x00','Entity nested array reference changed')
                die=int.from_bytes(desc[14:],'little')
            else:
                require(desc[12:]==b'\x55\x00'+element_type.to_bytes(2,'little'),
                        'Entity array has a different four-byte integer element type')
        size=4
        for count in dimensions:size*=count
        arrays[name]={'reference_address':address,'size':size,'dimensions':list(dimensions),
                      'element_fundamental_type':element_type,'declaration_die':off,'type_chain':chain}
    normalized={name:{'size':record['size'],'members':shape(name),'die_offset':record['die_offset']}
                for name,record in layouts.items()}
    return globals_by_name,normalized,arrays


def leaf_bounds(flow,address,size,name,callee_address,callee_name,callee_size):
    result=flow.bounds(address,size)
    require(TAILS.get((name,size))==(callee_name,callee_size) and result['reasons']==[
        'extent_not_terminal_return_delay','local_edge_outside_extent','no_return'],
        'Entity leaf has an unreviewed boundary issue')
    require(not any(result[k] for k in ('returns','direct_or_indirect_calls','stack_adjustments',
            'ra_saves','ra_loads','unreachable_zero_words')), 'Entity tail is not a covered frame-free leaf')
    terminal=flow.instruction(address+size-8)
    require(terminal['kind']=='jump' and terminal['target']==callee_address and flow.valid_delay(address+size-8),
            'Entity leaf tail does not reach its proven callee')
    for pc in range(address,address+size,4):
        require(31 not in gpr_writes(flow.words[pc]),'Entity leaf clobbers its inherited return address')
        inst=flow.instruction(pc)
        if inst['kind']=='jump' and not address<=inst['target']<address+size:
            require(pc==address+size-8,'Another transfer leaves the entity leaf')
    require(flow.bounds(callee_address,callee_size)['passes'],'Entity tail callee does not independently close')
    return {**result,'passes':True,'reasons':[],'boundary_kind':'reviewed-frame-free-leaf-tail',
            'tail_target_name':callee_name,'tail_target_address':callee_address,'terminal_instruction_offset':size-8}


def generate_unit(originals):
    target=originals[TARGET]
    flow=ControlFlow({s['address']+i*4:w for s in target.loaded
                      for i,w in enumerate(words(target.read(s['address'],s['file_size'])))})
    _,rooted_calls,_=rooted_graph(flow,target.metadata['entry_point'])
    functions,sequences,data_proofs,neighbors=[],[],[],[]
    for version in REFERENCES:
        original=originals[version]
        members=sorted((f for f in original.functions if f['source']==SOURCE),key=lambda f:f['low'])
        require(len(members)==48 and sum(f['high']-f['low'] for f in members)==19036,'Entity original membership changed')
        low,end=members[0]['low'],members[-1]['high'];by_address={f['low']:f for f in members}
        linkages=canonical_linkages(original.data,original.metadata)
        ref_flow=ControlFlow({s['address']+i*4:w for s in original.loaded
                              for i,w in enumerate(words(original.read(s['address'],s['file_size'])))})
        masks,pair_map,roles,callee_map={},{},{},{}
        context=None
        for index,function in enumerate(members):
            a,size,name=function['low'],function['high']-function['low'],function['name'];b=START+a-low
            require(a%16==b%16==0 and a in linkages,'Entity original alignment/linkage missing')
            if index:
                gap=a-members[index-1]['high']
                if gap>=16:
                    require((members[index-1]['name'],name,gap)==('xEntMove','xEntMotionToMatrix',156),
                            'Unreviewed interleaved entity context')
                    start=a-gap;french_start=b-gap
                    require(original.read(start,gap)==target.read(french_start,gap) and
                            not any(original.read(start,12)) and not any(original.read(a-8,8)),
                            'Interleaved entity context or zero alignment differs')
                    require(ref_flow.bounds(start+12,136)['passes'] and flow.bounds(french_start+12,136)['passes'],
                            'Unowned interleaved leaf context does not independently close')
                    require(not any(start<=f['low']<a for f in original.functions),
                            'Interleaved context gained original named ownership; review before assigning it')
                    require(context is None,'Multiple interleaved entity contexts require review')
                    context={'reference_address':start+12,'target_address':french_start+12,'size':136,
                             'sha256':digest(original.read(start+12,136)),'original_name':None,
                             'promoted_as_named_anchor':False,'included_in_source_unit':False}
                else:
                    require(0<=gap<16 and not any(original.read(a-gap,gap)) and not any(target.read(b-gap,gap)),
                            'Unrelated bytes interrupt the complete entity sequence')
            pairs,calls,mask=compare(original,target,a,b,size,address_resolver=entity_pair)
            masks.update({a-low+off:value for off,value in mask.items()})
            for pair in pairs:
                p,q=pair['reference_address'],pair['target_address']
                require(p not in pair_map or pair_map[p]['target_address']==q,'Entity data mapping changed')
                pair_map[p]=pair;roles.setdefault(p,set()).add(pair['opcode'])
            for call in calls:
                c,d=call['reference_address'],call['target_address']
                require(c not in callee_map or callee_map[c]==d,'Entity direct-callee mapping changed');callee_map[c]=d
            tail_proof=None
            if (name,size) in TAILS:
                transfers=[c for c in calls if c['offset']==size-8 and c['opcode']==2]
                require(len(transfers)==1,'Entity leaf has no exact terminal transfer')
                call=transfers[0];c,d=call['reference_address'],call['target_address'];callee=original.by_address.get(c)
                require(callee is not None and callee['source']==SOURCE and
                        (callee['name'],callee['high']-c)==TAILS[(name,size)] and d==START+c-low,
                        'Entity overload tail lacks an original same-TU identity')
                csize=callee['high']-c
                reference_bounds=leaf_bounds(ref_flow,a,size,name,c,callee['name'],csize)
                local=leaf_bounds(flow,b,size,name,d,callee['name'],csize)
                tail_proof={'reference_address':c,'target_address':d,'source':SOURCE,'name':callee['name'],
                            'size':csize,'same_tu_complete_body':True}
            else:
                reference_bounds=ref_flow.bounds(a,size);local=flow.bounds(b,size)
            require(reference_bounds['passes'] and local['passes'],'Entity original local bounds fail')
            proof={'version':version,'executable_sha1':original.sha1,'source_address':a,'name':name,'source':SOURCE,
                   'linkage_name':linkages[a],'reference_sha256':digest(original.read(a,size)),
                   'data_address_operands':pairs,'direct_transfers':calls}
            if tail_proof:proof.update(original_boundary_exception=reference_bounds,complete_tail_callee=tail_proof)
            if version==REFERENCES[0]:
                functions.append({'name':name,'source':SOURCE,'address':b,'size':size,'sha256':digest(target.read(b,size)),
                                  'boundary_confirmation':True,'confirmation_kind':KIND,'provenance':[proof],
                                  'corroboration':{'local_control_flow':local,'direct_rooted_call_sites':sorted(rooted_calls[b])}})
            else:
                f=functions[index]
                require((f['name'],f['address'],f['size'],f['provenance'][0]['linkage_name'])==
                        (name,b,size,linkages[a]),'Entity original ordered identities disagree');f['provenance'].append(proof)
        require(context is not None,'Expected original interleaved entity context missing')
        globals_by_name,layouts,arrays=original_data(original);accounted=set();global_records=[];callback_records=[];array_records=[];string_records=[]
        for name,record in globals_by_name.items():
            a=record['reference_address'];size=record['size']
            offsets=(0,4,8) if name=='g_O3' else (0,12,16,28,32,44,48,60) if name=='g_I3' else (0,)
            uses=[pair_map[a+off] for off in offsets];bases={p['target_address']-(p['reference_address']-a) for p in uses}
            require(len(bases)==1,'Entity aggregate components do not share one base')
            for off in offsets:
                expected={49} if name=='g_O3' else {35} if name=='g_I3' and off%16==12 else {9}
                require(roles[a+off]==expected,'Entity global operand role changed')
            b=bases.pop();region_name='initialized_data' if name=='g_O3' else 'runtime_bss'
            for binary,address in ((original,a),(target,b)):
                region=binary._stream_regions[region_name]
                require(region['address']<=address and address+size<=region['address']+region['size'],
                        'Typed entity global exceeds original storage')
            evidence={**record,'target_address':b,'component_offsets':list(offsets)}
            if name=='g_O3':
                require(original.read(a,size)==target.read(b,size),'Original zero-vector payload differs')
                evidence['sha256']=digest(original.read(a,size))
            global_records.append(evidence);accounted.update(a+off for off in offsets)
        for name,record in arrays.items():
            a=record['reference_address'];size=record['size'];offsets=(0,) if name=='offs' else tuple(range(0,64,4))
            require(size==(96 if name=='offs' else 60),'Entity original array extent changed')
            bases={pair_map[a+off]['target_address']-off for off in offsets};require(len(bases)==1,'Entity array component bases disagree')
            b=bases.pop()
            for off in offsets:
                expected={9} if name=='offs' or off==size else {9,43} if off==0 else {43}
                require(roles[a+off]==expected,'Entity array store/end-pointer role changed')
            region_name='initialized_data' if name=='offs' else 'runtime_bss'
            for binary,address in ((original,a),(target,b)):
                region=binary._stream_regions[region_name]
                require(region['address']<=address and address+size<=region['address']+region['size'],'Entity array exceeds original storage')
            evidence={**record,'target_address':b,'observed_offsets':list(offsets),'one_past_pointer_offset':60 if name=='receive_models' else None}
            if name=='offs':
                require(original.read(a,size)==target.read(b,size),'Complete original offset-array payload differs')
                evidence['sha256']=digest(original.read(a,size))
            array_records.append(evidence);accounted.update(a+off for off in offsets)
        for name in sorted(CALLBACKS):
            candidates=[f for f in members if f['name']==name];require(len(candidates)==1,'Entity callback name ambiguous')
            f=candidates[0];a=f['low'];b=START+a-low
            require(named_data(original,name,SOURCE)==a and pair_map[a]['target_address']==b and roles[a]=={9},
                    'Entity callback pointer does not reference its complete original body')
            callback_records.append({'name':name,'reference_address':a,'target_address':b,'size':f['high']-a})
            accounted.add(a)
        from platforms.france_tu_sequences import cstring
        for a in sorted(set(pair_map)-accounted):
            b=pair_map[a]['target_address'];require(roles[a]=={9},'Remaining entity data is not a string pointer')
            aa,bb=cstring(original,a),cstring(target,b)
            require(aa==bb and len(aa)==12,'Entity model-name string payload differs')
            for binary,address in ((original,a),(target,b)):
                region=binary._stream_regions['initialized_data'];require(region['address']<=address and address+len(aa)<=region['address']+region['size'],'Entity string exceeds initialized storage')
            string_records.append({'reference_address':a,'target_address':b,'size':len(aa),'sha256':digest(aa)});accounted.add(a)
        require(len(string_records)==15 and accounted==set(pair_map) and len(pair_map)==53 and
                len({p['target_address'] for p in pair_map.values()})==53,'Entity complete data inventory/bijection changed')
        require(len(set(callee_map.values()))==len(callee_map),'Distinct entity callees collapsed')
        for a,b in sorted(callee_map.items()):
            if low<=a<end:
                require((a in by_address or a==context['reference_address']) and b==START+a-low,
                        'Entity internal call does not enter a named body or the exact closed context leaf');continue
            neighbor=original.by_address.get(a);size=min(32,neighbor['high']-a) if neighbor else 32
            p,c,_=compare(original,target,a,b,size,address_resolver=entity_pair)
            neighbors.append({'version':version,'reference_address':a,'target_address':b,
                              'reference_name':neighbor['name'] if neighbor else None,
                              'reference_source':neighbor['source'] if neighbor else None,
                              'compared_entry_prefix_bytes':size,'reference_sha256':digest(original.read(a,size)),
                              'target_sha256':digest(target.read(b,size)),'data_address_operands':p,'direct_transfers':c,
                              'promoted_as_named_anchor':False})
        sequences.append({'version':version,'source':SOURCE,'source_start':low,'target_start':START,
                          'sequence_bytes_with_alignment':end-low,'uniqueness':unique_template(target,original.read(low,end-low),masks,START),
                          'data_address_count':len(pair_map),'direct_callee_count':len(callee_map),'immutable_interleaved_context':context})
        data_proofs.append({'version':version,'source':SOURCE,'original_aggregate_layouts':layouts,
                            'typed_globals':global_records,'complete_callback_entries':callback_records,'typed_arrays':array_records,'string_payloads':string_records})
    groups={}
    for n in neighbors:groups.setdefault(n['target_address'],[]).append(n)
    for entries in groups.values():
        require({e['version'] for e in entries}==set(REFERENCES) and len(entries)==3 and
                len({(e['reference_name'],e['reference_source'],e['compared_entry_prefix_bytes']) for e in entries})==1,
                'Entity external-neighbor identities disagree')
    require(sum(bool(f['corroboration']['direct_rooted_call_sites']) for f in functions)==34,'Entity rooted entry inventory changed')
    return {'functions':functions,'sequence_proofs':sequences,'data_proofs':data_proofs,'call_neighbors':neighbors,
            'counts':{'functions':48,'code_bytes':19036,'source_units':1,'rooted_direct_call_entries':34,
                      'closed_return_bodies':47,'reviewed_leaf_tail_functions':1,'reviewed_entity_callbacks':6}}
