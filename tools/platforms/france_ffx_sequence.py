"""Original-only French FFX completion using independent caller context.

Only seven missing FFX entries are added. Existing allocator call anchors stay
ineligible for progress, and two unchanged opaque math destinations remain
unnamed context with their JAL words unmasked in the full-TU comparison.
"""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow
from platforms.france_tu_sequences import KIND, compare, digest, unique_template, words
from platforms.ps2_source import canonical_linkages

SOURCE = 'SB/Core/x/xFFX.cpp'
START = 1945120
OPAQUE_CONTEXT = (1133272, 1132848)
MISSING = [('xFFXShakeFree', 16), ('xFFXShakeAlloc', 40), ('xFFXShakeUpdateEnt', 348),
           ('xFFXAddEffect', 40), ('xFFXTurnOff', 20), ('xFFXTurnOn', 16), ('xFFXAlloc', 40)]


def jal_destination(binary, address):
    word = binary.word(address)
    require(word >> 26 == 3, 'Independent FFX allocator witness is not JAL')
    return ((address + 4) & 0xf0000000) | ((word & 0x3ffffff) << 2)


def allocator_witnesses(originals, registry_dir, records):
    target = originals[TARGET]
    document = json.loads((registry_dir / 'reviewed-call-targets.json').read_text())
    require(document['executable_sha1'] == target.sha1, 'Existing allocator anchor targets another executable')
    anchors = [a for a in document['anchors'] if a['name'] == 'xMemAlloc']
    require(len(anchors) == 1, 'Expected one existing allocator call anchor')
    anchor = anchors[0]
    require(anchor['identity_confirmation'] and anchor['eligible_for_progress'] is False and
            digest(target.read(anchor['address'], anchor['size'])) == anchor['sha256'],
            'Existing allocator anchor identity or bytes changed')
    result = {}
    for f in records:
        if f['source'] == SOURCE:
            continue
        for provenance in f['provenance']:
            version = provenance['version']
            original = originals[version]
            caller = original.by_address.get(provenance['source_address'])
            for call in provenance.get('transfer_differences', []):
                if call.get('reference_callee', {}).get('name') != 'xMemAlloc':
                    continue
                require(caller is not None and
                        (caller['name'], caller['source'], caller['high'] - caller['low']) ==
                        (f['name'], f['source'], f['size']) and
                        provenance['executable_sha1'] == original.sha1 and f['boundary_confirmation'] and
                        digest(original.read(caller['low'], f['size'])) == provenance['reference_sha256'] and
                        digest(target.read(f['address'], f['size'])) == f['sha256'],
                        'Independent allocator caller identity or full body changed')
                offset = call['offset']
                require(offset % 4 == 0 and 0 <= offset <= f['size'] - 4 and call['opcode'] == 3,
                        'Allocator caller witness offset/opcode differs')
                ra = jal_destination(original, caller['low'] + offset)
                ta = jal_destination(target, f['address'] + offset)
                require((ra, ta) == (call['reference_target'], call['french_target']) and ta == anchor['address'],
                        'Recorded allocator witness disagrees with actual original JAL destinations')
                callee = original.by_address.get(ra)
                require(callee is not None and
                        (callee['name'], callee['source'], callee['high'] - callee['low']) ==
                        (anchor['name'], anchor['source'], anchor['size']), 'Original allocator identity disagrees')
                prior = [p for p in anchor['provenance'] if p['version'] == version]
                require(len(prior) == 1 and prior[0]['source_address'] == ra and
                        prior[0]['executable_sha1'] == original.sha1 and
                        digest(original.read(ra, anchor['size'])) == prior[0]['reference_sha256'],
                        'Allocator original body no longer agrees with the existing anchor')
                result.setdefault((version, ra, ta), []).append({
                    'caller': f['name'], 'source': f['source'], 'reference_address': caller['low'],
                    'target_address': f['address'], 'size': f['size'], 'offset': offset,
                    'reference_sha256': provenance['reference_sha256'], 'target_sha256': f['sha256'],
                    'actual_original_and_target_jal_decoded': True})
    for key, rows in result.items():
        require(len({w['source'] for w in rows}) >= 2,
                'Allocator mapping needs at least two independent non-FFX source callers')
    require({key[0] for key in result} == set(REFERENCES), 'Allocator witnesses must cover all three originals')
    return result


