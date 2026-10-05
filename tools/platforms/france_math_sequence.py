"""Original-only French math TU sequence and float-address ownership evidence."""
from __future__ import annotations
from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow, rooted_graph
from platforms.france_tu_sequences import (KIND, address_pair, compare, digest, gpr_writes, named_data, unique_template, words)
from platforms.ps2_source import canonical_linkages

SOURCE = 'SB/Core/x/xMath3.cpp'
START = 0x1ee5c0
TAILS = {('xQuatMul', 136): 'xQuatNormalize', ('xMat3x3Euler', 16): 'xMat3x3Euler',
         ('xMath3Exit', 8): 'iMath3Exit'}


def math_gpr_writes(word):
    # Actual MUL.S, NEG.S, ADDA.S, SUBA.S, MULA.S, MADD.S and MSUB.S
    # write an FPR or FP accumulator,
    # never a GPR. Register-transfer encodings use different rs values and still
    # go through the shared conservative decoder.
    if (word >> 26 == 17 and word >> 21 & 31 == 16 and
            word & 63 in (2, 7, 24, 25, 26, 28, 29)):
        return set()
    return gpr_writes(word)


def float_pair(body, address, offset, *, allow_return_store=False):
    code = words(body)
    low = code[offset // 4]
    if low >> 26 not in (49, 57):
        return address_pair(body, address, offset, allow_return_store=allow_return_store)
    register = low >> 21 & 31
    require(register not in (0, 31), 'Float address lacks an ordinary base register')
    high_offset = None
    for off in range(offset - 4, -1, -4):
        w = code[off // 4]
        if register in math_gpr_writes(w):
            require(w >> 26 == 15 and w >> 21 & 31 == 0, 'Float base is not produced by LUI')
            high_offset = off
            break
    require(high_offset is not None, 'Float address lacks a reaching LUI')
    flow = ControlFlow({address + i * 4: w for i, w in enumerate(code)})
    if high_offset >= 4:
        previous = flow.instruction(address + high_offset - 4)
        require(previous['kind'] == 'normal' or
                (previous['kind'] == 'branch' and not previous.get('likely')),
                'Float LUI is in an unproved delay slot')
    for off in range(high_offset + 4, offset, 4):
        inst = flow.instruction(address + off)
        require(inst['kind'] in ('normal', 'branch'), 'Transfer interrupts float-address lifetime')
    for off in range(0, len(body), 4):
        inst = flow.instruction(address + off)
        destination = inst.get('target')
        if destination is not None and address + high_offset < destination <= address + offset:
            require(off == high_offset - 4 and inst['kind'] == 'branch', 'Edge bypasses float-address producer')
    immediate = (low & 65535) - (65536 if low & 32768 else 0)
    return high_offset, (((code[high_offset // 4] & 65535) << 16) + immediate) & 0xffffffff


def tail_bounds(flow, address, size, name, callee_address, callee_name, callee_size):
    bounds = flow.bounds(address, size)
    if bounds['passes']:
        return bounds
    require(TAILS.get((name, size)) == callee_name and bounds['reasons'] == [
        'extent_not_terminal_return_delay', 'local_edge_outside_extent', 'no_return'],
        'Math function has an unreviewed tail exception')
    require(not any(bounds[k] for k in ('returns','direct_or_indirect_calls','stack_adjustments',
            'ra_saves','ra_loads','unreachable_zero_words')), 'Math tail is not a covered frame-free leaf')
    terminal = flow.instruction(address + size - 8)
    require(terminal['kind'] == 'jump' and terminal['target'] == callee_address and
            flow.valid_delay(address + size - 8), 'Math tail does not reach its proven callee')
    for pc in range(address, address + size, 4):
        require(31 not in math_gpr_writes(flow.words[pc]), 'Math leaf clobbers its return address')
        inst = flow.instruction(pc)
        if inst['kind'] == 'jump' and not address <= inst['target'] < address + size:
            require(pc == address + size - 8, 'Another transfer leaves the math leaf')
    require(flow.bounds(callee_address, callee_size)['passes'], 'Math tail callee lacks closed original bounds')
    return {**bounds, 'passes':True, 'reasons':[], 'boundary_kind':'reviewed-frame-free-leaf-tail',
            'tail_target_name':callee_name, 'tail_target_address':callee_address,
            'terminal_instruction_offset':size-8}


def original_data(original):
    from platforms.dwarf1 import iter_dies
    from platforms.ps2_type_layouts import aggregate_layouts
    section = next(s for s in original.metadata['sections'] if s['name'] == '.debug' and s['size'])
    debug = original.data[section['offset']:section['offset'] + section['size']]
    rows = list(iter_dies(debug))
    layouts = aggregate_layouts(debug, SOURCE, {'xVec3', 'xQuat', 'xMat3x3', 'xMat4x3'})
    shape = lambda name: {m['name']: m['offset'] for m in layouts[name]['members']}
    require(layouts['xVec3']['size'] == 12 and shape('xVec3') == {'x':0, 'y':4, 'z':8} and
            layouts['xQuat']['size'] == 16 and shape('xQuat') == {'v':0, 's':12} and
            layouts['xMat3x3']['size'] == 48 and shape('xMat3x3') ==
                {'right':0, 'flags':12, 'up':16, 'pad1':28, 'at':32, 'pad2':44} and
            layouts['xMat4x3']['size'] == 64 and shape('xMat4x3') == {'pos':48, 'pad3':60},
            'Original math aggregate component layout changed')
    by_offset = {off: attrs for off, _, _, attrs in rows}
    matrix = layouts['xMat4x3']['die_offset']
    bases = [attrs for off, tag, _, attrs in rows if matrix < off < by_offset[matrix][1] and tag == 0x1c]
    require(len(bases) == 1 and bases[0].get(7) == layouts['xMat3x3']['die_offset'] and
            bases[0].get(2) == bytes.fromhex('040000000007'), 'Matrix inheritance is not at offset zero')
    globals_by_name = {}
    for name, type_name in [('g_O3','xVec3'),('g_X3','xVec3'),('g_Y3','xVec3'),('g_Z3','xVec3'),
                            ('g_IQ','xQuat'),('g_I3','xMat4x3')]:
        declarations = [(off, attrs) for off, tag, owner, attrs in rows if tag in (7,12) and
                        owner.replace('\\','/').endswith(SOURCE) and attrs.get(3) == name]
        require(len(declarations) == 1 and declarations[0][1].get(7) == layouts[type_name]['die_offset'],
                'Math global lacks its original named aggregate type')
        address = named_data(original, name, SOURCE)
        globals_by_name[name] = {'reference_address':address, 'type':type_name,
                                 'size':layouts[type_name]['size'], 'declaration_die':declarations[0][0]}
    nxt = [(off, attrs) for off, tag, owner, attrs in rows if tag == 12 and
           owner.replace('\\','/').endswith(SOURCE) and attrs.get(3) == 'nxt']
    require(len(nxt) == 1, 'Original quaternion index array is ambiguous')
    array_offset = nxt[0][1].get(7)
    array = [(tag, attrs) for off, tag, _, attrs in rows if off == array_offset]
    # This original DWARF1 descriptor is one constant 0..2 dimension followed
    # by the four-byte long element type. No generic array inference is used.
    require(len(array) == 1 and array[0][0] == 1 and array[0][1].get(9) == 0 and
            array[0][1].get(10) == bytes.fromhex('000a0000000000020000000855000800'),
            'Original nxt array descriptor changed')
    array_record = {'reference_address':named_data(original,'nxt',SOURCE),'size':12,
                    'declaration_die':nxt[0][0],'type_die':array_offset,
                    'subscript_descriptor':array[0][1][10].hex()}
    return globals_by_name, {name:{'size':record['size'], 'members':shape(name), 'die_offset':record['die_offset']}
                            for name,record in layouts.items()}, array_record


def generate_unit(originals):
    target = originals[TARGET]
    flow = ControlFlow({s['address']+i*4:w for s in target.loaded
                        for i,w in enumerate(words(target.read(s['address'],s['file_size'])))})
    _, rooted_calls, _ = rooted_graph(flow, target.metadata['entry_point'])
    functions, sequences, data_proofs, neighbors = [], [], [], []
    for version in REFERENCES:
        original = originals[version]
        members = sorted((f for f in original.functions if f['source']==SOURCE),key=lambda f:f['low'])
        require(len(members)==35 and sum(f['high']-f['low'] for f in members)==8600, 'Math TU membership changed')
        low,end=members[0]['low'],members[-1]['high'];by_address={f['low']:f for f in members}
        linkages=canonical_linkages(original.data,original.metadata)
        ref_flow=ControlFlow({s['address']+i*4:w for s in original.loaded
                             for i,w in enumerate(words(original.read(s['address'],s['file_size'])))})
        masks,all_pairs,transfer_map={},{},{}
        header_gaps=[]
        data_roles={}
        for index,function in enumerate(members):
            a,size,name=function['low'],function['high']-function['low'],function['name'];b=START+a-low
            require(a%16==b%16==0 and a in linkages,'Math alignment/linkage missing')
            if index:
                gap=a-members[index-1]['high']
                if gap < 16:
                    require(gap>=0 and not any(original.read(a-gap,gap)) and not any(target.read(b-gap,gap)),
                            'Non-padding interrupts complete math TU')
                else:
                    require(gap==120 and members[index-1]['name']=='xMat3x3Normalize' and name=='xBoxFromCone',
                            'Math sequence has an unknown interleaved owner')
                    header=original.by_address.get(a-112)
                    require(header is not None and header['source']=='SB/Core/x/xVec3.h' and header['name']=='__mi' and
                            header['high']==a and a-112 in linkages and
                            not any(original.read(a-120,8)) and not any(target.read(b-120,8)) and
                            original.read(a-112,112)==target.read(b-112,112) and
                            ref_flow.bounds(a-112,112)['passes'] and flow.bounds(b-112,112)['passes'],
                            'Interleaved vector operator lacks exact original ownership/bounds')
                    header_gaps.append({'reference_address':a-112,'target_address':b-112,'size':112,
                                        'source':header['source'],'name':header['name'],'linkage_name':linkages[a-112],
                                        'sha256':digest(original.read(a-112,112)),'promoted_as_math_function':False})
            pairs,calls,mask=compare(original,target,a,b,size,address_resolver=float_pair)
            masks.update({a-low+off:value for off,value in mask.items()})
            for pair in pairs:
                p=pair['reference_address'];q=pair['target_address']
                require(p not in all_pairs or all_pairs[p]['target_address']==q,'Math data mapping inconsistent')
                data_roles.setdefault(p,set()).add(pair['opcode'])
                all_pairs[p]=pair
            for call in calls:
                c,d=call['reference_address'],call['target_address']
                require(c not in transfer_map or transfer_map[c]==d,'Math callee map inconsistent');transfer_map[c]=d
            if (name,size) in TAILS:
                transfers=[c for c in calls if c['offset']==size-8 and c['opcode']==2]
                require(len(transfers)==1,'Math leaf tail has no exact direct transfer')
                call=transfers[0];c,d=call['reference_address'],call['target_address'];callee=original.by_address.get(c)
                require(callee is not None and callee['name']==TAILS[(name,size)],'Math tail lacks original named callee')
                csize=callee['high']-c
                if callee['source']==SOURCE:
                    require(c in by_address and d==START+c-low,'Math internal tail changed correspondence')
                else:
                    require(callee['source']=='SB/Core/p2/iMath3.cpp' and callee['name']=='iMath3Exit' and csize==8 and
                            original.read(c,csize)==target.read(d,csize),'Platform math exit body is not independently identical')
                reference_bounds=tail_bounds(ref_flow,a,size,name,c,callee['name'],csize)
                local=tail_bounds(flow,b,size,name,d,callee['name'],csize)
            else:
                reference_bounds=ref_flow.bounds(a,size);local=flow.bounds(b,size)
            require(reference_bounds['passes'] and local['passes'],'Math original bounds do not close')
            proof={'version':version,'executable_sha1':original.sha1,'source_address':a,'name':name,'source':SOURCE,
                   'linkage_name':linkages[a],'reference_sha256':digest(original.read(a,size)),
                   'data_address_operands':pairs,'direct_transfers':calls}
            if 'boundary_kind' in reference_bounds:proof['original_boundary_exception']=reference_bounds
            if version==REFERENCES[0]:
                functions.append({'name':name,'source':SOURCE,'address':b,'size':size,'sha256':digest(target.read(b,size)),
                                  'boundary_confirmation':True,'confirmation_kind':KIND,'provenance':[proof],
                                  'corroboration':{'local_control_flow':local,'direct_rooted_call_sites':sorted(rooted_calls[b])}})
            else:
                f=functions[index];require((f['name'],f['address'],f['size'],f['provenance'][0]['linkage_name'])==
                                          (name,b,size,linkages[a]),'Math original ordered identities disagree');f['provenance'].append(proof)
        declarations,layouts,array_record=original_data(original);data_records=[];accounted=set()
        for name,record in declarations.items():
            a=record['reference_address'];size=record['size'];is_matrix=name=='g_I3'
            offsets=tuple(row+col for row in (0,16,32,48) for col in (0,4,8)) if is_matrix else tuple(range(0,size,4))
            pairs=[all_pairs[a+off] for off in offsets]
            bases={p['target_address']-(p['reference_address']-a) for p in pairs}
            require(len(bases)==1 and all(data_roles[a+off]==({9,57} if is_matrix and off==0 else
                                                           {57} if is_matrix else {49}) for off in offsets),
                    'Math named component accesses changed base or load/store role')
            b=bases.pop();region_name='runtime_bss' if is_matrix else 'initialized_data'
            for binary,address in ((original,a),(target,b)):
                region=binary._stream_regions[region_name]
                require(region['address']<=address and address+size<=region['address']+region['size'],
                        'Math aggregate exceeds authenticated original storage')
            evidence={**record,'target_address':b,'component_offsets':list(offsets),
                      'opcodes_by_component':{str(off):sorted(data_roles[a+off]) for off in offsets}}
            if not is_matrix:
                first,second=original.read(a,size),target.read(b,size)
                require(first==second,'Math original initialized aggregate payload differs');evidence['sha256']=digest(first)
            data_records.append(evidence);accounted.update(a+off for off in offsets)
        array_address=array_record['reference_address'];array_pair=all_pairs[array_address]
        require(data_roles[array_address]=={9} and array_pair['storage']=='file_backed',
                'Original quaternion index-array use changed')
        target_array=array_pair['target_address']
        for binary,address in ((original,array_address),(target,target_array)):
            region=binary._stream_regions['initialized_data']
            require(region['address']<=address and address+12<=region['address']+region['size'],
                    'Quaternion index array exceeds initialized storage')
        array_bytes=original.read(array_address,12)
        require(array_bytes==target.read(target_array,12) and words(array_bytes)==[1,2,0],
                'Original quaternion cyclic-index table differs')
        array_record.update(target_address=target_array,sha256=digest(array_bytes))
        accounted.add(array_address)
        require(accounted==set(all_pairs) and len(all_pairs)==29 and len({p['target_address'] for p in all_pairs.values()})==29,
                'Math data inventory/bijection changed')
        require(len(set(transfer_map.values()))==len(transfer_map),'Distinct math callees collapsed')
        for a,b in sorted(transfer_map.items()):
            if low<=a<end:
                header_targets={p['reference_address']:p['target_address'] for p in header_gaps}
                require((a in by_address and b==START+a-low) or header_targets.get(a)==b,
                        'Math internal call changed ownership');continue
            neighbor=original.by_address.get(a);size=min(32,neighbor['high']-a) if neighbor else 32
            pairs,calls,_=compare(original,target,a,b,size,address_resolver=float_pair)
            neighbors.append({'version':version,'reference_address':a,'target_address':b,
                              'reference_name':neighbor['name'] if neighbor else None,
                              'reference_source':neighbor['source'] if neighbor else None,
                              'compared_entry_prefix_bytes':size,'reference_sha256':digest(original.read(a,size)),
                              'target_sha256':digest(target.read(b,size)),'data_address_operands':pairs,
                              'direct_transfers':calls,'promoted_as_named_anchor':False})
        sequences.append({'version':version,'source':SOURCE,'source_start':low,'target_start':START,
                          'sequence_bytes_with_alignment':end-low,'uniqueness':unique_template(target,original.read(low,end-low),masks,START),
                          'data_address_count':len(all_pairs),'direct_callee_count':len(transfer_map),
                          'interleaved_header_anchors':header_gaps})
        data_proofs.append({'version':version,'source':SOURCE,'original_aggregate_layouts':layouts,'named_globals':data_records,'quaternion_index_array':array_record})
    groups={}
    for neighbor in neighbors:groups.setdefault(neighbor['target_address'],[]).append(neighbor)
    for entries in groups.values():
        require({e['version'] for e in entries}==set(REFERENCES) and len(entries)==3 and
                len({(e['reference_name'],e['reference_source'],e['compared_entry_prefix_bytes']) for e in entries})==1,
                'Math original call-neighbor identities differ')
    require(sum(bool(f['corroboration']['direct_rooted_call_sites']) for f in functions)==23,'Math entry witness inventory changed')
    return {'functions':functions,'sequence_proofs':sequences,'data_proofs':data_proofs,'call_neighbors':neighbors,
            'counts':{'functions':35,'code_bytes':8600,'source_units':1,'rooted_direct_call_entries':23,
                      'closed_return_bodies':32,'reviewed_leaf_tail_functions':3,'reviewed_named_math_globals':6}}
