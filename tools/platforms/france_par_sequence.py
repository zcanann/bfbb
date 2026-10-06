"""Original-only French particle initializer completion and typed BSS pool context."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow
from platforms.france_tu_sequences import KIND, compare, digest, unique_template, words
from platforms.ps2_source import canonical_linkages
from platforms.dwarf1 import iter_dies

SOURCE = 'SB/Core/x/xPar.cpp'
START = 2049872


def pool_declaration(original):
    section = next(s for s in original.metadata['sections'] if s['name'] == '.debug' and s['size'])
    dies = list(iter_dies(original.data[section['offset']:section['offset'] + section['size']]))
    index = {off: (tag, owner, attrs) for off, tag, owner, attrs in dies}
    declarations = [(off, attrs) for off, tag, owner, attrs in dies if tag == 7 and
                    owner.replace('\\', '/').endswith(SOURCE) and attrs.get(3) == 'gParPool']
    require(len(declarations) == 1, 'Particle pool original declaration is ambiguous')
    die, attrs = declarations[0]
    location = attrs.get(2)
    require(isinstance(location, bytes) and len(location) == 5 and location[0] == 3,
            'Particle pool lacks original absolute location')
    require(attrs.get(7) in index, 'Particle pool original type is missing')
    tag, owner, typ = index[attrs[7]]
    descriptor = typ.get(10)
    require(tag == 1 and typ.get(9) == 0 and isinstance(descriptor, bytes) and len(descriptor) == 18 and
            descriptor[:-4] == bytes.fromhex('000a0000000000cf070000087200'),
            'Particle pool is not the original zero-based row-major 2000-element array')
    element_die = int.from_bytes(descriptor[-4:], 'little')
    require(element_die in index, 'Particle pool element declaration is missing')
    tag, owner, element = index[element_die]
    require(tag == 2 and element.get(3) == 'xPar' and element.get(11) == 96,
            'Particle pool element is not original 96-byte xPar')
    return {'name': 'gParPool', 'reference_address': int.from_bytes(location[1:], 'little'),
            'declaration_die': die, 'type_die': attrs[7], 'element_die': element_die,
            'subscript_descriptor': descriptor.hex(), 'elements': 2000, 'element_bytes': 96,
            'size': 192000, 'context_only': True, 'data_extent_promoted': False}


def generate_unit(originals, registry_dir):
    target = originals[TARGET]
    neighbors = []
    for filename in ('corroborated-functions.json', 'relocation-corroborated-functions.json'):
        neighbors.extend(f for f in json.loads((registry_dir / filename).read_text())['functions'] if f['source'] == SOURCE)
    require(len(neighbors) == 3 and sum(f['size'] for f in neighbors) == 276,
            'Expected three previously confirmed particle functions')
    expected = {(f['address'], f['name'], f['size']) for f in neighbors}
    for f in neighbors:
        require(f['boundary_confirmation'] and digest(target.read(f['address'], f['size'])) == f['sha256'],
                'Established particle neighbor bytes changed')
    functions, sequences, data_proofs = {}, [], []
    for version in REFERENCES:
        original = originals[version]
        members = sorted((f for f in original.functions if f['source'] == SOURCE), key=lambda f: f['low'])
        require([(f['name'], f['high'] - f['low']) for f in members] ==
                [('xParInit', 128), ('xParFree', 84), ('xParAlloc', 64), ('xParMemInit', 76)],
                'Original particle full membership changed')
        low, end = members[0]['low'], members[-1]['high']
        require(end - low == 364, 'Original particle placement differs')
        require({(START + f['low'] - low, f['name'], f['high'] - f['low']) for f in members[:3]} == expected,
                'All three established particle neighbor placements must agree')
        flow = ControlFlow({START + i * 4: w for i, w in enumerate(words(target.read(START, end - low + 16)))})
        ref_flow = ControlFlow({low + i * 4: w for i, w in enumerate(words(original.read(low, end - low + 16)))})
        linkages = canonical_linkages(original.data, original.metadata)
        masks = {}
        for i, f in enumerate(members):
            a, size, name = f['low'], f['high'] - f['low'], f['name']
            b = START + a - low
            require(a % 16 == b % 16 == 0 and a in linkages, 'Particle alignment or original linkage missing')
            if i:
                gap = a - members[i - 1]['high']
                require(0 <= gap < 16 and not any(original.read(a - gap, gap)) and not any(target.read(b - gap, gap)),
                        'Nonzero or excessive alignment interrupts particle sequence')
            pairs, calls, mask = compare(original, target, a, b, size)
            require(not calls, 'Reviewed particle bodies must have no direct calls or tails')
            masks.update({a - low + off: value for off, value in mask.items()})
            bounds = flow.bounds(b, size)
            require(bounds['passes'] and ref_flow.bounds(a, size)['passes'], 'Particle strict CFG/frame proof fails')
            if i < 3:
                require(not pairs, 'Established particle member has an unexpected address change')
                continue
            require(len(pairs) == 1, 'Particle initializer must have exactly one changed address producer')
            pair = pairs[0]
            require((pair['hi_offset'], pair['lo_offset'], pair['opcode'], pair['storage']) == (0, 8, 9, 'zero_fill'),
                    'Particle pool LUI/ADDIU formation differs')
            declaration = pool_declaration(original)
            require(pair['reference_address'] == declaration['reference_address'], 'Changed pointer is not original gParPool')
            for binary, address in ((original, pair['reference_address']), (target, pair['target_address'])):
                region = binary._stream_regions['runtime_bss']
                require(region['address'] <= address and address + declaration['size'] <= region['address'] + region['size'],
                        'Particle pool exceeds authenticated runtime BSS')
            data_proofs.append({'version': version, **declaration, 'target_address': pair['target_address']})
            if b not in functions:
                functions[b] = {'name': name, 'source': SOURCE, 'address': b, 'size': size,
                                'sha256': digest(target.read(b, size)), 'boundary_confirmation': True,
                                'confirmation_kind': KIND, 'provenance': [],
                                'corroboration': {'local_control_flow': bounds,
                                                  'previously_confirmed_neighbor_entries': sorted(f['address'] for f in neighbors)}}
            record = functions[b]
            require(not record['provenance'] or record['provenance'][0]['linkage_name'] == linkages[a],
                    'Particle original identities disagree')
            record['provenance'].append({'version': version, 'executable_sha1': original.sha1, 'source_address': a,
                                         'name': name, 'source': SOURCE, 'linkage_name': linkages[a],
                                         'reference_sha256': digest(original.read(a, size)),
                                         'data_address_operands': pairs, 'direct_transfers': []})
        sequences.append({'version': version, 'source': SOURCE, 'source_start': low, 'target_start': START,
                          'complete_original_members': 4, 'previously_confirmed_members': 3,
                          'sequence_bytes_with_alignment': end - low,
                          'uniqueness': unique_template(target, original.read(low, end - low), masks, START)})
    require(len(functions) == 1 and all(len(f['provenance']) == 3 for f in functions.values()),
            'Particle initializer needs all three originals')
    require(len({p['target_address'] for p in data_proofs}) == 1, 'Originals disagree on French particle pool address')
    return {'functions': list(functions.values()), 'sequence_proofs': sequences,
            'data_proofs': data_proofs, 'call_neighbors': [],
            'counts': {'functions': 1, 'code_bytes': 76, 'source_units': 1,
                       'closed_return_bodies': 1, 'reviewed_particle_pool_arrays': 1}}
