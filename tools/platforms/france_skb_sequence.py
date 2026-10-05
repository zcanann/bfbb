"""Original-only French SKB animation completion with a typed polynomial.

The complete four-member original TU and both established neighbors provide
identity. No compiled implementation supplies names, sizes, or array contents.
"""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow
from platforms.france_tu_sequences import KIND, compare, digest, unique_template, words
from platforms.ps2_source import canonical_linkages
from platforms.dwarf1 import iter_dies

SOURCE = 'SB/Core/p2/iAnimSKB.cpp'
START = 1722160


def polynomial_declaration(original):
    section = next(s for s in original.metadata['sections'] if s['name'] == '.debug' and s['size'])
    dies = list(iter_dies(original.data[section['offset']:section['offset'] + section['size']]))
    by_offset = {off: (tag, owner, attrs) for off, tag, owner, attrs in dies}
    declarations = [(off, attrs) for off, tag, owner, attrs in dies if tag == 7 and
                    owner.replace('\\', '/').endswith(SOURCE) and attrs.get(3) == 'slerpPolynomial']
    require(len(declarations) == 1, 'SKB polynomial original declaration is ambiguous')
    die, attrs = declarations[0]
    location = attrs.get(2)
    require(isinstance(location, bytes) and len(location) == 5 and location[0] == 3,
            'SKB polynomial lacks an original absolute location')
    type_die = attrs.get(7)
    require(type_die in by_offset, 'SKB polynomial type is absent')
    tag, owner, typ = by_offset[type_die]
    descriptor = bytes.fromhex('000a0000000000170000000855000e00')
    require(tag == 1 and typ.get(9) == 0 and typ.get(10) == descriptor,
            'SKB polynomial must be the original zero-based row-major 24-float array')
    return {'name': 'slerpPolynomial', 'reference_address': int.from_bytes(location[1:], 'little'),
            'size': 96, 'elements': 24, 'element_fundamental_type': 14,
            'declaration_die': die, 'type_die': type_die, 'subscript_descriptor': descriptor.hex()}


