"""Original-only French bounds TU completion and typed quick-cull global context."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow
from platforms.france_tu_sequences import KIND, compare, digest, unique_template, words
from platforms.france_update_cull_sequence import checked_identity
from platforms.ps2_source import canonical_linkages
from platforms.dwarf1 import iter_dies

SOURCE = 'SB/Core/x/xBound.cpp'
START = 1837712
MISSING = [('xBoundDraw',8),('xVecHitsBound',276),('xSphereHitsBound',132),
           ('xBoundHitsBound',400),('xBoundGetSphere',600),('xBoundUpdate',436)]


def control_declaration(original, address):
    section = next(s for s in original.metadata['sections'] if s['name'] == '.debug' and s['size'])
    dies = list(iter_dies(original.data[section['offset']:section['offset']+section['size']]))
    index = {off:(tag,owner,attrs) for off,tag,owner,attrs in dies}
    candidates = [(off,attrs) for off,tag,owner,attrs in dies if tag in (7,12) and
                  attrs.get(3) == 'xqc_def_ctrl' and owner.replace('\\','/').endswith(SOURCE)]
    require(len(candidates) == 1, 'Original quick-cull declaration is ambiguous')
    die,attrs = candidates[0]
    location = attrs.get(2)
    require(isinstance(location,bytes) and len(location) == 5 and location[0] == 3 and
            int.from_bytes(location[1:],'little') == address and attrs.get(7) in index,
            'Quick-cull address disagrees with its original typed declaration')
    tag,owner,typ = index[attrs[7]]
    require(tag == 2 and typ.get(3) == 'xQCControl' and typ.get(11) == 60,
            'Quick-cull global is not original 60-byte xQCControl')
    return {'name':'xqc_def_ctrl','declaration_die':die,'type_die':attrs[7],
            'type':'xQCControl','size':60,'reference_address':address,'data_extent_promoted':False}


def generate_unit(originals, registry_dir):
    target = originals[TARGET]
    known = {}
    for filename in ('corroborated-functions.json','relocation-corroborated-functions.json','tu-corroborated-functions.json'):
        for f in json.loads((registry_dir/filename).read_text())['functions']:
            # This module must not rely on its own output during regeneration.
            if filename == 'tu-corroborated-functions.json' and f['source'] == SOURCE:
                continue
            if f['address'] in known:
                prior = known[f['address']]
                require(all(prior[key] == f[key] for key in ('name','source','size','sha256')),
                        'Repeated independently reviewed function disagrees')
            known[f['address']] = f
    # Some older exact-body geometry records have only one/two reference versions.
    # Replay the already reviewed complete iMath3 sequence before using those
    # original identities as call contexts; do not rewrite their public records.
    from platforms.france_imath3_sequence import generate_unit as generate_imath3
    dependency = generate_imath3(originals,registry_dir)
    for proof in dependency['sequence_proofs']:
        original = originals[proof['version']]
        for f in original.functions:
            if f['source'] != 'SB/Core/p2/iMath3.cpp':
                continue
            address = proof['target_start'] + f['low'] - proof['source_start']
            record = known.get(address)
            require(record is not None and
                    (record['name'],record['source'],record['size']) == (f['name'],f['source'],f['high']-f['low']) and
                    digest(target.read(address,record['size'])) == record['sha256'],
                    'Replayed complete geometry dependency disagrees with prior identity')
            if not any(p['version'] == original.version for p in record['provenance']):
                record = {**record,'provenance':list(record['provenance'])}
                record['provenance'].append({'version':original.version,'source_address':f['low'],
                    'executable_sha1':original.sha1,'reference_sha256':digest(original.read(f['low'],record['size'])),
                    'context_only':'Independently replayed complete platform-geometry sequence'})
                known[address] = record
    neighbors = sorted((f for f in known.values() if f['source'] == SOURCE),key=lambda f:f['address'])
    require(len(neighbors) == 3 and sum(f['size'] for f in neighbors) == 1396,
            'Expected three independently reviewed bounds neighbors')
    expected = {(f['address'],f['name'],f['size']) for f in neighbors}
    functions,sequences,contexts,data_proofs = {},[],[],[]
    for version in REFERENCES:
        original = originals[version]
        members = sorted((f for f in original.functions if f['source'] == SOURCE),key=lambda f:f['low'])
        require(len(members) == 9 and sum(f['high']-f['low'] for f in members) == 3248,
                'Complete original bounds membership differs')
        low,end = members[0]['low'],members[-1]['high']
        missing = [f for f in members if START+f['low']-low not in known]
        require([(f['name'],f['high']-f['low']) for f in missing] == MISSING, 'Unexpected missing bounds members')
        require({(START+f['low']-low,f['name'],f['high']-f['low']) for f in members if f not in missing} == expected,
                'All independent bounds neighbors must agree on placement')
        for f in members:
            if f not in missing:
                checked_identity(original,target,f['low'],START+f['low']-low,known)
        flow = ControlFlow({START+4*i:w for i,w in enumerate(words(target.read(START,end-low+16)))})
        ref_flow = ControlFlow({low+4*i:w for i,w in enumerate(words(original.read(low,end-low+16)))})
        linkages = canonical_linkages(original.data,original.metadata)
        masks,callees = {},{}
        entries = {f['low'] for f in members}
        for i,f in enumerate(members):
            a,size,name = f['low'],f['high']-f['low'],f['name']
            b = START+a-low
            require(a%16 == b%16 == 0 and a in linkages, 'Bounds alignment or original linkage missing')
            if i:
                gap = a-members[i-1]['high']
                require(0 <= gap < 16 and not any(original.read(a-gap,gap)) and not any(target.read(b-gap,gap)),
                        'Bounds sequence contains unexpected alignment')
            pairs,calls,mask = compare(original,target,a,b,size)
            if name == 'xBoundUpdate':
                require(len(pairs) == 1, 'Expected one quick-cull address producer')
                pair = pairs[0]
                require((pair['hi_offset'],pair['lo_offset'],pair['opcode'],pair['storage']) == (408,420,9,'zero_fill'),
                        'Quick-cull producer differs')
                declaration = control_declaration(original,pair['reference_address'])
                for binary,addr in ((original,pair['reference_address']),(target,pair['target_address'])):
                    region = binary._stream_regions['runtime_bss']
                    require(region['address'] <= addr and addr+60 <= region['address']+region['size'],
                            'Quick-cull object exceeds original BSS')
                data_proofs.append({'version':version,**declaration,'target_address':pair['target_address']})
            else:
                require(not pairs, 'Unreviewed bounds data-address difference')
            masks.update({a-low+off:value for off,value in mask.items()})
            bounds = flow.bounds(b,size)
            require(bounds['passes'] and ref_flow.bounds(a,size)['passes'], 'Bounds strict CFG/frame proof fails')
            for call in calls:
                ra,tb = call['reference_address'],call['target_address']
                require(ra not in callees or callees[ra] == tb, 'Inconsistent bounds callee mapping')
                callees[ra] = tb
                if low <= ra < end:
                    require(ra in entries and tb == START+ra-low, 'Internal bounds call changes member ownership')
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
            require(record['size'] == size and (not record['provenance'] or record['provenance'][0]['linkage_name'] == linkages[a]),
                    'Original bounds identities disagree')
            record['provenance'].append({'version':version,'executable_sha1':original.sha1,'source_address':a,
                'name':name,'source':SOURCE,'linkage_name':linkages[a],
                'reference_sha256':digest(original.read(a,size)),'data_address_operands':pairs,'direct_transfers':calls})
        require(len(set(callees.values())) == len(callees), 'Distinct bounds callees collapse')
        sequences.append({'version':version,'source':SOURCE,'source_start':low,'target_start':START,
            'complete_original_members':9,'previously_confirmed_members':3,'sequence_bytes_with_alignment':end-low,
            'uniqueness':unique_template(target,original.read(low,end-low),masks,START)})
    require(len(functions) == 6 and all(len(f['provenance']) == 3 for f in functions.values()),
            'Six missing bounds entries require all three original witnesses')
    require(len({p['target_address'] for p in data_proofs}) == 1, 'Originals disagree on quick-cull global')
    return {'functions':sorted(functions.values(),key=lambda f:f['address']),'sequence_proofs':sequences,
            'call_neighbors':contexts,'data_proofs':data_proofs,
            'counts':{'functions':6,'code_bytes':1852,'source_units':1,'closed_return_bodies':6}}
