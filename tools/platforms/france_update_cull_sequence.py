"""Original-only French update-culling sequence and nonpromoted caller contexts."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow
from platforms.france_tu_sequences import KIND, compare, digest, unique_template, words
from platforms.ps2_source import canonical_linkages

SOURCE = 'SB/Core/x/xUpdateCull.cpp'
START = 3314512
MISSING = [('xUpdateCull_Init', 1484), ('xUpdateCull_DistanceSquaredCB', 128),
           ('xUpdateCull_AlwaysTrueCB', 8)]


def checked_identity(original, target, reference_address, target_address, known):
    """Recheck full original identity and bytes of an independently reviewed entry."""
    ref, record = original.by_address.get(reference_address), known.get(target_address)
    require(ref is not None and record is not None and
            (record.get('boundary_confirmation') or record.get('identity_confirmation')),
            'External entry lacks independently reviewed identity')
    require((ref['name'], ref['source'], ref['high'] - ref['low']) ==
            (record['name'], record['source'], record['size']) and
            digest(target.read(target_address, record['size'])) == record['sha256'],
            'External original identity or target body changed')
    prior = [p for p in record['provenance'] if p['version'] == original.version]
    require(len(prior) == 1 and prior[0]['source_address'] == reference_address and
            prior[0]['executable_sha1'] == original.sha1 and
            prior[0]['reference_sha256'] == digest(original.read(reference_address, record['size'])),
            f'External original provenance or body changed: {original.version} {ref["name"]} {reference_address:#x}')
    return record


def group_count_context(original, target, address, destination, known):
    count = original.by_address[address]
    require((count['name'], count['source'], count['high'] - address) ==
            ('xGroupGetCount', 'SB/Core/x/xGroup.cpp', 12), 'Unexpected group getter')
    matches = [f for f in original.functions if f['source'] == count['source'] and f['name'] == 'xGroupGetItemPtr']
    require(len(matches) == 1, 'Original group accessor is ambiguous')
    previous = matches[0]
    require(previous['high'] - previous['low'] == 200 and address - previous['high'] == 8,
            'Group getter must follow the complete 200-byte accessor and eight alignment bytes')
    start = destination - (address - previous['low'])
    neighbor = known.get(start)
    require(neighbor is not None and neighbor['boundary_confirmation'] and
            (neighbor['name'],neighbor['source'],neighbor['size']) == ('xGroupGetItemPtr',count['source'],200) and
            digest(target.read(start,200)) == neighbor['sha256'], 'Independent accessor identity changed')
    # The prior exact-body registry has USA/PAL provenance only. German is
    # independently re-proved below by this complete sequence and named call.
    if any(p['version'] == original.version for p in neighbor['provenance']):
        checked_identity(original,target,previous['low'],start,known)
    length = count['high'] - previous['low']
    pairs, calls, masks = compare(original, target, previous['low'], start, length)
    require(length == 220 and not pairs and len(calls) == 1, 'Group context operands differ')
    call = calls[0]
    require(call['offset'] == 88 and call['opcode'] == 3, 'Group context call offset/opcode differs')
    callee = checked_identity(original, target, call['reference_address'], call['target_address'], known)
    require((callee['name'], callee['source'], callee['size']) ==
            ('zSceneFindObject', 'SB/Game/zScene.cpp', 128), 'Group context callee differs')
    require(not any(original.read(previous['high'], 8)) and not any(target.read(destination - 8, 8)),
            'Group context alignment is not zero')
    for binary, base in ((original, previous['low']), (target, start)):
        flow = ControlFlow({base + 4*i: w for i, w in enumerate(words(binary.read(base, length + 16)))})
        require(flow.bounds(base, 200)['passes'] and flow.bounds(base + 208, 12)['passes'],
                'Group context strict CFG is not closed')
    return {'previous_named_neighbor': neighbor['name'], 'reference_start': previous['low'],
            'target_start': start, 'sequence_bytes': length,
            'reference_sha256': digest(original.read(previous['low'], length)),
            'target_sha256': digest(target.read(start, length)),
            'uniqueness': unique_template(target, original.read(previous['low'], length), masks, start),
            'checked_direct_call': call, 'no_progress_or_new_anchor': True}


def generate_unit(originals, registry_dir):
    target = originals[TARGET]
    known = {}
    for filename in ('corroborated-functions.json', 'relocation-corroborated-functions.json'):
        for f in json.loads((registry_dir / filename).read_text())['functions']:
            require(f['address'] not in known, 'Duplicate independent function')
            known[f['address']] = f
    neighbors = sorted((f for f in known.values() if f['source'] == SOURCE), key=lambda f: f['address'])
    require(len(neighbors) == 4 and sum(f['size'] for f in neighbors) == 1420,
            'Expected four previously confirmed update-cull neighbors')
    expected = {(f['address'], f['name'], f['size']) for f in neighbors}
    reviewed = json.loads((registry_dir / 'reviewed-call-targets.json').read_text())
    require(reviewed['executable_sha1'] == target.sha1, 'Allocator registry targets another original')
    for f in reviewed['anchors']:
        require(f['eligible_for_progress'] is False and f['identity_confirmation'] and
                digest(target.read(f['address'], f['size'])) == f['sha256'], 'Allocator context identity changed')
        known.setdefault(f['address'], f)
    anchors = [f for f in reviewed['anchors'] if f['name'] == 'xMemPopTemp']
    require(len(anchors) == 1, 'Expected one independent allocator-neighborhood anchor')
    anchor = anchors[0]
    neighborhood = anchor['corroboration']['allocator_neighborhood']
    require(neighborhood['size'] == 360 and
            digest(target.read(neighborhood['address'], 360)) == neighborhood['sha256'],
            'Reviewed allocator neighborhood changed')
    functions, sequences, contexts, data_proofs = {}, [], [], []
    for version in REFERENCES:
        original = originals[version]
        members = sorted((f for f in original.functions if f['source'] == SOURCE), key=lambda f: f['low'])
        require(len(members) == 7 and sum(f['high'] - f['low'] for f in members) == 3040,
                'Complete original update-cull membership changed')
        low, end = members[0]['low'], members[-1]['high']
        require(end - low == 3060, 'Original update-cull sequence span changed')
        missing = [f for f in members if START + f['low'] - low not in known]
        require([(f['name'], f['high'] - f['low']) for f in missing] == MISSING, 'Missing update-cull membership differs')
        require({(START + f['low'] - low, f['name'], f['high'] - f['low']) for f in members if f not in missing} == expected,
                'All four independent neighbors must agree on complete placement')
        for f in members:
            if f not in missing:
                checked_identity(original, target, f['low'], START + f['low'] - low, known)
        witness = [p for p in anchor['provenance'] if p['version'] == version]
        require(len(witness) == 1, 'Allocator context original provenance is ambiguous')
        witness = witness[0]
        checked_identity(original, target, witness['source_address'], anchor['address'], known)
        require(witness['executable_sha1'] == original.sha1 and
                digest(original.read(witness['neighborhood_address'], 360)) == witness['neighborhood_sha256'],
                'Original allocator neighborhood changed')
        context_pairs, _, _ = compare(original, target, witness['neighborhood_address'], neighborhood['address'], 360)
        slot_map = {}
        for pair in context_pairs:
            if pair['storage'] == 'zero_fill' and pair['opcode'] == 35:
                ra, tb = pair['reference_address'], pair['target_address']
                require(ra not in slot_map or slot_map[ra] == tb, 'Allocator slots map inconsistently')
                slot_map[ra] = tb
        require(len(set(slot_map.values())) == len(slot_map), 'Distinct allocator slots collapse')
        entries = {f['low']: f for f in members}
        flow = ControlFlow({START + 4*i: w for i,w in enumerate(words(target.read(START, end-low+16)))})
        ref_flow = ControlFlow({low + 4*i: w for i,w in enumerate(words(original.read(low, end-low+16)))})
        linkages = canonical_linkages(original.data, original.metadata)
        masks, callees = {}, {}
        for i, f in enumerate(members):
            a, size, name = f['low'], f['high']-f['low'], f['name']
            b = START + a-low
            require(a % 16 == b % 16 == 0 and a in linkages, 'Original alignment or linkage missing')
            if i:
                gap = a-members[i-1]['high']
                require(0 <= gap < 16 and not any(original.read(a-gap, gap)) and not any(target.read(b-gap, gap)),
                        'Unexpected update-cull alignment bytes')
            pairs, calls, mask = compare(original, target, a, b, size)
            require(not pairs or (name == 'xUpdateCull_Init' and len(pairs) == 5), 'Unreviewed data address change')
            if pairs:
                require([(p['hi_offset'],p['lo_offset'],p['opcode']) for p in pairs] ==
                        [(72,76,35),(200,216,35),(780,784,9),(1248,1256,9),(1384,1388,9)],
                        'Initializer address-producer positions changed')
            for pair in pairs:
                ra, tb = pair['reference_address'], pair['target_address']
                if pair['storage'] == 'zero_fill':
                    require(slot_map.get(ra) == tb, 'Pointer slot lacks independent allocator context')
                    for binary, addr in ((original,ra),(target,tb)):
                        region = binary._stream_regions['runtime_bss']
                        require(region['address'] <= addr and addr+4 <= region['address']+region['size'],
                                'Allocator pointer slot exceeds original BSS')
                    pair['scope'] = 'Independently reviewed allocator-neighborhood slot; no name or extent promoted'
                else:
                    require(ra in entries and entries[ra]['name'] == 'xUpdateCull_AlwaysTrueCB' and tb == START+ra-low,
                            'Callback producer is not the actual complete-sequence entry')
                    pair['scope'] = 'Complete-sequence original AlwaysTrue callback entry'
                data_proofs.append({'version':version,'function_address':b,**pair,'data_extent_promoted':False})
            masks.update({a-low+k:v for k,v in mask.items()})
            bounds = flow.bounds(b,size)
            require(bounds['passes'] and ref_flow.bounds(a,size)['passes'], 'Strict update-cull CFG/frame proof fails')
            for call in calls:
                ra,tb = call['reference_address'],call['target_address']
                require(ra not in callees or callees[ra] == tb, 'Callee relationship is inconsistent')
                callees[ra] = tb
                if low <= ra < end:
                    require(ra in entries and tb == START+ra-low, 'Internal transfer changes complete member ownership')
                elif ra == tb and tb in (1149520,1149960):
                    require(original.word(a+call['offset']) == target.word(b+call['offset']) and
                            original.read(ra,64) == target.read(tb,64), 'Opaque runtime literal/context differs')
                    masks.pop(a-low+call['offset'],None)
                    call.update(opaque_context_prefix_sha256=digest(target.read(tb,64)),
                                opaque_context_bytes=64,no_identity_or_extent_claim=True,transfer_word_unmasked=True)
                elif original.by_address.get(ra,{}).get('name') == 'xGroupGetCount':
                    call['group_count_context'] = group_count_context(original,target,ra,tb,known)
                elif original.by_address.get(ra,{}).get('name') == 'xGroupGetItemPtr':
                    call['group_count_context'] = group_count_context(original,target,ra+208,tb+208,known)
                else:
                    checked_identity(original,target,ra,tb,known)
                if not low <= ra < end:
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
                    record['provenance'][0]['linkage_name'] == linkages[a]), 'Original function identities disagree')
            record['provenance'].append({'version':version,'executable_sha1':original.sha1,'source_address':a,
                'name':name,'source':SOURCE,'linkage_name':linkages[a],
                'reference_sha256':digest(original.read(a,size)),'data_address_operands':pairs,'direct_transfers':calls})
        require(len(set(callees.values())) == len(callees), 'Distinct original callees collapse')
        sequences.append({'version':version,'source':SOURCE,'source_start':low,'target_start':START,
            'complete_original_members':7,'previously_confirmed_members':4,'sequence_bytes_with_alignment':end-low,
            'uniqueness':unique_template(target,original.read(low,end-low),masks,START)})
    require(len(functions) == 3 and all(len(f['provenance']) == 3 for f in functions.values()),
            'All three new entries require all three original witnesses')
    return {'functions':sorted(functions.values(),key=lambda f:f['address']), 'sequence_proofs':sequences,
            'call_neighbors':contexts,'data_proofs':data_proofs,
            'counts':{'functions':3,'code_bytes':1620,'source_units':1,'closed_return_bodies':3}}
