"""Original-only completion of the two French QuickCull initializers.

Nine previously corroborated functions locate the complete eleven-member TU.
The 28-byte overload has an explicitly checked tail transfer; generic CFG
acceptance remains unchanged. No compiled candidate supplies names or bounds.
"""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow
from platforms.france_tu_sequences import KIND, compare, digest, unique_template, words
from platforms.ps2_source import canonical_linkages

SOURCE = 'SB/Core/x/xQuickCull.cpp'
START = 2074816


def tail_wrapper(binary, address, destination):
    code = words(binary.read(address, 28))
    loads = {0: (13, 16), 4: (14, 20), 8: (15, 0),
             12: (16, 4), 16: (17, 8), 24: (12, 12)}
    require(all(code[off // 4] == ((49 << 26) | (5 << 21) | (reg << 16) | imm)
                for off, (reg, imm) in loads.items()),
            'QuickCull wrapper is not the six reviewed LWC1 argument loads')
    require(code[5] >> 26 == 2 and
            (((address + 24) & 0xf0000000) | ((code[5] & 0x3ffffff) << 2)) == destination == address + 32,
            'QuickCull wrapper must tail-jump to the adjacent full initializer')
    return {'method': 'Exact six LWC1 words and terminal J with LWC1 delay slot',
            'tail_instruction_offset': 20, 'tail_target': destination,
            'stack_and_return_address_unchanged': True,
            'argument_loads': [{'offset': off, 'base_register': 5, 'float_register': reg,
                                'displacement': imm} for off, (reg, imm) in loads.items()]}


def generate_unit(originals, registry_dir):
    target = originals[TARGET]
    known = {}
    for filename in ('corroborated-functions.json', 'relocation-corroborated-functions.json'):
        for f in json.loads((registry_dir / filename).read_text())['functions']:
            require(f['address'] not in known, 'Duplicate independent French function')
            known[f['address']] = f
    neighbors = sorted((f for f in known.values() if f['source'] == SOURCE), key=lambda f: f['address'])
    require(len(neighbors) == 9 and sum(f['size'] for f in neighbors) == 1956,
            'Expected nine independently established QuickCull neighbors')
    expected_neighbors = {(f['address'], f['name'], f['size']) for f in neighbors}
    for f in neighbors:
        require(f['boundary_confirmation'] and digest(target.read(f['address'], f['size'])) == f['sha256'],
                'QuickCull neighbor bytes or confirmed boundary changed')
    functions, sequences, contexts = {}, [], []
    for version in REFERENCES:
        original = originals[version]
        members = sorted((f for f in original.functions if f['source'] == SOURCE), key=lambda f: f['low'])
        require(len(members) == 11 and sum(f['high'] - f['low'] for f in members) == 2236,
                'Complete original QuickCull membership changed')
        low, end = members[0]['low'], members[-1]['high']
        require(end - low == 2300, 'Original QuickCull placement changed')
        missing = [f for f in members if START + f['low'] - low not in known]
        require([(f['name'], f['high'] - f['low']) for f in missing] ==
                [('xQuickCullInit', 28), ('xQuickCullInit', 252)], 'Only the two Init overloads may be added')
        require({(START + f['low'] - low, f['name'], f['high'] - f['low']) for f in members if f not in missing}
                == expected_neighbors, 'All nine independent neighbor placements must agree')
        flow = ControlFlow({START + 4 * i: w for i, w in enumerate(words(target.read(START, end - low + 4)))})
        ref_flow = ControlFlow({low + 4 * i: w for i, w in enumerate(words(original.read(low, end - low + 4)))})
        linkages = canonical_linkages(original.data, original.metadata)
        entries = {f['low'] for f in members}
        masks, callees = {}, {}
        for i, f in enumerate(members):
            a, size, name = f['low'], f['high'] - f['low'], f['name']
            b = START + a - low
            require(a % 16 == b % 16 == 0 and a in linkages, 'QuickCull entry alignment or original linkage missing')
            if i:
                gap = a - members[i - 1]['high']
                require(0 <= gap < 16 and not any(original.read(a - gap, gap)) and not any(target.read(b - gap, gap)),
                        'QuickCull alignment contains nonzero or excessive data')
            pairs, calls, mask = compare(original, target, a, b, size)
            require(not pairs, 'QuickCull gained an unreviewed data-address difference')
            masks.update({a - low + off: value for off, value in mask.items()})
            bounds, ref_bounds = flow.bounds(b, size), ref_flow.bounds(a, size)
            corroboration = {'local_control_flow': bounds,
                             'previously_confirmed_neighbor_entries': [n['address'] for n in neighbors]}
            if size == 28 and name == 'xQuickCullInit':
                require(i + 1 < len(members) and members[i + 1] == missing[1] and
                        members[i + 1]['low'] == a + 32, 'Tail destination is not the full original Init overload')
                reasons = ['extent_not_terminal_return_delay', 'local_edge_outside_extent', 'no_return']
                require(bounds['reasons'] == ref_bounds['reasons'] == reasons,
                        'Unexpected QuickCull wrapper control-flow rejection')
                tail_wrapper(original, a, a + 32)
                corroboration['scoped_terminal_tail'] = tail_wrapper(target, b, b + 32)
                require(flow.bounds(b + 32, 252)['passes'] and ref_flow.bounds(a + 32, 252)['passes'],
                        'QuickCull tail destination must independently return with balanced frame')
            else:
                require(bounds['passes'] and ref_bounds['passes'], 'QuickCull ordinary member CFG is not closed')
            for call in calls:
                ra, tb = call['reference_address'], call['target_address']
                require(ra not in callees or callees[ra] == tb, 'QuickCull callee mapping is inconsistent')
                callees[ra] = tb
                if low <= ra < end:
                    require(ra in entries and tb == START + ra - low, 'Internal QuickCull transfer changes member ownership')
                else:
                    rf, tf = original.by_address.get(ra), known.get(tb)
                    require(rf is not None and tf is not None and tf['boundary_confirmation'] and
                            (rf['name'], rf['source'], rf['high'] - rf['low']) == (tf['name'], tf['source'], tf['size']) and
                            digest(target.read(tb, tf['size'])) == tf['sha256'], 'QuickCull external callee lacks independent identity')
                    contexts.append({'version': version, 'reference_address': ra, 'target_address': tb,
                                     'reference_name': rf['name'], 'reference_source': rf['source'],
                                     'size': tf['size'], 'target_sha256': tf['sha256'],
                                     'promoted_as_named_anchor': False})
            if f not in missing:
                continue
            if b not in functions:
                functions[b] = {'name': name, 'source': SOURCE, 'address': b, 'size': size,
                                'sha256': digest(target.read(b, size)), 'boundary_confirmation': True,
                                'confirmation_kind': KIND, 'provenance': [], 'corroboration': corroboration}
            record = functions[b]
            require(record['size'] == size and (not record['provenance'] or
                    record['provenance'][0]['linkage_name'] == linkages[a]), 'QuickCull original overload identities disagree')
            record['provenance'].append({'version': version, 'executable_sha1': original.sha1, 'source_address': a,
                                         'name': name, 'source': SOURCE, 'linkage_name': linkages[a],
                                         'reference_sha256': digest(original.read(a, size)),
                                         'data_address_operands': [], 'direct_transfers': calls})
        require(len(set(callees.values())) == len(callees), 'Distinct QuickCull callees collapse')
        sequences.append({'version': version, 'source': SOURCE, 'source_start': low, 'target_start': START,
                          'complete_original_members': 11, 'previously_confirmed_members': 9,
                          'sequence_bytes_with_alignment': end - low,
                          'uniqueness': unique_template(target, original.read(low, end - low), masks, START)})
    require(len(functions) == 2 and all(len(f['provenance']) == 3 for f in functions.values()),
            'Both QuickCull overloads need all three original witnesses')
    return {'functions': sorted(functions.values(), key=lambda f: f['address']),
            'sequence_proofs': sequences, 'call_neighbors': contexts,
            'counts': {'functions': 2, 'code_bytes': 280, 'source_units': 1,
                       'closed_return_bodies': 1, 'reviewed_tail_wrappers': 1}}
