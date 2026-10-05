"""Original-only French collision sequence, callback and typed-global proof."""
from __future__ import annotations
from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow, rooted_graph
from platforms.france_tu_sequences import KIND, compare, digest, gpr_writes, named_data, unique_template, words
from platforms.france_math_sequence import float_pair
from platforms.ps2_source import canonical_linkages

SOURCE = 'SB/Core/x/xCollide.cpp'
START = 0x1c5d50
TAILS = {('sphereHitsModelCB',28):('sphereHitsEnvCB',472), ('xCollideInit',8):('iCollideInit',8)}
CALLBACKS = {'SweptSphereHitsEntCB','SweptSphereModelCB','SweptSphereLeafNodeCB',
             'SweptSphereHitsEnvCB','xParabolaEnvCB','sphereHitsModelCB'}
GLOBAL_TYPES = {'g_O3':'xVec3','g_I3':'xMat4x3','anim_coll_old_mt':'RpMorphTarget',
                'xqc_def_ctrl':'xQCControl','colls_grid':'xGrid','colls_oso_grid':'xGrid','npcs_grid':'xGrid'}


def original_globals(original):
    from platforms.dwarf1 import iter_dies
    from platforms.ps2_type_layouts import aggregate_layouts
    section=next(s for s in original.metadata['sections'] if s['name']=='.debug' and s['size'])
    debug=original.data[section['offset']:section['offset']+section['size']]
    layouts=aggregate_layouts(debug,SOURCE,set(GLOBAL_TYPES.values())|{'RwV3d'})
    rows=list(iter_dies(debug));shape=lambda name:{m['name']:m['offset'] for m in layouts[name]['members']}
    require({name:record['size'] for name,record in layouts.items()}==
            {'xVec3':12,'RwV3d':12,'xMat4x3':64,'RpMorphTarget':28,'xQCControl':60,'xGrid':52},
            'Original collision global type sizes changed')
    require(shape('xVec3')==shape('RwV3d')=={'x':0,'y':4,'z':8} and
            shape('RpMorphTarget')=={'parentGeom':0,'boundingSphere':4,'verts':20,'normals':24},
            'Original collision component ownership changed')
    globals_by_name={}
    for name,type_name in GLOBAL_TYPES.items():
        declarations=[(off,a) for off,tag,owner,a in rows if tag in (7,12) and
                      owner.replace('\\','/').endswith(SOURCE) and a.get(3)==name]
        require(len(declarations)==1 and declarations[0][1].get(7)==layouts[type_name]['die_offset'],
                'Collision global lacks one original named type declaration')
        off,attributes=declarations[0];location=attributes.get(2)
        require(isinstance(location,bytes) and len(location)==5 and location[0]==3,
                'Collision global lacks an original absolute location')
        globals_by_name[name]={'reference_address':int.from_bytes(location[1:],'little'),
                               'type':type_name,'size':layouts[type_name]['size'],'declaration_die':off}
    # The one nonzero field address is a real four-byte pointer member, not an
    # inferred offset into an anonymous SDK object.
    verts=next(m for m in layouts['RpMorphTarget']['members'] if m['name']=='verts')
    pointer=bytes.fromhex(verts['type_attributes']['8'])
    require(len(pointer)==5 and pointer[0]==1 and int.from_bytes(pointer[1:],'little')==layouts['RwV3d']['die_offset'],
            'Morph-target verts is not the original pointer-to-vector field')
    normalized={name:{'size':record['size'],'members':shape(name),'die_offset':record['die_offset']}
                for name,record in layouts.items()}
    return globals_by_name,normalized


