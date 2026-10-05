"""Original-only French xGrid completion with full neighboring-TU context.

Only xGridCheckPosition is newly named. Its eleven independently corroborated
neighbors remain context, with unchanged extents. No compiled objects are read.
"""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow
from platforms.france_tu_sequences import KIND, compare, digest, unique_template, words
from platforms.ps2_source import canonical_linkages
from platforms.dwarf1 import iter_dies

SOURCE = 'SB/Core/x/xGrid.cpp'
START = 0x304a00
SIZE = 1984


def original_array(original):
    section = next(s for s in original.metadata['sections'] if s['name'] == '.debug' and s['size'])
    rows = list(iter_dies(original.data[section['offset']:section['offset'] + section['size']]))
    by = {off: (tag, owner, attrs) for off, tag, owner, attrs in rows}
    declarations = [(off, a) for off, tag, owner, a in rows if tag in (7, 12) and
                    owner.replace('\\', '/').endswith(SOURCE) and a.get(3) == 'offs']
    require(len(declarations) == 1, 'xGrid offs declaration is ambiguous')
    declaration, attrs = declarations[0]
    location = attrs.get(2)
    require(isinstance(location, bytes) and len(location) == 5 and location[0] == 3,
            'xGrid offs lacks an original absolute location')
    die, chain = attrs.get(7), []
    for index, count in enumerate((4, 3, 2)):
        require(die in by, 'xGrid offs array type is absent')
        tag, owner, attrs = by[die]
        desc = attrs.get(10)
        prefix = bytes.fromhex('000a0000000000') + (count - 1).to_bytes(4, 'little') + b'\x08'
        require(tag == 1 and attrs.get(9) == 0 and isinstance(desc, bytes) and desc.startswith(prefix),
                'xGrid offs zero-based bound or row-major order differs')
        chain.append({'die': die, 'subscript_descriptor': desc.hex()})
        if index < 2:
            require(len(desc) == 18 and desc[12:14] == b'\x72\x00',
                    'xGrid offs nested-array reference differs')
            die = int.from_bytes(desc[14:], 'little')
        else:
            # DWARF1 FT_signed_integer, the PS2 ABI's four-byte signed int.
            require(desc[12:] == b'\x55\x00\x08\x00',
                    'xGrid offs is not a signed 32-bit integer array')
    return {'name': 'offs', 'reference_address': int.from_bytes(location[1:], 'little'),
            'size': 96, 'dimensions': [4, 3, 2], 'element_fundamental_type': 8,
            'declaration_die': declaration, 'type_chain': chain}


