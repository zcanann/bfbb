"""Original-only complete French group sequence and scoped base tail wrappers."""
from __future__ import annotations

import json

from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow
from platforms.france_tu_sequences import KIND, compare, digest, unique_template, words
from platforms.france_update_cull_sequence import checked_identity, group_count_context
from platforms.ps2_source import canonical_linkages

SOURCE = 'SB/Core/x/xGroup.cpp'
START = 0x2a0070
MEMBERS = [('get_any', 64), ('xGroupGetItem', 20), ('xGroupGetItemPtr', 200),
           ('xGroupGetCount', 12), ('xGroupEventCB', 712), ('xGroupLoad', 8),
           ('xGroupSave', 8), ('xGroupSetup', 156), ('xGroupInit', 172)]


def base_tail(original, target, reference, address, name, calls, known):
    require(name in ('xGroupLoad', 'xGroupSave') and len(calls) == 1 and
            calls[0]['offset'] == 0 and calls[0]['opcode'] == 2,
            'Group wrapper is not its sole original tail jump')
    call = calls[0]
    callee = checked_identity(original, target, call['reference_address'], call['target_address'], known)
    expected = ('xBaseLoad', 96) if name == 'xGroupLoad' else ('xBaseSave', 76)
    require((callee['name'], callee['size']) == expected and callee['source'] == 'SB/Core/x/xBase.cpp',
            'Group wrapper changed independently known base callee')
    for binary, entry, destination in ((original, reference, call['reference_address']),
                                       (target, address, call['target_address'])):
        require(binary.word(entry) >> 26 == 2 and binary.word(entry + 4) == 0,
                'Group wrapper is not an unchanged-frame J/NOP')
        flow = ControlFlow({destination + 4*i: w for i, w in
                            enumerate(words(binary.read(destination, callee['size'] + 16)))})
        require(flow.bounds(destination, callee['size'])['passes'], 'Known base callee does not strictly return')
    return {'passes': True, 'boundary_kind': 'reviewed-group-j-nop-to-known-base-return',
            'known_callee_name': callee['name'], 'known_callee_address': callee['address'],
            'unchanged_sp_ra': True}