def generate_unit(originals, registry_dir):
    target = originals[TARGET]
    known = {}
    for filename in ('corroborated-functions.json', 'relocation-corroborated-functions.json'):
        for f in json.loads((registry_dir / filename).read_text())['functions']:
            require(f['address'] not in known, 'Duplicate independent French function')
            known[f['address']] = f
    relocation = json.loads((registry_dir / 'relocation-corroborated-functions.json').read_text())['functions']
    witnesses = allocator_witnesses(originals, registry_dir, relocation)
    neighbors = sorted((f for f in known.values() if f['source'] == SOURCE), key=lambda f: f['address'])
    require(len(neighbors) == 7 and sum(f['size'] for f in neighbors) == 1068,
            'Expected seven independently confirmed FFX neighbors')
    expected = {(f['address'], f['name'], f['size']) for f in neighbors}
    for f in neighbors:
        require(f['boundary_confirmation'] and digest(target.read(f['address'], f['size'])) == f['sha256'],
                'FFX neighbor identity or bytes changed')
    functions, sequences, contexts = {}, [], []
    for version in REFERENCES:
        original = originals[version]
        members = sorted((f for f in original.functions if f['source'] == SOURCE), key=lambda f: f['low'])
        require(len(members) == 14 and sum(f['high'] - f['low'] for f in members) == 1588,
                'Original complete FFX membership changed')
        low, end = members[0]['low'], members[-1]['high']
        require(end - low == 1680, 'Original complete FFX placement changed')
        missing = [f for f in members if START + f['low'] - low not in known]
        require([(f['name'], f['high'] - f['low']) for f in missing] == MISSING, 'Unexpected missing FFX entries')
        require({(START + f['low'] - low, f['name'], f['high'] - f['low']) for f in members if f not in missing} == expected,
                'All seven original FFX neighbor placements must agree')
        flow = ControlFlow({START + 4 * i: w for i, w in enumerate(words(target.read(START, end - low + 16)))})
        ref_flow = ControlFlow({low + 4 * i: w for i, w in enumerate(words(original.read(low, end - low + 16)))})
        linkages = canonical_linkages(original.data, original.metadata)
        entries = {f['low'] for f in members}
        masks, callee_map = {}, {}
        for i, f in enumerate(members):
            a, size, name = f['low'], f['high'] - f['low'], f['name']
            b = START + a - low
            require(a % 16 == b % 16 == 0 and a in linkages, 'FFX alignment or original canonical linkage missing')
            if i:
                gap = a - members[i - 1]['high']
                require(0 <= gap < 16 and not any(original.read(a - gap, gap)) and not any(target.read(b - gap, gap)),
                        'FFX alignment contains nonzero or excessive bytes')
            pairs, calls, mask = compare(original, target, a, b, size)
            require(not pairs, 'FFX has an unreviewed data address difference')
            masks.update({a - low + off: value for off, value in mask.items()})
            bounds = flow.bounds(b, size)
            require(bounds['passes'] and ref_flow.bounds(a, size)['passes'], 'FFX strict CFG/frame proof fails')
            for call in calls:
                ra, tb = call['reference_address'], call['target_address']
                require(ra not in callee_map or callee_map[ra] == tb, 'FFX callee mapping is inconsistent')
                callee_map[ra] = tb
                if low <= ra < end:
                    require(ra in entries and tb == START + ra - low, 'FFX internal call changes member ownership')
                elif (version, ra, tb) in witnesses:
                    call['prior_independent_caller_witnesses'] = witnesses[(version, ra, tb)]
                    call['context_only'] = True
                elif ra == tb and tb in OPAQUE_CONTEXT:
                    require(original.word(a + call['offset']) == target.word(b + call['offset']) and
                            original.read(ra, 64) == target.read(tb, 64), 'Opaque FFX callee bytes or literal JAL differ')
                    masks.pop(a - low + call['offset'], None)
                    call['opaque_context_prefix_sha256'] = digest(target.read(tb, 64))
                    call['opaque_context_bytes'] = 64
                    call['no_identity_or_extent_claim'] = True
                    call['transfer_word_unmasked'] = True
                else:
                    rf, tf = original.by_address.get(ra), known.get(tb)
                    require(rf is not None and tf is not None and tf['boundary_confirmation'] and
                            (rf['name'], rf['source'], rf['high'] - rf['low']) == (tf['name'], tf['source'], tf['size']) and
                            digest(target.read(tb, tf['size'])) == tf['sha256'], 'FFX external callee is unconfirmed')
                if not low <= ra < end:
                    contexts.append({'version': version, 'function_address': b, **call, 'promoted_as_named_anchor': False})
            if f not in missing:
                continue
            if b not in functions:
                functions[b] = {'name': name, 'source': SOURCE, 'address': b, 'size': size,
                                'sha256': digest(target.read(b, size)), 'boundary_confirmation': True,
                                'confirmation_kind': KIND, 'provenance': [],
                                'corroboration': {'local_control_flow': bounds,
                                                  'previously_confirmed_neighbor_entries': [n['address'] for n in neighbors]}}
            record = functions[b]
            require(record['size'] == size and (not record['provenance'] or
                    record['provenance'][0]['linkage_name'] == linkages[a]), 'FFX original identities disagree')
            record['provenance'].append({'version': version, 'executable_sha1': original.sha1, 'source_address': a,
                                         'name': name, 'source': SOURCE, 'linkage_name': linkages[a],
                                         'reference_sha256': digest(original.read(a, size)),
                                         'data_address_operands': [], 'direct_transfers': calls})
        require(len(set(callee_map.values())) == len(callee_map), 'Distinct FFX callees collapse')
        sequences.append({'version': version, 'source': SOURCE, 'source_start': low, 'target_start': START,
                          'complete_original_members': 14, 'previously_confirmed_members': 7,
                          'sequence_bytes_with_alignment': end - low,
                          'uniqueness': unique_template(target, original.read(low, end - low), masks, START)})
    require(len(functions) == 7 and all(len(f['provenance']) == 3 for f in functions.values()),
            'All seven missing FFX functions require three original witnesses')
    return {'functions': sorted(functions.values(), key=lambda f: f['address']),
            'sequence_proofs': sequences, 'call_neighbors': contexts,
            'counts': {'functions': 7, 'code_bytes': 520, 'source_units': 1, 'closed_return_bodies': 7}}
