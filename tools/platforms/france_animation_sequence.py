"""Original-only French animation TU sequence and bounded CFG exceptions."""
from __future__ import annotations

from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow, rooted_graph
from platforms.france_tu_sequences import (
    KIND, compare, cstring, digest, gpr_writes, named_data, unique_template, words,
)
from platforms.ps2_source import canonical_linkages

SOURCE = 'SB/Core/x/xAnim.cpp'
START = 0x210df0
TAILS = {'xAnimPlaySetup': ('xAnimPlaySetState', 212),
         'xAnimTableAddTransition': ('_xAnimTableAddTransition', 8)}


def animation_bounds(flow, address, size, name, members):
    """Accept only the reviewed original leaf tails and dead delay-slot copies.

    The shared ControlFlow checker and all original instruction words stay
    unchanged. These checks are not used for arbitrary units or unknown extents.
    """
    bounds = flow.bounds(address, size)
    if bounds['passes']:
        return bounds
    if name in TAILS:
        callee_name, expected_size = TAILS[name]
        callee = members[callee_name]
        require(size == expected_size and bounds['reasons'] == [
            'extent_not_terminal_return_delay', 'local_edge_outside_extent', 'no_return'],
            'Animation tail has other unresolved boundary issues')
        require(not any(bounds[key] for key in ('returns', 'direct_or_indirect_calls',
                'stack_adjustments', 'ra_saves', 'ra_loads', 'unreachable_zero_words')),
                'Reviewed animation tail is not a completely covered frame-free leaf')
        terminal = flow.instruction(address + size - 8)
        require(terminal['kind'] == 'jump' and terminal['target'] == callee['low'] and
                flow.valid_delay(address + size - 8), 'Leaf tail does not target its same-TU callee')
        for pc in range(address, address + size, 4):
            instruction = flow.instruction(pc)
            require(31 not in gpr_writes(flow.words[pc]), 'Leaf tail clobbers its inherited return address')
            if instruction['kind'] == 'jump' and not address <= instruction['target'] < address + size:
                require(pc == address + size - 8, 'Another jump leaves the reviewed leaf')
        require(flow.bounds(callee['low'], callee['high'] - callee['low'])['passes'],
                'Tail destination does not have independently closed local bounds')
        return {**bounds, 'passes': True, 'reasons': [], 'boundary_kind': 'reviewed-frame-free-leaf-tail',
                'tail_target_name': callee_name, 'tail_target_address': callee['low'],
                'terminal_instruction_offset': size - 8}
    require(name == 'xAnimTableNewTransition' and size == 824 and
            bounds['reasons'] == ['unreachable_nonpadding_words'],
            'Animation function has an unreviewed boundary exception')
    uncovered = set(bounds['unreachable_zero_words'])
    nonzero = sorted(pc for pc in uncovered if flow.words[pc] != 0)
    require(len(nonzero) == 3, 'Unexpected animation dead-instruction inventory')
    duplicates = []
    for pc in nonzero:
        word = flow.words[pc]
        require(word >> 26 == 15 or word >> 26 == 0 and word & 63 == 2,
                'Dead word is not the observed LUI or SRL')
        previous = flow.instruction(pc - 8)
        require(previous['kind'] == 'branch' and previous.get('taken') is True and
                not previous.get('likely') and previous['target'] > pc and flow.valid_delay(pc - 8),
                'Dead copy does not follow an unconditional branch and its delay slot')
        candidates = []
        for branch_pc, destination in bounds['branches']:
            branch = flow.instruction(branch_pc)
            if (destination == pc + 4 and pc - 24 <= branch_pc < pc - 8 and
                    branch.get('taken') is None and not branch.get('likely') and
                    branch_pc + 4 not in uncovered and flow.words[branch_pc + 4] == word):
                candidates.append(branch_pc)
        require(len(candidates) == 1, 'Dead word lacks one identical executed conditional delay-slot producer')
        duplicates.append({'unreachable_offset': pc - address,
                           'executed_delay_offset': candidates[0] + 4 - address,
                           'conditional_branch_offset': candidates[0] - address,
                           'conditional_target_offset': pc + 4 - address,
                           'skipping_branch_offset': pc - 8 - address})
    return {**bounds, 'passes': True, 'reasons': [],
            'boundary_kind': 'reviewed-dead-copies-of-executed-delay-instructions',
            'duplicated_dead_instructions': duplicates}