def generate_unit(originals, registry_dir):
    target = originals[TARGET]
    known = {}
    for path in sorted(registry_dir.glob('*functions.json')):
        for f in json.loads(path.read_text()).get('functions', []):
            if f['source'] == SOURCE and f['name'] != 'xGroupGetItemPtr':
                continue
            if f['address'] in known:
                require(all(known[f['address']][k] == f[k] for k in ('name', 'source', 'size', 'sha256')),
                        'Conflicting original group neighbor')
            known[f['address']] = f
    anchors = [f for f in known.values() if f['source'] == SOURCE]
    require(len(anchors) == 1 and (anchors[0]['name'], anchors[0]['address'], anchors[0]['size']) ==
            ('xGroupGetItemPtr', 0x2a00d0, 200), 'Independent group accessor anchor changed')
    reviewed = json.loads((registry_dir / 'reviewed-call-targets.json').read_text())
    require(reviewed['executable_sha1'] == target.sha1, 'Group allocator registry targets another original')
    allocator = [f for f in reviewed['anchors'] if f['name'] == 'xMemAlloc']
    require(len(allocator) == 1 and allocator[0]['eligible_for_progress'] is False,
            'Expected one independently reviewed nonprogress allocator identity')
    known[allocator[0]['address']] = allocator[0]
    functions, sequences, contexts = {}, [], []
    for version in REFERENCES:
        original = originals[version]
        members = sorted((f for f in original.functions if f['source'] == SOURCE), key=lambda f: f['low'])
        require([(f['name'], f['high'] - f['low']) for f in members] == MEMBERS,
                'Complete original group membership changed')
        low, end = members[0]['low'], members[-1]['high']
        require(end - low == 1404, 'Original group sequence span changed')
        shift = START - low
        count = next(f for f in members if f['name'] == 'xGroupGetCount')
        anchor_proof = group_count_context(original, target, count['low'], count['low'] + shift, known)
        entries = {f['low']: f for f in members}
        linkages = canonical_linkages(original.data, original.metadata)
        masks, callees = {}, {}
        for i, f in enumerate(members):
            a, b, size, name = f['low'], f['low'] + shift, f['high'] - f['low'], f['name']
            require(a in linkages and a % 16 == b % 16 == 0, 'Original group linkage/alignment changed')
            if i:
                gap = a - members[i - 1]['high']
                require(0 <= gap < 16 and not any(original.read(a - gap, gap)) and
                        not any(target.read(b - gap, gap)), 'Group alignment bytes changed')
            pairs, calls, mask = compare(original, target, a, b, size)
            require(not pairs or (name == 'xGroupInit' and len(pairs) == 1),
                    'Unexpected group address operand')
            for pair in pairs:
                owner = entries.get(pair['reference_address'])
                require(owner is not None and owner['name'] == 'xGroupEventCB' and
                        pair['target_address'] == pair['reference_address'] + shift and
                        (pair['hi_offset'], pair['lo_offset'], pair['opcode'], pair['storage']) ==
                        (28, 32, 9, 'file_backed'), 'Group callback address is not the original local entry')
            bounds = None
            for binary, entry in ((original, a), (target, b)):
                flow = ControlFlow({entry + 4*j: w for j, w in enumerate(words(binary.read(entry, size + 16)))})
                bounds = flow.bounds(entry, size)
                if not bounds['passes']:
                    require(size == 8, 'Unreviewed group control-flow boundary')
                    bounds = base_tail(original, target, a, b, name, calls, known)
            for call in calls:
                ra, tb = call['reference_address'], call['target_address']
                require(ra not in callees or callees[ra] == tb, 'Group external destinations disagree')
                callees[ra] = tb
                if tb in known:
                    checked_identity(original, target, ra, tb, known)
                else:
                    callee = original.by_address.get(ra)
                    require(callee is not None and (callee['source'], callee['name'], callee['high'] - ra, tb) ==
                            ('SB/Core/x/xEvent.cpp', 'zEntEvent', 32, 0x283a20),
                            'Unreviewed external group context')
                    cp, cc, _ = compare(original, target, ra, tb, 32)
                    require(not cp and len(cc) == 1 and cc[0]['offset'] == 24 and cc[0]['opcode'] == 2,
                            'Event wrapper context changed')
                    if not any(c['version'] == version for c in contexts):
                        contexts.append({'version': version, 'reference_address': ra, 'target_address': tb,
                            'reference_name': callee['name'], 'reference_source': callee['source'], 'size': 32,
                            'reference_sha256': digest(original.read(ra, 32)), 'target_sha256': digest(target.read(tb, 32)),
                            'direct_transfers': cc, 'promoted_as_named_anchor': False, 'target_extent_claimed': False})
            masks.update({a - low + off: value for off, value in mask.items()})
            if b not in known:
                if b not in functions:
                    functions[b] = {'name': name, 'source': SOURCE, 'address': b, 'size': size,
                        'sha256': digest(target.read(b, size)), 'boundary_confirmation': True,
                        'confirmation_kind': KIND, 'provenance': [],
                        'corroboration': {'local_control_flow': bounds,
                            'previously_confirmed_neighbor_entries': [anchors[0]['address']]}}
                record = functions[b]
                require((record['name'], record['size']) == (name, size) and (not record['provenance'] or
                        record['provenance'][0]['linkage_name'] == linkages[a]), 'Original group identities disagree')
                record['provenance'].append({'version': version, 'executable_sha1': original.sha1,
                    'source_address': a, 'name': name, 'source': SOURCE, 'linkage_name': linkages[a],
                    'reference_sha256': digest(original.read(a, size)),
                    'data_address_operands': pairs, 'direct_transfers': calls})
        require(len(set(callees.values())) == len(callees), 'Distinct original group callees collapse')
        sequences.append({'version': version, 'source': SOURCE, 'source_start': low, 'target_start': START,
            'complete_original_members': 9, 'previously_confirmed_members': 1,
            'sequence_bytes_with_alignment': end - low, 'independent_accessor_context': anchor_proof,
            'uniqueness': unique_template(target, original.read(low, end - low), masks, START)})
    require(len(functions) == 8 and sum(f['size'] for f in functions.values()) == 1152 and
            all(len(f['provenance']) == 3 for f in functions.values()),
            'All eight added group members need three original witnesses')
    require(len(contexts) == 3 and {c['version'] for c in contexts} == set(REFERENCES),
            'All original group callers need the complete event context')
    return {'functions': sorted(functions.values(), key=lambda f: f['address']),
            'sequence_proofs': sequences, 'call_neighbors': contexts, 'data_proofs': [],
            'counts': {'functions': 8, 'code_bytes': 1152, 'source_units': 1,
                       'closed_return_bodies': 6, 'reviewed_group_base_tails': 2}}
