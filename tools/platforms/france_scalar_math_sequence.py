"""Original-only completion of the French scalar-math TU, with two scoped tails."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow
from platforms.france_tu_sequences import KIND, compare, digest, unique_template, words
from platforms.france_update_cull_sequence import checked_identity
from platforms.ps2_source import canonical_linkages

SOURCE = 'SB/Core/x/xMath.cpp'
START = 2021088
MISSING = [('xFuncPiece_ShiftPiece',284),('xFuncPiece_EndPoints',60),('xAccelStop',336),
           ('xAccelMove',312),('xAccelMoveTime',120),('xAccelMove',744),('xsrand',8),('xatof',8),('xMathExit',24)]
OPAQUE_CONTEXT = (1133296,1133344,1133248,1132176,1132848)


def endpoint_tail(original, target, reference, address, callee):
    """Only the exact original 60-byte EndPoints wrapper can use this rule."""
    expected = (0x460c6801,0x3c033f80,0x24020001,0x0080282d,0xe4800014,
                0xc4810014,0x44831000,0x460e7801,0x46011043,0xac820018,
                0x46010002,0xe48e0000,0x46006307,None,0xe4800004)
    require(callee['name'] == 'xFuncPiece_ShiftPiece' and callee['high']-callee['low'] == 284 and
            callee['high']+4 == reference, 'Original EndPoints tail callee placement changed')
    for binary,entry,dest in ((original,reference,callee['low']),
                              (target,address,callee['low']+address-reference)):
        code = words(binary.read(entry,60))
        require(all(value is None or code[i] == value for i,value in enumerate(expected)),
                'EndPoints non-transfer instruction semantics changed')
        require(code[13] >> 26 == 2 and (((entry+56)&0xf0000000)|((code[13]&0x3ffffff)<<2)) == dest,
                'EndPoints tail does not reach its independently bounded ShiftPiece')
        require(binary.read(entry+60,4) == bytes(4), 'EndPoints alignment changed')
        flow = ControlFlow({dest+4*i:w for i,w in enumerate(words(binary.read(dest,288)))})
        require(flow.bounds(dest,284)['passes'], 'ShiftPiece destination fails unchanged strict CFG')
    return {'passes':True,'boundary_kind':'reviewed-exact-original-60-byte-leaf-tail',
            'all14_nontransfer_words_exact':True,'unchanged_sp_ra':True,
            'terminal_instruction_offset':52,'tail_target_name':callee['name'],
            'tail_target_address':callee['low']+address-reference,'callee_size':284,
            'callee_closed_in_both_originals':True,'following_zero_alignment':4}


def generate_unit(originals, registry_dir):
    target = originals[TARGET]
    known = {}
    for filename in ('corroborated-functions.json','relocation-corroborated-functions.json'):
        for f in json.loads((registry_dir/filename).read_text())['functions']:
            require(f['address'] not in known, 'Duplicate independent scalar-math record')
            known[f['address']] = f
    neighbors = sorted((f for f in known.values() if f['source'] == SOURCE),key=lambda f:f['address'])
    require(len(neighbors) == 9 and sum(f['size'] for f in neighbors) == 1804,
            'Expected nine independent scalar-math neighbors')
    expected = {(f['address'],f['name'],f['size']) for f in neighbors}
    functions,sequences,contexts = {},[],[]
    for version in REFERENCES:
        original = originals[version]
        members = sorted((f for f in original.functions if f['source'] == SOURCE),key=lambda f:f['low'])
        require(len(members) == 18 and sum(f['high']-f['low'] for f in members) == 3700,
                'Complete original scalar-math membership changed')
        low,end = members[0]['low'],members[-1]['high']
        require(end-low == 3808, 'Complete scalar-math placement changed')
        missing = [f for f in members if START+f['low']-low not in known]
        require([(f['name'],f['high']-f['low']) for f in missing] == MISSING, 'Unexpected missing math members')
        require({(START+f['low']-low,f['name'],f['high']-f['low']) for f in members if f not in missing} == expected,
                'All nine independent neighbors must corroborate whole-TU placement')
        for f in members:
            if f not in missing:
                checked_identity(original,target,f['low'],START+f['low']-low,known)
        flow = ControlFlow({START+4*i:w for i,w in enumerate(words(target.read(START,end-low+16)))})
        ref_flow = ControlFlow({low+4*i:w for i,w in enumerate(words(original.read(low,end-low+16)))})
        linkages = canonical_linkages(original.data,original.metadata)
        entries = {f['low'] for f in members}
        masks,callees = {},{}
        for i,f in enumerate(members):
            a,size,name = f['low'],f['high']-f['low'],f['name']
            b = START+a-low
            require(a%16 == b%16 == 0 and a in linkages, 'Original math alignment or linkage missing')
            if i:
                gap = a-members[i-1]['high']
                require(0 <= gap < 16 and not any(original.read(a-gap,gap)) and not any(target.read(b-gap,gap)),
                        'Unexpected math alignment bytes')
            pairs,calls,mask = compare(original,target,a,b,size)
            require(not pairs, 'Unreviewed scalar-math data producer')
            masks.update({a-low+off:value for off,value in mask.items()})
            bounds = flow.bounds(b,size)
            if name == 'xatof':
                code = words(original.read(a,size))
                require(size == 8 and code[0] >> 26 == 2 and code[1] == 0 and
                        original.read(a,size) == target.read(b,size) and len(calls) == 1 and
                        calls[0]['offset'] == 0 and calls[0]['opcode'] == 2,
                        'xatof is not the original unchanged eight-byte J/NOP tail')
                runtime = calls[0]['reference_address']
                require(runtime == calls[0]['target_address'] and original.read(runtime,64) == target.read(runtime,64),
                        'Opaque xatof runtime destination context changed')
                masks.pop(a-low,None)
                bounds = {'passes':True,'boundary_kind':'reviewed-literal-original-j-nop-tail',
                          'original_named_extent':8,'literal_transfer_unmasked':True,'unchanged_sp_ra':True,
                          'opaque_context_sha256':digest(target.read(runtime,64)),
                          'no_runtime_identity_or_extent_claim':True}
            elif name == 'xFuncPiece_EndPoints':
                require(size == 60 and len(calls) == 1 and calls[0]['offset'] == 52 and calls[0]['opcode'] == 2,
                        'Unexpected EndPoints tail extent or transfers')
                matches = [m for m in members if m['name'] == 'xFuncPiece_ShiftPiece']
                require(len(matches) == 1, 'Original EndPoints tail callee is ambiguous')
                bounds = endpoint_tail(original,target,a,b,matches[0])
            else:
                require(bounds['passes'] and ref_flow.bounds(a,size)['passes'], 'Strict scalar-math CFG/frame proof fails')
            for call in calls:
                ra,tb = call['reference_address'],call['target_address']
                require(ra not in callees or callees[ra] == tb, 'Math callee mapping is inconsistent')
                callees[ra] = tb
                if low <= ra < end:
                    require(ra in entries and tb == START+ra-low, 'Internal math transfer changes named member ownership')
                elif name == 'xatof' or ra == tb and tb in OPAQUE_CONTEXT:
                    require(ra == tb and original.word(a+call['offset']) == target.word(b+call['offset']) and
                            original.read(ra,64) == target.read(tb,64), 'Opaque math runtime context changed')
                    masks.pop(a-low+call['offset'],None)
                    call.update(opaque_context_prefix_sha256=digest(target.read(tb,64)),opaque_context_bytes=64,
                                no_identity_or_extent_claim=True,transfer_word_unmasked=True)
                    contexts.append({'version':version,'function_address':b,**call,'promoted_as_named_anchor':False})
                else:
                    checked_identity(original,target,ra,tb,known)
                    contexts.append({'version':version,'function_address':b,**call,'promoted_as_named_anchor':False})
            if f not in missing:
                continue
            if b not in functions:
                functions[b] = {'name':name,'source':SOURCE,'address':b,'size':size,
                    'sha256':digest(target.read(b,size)),'boundary_confirmation':True,'confirmation_kind':KIND,
                    'provenance':[],'corroboration':{'local_control_flow':bounds,
                    'previously_confirmed_neighbor_entries':[n['address'] for n in neighbors]}}
            record = functions[b]
            require(record['size'] == size and (not record['provenance'] or
                    record['provenance'][0]['linkage_name'] == linkages[a]), 'Original math identities disagree')
            record['provenance'].append({'version':version,'executable_sha1':original.sha1,'source_address':a,
                'name':name,'source':SOURCE,'linkage_name':linkages[a],
                'reference_sha256':digest(original.read(a,size)),'data_address_operands':[],'direct_transfers':calls})
        require(len(set(callees.values())) == len(callees), 'Distinct math callees collapse')
        require(len(masks) == 3, 'Only three internal math transfers may be masked')
        sequences.append({'version':version,'source':SOURCE,'source_start':low,'target_start':START,
            'complete_original_members':18,'previously_confirmed_members':9,'sequence_bytes_with_alignment':end-low,
            'uniqueness':unique_template(target,original.read(low,end-low),masks,START)})
    require(len(functions) == 9 and all(len(f['provenance']) == 3 for f in functions.values()),
            'All nine new scalar-math functions require all three originals')
    return {'functions':sorted(functions.values(),key=lambda f:f['address']),'sequence_proofs':sequences,
            'call_neighbors':contexts,'counts':{'functions':9,'code_bytes':1896,'source_units':1,
                                              'closed_return_bodies':7,'reviewed_scalar_math_tails':2}}
