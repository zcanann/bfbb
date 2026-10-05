"""Original-only clump-collision and spline sequences with typed-array evidence."""
from __future__ import annotations
from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow, rooted_graph
from platforms.france_tu_sequences import KIND, compare, digest, named_data, unique_template, words
from platforms.ps2_source import canonical_linkages

UNITS = (
    {'source':'SB/Core/x/xClumpColl.cpp','start':0x3021c0,'count':11,'bytes':10260,'rooted':4,
     'callbacks':('LeafNodeBoxPolyIntersect','LeafNodeSpherePolyIntersect','LeafNodeLinePolyIntersect','AddAtomicCB'),
     'basis':(),'slots':(0x134,),'slot_opcodes':(35,)},
    {'source':'SB/Core/x/xSpline.cpp','start':0x20b080,'count':13,'bytes':7152,'rooted':8,
     'callbacks':(),'basis':('sBasisBezier','sBasisHermite'),'slots':(0x134,0x138),'slot_opcodes':(9,)},
)


def original_arrays(original, source, basis):
    from platforms.dwarf1 import iter_dies
    section=next(s for s in original.metadata['sections'] if s['name']=='.debug' and s['size'])
    rows=list(iter_dies(original.data[section['offset']:section['offset']+section['size']]))
    by={off:(tag,owner,attrs) for off,tag,owner,attrs in rows}
    result={}
    for name,dimensions,element_type in [('ourGlobals',(4096,),9),*((n,(4,4),14) for n in basis)]:
        declarations=[(off,a) for off,tag,owner,a in rows if tag in(7,12) and
                      owner.replace('\\','/').endswith(source) and a.get(3)==name]
        require(len(declarations)==1,'Original geometry array declaration is ambiguous')
        off,a=declarations[0];loc=a.get(2)
        require(isinstance(loc,bytes) and len(loc)==5 and loc[0]==3,'Original geometry array lacks absolute location')
        die=a.get(7);chain=[]
        for index,count in enumerate(dimensions):
            tag,owner,attrs=by[die];desc=attrs.get(10)
            prefix=bytes.fromhex('000a0000000000')+(count-1).to_bytes(4,'little')+b'\x08'
            require(tag==1 and attrs.get(9)==0 and isinstance(desc,bytes) and desc.startswith(prefix),
                    'Original geometry array bound or storage order changed')
            chain.append({'die':die,'subscript_descriptor':desc.hex()})
            if index+1<len(dimensions):
                require(len(desc)==18 and desc[12:14]==b'\x72\x00','Original geometry nested type reference changed')
                die=int.from_bytes(desc[14:],'little')
            else:
                require(desc[12:]==b'\x55\x00'+element_type.to_bytes(2,'little'),
                        'Original geometry array four-byte scalar type changed')
        size=4
        for count in dimensions:size*=count
        result[name]={'name':name,'reference_address':int.from_bytes(loc[1:],'little'),
                      'size':size,'dimensions':list(dimensions),'element_fundamental_type':element_type,
                      'declaration_die':off,'type_chain':chain}
    return result