def generate_unit(originals, registry_dir):
    target = originals[TARGET]
    confirmed = []
    for filename in ('corroborated-functions.json', 'relocation-corroborated-functions.json'):
        confirmed.extend(json.loads((registry_dir / filename).read_text())['functions'])
    known = {f['address']: f for f in confirmed}
    neighbors = sorted((f for f in known.values() if f['source'] == SOURCE), key=lambda f: f['address'])
    require(len(neighbors) == 11 and not any(f['address'] == START for f in neighbors),
            'Expected exactly eleven pre-existing independent xGrid neighbors')
    for f in neighbors:
        require(f['boundary_confirmation'] and digest(target.read(f['address'], f['size'])) == f['sha256'],
                'Existing xGrid neighbor bytes or boundary status changed')
    functions, sequences, data_proofs, call_neighbors = [], [], [], []
    expected_neighbors = {(f['address'], f['name'], f['size']) for f in neighbors}
    for version in REFERENCES:
        original = originals[version]
        members = sorted((f for f in original.functions if f['source'] == SOURCE), key=lambda f: f['low'])
        require(len(members) == 12 and sum(f['high'] - f['low'] for f in members) == 5732 and
                members[0]['name'] == 'xGridCheckPosition' and members[0]['high'] - members[0]['low'] == SIZE,
                'Original complete xGrid membership differs')
        low, end = members[0]['low'], members[-1]['high']
        require(end - low == 5772, 'Original complete xGrid placement differs')
        require({(START + f['low'] - low, f['name'], f['high'] - f['low']) for f in members[1:]} == expected_neighbors,
                'All eleven original neighbor placements must agree')
        target_span = target.read(START, end - low + 4)
        ref_span = original.read(low, end - low + 4)
        flow = ControlFlow({START + i * 4: w for i, w in enumerate(words(target_span))})
        ref_flow = ControlFlow({low + i * 4: w for i, w in enumerate(words(ref_span))})
        linkages = canonical_linkages(original.data, original.metadata)
        masks, callees, new_proof = {}, {}, None
        entries = {f['low'] for f in members}
        for index, f in enumerate(members):
            a, size, name = f['low'], f['high'] - f['low'], f['name']
            b = START + a - low
            require(a % 16 == b % 16 == 0 and a in linkages, 'xGrid entry alignment or canonical identity differs')
            if index:
                gap = a - members[index - 1]['high']
                require(0 <= gap < 16 and not any(original.read(a - gap, gap)) and not any(target.read(b - gap, gap)),
                        'Non-padding bytes interrupt the xGrid sequence')
            pairs, calls, mask = compare(original, target, a, b, size)
            masks.update({a - low + off: value for off, value in mask.items()})
            bounds = flow.bounds(b, size)
            require(bounds['passes'] and ref_flow.bounds(a, size)['passes'], 'xGrid local CFG does not strictly close')
            for call in calls:
                ra, rb = call['reference_address'], call['target_address']
                require(ra not in callees or callees[ra] == rb, 'One xGrid callee maps inconsistently')
                callees[ra] = rb
            if index:
                require(not pairs, 'Existing xGrid neighbor gained an unreviewed data-address difference')
                continue
            require(len(pairs) == 1 and len(calls) == 4, 'xGrid candidate operand inventory changed')
            pair = pairs[0]
            require((pair['hi_offset'], pair['lo_offset'], pair['opcode'], pair['storage']) ==
                    (732, 740, 9, 'file_backed'), 'xGrid offs address producer changed')
            array = original_array(original)
            require(pair['reference_address'] == array['reference_address'], 'xGrid changed pointer is not named offs')
            for binary, address in ((original, pair['reference_address']), (target, pair['target_address'])):
                region = binary._stream_regions['initialized_data']
                require(region['address'] <= address and address + 96 <= region['address'] + region['size'],
                        'Complete xGrid array exceeds authenticated initialized data')
            require(original.read(array['reference_address'], 96) == target.read(pair['target_address'], 96),
                    'Complete xGrid offs contents differ')
            data_proofs.append({'version': version, **array, 'target_address': pair['target_address'],
                                'sha256': digest(target.read(pair['target_address'], 96))})
            for call in calls:
                rf = original.by_address.get(call['reference_address'])
                tf = known.get(call['target_address'])
                require(rf is not None and tf is not None and
                        (rf['name'], rf['source'], rf['high'] - rf['low']) == (tf['name'], tf['source'], tf['size']) and
                        digest(target.read(tf['address'], tf['size'])) == tf['sha256'],
                        'xGrid candidate call lacks an independently named target')
                # These two known callees contain no direct transfers or data-address differences.
                require(original.read(rf['low'], tf['size']) == target.read(tf['address'], tf['size']),
                        'xGrid candidate callee complete body changed')
            new_proof = {'version': version, 'executable_sha1': original.sha1, 'source_address': a,
                         'name': name, 'source': SOURCE, 'linkage_name': linkages[a],
                         'reference_sha256': digest(original.read(a, size)),
                         'data_address_operands': pairs, 'direct_transfers': calls}
            if not functions:
                functions.append({'name': name, 'source': SOURCE, 'address': b, 'size': size,
                                  'sha256': digest(target.read(b, size)), 'boundary_confirmation': True,
                                  'confirmation_kind': KIND, 'provenance': [],
                                  'corroboration': {'local_control_flow': bounds,
                                                    'previously_confirmed_neighbor_entries': [f['address'] for f in neighbors]}})
            require(functions[0]['name'] == name and (not functions[0]['provenance'] or
                    functions[0]['provenance'][0]['linkage_name'] == linkages[a]), 'xGrid canonical identity differs')
            functions[0]['provenance'].append(new_proof)
        require(len(set(callees.values())) == len(callees), 'Distinct xGrid callees collapse')
        for a, b in sorted(callees.items()):
            if low <= a < end:
                require(a in entries and b == START + a - low, 'Internal xGrid call changes entry ownership')
                continue
            neighbor = original.by_address.get(a)
            size = min(32, neighbor['high'] - a) if neighbor else 32
            pairs, calls, _ = compare(original, target, a, b, size)
            call_neighbors.append({'version': version, 'reference_address': a, 'target_address': b,
                                   'reference_name': neighbor['name'] if neighbor else None,
                                   'reference_source': neighbor['source'] if neighbor else None,
                                   'compared_entry_prefix_bytes': size,
                                   'reference_sha256': digest(original.read(a, size)),
                                   'target_sha256': digest(target.read(b, size)),
                                   'data_address_operands': pairs, 'direct_transfers': calls,
                                   'promoted_as_named_anchor': False})
        sequences.append({'version': version, 'source': SOURCE, 'source_start': low, 'target_start': START,
                          'complete_original_members': 12, 'previously_confirmed_members': 11,
                          'sequence_bytes_with_alignment': end - low,
                          'uniqueness': unique_template(target, original.read(low, end - low), masks, START),
                          'candidate_uniqueness': unique_template(target, original.read(low, SIZE),
                                                                {k: v for k, v in masks.items() if k < SIZE}, START)})
    groups = {}
    for n in call_neighbors:
        groups.setdefault(n['target_address'], []).append(n)
    for group in groups.values():
        require({n['version'] for n in group} == set(REFERENCES) and len(group) == 3 and
                len({(n['reference_name'], n['reference_source'], n['compared_entry_prefix_bytes']) for n in group}) == 1,
                'xGrid external context identities differ across originals')
    return {'functions': functions, 'sequence_proofs': sequences, 'data_proofs': data_proofs,
            'call_neighbors': call_neighbors,
            'counts': {'functions': 1, 'code_bytes': SIZE, 'source_units': 1,
                       'closed_return_bodies': 1, 'reviewed_grid_arrays': 1}}
