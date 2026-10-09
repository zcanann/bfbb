"""Original-only French event sequence rooted in the proven group callback."""
from __future__ import annotations

import json

from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow
from platforms.france_tu_sequences import KIND, compare, digest, unique_template, words
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_group_sequence import generate_unit as generate_group
from platforms.ps2_source import canonical_linkages

SOURCE = 'SB/Core/x/xEvent.cpp'
START = 0x283840
MEMBERS = [('zEntEvent__FP5xBaseUiP5xBaseUiPCfP5xBasei', 412),
           ('zEntEvent__FP5xBaseP5xBaseUiPCf', 32), ('zEntEvent__FP5xBaseP5xBaseUi', 32),
           ('zEntEvent__FP5xBaseUiPCfP5xBase', 32), ('zEntEvent__FP5xBaseUiPCf', 32),
           ('zEntEvent__FP5xBaseUiffff', 68), ('zEntEvent__FP5xBaseUi', 32),
           ('zEntEvent__FUiUiffff', 88), ('zEntEvent__FUiUi', 72), ('zEntEvent__FPcUi', 80)]


def shuffle_tail(original, target, reference, address, reference_main, calls):
    require(len(calls) == 1 and (calls[0]['offset'], calls[0]['opcode'],
            calls[0]['reference_address'], calls[0]['target_address']) ==
            (24, 2, reference_main, START), 'Event tail does not target original main body')
    for binary, entry, main in ((original, reference, reference_main), (target, address, START)):
        code = words(binary.read(entry, 32))
        require(code[6] >> 26 == 2, 'Event shuffle does not end with J and its delay slot')
        for i, word in enumerate(code):
            if i == 6:
                continue
            rs, rt, rd = word >> 21 & 31, word >> 16 & 31, word >> 11 & 31
            require(word >> 26 == 0 and word & 63 == 45 and word >> 6 & 31 == 0 and
                    rt == 0 and rs in (0, 2, 4, 5, 6, 7) and rd in (2, 4, 5, 6, 7, 8, 9, 10),
                    'Event tail changes more than original argument registers')
        flow = ControlFlow({main + 4*i: w for i, w in enumerate(words(binary.read(main, 428)))})
        require(flow.bounds(main, 412)['passes'], 'Event main body lacks strict complete return')
    return {'passes': True, 'boundary_kind': 'reviewed-event-seven-daddu-tail-to-local-main',
            'main_entry': START, 'main_bytes': 412, 'unchanged_sp_ra': True}


