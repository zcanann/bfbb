"""Original-only completion of the French platform geometry TU.

All fifteen named original members participate in placement/identity. Opaque
runtime calls stay literal instructions; no runtime symbol or extent is invented.
"""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow
from platforms.france_tu_sequences import KIND, compare, digest, unique_template, words
from platforms.ps2_source import canonical_linkages

SOURCE = 'SB/Core/p2/iMath3.cpp'
START = 1750992
MEMBERS = [('iBoxBoundVec', 220), ('iBoxInitBoundVec', 52), ('iBoxIsectSphere', 884),
           ('iBoxIsectRay', 652), ('ClipPlane', 216), ('iBoxIsectVec', 136),
           ('iBoxVecDist', 2568), ('iCylinderIsectVec', 128), ('iSphereBoundVec', 316),
           ('iSphereInitBoundVec', 40), ('iSphereIsectSphere', 124), ('iSphereIsectRay', 492),
           ('iSphereIsectVec', 100), ('iMath3Exit', 8), ('iMath3Init', 8)]


def generate_unit(originals, registry_dir):
    target = originals[TARGET]
    known = {}
    for filename in ('corroborated-functions.json', 'relocation-corroborated-functions.json'):
        document = json.loads((registry_dir / filename).read_text())
        require(document['executable_sha1'] == target.sha1, 'Geometry neighbor registry targets another executable')
        for f in document['functions']:
            require(f['address'] not in known, 'Duplicate independent geometry neighbor')
            known[f['address']] = f
    neighbors = sorted((f for f in known.values() if f['source'] == SOURCE), key=lambda f: f['address'])
    require(len(neighbors) == 6 and sum(f['size'] for f in neighbors) == 1760,
            'Expected six previously confirmed geometry neighbors')
    expected = {(f['address'], f['name'], f['size']) for f in neighbors}
    for f in neighbors:
        require(f['boundary_confirmation'] and digest(target.read(f['address'], f['size'])) == f['sha256'],
                'Established geometry neighbor bytes changed')
    functions, sequences, contexts = {}, [], []
    for version in REFERENCES:
        original = originals[version]
        members = sorted((f for f in original.functions if f['source'] == SOURCE), key=lambda f: f['low'])
        require([(f['name'], f['high'] - f['low']) for f in members] == MEMBERS,
                'Original complete geometry membership changed')
        low, end = members[0]['low'], members[-1]['high']
        require(end - low == 6040, 'Original complete geometry placement changed')
        missing = [f for f in members if START + f['low'] - low not in known]
        require(len(missing) == 9 and sum(f['high'] - f['low'] for f in missing) == 4184,
                'Unexpected missing geometry entries')
        require({(START + f['low'] - low, f['name'], f['high'] - f['low']) for f in members if f not in missing} == expected,
                'All six original geometry neighbor placements must agree')
        flow = ControlFlow({START + 4 * i: w for i, w in enumerate(words(target.read(START, end - low + 16)))})
        ref_flow = ControlFlow({low + 4 * i: w for i, w in enumerate(words(original.read(low, end - low + 16)))})
        linkages = canonical_linkages(original.data, original.metadata)
        entries = {f['low'] for f in members}
        masks, callee_map = {}, {}
        opaque_calls = 0
        for i, f in enumerate(members):
            a, size, name = f['low'], f['high'] - f['low'], f['name']
            b = START + a - low
            require(a % 16 == b % 16 == 0 and a in linkages, 'Geometry alignment or original linkage missing')
            if i:
                gap = a - members[i - 1]['high']
                require(0 <= gap < 16 and not any(original.read(a - gap, gap)) and not any(target.read(b - gap, gap)),
                        'Geometry alignment contains nonzero or excessive bytes')
            pairs, calls, mask = compare(original, target, a, b, size)
            require(not pairs, 'Geometry has an unreviewed data-address difference')
            masks.update({a - low + off: value for off, value in mask.items()})
            bounds = flow.bounds(b, size)
            require(bounds['passes'] and ref_flow.bounds(a, size)['passes'], 'Geometry strict CFG/frame proof fails')
            for call in calls:
                ra, tb = call['reference_address'], call['target_address']
                require(ra not in callee_map or callee_map[ra] == tb, 'Geometry callee mapping is inconsistent')
                callee_map[ra] = tb
                if low <= ra < end:
                    require(ra in entries and tb == START + ra - low, 'Geometry internal call changes member ownership')
                elif ra == tb == 1149520:
                    require(name == 'iSphereBoundVec' and call['opcode'] == 3 and
                            original.word(a + call['offset']) == target.word(b + call['offset']) and
                            original.read(ra, 64) == target.read(tb, 64), 'Opaque geometry callee bytes or literal JAL differ')
                    masks.pop(a - low + call['offset'], None)
                    call['opaque_context_prefix_sha256'] = digest(target.read(tb, 64))
                    call['opaque_context_bytes'] = 64
                    call['no_identity_or_extent_claim'] = True
                    call['transfer_word_unmasked'] = True
                    opaque_calls += 1
                else:
                    rf, tf = original.by_address.get(ra), known.get(tb)
                    require(rf is not None and tf is not None and tf['boundary_confirmation'] and
                            (rf['name'], rf['source'], rf['high'] - rf['low']) == (tf['name'], tf['source'], tf['size']) and
                            digest(target.read(tb, tf['size'])) == tf['sha256'], 'Geometry external callee is unconfirmed')
                    witnesses = [p for p in tf['provenance'] if p['version'] == version]
                    require(len(witnesses) == 1 and witnesses[0]['source_address'] == ra and
                            witnesses[0]['executable_sha1'] == original.sha1 and
                            digest(original.read(ra, tf['size'])) == witnesses[0]['reference_sha256'],
                            'Geometry external original callee provenance differs')
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
                    record['provenance'][0]['linkage_name'] == linkages[a]), 'Geometry original identities disagree')
            record['provenance'].append({'version': version, 'executable_sha1': original.sha1, 'source_address': a,
                                         'name': name, 'source': SOURCE, 'linkage_name': linkages[a],
                                         'reference_sha256': digest(original.read(a, size)),
                                         'data_address_operands': [], 'direct_transfers': calls})
        require(opaque_calls == 2, 'Expected exactly two unchanged opaque geometry calls')
        require(len(set(callee_map.values())) == len(callee_map), 'Distinct geometry callees collapse')
        sequences.append({'version': version, 'source': SOURCE, 'source_start': low, 'target_start': START,
                          'complete_original_members': 15, 'previously_confirmed_members': 6,
                          'sequence_bytes_with_alignment': end - low,
                          'uniqueness': unique_template(target, original.read(low, end - low), masks, START)})
    require(len(functions) == 9 and all(len(f['provenance']) == 3 for f in functions.values()),
            'All nine missing geometry functions require three original witnesses')
    return {'functions': sorted(functions.values(), key=lambda f: f['address']),
            'sequence_proofs': sequences, 'call_neighbors': contexts,
            'counts': {'functions': 9, 'code_bytes': 4184, 'source_units': 1, 'closed_return_bodies': 9}}