def generate_unit(originals, registry_dir):
    target = originals[TARGET]
    neighbors = []
    for filename in ('corroborated-functions.json', 'relocation-corroborated-functions.json'):
        neighbors.extend(f for f in json.loads((registry_dir / filename).read_text())['functions'] if f['source'] == SOURCE)
    require(len(neighbors) == 2 and sum(f['size'] for f in neighbors) == 2704,
            'Expected two previously confirmed SKB functions')
    expected = {(f['address'], f['name'], f['size']) for f in neighbors}
    for f in neighbors:
        require(f['boundary_confirmation'] and digest(target.read(f['address'], f['size'])) == f['sha256'],
                'Established SKB neighbor bytes changed')
    functions, sequences, data_proofs = {}, [], []
    for version in REFERENCES:
        original = originals[version]
        members = sorted((f for f in original.functions if f['source'] == SOURCE), key=lambda f: f['low'])
        require([(f['name'], f['high'] - f['low']) for f in members] ==
                [('_iAnimSKBExtractTranslate', 1464), ('_iAnimSKBAdjustTranslate', 1240),
                 ('iAnimDurationSKB', 32), ('iAnimEvalSKB', 1480)], 'Original SKB full membership changed')
        low, end = members[0]['low'], members[-1]['high']
        require(end - low == 4232, 'Original SKB placement differs')
        require({(START + f['low'] - low, f['name'], f['high'] - f['low']) for f in members[:2]} == expected,
                'Both established SKB neighbor placements must agree')
        # Include actual following alignment words, rather than synthesizing zero bytes.
        flow = ControlFlow({START + i * 4: w for i, w in enumerate(words(target.read(START, end - low + 16)))})
        ref_flow = ControlFlow({low + i * 4: w for i, w in enumerate(words(original.read(low, end - low + 16)))})
        linkages = canonical_linkages(original.data, original.metadata)
        masks = {}
        for i, f in enumerate(members):
            a, size, name = f['low'], f['high'] - f['low'], f['name']
            b = START + a - low
            require(a % 16 == b % 16 == 0 and a in linkages, 'SKB original alignment or linkage missing')
            if i:
                gap = a - members[i - 1]['high']
                require(0 <= gap < 16 and not any(original.read(a - gap, gap)) and not any(target.read(b - gap, gap)),
                        'Nonzero or excessive alignment interrupts SKB sequence')
            pairs, calls, mask = compare(original, target, a, b, size)
            require(not calls, 'Reviewed SKB bodies must have no direct calls or tails')
            masks.update({a - low + off: value for off, value in mask.items()})
            bounds = flow.bounds(b, size)
            require(bounds['passes'] and ref_flow.bounds(a, size)['passes'], 'SKB strict CFG/frame proof fails')
            if name == 'iAnimEvalSKB':
                require(len(pairs) == 1, 'SKB Eval must have exactly one changed address producer')
                pair = pairs[0]
                require((pair['hi_offset'], pair['lo_offset'], pair['opcode'], pair['storage']) ==
                        (504, 508, 9, 'file_backed'), 'SKB polynomial LUI/ADDIU address formation differs')
                declaration = polynomial_declaration(original)
                require(pair['reference_address'] == declaration['reference_address'], 'Changed pointer is not original slerpPolynomial')
                for binary, address in ((original, pair['reference_address']), (target, pair['target_address'])):
                    region = binary._stream_regions['initialized_data']
                    require(region['address'] <= address and address + 96 <= region['address'] + region['size'],
                            'SKB polynomial exceeds authenticated initialized data')
                require(original.read(pair['reference_address'], 96) == target.read(pair['target_address'], 96),
                        'Complete original SKB polynomial contents differ')
                data_proofs.append({'version': version, **declaration, 'target_address': pair['target_address'],
                                    'sha256': digest(target.read(pair['target_address'], 96))})
            else:
                require(not pairs, 'Unexpected SKB changed data operand')
            if i < 2:
                continue
            if b not in functions:
                functions[b] = {'name': name, 'source': SOURCE, 'address': b, 'size': size,
                                'sha256': digest(target.read(b, size)), 'boundary_confirmation': True,
                                'confirmation_kind': KIND, 'provenance': [],
                                'corroboration': {'local_control_flow': bounds,
                                                  'previously_confirmed_neighbor_entries': sorted(f['address'] for f in neighbors)}}
            record = functions[b]
            require(record['size'] == size and (not record['provenance'] or
                    record['provenance'][0]['linkage_name'] == linkages[a]), 'SKB original identities disagree')
            record['provenance'].append({'version': version, 'executable_sha1': original.sha1, 'source_address': a,
                                         'name': name, 'source': SOURCE, 'linkage_name': linkages[a],
                                         'reference_sha256': digest(original.read(a, size)),
                                         'data_address_operands': pairs, 'direct_transfers': []})
        sequences.append({'version': version, 'source': SOURCE, 'source_start': low, 'target_start': START,
                          'complete_original_members': 4, 'previously_confirmed_members': 2,
                          'sequence_bytes_with_alignment': end - low,
                          'uniqueness': unique_template(target, original.read(low, end - low), masks, START)})
    require(len(functions) == 2 and all(len(f['provenance']) == 3 for f in functions.values()),
            'Both SKB functions need all three originals')
    require(len({p['target_address'] for p in data_proofs}) == 1, 'Originals disagree on French polynomial address')
    return {'functions': sorted(functions.values(), key=lambda f: f['address']),
            'sequence_proofs': sequences, 'data_proofs': data_proofs, 'call_neighbors': [],
            'counts': {'functions': 2, 'code_bytes': 1512, 'source_units': 1,
                       'closed_return_bodies': 2, 'reviewed_skb_polynomials': 1}}