def unit_proof(originals, spec, flow, rooted_calls):
    target=originals[TARGET];source,start=spec['source'],spec['start']
    functions,sequences,data_proofs,neighbors=[],[],[],[]
    for version in REFERENCES:
        original=originals[version]
        members=sorted((f for f in original.functions if f['source']==source),key=lambda f:f['low'])
        require(len(members)==spec['count'] and sum(f['high']-f['low'] for f in members)==spec['bytes'],
                'Original geometry whole-unit membership differs')
        low,end=members[0]['low'],members[-1]['high'];entries={f['low'] for f in members}
        linkages=canonical_linkages(original.data,original.metadata)
        ref_flow=ControlFlow({s['address']+i*4:w for s in original.loaded
                              for i,w in enumerate(words(original.read(s['address'],s['file_size'])))})
        masks,pairs_by_address,roles,callees={},{},{},{}
        for index,function in enumerate(members):
            a,size,name=function['low'],function['high']-function['low'],function['name'];b=start+a-low
            require(a%16==b%16==0 and a in linkages,'Original geometry alignment or linkage missing')
            if index:
                gap=a-members[index-1]['high']
                require(0<=gap<16 and not any(original.read(a-gap,gap)) and not any(target.read(b-gap,gap)),
                        'Unreviewed bytes interrupt original geometry sequence')
            pairs,calls,mask=compare(original,target,a,b,size)
            masks.update({a-low+off:value for off,value in mask.items()})
            for p in pairs:
                ra,rb=p['reference_address'],p['target_address']
                require(ra not in pairs_by_address or pairs_by_address[ra]['target_address']==rb,
                        'One geometry data address maps inconsistently')
                pairs_by_address[ra]=p;roles.setdefault(ra,set()).add(p['opcode'])
            for c in calls:
                ra,rb=c['reference_address'],c['target_address']
                require(ra not in callees or callees[ra]==rb,'One geometry callee maps inconsistently');callees[ra]=rb
            bounds=flow.bounds(b,size)
            require(bounds['passes'] and ref_flow.bounds(a,size)['passes'],'Original geometry CFG does not strictly close')
            proof={'version':version,'executable_sha1':original.sha1,'source_address':a,'name':name,'source':source,
                   'linkage_name':linkages[a],'reference_sha256':digest(original.read(a,size)),
                   'data_address_operands':pairs,'direct_transfers':calls}
            if version==REFERENCES[0]:
                functions.append({'name':name,'source':source,'address':b,'size':size,'sha256':digest(target.read(b,size)),
                                  'boundary_confirmation':True,'confirmation_kind':KIND,'provenance':[proof],
                                  'corroboration':{'local_control_flow':bounds,'direct_rooted_call_sites':sorted(rooted_calls[b])}})
            else:
                f=functions[index]
                require((f['name'],f['address'],f['size'],f['provenance'][0]['linkage_name'])==
                        (name,b,size,linkages[a]),'Original geometry ordered identities disagree')
                f['provenance'].append(proof)
        arrays=original_arrays(original,source,spec['basis']);accounted=set();array_records=[];callback_records=[]
        for name,record in arrays.items():
            a=record['reference_address'];size=record['size'];offsets=spec['slots'] if name=='ourGlobals' else (0,)
            bases={pairs_by_address[a+off]['target_address']-off for off in offsets}
            require(len(bases)==1,'Original geometry array components have inconsistent mapped bases')
            b=bases.pop();region_name='runtime_bss' if name=='ourGlobals' else 'initialized_data'
            for off in offsets:
                expected=set(spec['slot_opcodes']) if name=='ourGlobals' else {9}
                require(0<=off and off+4<=size and roles[a+off]==expected,'Original geometry array component role differs')
            for binary,address in ((original,a),(target,b)):
                region=binary._stream_regions[region_name]
                require(region['address']<=address and address+size<=region['address']+region['size'],
                        'Original typed geometry array exceeds authenticated storage')
            evidence={**record,'target_address':b,'component_offsets':list(offsets)}
            if name!='ourGlobals':
                require(size==64 and original.read(a,size)==target.read(b,size),'Original full spline coefficient matrix differs')
                evidence['sha256']=digest(original.read(a,size))
            array_records.append(evidence);accounted.update(a+off for off in offsets)
        for name in spec['callbacks']:
            candidates=[f for f in members if f['name']==name];require(len(candidates)==1,'Original geometry callback identity ambiguous')
            f=candidates[0];a=f['low'];b=start+a-low
            require(named_data(original,name,source)==a and pairs_by_address[a]['target_address']==b and roles[a]=={9},
                    'Original callback address does not reference its complete same-unit body')
            callback_records.append({'name':name,'reference_address':a,'target_address':b,'size':f['high']-a});accounted.add(a)
        require(accounted==set(pairs_by_address) and len({p['target_address'] for p in pairs_by_address.values()})==len(accounted),
                'Original geometry data inventory is incomplete or many-to-one')
        require(len(set(callees.values()))==len(callees),'Distinct geometry callees collapse')
        for a,b in sorted(callees.items()):
            if low<=a<end:
                require(a in entries and b==start+a-low,'Original internal geometry call changes ownership');continue
            neighbor=original.by_address.get(a);size=min(32,neighbor['high']-a) if neighbor else 32
            p,c,_=compare(original,target,a,b,size)
            neighbors.append({'version':version,'reference_address':a,'target_address':b,
                              'reference_name':neighbor['name'] if neighbor else None,
                              'reference_source':neighbor['source'] if neighbor else None,'compared_entry_prefix_bytes':size,
                              'reference_sha256':digest(original.read(a,size)),'target_sha256':digest(target.read(b,size)),
                              'data_address_operands':p,'direct_transfers':c,'promoted_as_named_anchor':False})
        sequences.append({'version':version,'source':source,'source_start':low,'target_start':start,
                          'sequence_bytes_with_alignment':end-low,'uniqueness':unique_template(target,original.read(low,end-low),masks,start),
                          'data_address_count':len(accounted),'direct_callee_count':len(callees)})
        data_proofs.append({'version':version,'source':source,'typed_arrays':array_records,'complete_callback_entries':callback_records})
    groups={}
    for n in neighbors:groups.setdefault(n['target_address'],[]).append(n)
    for rows in groups.values():
        require({n['version'] for n in rows}==set(REFERENCES) and len(rows)==3 and
                len({(n['reference_name'],n['reference_source'],n['compared_entry_prefix_bytes']) for n in rows})==1,
                'Original geometry external entry identities disagree')
    require(sum(bool(f['corroboration']['direct_rooted_call_sites']) for f in functions)==spec['rooted'],
            'Original geometry rooted entry inventory differs')
    return {'functions':functions,'sequence_proofs':sequences,'data_proofs':data_proofs,'call_neighbors':neighbors}


def generate_unit(originals):
    target=originals[TARGET]
    flow=ControlFlow({s['address']+i*4:w for s in target.loaded
                      for i,w in enumerate(words(target.read(s['address'],s['file_size'])))})
    _,rooted_calls,_=rooted_graph(flow,target.metadata['entry_point'])
    result={key:[] for key in ('functions','sequence_proofs','data_proofs','call_neighbors')}
    for spec in UNITS:
        unit=unit_proof(originals,spec,flow,rooted_calls)
        for key in result:result[key].extend(unit[key])
    # Both independent TUs must identify the same original array base in each
    # version, and the same French base. This proves only array-relative slots;
    # it does not infer an undocumented RenderWare struct layout or function name.
    for version in REFERENCES:
        globals_=[a for p in result['data_proofs'] if p['version']==version for a in p['typed_arrays'] if a['name']=='ourGlobals']
        require(len(globals_)==2 and len({(a['reference_address'],a['target_address'],a['size']) for a in globals_})==1,
                'Independent original units disagree on allocator-array ownership')
    result['counts']={'functions':24,'code_bytes':17412,'source_units':2,'rooted_direct_call_entries':12,
                      'closed_return_bodies':24,'reviewed_geometry_callbacks':4,'reviewed_spline_matrices':2}
    return result