def leaf_bounds(flow,address,size,name,callee_address,callee_name,callee_size):
    result=flow.bounds(address,size)
    require(TAILS.get((name,size))==(callee_name,callee_size) and result['reasons']==[
        'extent_not_terminal_return_delay','local_edge_outside_extent','no_return'],
        'Collision leaf has an unreviewed boundary issue')
    require(not any(result[k] for k in ('returns','direct_or_indirect_calls','stack_adjustments',
            'ra_saves','ra_loads','unreachable_zero_words')), 'Collision tail is not a covered frame-free leaf')
    terminal=flow.instruction(address+size-8)
    require(terminal['kind']=='jump' and terminal['target']==callee_address and flow.valid_delay(address+size-8),
            'Collision leaf tail does not reach its proven callee')
    for pc in range(address,address+size,4):
        require(31 not in gpr_writes(flow.words[pc]),'Collision leaf clobbers its inherited return address')
        inst=flow.instruction(pc)
        if inst['kind']=='jump' and not address<=inst['target']<address+size:
            require(pc==address+size-8,'Another transfer leaves the collision leaf')
    require(flow.bounds(callee_address,callee_size)['passes'],'Collision tail callee does not independently close')
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
        require(len(members)==36 and sum(f['high']-f['low'] for f in members)==36648,'Collision original membership changed')
        low,end=members[0]['low'],members[-1]['high'];by_address={f['low']:f for f in members}
        linkages=canonical_linkages(original.data,original.metadata)
        ref_flow=ControlFlow({s['address']+i*4:w for s in original.loaded
                              for i,w in enumerate(words(original.read(s['address'],s['file_size'])))})
        masks,pair_map,roles,callee_map={},{},{},{}
        for index,function in enumerate(members):
            a,size,name=function['low'],function['high']-function['low'],function['name'];b=START+a-low
            require(a%16==b%16==0 and a in linkages,'Collision original alignment/linkage missing')
            if index:
                gap=a-members[index-1]['high']
                require(0<=gap<16 and not any(original.read(a-gap,gap)) and not any(target.read(b-gap,gap)),
                        'Unrelated bytes interrupt the complete collision sequence')
            pairs,calls,mask=compare(original,target,a,b,size,address_resolver=float_pair)
            masks.update({a-low+off:value for off,value in mask.items()})
            for pair in pairs:
                p,q=pair['reference_address'],pair['target_address']
                require(p not in pair_map or pair_map[p]['target_address']==q,'Collision data mapping changed')
                pair_map[p]=pair;roles.setdefault(p,set()).add(pair['opcode'])
            for call in calls:
                c,d=call['reference_address'],call['target_address']
                require(c not in callee_map or callee_map[c]==d,'Collision direct-callee mapping changed');callee_map[c]=d
            tail_proof=None
            if (name,size) in TAILS:
                transfers=[c for c in calls if c['offset']==size-8 and c['opcode']==2]
                require(len(transfers)==1,'Collision leaf has no exact terminal transfer')
                call=transfers[0];c,d=call['reference_address'],call['target_address'];callee=original.by_address.get(c)
                require(callee is not None and callee['source']=='SB/Core/p2/iCollide.cpp' and
                        (callee['name'],callee['high']-c)==TAILS[(name,size)],'Collision platform tail lacks original identity')
                csize=callee['high']-c
                cp,cc,_=compare(original,target,c,d,csize,address_resolver=float_pair)
                require(not cp,'Reviewed platform collision tail body unexpectedly gained absolute data references')
                reference_bounds=leaf_bounds(ref_flow,a,size,name,c,callee['name'],csize)
                local=leaf_bounds(flow,b,size,name,d,callee['name'],csize)
                tail_proof={'reference_address':c,'target_address':d,'source':callee['source'],'name':callee['name'],
                            'size':csize,'reference_sha256':digest(original.read(c,csize)),
                            'target_sha256':digest(target.read(d,csize)),'direct_transfers':cc,
                            'promoted_as_named_anchor':False}
            else:
                reference_bounds=ref_flow.bounds(a,size);local=flow.bounds(b,size)
            require(reference_bounds['passes'] and local['passes'],'Collision original local bounds fail')
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
                        (name,b,size,linkages[a]),'Collision original ordered identities disagree');f['provenance'].append(proof)
        globals_by_name,layouts=original_globals(original);accounted=set();global_records=[];callback_records=[]
        for name,record in globals_by_name.items():
            a=record['reference_address'];size=record['size'];offsets=(0,4,8) if name=='g_O3' else (20,) if name=='anim_coll_old_mt' else (0,)
            uses=[pair_map[a+off] for off in offsets];bases={p['target_address']-(p['reference_address']-a) for p in uses}
            require(len(bases)==1,'Collision aggregate component references do not share one base')
            opcodes={49} if name=='g_O3' else {35,43} if name=='anim_coll_old_mt' else {9}
            require(all(roles[a+off]==opcodes for off in offsets),'Collision global read/write role changed')
            b=bases.pop();region_name='initialized_data' if name=='g_O3' else 'runtime_bss'
            for binary,address in ((original,a),(target,b)):
                region=binary._stream_regions[region_name]
                require(region['address']<=address and address+size<=region['address']+region['size'],
                        'Typed collision global exceeds original storage')
            evidence={**record,'target_address':b,'component_offsets':list(offsets),'opcodes':sorted(opcodes)}
            if name=='g_O3':
                require(original.read(a,size)==target.read(b,size),'Original zero-vector payload differs')
                evidence['sha256']=digest(original.read(a,size))
            global_records.append(evidence);accounted.update(a+off for off in offsets)
        for name in sorted(CALLBACKS):
            candidates=[f for f in members if f['name']==name];require(len(candidates)==1,'Collision callback name ambiguous')
            f=candidates[0];a=f['low'];b=START+a-low
            require(named_data(original,name,SOURCE)==a and pair_map[a]['target_address']==b and roles[a]=={9},
                    'Collision callback pointer does not reference its complete original body')
            callback_records.append({'name':name,'reference_address':a,'target_address':b,'size':f['high']-a})
            accounted.add(a)
        require(accounted==set(pair_map) and len(pair_map)==15 and len({p['target_address'] for p in pair_map.values()})==15,
                'Collision complete data-address inventory/bijection changed')
        require(len(set(callee_map.values()))==len(callee_map),'Distinct collision callees collapsed')
        for a,b in sorted(callee_map.items()):
            if low<=a<end:
                require(a in by_address and b==START+a-low,'Collision internal call changed ownership');continue
            neighbor=original.by_address.get(a);size=min(32,neighbor['high']-a) if neighbor else 32
            p,c,_=compare(original,target,a,b,size,address_resolver=float_pair)
            neighbors.append({'version':version,'reference_address':a,'target_address':b,
                              'reference_name':neighbor['name'] if neighbor else None,
                              'reference_source':neighbor['source'] if neighbor else None,
                              'compared_entry_prefix_bytes':size,'reference_sha256':digest(original.read(a,size)),
                              'target_sha256':digest(target.read(b,size)),'data_address_operands':p,'direct_transfers':c,
                              'promoted_as_named_anchor':False})
        sequences.append({'version':version,'source':SOURCE,'source_start':low,'target_start':START,
                          'sequence_bytes_with_alignment':end-low,'uniqueness':unique_template(target,original.read(low,end-low),masks,START),
                          'data_address_count':len(pair_map),'direct_callee_count':len(callee_map)})
        data_proofs.append({'version':version,'source':SOURCE,'original_aggregate_layouts':layouts,
                            'typed_globals':global_records,'complete_callback_entries':callback_records})
    groups={}
    for n in neighbors:groups.setdefault(n['target_address'],[]).append(n)
    for entries in groups.values():
        require({e['version'] for e in entries}==set(REFERENCES) and len(entries)==3 and
                len({(e['reference_name'],e['reference_source'],e['compared_entry_prefix_bytes']) for e in entries})==1,
                'Collision external-neighbor identities disagree')
    require(sum(bool(f['corroboration']['direct_rooted_call_sites']) for f in functions)==6,'Collision rooted entry inventory changed')
    return {'functions':functions,'sequence_proofs':sequences,'data_proofs':data_proofs,'call_neighbors':neighbors,
            'counts':{'functions':36,'code_bytes':36648,'source_units':1,'rooted_direct_call_entries':6,
                      'closed_return_bodies':34,'reviewed_leaf_tail_functions':2,'reviewed_collision_callbacks':6}}