def generate_unit(originals):
    target = originals[TARGET]
    flow = ControlFlow({s['address'] + i * 4: w for s in target.loaded
                        for i, w in enumerate(words(target.read(s['address'], s['file_size'])))})
    _, rooted_calls, _ = rooted_graph(flow, target.metadata['entry_point'])
    functions, sequence_proofs, data_proofs, neighbors = [], [], [], []
    for version in REFERENCES:
        original = originals[version]
        members = sorted((f for f in original.functions if f['source'] == SOURCE), key=lambda f: f['low'])
        require(len(members) == 34 and sum(f['high'] - f['low'] for f in members) == 17656,
                'Reviewed animation TU membership changed')
        low, end = members[0]['low'], members[-1]['high']
        original_members = {f['name']: f for f in members}
        require(len(original_members) == len(members), 'Animation original names are ambiguous')
        target_members = {name: {**f, 'low': START + f['low'] - low, 'high': START + f['high'] - low}
                          for name, f in original_members.items()}
        reference_flow = ControlFlow({low + i * 4: w for i, w in
                                     enumerate(words(original.read(low, end - low + 16)))})
        linkages = canonical_linkages(original.data, original.metadata)
        masks, all_pairs, transfer_map = {}, [], {}
        for index, function in enumerate(members):
            a, size = function['low'], function['high'] - function['low']
            b = START + a - low
            require(a % 16 == b % 16 == 0 and a in linkages, 'Animation alignment/linkage missing')
            if index:
                gap = a - members[index - 1]['high']
                require(0 <= gap < 16 and not any(original.read(a - gap, gap)) and
                        not any(target.read(b - gap, gap)), 'Non-padding data interrupts the complete animation TU')
            pairs, calls, mask = compare(original, target, a, b, size)
            masks.update({a - low + off: value for off, value in mask.items()})
            all_pairs.extend(pairs)
            local = animation_bounds(flow, b, size, function['name'], target_members)
            reference_bounds = animation_bounds(reference_flow, a, size, function['name'], original_members)
            require(local.get('boundary_kind') == reference_bounds.get('boundary_kind') and
                    local.get('duplicated_dead_instructions') == reference_bounds.get('duplicated_dead_instructions'),
                    'Original boundary exception patterns differ')
            for call in calls:
                source_dest, target_dest = call['reference_address'], call['target_address']
                require(source_dest not in transfer_map or transfer_map[source_dest] == target_dest,
                        'Repeated animation callee relationship changed')
                transfer_map[source_dest] = target_dest
            proof = {'version': version, 'executable_sha1': original.sha1, 'source_address': a,
                     'name': function['name'], 'source': SOURCE, 'linkage_name': linkages[a],
                     'reference_sha256': digest(original.read(a, size)),
                     'data_address_operands': pairs, 'direct_transfers': calls}
            if 'boundary_kind' in reference_bounds:
                proof['original_boundary_exception'] = reference_bounds
            if version == REFERENCES[0]:
                functions.append({'name': function['name'], 'source': SOURCE, 'address': b, 'size': size,
                                  'sha256': digest(target.read(b, size)), 'boundary_confirmation': True,
                                  'confirmation_kind': KIND, 'provenance': [proof],
                                  'corroboration': {'local_control_flow': local,
                                                   'direct_rooted_call_sites': sorted(rooted_calls[b])}})
            else:
                entry = functions[index]
                require((entry['name'], entry['address'], entry['size'], entry['provenance'][0]['linkage_name']) ==
                        (function['name'], b, size, linkages[a]), 'Animation original ordered identities disagree')
                entry['provenance'].append(proof)
        pool = named_data(original, 'sxAnimTempTranPool', SOURCE)
        impure = named_data(original, '_impure_ptr', SOURCE)
        by_address = {f['low']: f for f in members}
        data_map, literal_proofs = {}, []
        for pair in all_pairs:
            a, b = pair['reference_address'], pair['target_address']
            require(a not in data_map or data_map[a] == b, 'Animation data mapping is inconsistent')
            if a in data_map:
                continue
            data_map[a] = b
            if pair['storage'] == 'zero_fill':
                require(a == pool, 'Unknown animation zero-fill reference')
            elif low <= a < end:
                require(a in by_address and by_address[a]['name'] == 'xAnimPoolCB' and b == START + a - low,
                        'Animation callback pointer is not the corresponding original body')
            else:
                for binary, address in ((original, a), (target, b)):
                    region = binary._stream_regions['initialized_data']
                    require(region['address'] <= address < region['address'] + region['size'],
                            'Animation data reference is outside initialized data')
                if a == impure:
                    first, second = original.word(a), target.word(b)
                    require(first - a == second - b, 'Original runtime pointer relationship changed')
                    for binary, address in ((original, first), (target, second)):
                        region = binary._stream_regions['initialized_data']
                        require(region['address'] <= address < region['address'] + region['size'],
                                'Runtime pointer no longer targets initialized data')
                else:
                    first, second = cstring(original, a), cstring(target, b)
                    require(first == second, 'Animation literal bytes changed')
                    literal_proofs.append({'reference_address': a, 'target_address': b,
                                           'size_with_terminator': len(first), 'sha256': digest(first)})
        require(len(data_map) == 7 and len(literal_proofs) == 4 and len(set(data_map.values())) == len(data_map),
                'Reviewed animation data inventory changed')
        require(len(transfer_map) == 37 and len(set(transfer_map.values())) == len(transfer_map),
                'Animation direct-callee inventory or bijection changed')
        for a, b in sorted(transfer_map.items()):
            if low <= a < end:
                require(a in by_address and b == START + a - low, 'Animation internal call changed ownership')
                continue
            neighbor = original.by_address.get(a)
            size = neighbor['high'] - a if neighbor else 32
            pairs, calls, _ = compare(original, target, a, b, size)
            record = {'version': version, 'reference_address': a, 'target_address': b,
                      'reference_sha256': digest(original.read(a, size)), 'target_sha256': digest(target.read(b, size)),
                      'address_operand_count': len(pairs), 'direct_transfer_count': len(calls),
                      'promoted_as_named_anchor': False}
            if neighbor:
                record.update(reference_name=neighbor['name'], reference_source=neighbor['source'], size=size)
            else:
                record['compared_entry_prefix_bytes'] = size
            neighbors.append(record)
        data_proofs.append({'version': version, 'reference_pool_address': pool,
                            'target_pool_address': data_map[pool], 'reference_runtime_pointer_address': impure,
                            'target_runtime_pointer_address': data_map[impure],
                            'runtime_pointer_relative_target': original.word(impure) - impure,
                            'literals': literal_proofs, 'original_data_sizes_inferred': False})
        sequence_proofs.append({'version': version, 'source': SOURCE, 'source_start': low,
                                'target_start': START, 'sequence_bytes_with_alignment': end - low,
                                'uniqueness': unique_template(target, original.read(low, end - low), masks, START),
                                'data_address_count': len(data_map), 'direct_callee_count': len(transfer_map)})
    groups = {}
    for neighbor in neighbors:
        groups.setdefault(neighbor['target_address'], []).append(neighbor)
    for group in groups.values():
        require(len(group) == 3 and {n['version'] for n in group} == set(REFERENCES) and
                len({(n.get('reference_name'), n.get('reference_source'), n.get('size')) for n in group}) == 1,
                'Animation call neighbors disagree across debug originals')
    require(sum(bool(f['corroboration']['direct_rooted_call_sites']) for f in functions) == 29,
            'Reviewed animation rooted entries changed')
    return {'source': SOURCE, 'functions': functions, 'sequence_proofs': sequence_proofs,
            'data_proofs': data_proofs, 'call_neighbors': neighbors,
            'counts': {'functions': 34, 'code_bytes': 17656, 'source_units': 1,
                       'closed_return_bodies': 32, 'rooted_direct_call_entries': 29,
                       'reviewed_leaf_tail_functions': 2, 'reviewed_dead_delay_copies': 3}}