def generate_unit(originals, registry_dir):
    target = originals[TARGET]
    # Re-run the independent group/accessor proof; never accept a generated event
    # record as its own anchor. Its three callback calls establish this entry.
    group = generate_group(originals, registry_dir)
    caller = next(f for f in group['functions'] if f['name'] == 'xGroupEventCB')
    require((caller['address'], caller['size']) == (0x2a01b0, 712), 'Independent group caller changed')
    known = {}
    for path in sorted(registry_dir.glob('*functions.json')):
        for f in json.loads(path.read_text()).get('functions', []):
            if f['source'] != SOURCE:
                known[f['address']] = f
    functions, sequences = {}, []
    for version in REFERENCES:
        original = originals[version]
        linkages = canonical_linkages(original.data, original.metadata)
        members = sorted((f for f in original.functions if f['source'] == SOURCE), key=lambda f: f['low'])
        require([(linkages.get(f['low']), f['high'] - f['low']) for f in members] == MEMBERS,
                'Complete original event overloads changed')
        low, end = members[0]['low'], members[-1]['high']
        require(end - low == 912, 'Original event sequence span changed')
        shift = START - low
        witness = [p for p in caller['provenance'] if p['version'] == version]
        require(len(witness) == 1, 'Original group caller provenance is ambiguous')
        witness = witness[0]
        caller_calls = [c for c in witness['direct_transfers'] if c['offset'] in (368, 520, 632)]
        entry = members[3]['low']
        require(len(caller_calls) == 3 and all((c['opcode'], c['reference_address'], c['target_address']) ==
                (3, entry, 0x283a20) for c in caller_calls), 'Three independent caller edges do not name this overload')
        masks, mapping = {}, {}
        inside = {f['low']: f for f in members}
        for i, f in enumerate(members):
            a, b, size = f['low'], f['low'] + shift, f['high'] - f['low']
            require(a % 16 == b % 16 == 0, 'Original event entry alignment changed')
            if i:
                gap = a - members[i - 1]['high']
                require(0 <= gap < 16 and not any(original.read(a - gap, gap)) and
                        not any(target.read(b - gap, gap)), 'Original event alignment bytes changed')
            pairs, calls, mask = compare(original, target, a, b, size)
            require(not pairs, 'Event proof cannot normalize data operands')
            for call in calls:
                ra, tb = call['reference_address'], call['target_address']
                require(ra not in mapping or mapping[ra] == tb, 'Event callee mappings disagree')
                mapping[ra] = tb
                if ra in inside:
                    require(ra == low and tb == START, 'Event internal transfer changes original main ownership')
                else:
                    record = checked_identity(original, target, ra, tb, known)
                    require((record['name'], record['source'], record['size']) in (
                        ('zSceneFindObject', 'SB/Game/zScene.cpp', 128),
                        ('xSTFindAsset', 'SB/Core/x/xstransvc.cpp', 444),
                        ('xStrHash', 'SB/Core/x/xString.cpp', 88)), 'Unreviewed external event identity')
            bounds = None
            for binary, address in ((original, a), (target, b)):
                flow = ControlFlow({address + 4*j: w for j, w in enumerate(words(binary.read(address, size + 16)))})
                bounds = flow.bounds(address, size)
                if not bounds['passes']:
                    require(size == 32 and i in (1, 2, 3, 4, 6), 'Unreviewed event boundary')
                    bounds = shuffle_tail(original, target, a, b, low, calls)
            masks.update({a - low + off: value for off, value in mask.items()})
            if b not in functions:
                functions[b] = {'name': f['name'], 'source': SOURCE, 'address': b, 'size': size,
                    'sha256': digest(target.read(b, size)), 'boundary_confirmation': True,
                    'confirmation_kind': KIND, 'provenance': [],
                    'corroboration': {'local_control_flow': bounds,
                        'previously_confirmed_caller_entry': caller['address'],
                        'caller_offsets': [368, 520, 632]}}
            record = functions[b]
            require((record['name'], record['size']) == (f['name'], size) and (not record['provenance'] or
                    record['provenance'][0]['linkage_name'] == linkages[a]), 'Original event overload identity changed')
            record['provenance'].append({'version': version, 'executable_sha1': original.sha1,
                'source_address': a, 'name': f['name'], 'source': SOURCE, 'linkage_name': linkages[a],
                'reference_sha256': digest(original.read(a, size)), 'data_address_operands': [],
                'direct_transfers': calls})
        require(len(set(mapping.values())) == len(mapping), 'Distinct event callees collapse')
        sequences.append({'version': version, 'source': SOURCE, 'source_start': low, 'target_start': START,
            'complete_original_members': 10, 'previously_confirmed_members': 0,
            'independently_proven_caller': caller['address'], 'caller_reference': witness['source_address'],
            'checked_caller_edges': caller_calls, 'sequence_bytes_with_alignment': 912,
            'uniqueness': unique_template(target, original.read(low, 912), masks, START)})
    require(len(functions) == 10 and sum(f['size'] for f in functions.values()) == 880 and
            all(len(f['provenance']) == 3 for f in functions.values()), 'All ten event members need three witnesses')
    return {'functions': sorted(functions.values(), key=lambda f: f['address']), 'sequence_proofs': sequences,
            'call_neighbors': [], 'data_proofs': [], 'counts': {'functions': 10, 'code_bytes': 880,
                'source_units': 1, 'closed_return_bodies': 5, 'reviewed_event_shuffle_tails': 5}}
