"""Original-only proof for the reviewed French particle-command TU sequence.

The original initializer's concrete stores, not compiled source, corroborate
callback ownership. This module adds no relocation symbols or source scores.
"""
from __future__ import annotations

from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow, rooted_graph
from platforms.france_tu_sequences import (
    KIND, compare, digest, memory_kind, named_data, unique_template, words,
)
from platforms.ps2_source import canonical_linkages
from platforms.ps2_type_layouts import aggregate_layouts

SOURCE = 'SB/Core/x/xParCmd.cpp'
START = 0x1f48c0


def initializer_stores(original, address, size):
    """Replay only the actual leaf initializer's LUI/ADDIU/SW/JR instructions."""
    code = words(original.read(address, size))
    registers = {0: 0}
    stores = {}
    for index, word in enumerate(code):
        op, rs, rt = word >> 26, word >> 21 & 31, word >> 16 & 31
        immediate = (word & 65535) - (65536 if word & 32768 else 0)
        if op == 15:
            require(rs == 0 and rt != 0, 'Unexpected initializer LUI')
            registers[rt] = (word & 65535) << 16
        elif op == 9:
            require(rs in registers and rt != 0, 'Initializer ADDIU has unknown input')
            registers[rt] = (registers[rs] + immediate) & 0xffffffff
        elif op == 43:
            require(rs in registers and rt in registers, 'Initializer store has unknown input')
            destination = (registers[rs] + immediate) & 0xffffffff
            require(destination not in stores and destination % 4 == 0 and
                    memory_kind(original, destination) == 'zero_fill', 'Invalid or repeated callback-table store')
            stores[destination] = {'value': registers[rt], 'instruction_offset': index * 4}
        else:
            require(word == 0x03e00008 and index == len(code) - 2 and code[-1] >> 26 == 43,
                    'Initializer is not the reviewed straight-line leaf with final return-store delay slot')
    require(len(stores) == 81, 'Original initializer store inventory changed')
    return stores


def table_records(stores, base, record_size, members):
    offsets = {address - base for address in stores}
    require(all(offset >= 0 and offset % 4 == 0 for offset in offsets), 'Store precedes table')
    indices = sorted({offset // record_size for offset in offsets})
    result = []
    for index in indices:
        record = {}
        for name, offset in members:
            address = base + index * record_size + offset
            require(address in stores, 'Incomplete original callback record')
            record[name] = stores[address]['value']
        require(record['type'] == index and record['size'] > 0, 'Original record index/type mismatch')
        result.append({'index': index, **record})
    require(len(result) == 27, 'Unexpected number of initialized callback records')
    return result


def generate_unit(originals):
    target = originals[TARGET]
    flow = ControlFlow({s['address'] + i * 4: w for s in target.loaded
                        for i, w in enumerate(words(target.read(s['address'], s['file_size'])))})
    _, rooted_calls, _ = rooted_graph(flow, target.metadata['entry_point'])
    functions, sequence_proofs, registration_proofs, neighbors = [], [], [], []
    for version in REFERENCES:
        original = originals[version]
        members = sorted((f for f in original.functions if f['source'] == SOURCE), key=lambda f: f['low'])
        require(len(members) == 30 and sum(f['high'] - f['low'] for f in members) == 7544,
                'Reviewed particle TU membership changed')
        low, end = members[0]['low'], members[-1]['high']
        linkages = canonical_linkages(original.data, original.metadata)
        masks, all_pairs, transfer_map = {}, [], {}
        for index, function in enumerate(members):
            a, size = function['low'], function['high'] - function['low']
            b = START + a - low
            require(a % 16 == b % 16 == 0 and a in linkages, 'Particle entry alignment/linkage missing')
            if index:
                gap = a - members[index - 1]['high']
                require(0 <= gap < 16 and not any(original.read(a - gap, gap)) and
                        not any(target.read(b - gap, gap)), 'Non-padding bytes interrupt the complete particle TU')
            pairs, calls, mask = compare(original, target, a, b, size,
                                         allow_return_store=function['name'] == 'xParCmdInit')
            masks.update({a - low + off: value for off, value in mask.items()})
            all_pairs.extend(pairs)
            local = flow.bounds(b, size)
            require(local['passes'], f'{function["name"]}: particle CFG does not close')
            for call in calls:
                source_dest, target_dest = call['reference_address'], call['target_address']
                require(source_dest not in transfer_map or transfer_map[source_dest] == target_dest,
                        'Repeated particle callee relationship changed')
                transfer_map[source_dest] = target_dest
            proof = {'version': version, 'executable_sha1': original.sha1, 'source_address': a,
                     'name': function['name'], 'source': SOURCE, 'linkage_name': linkages[a],
                     'reference_sha256': digest(original.read(a, size)),
                     'data_address_operands': pairs, 'direct_transfers': calls}
            if version == REFERENCES[0]:
                functions.append({'name': function['name'], 'source': SOURCE, 'address': b, 'size': size,
                                  'sha256': digest(target.read(b, size)), 'boundary_confirmation': True,
                                  'confirmation_kind': KIND, 'provenance': [proof],
                                  'corroboration': {'local_control_flow': local,
                                                   'direct_rooted_call_sites': sorted(rooted_calls[b])}})
            else:
                entry = functions[index]
                require((entry['name'], entry['address'], entry['size'], entry['provenance'][0]['linkage_name']) ==
                        (function['name'], b, size, linkages[a]), 'Particle original identities disagree')
                entry['provenance'].append(proof)
        # Original DWARF establishes the table record shape; no C layout enters proof.
        section = next(s for s in original.metadata['sections'] if s['name'] == '.debug' and s['size'])
        debug = original.data[section['offset']:section['offset'] + section['size']]
        layout = aggregate_layouts(debug, SOURCE, {'xCmdInfo'})['xCmdInfo']
        shape = [(m['name'], m['offset']) for m in layout['members']]
        require(layout['size'] == 12 and shape == [('type', 0), ('size', 4), ('func', 8)],
                'Original callback record shape changed')
        table = named_data(original, 'sCmdInfo', SOURCE)
        table_targets = {p['target_address'] for p in all_pairs if p['reference_address'] == table}
        require(len(table_targets) == 1, 'No unique observed French registration table')
        french_table = table_targets.pop()
        initializer = members[-1]
        require(initializer['name'] == 'xParCmdInit', 'TU does not end with the reviewed initializer')
        init_size = initializer['high'] - initializer['low']
        original_stores = initializer_stores(original, initializer['low'], init_size)
        french_stores = initializer_stores(target, START + initializer['low'] - low, init_size)
        first = table_records(original_stores, table, layout['size'], shape)
        second = table_records(french_stores, french_table, layout['size'], shape)
        callbacks = set()
        reference_entries = {f['low']: f for f in members}
        for a, b in zip(first, second):
            require((a['index'], a['type'], a['size']) == (b['index'], b['type'], b['size']),
                    'Original callback metadata changed')
            callback = a['func']
            if callback:
                require(callback in reference_entries and b['func'] == START + callback - low,
                        'Registered callback is not the corresponding complete named function')
                callbacks.add(b['func'])
            else:
                require(b['func'] == 0, 'Null callback changed')
        expected_callbacks = {START + f['low'] - low for f in members if f['name'].endswith('_Update')}
        require(len(callbacks) == 26 and callbacks == expected_callbacks,
                'Original registration does not cover every update callback')
        # All zero-fill references preserve table-relative addresses. The sole
        # initialized-data reference is the independently named coefficient array.
        polynomial = named_data(original, 'cosSinPolynomial', SOURCE)
        data_map = {}
        for pair in all_pairs:
            a, b = pair['reference_address'], pair['target_address']
            require(a not in data_map or data_map[a] == b, 'Inconsistent particle address relationship')
            data_map[a] = b
            if pair['storage'] == 'zero_fill':
                require(a - table == b - french_table, 'Particle table relative layout differs')
            elif low <= a < end:
                require(a in reference_entries and b == START + a - low, 'Callback address changed ownership')
            else:
                require(a == polynomial, 'Unreviewed particle initialized-data reference')
                for binary, address in ((original, a), (target, b)):
                    region = binary._stream_regions['initialized_data']
                    require(region['address'] <= address < region['address'] + region['size'],
                            'Coefficient reference is outside initialized data')
                require(original.read(a, 4) == target.read(b, 4), 'Original coefficient anchor prefix differs')
        require(len(set(data_map.values())) == len(data_map), 'Distinct particle addresses collapsed')
        require(len(set(transfer_map.values())) == len(transfer_map), 'Distinct particle callees collapsed')
        for a, b in sorted(transfer_map.items()):
            neighbor = original.by_address.get(a)
            require(neighbor is not None, 'Particle external callee lacks original DWARF identity')
            size = neighbor['high'] - a
            pairs, calls, _ = compare(original, target, a, b, size)
            neighbors.append({'version': version, 'reference_address': a, 'target_address': b,
                              'reference_name': neighbor['name'], 'reference_source': neighbor['source'],
                              'size': size, 'reference_sha256': digest(original.read(a, size)),
                              'target_sha256': digest(target.read(b, size)),
                              'address_operand_count': len(pairs), 'direct_transfer_count': len(calls),
                              'promoted_as_named_anchor': False})
        registration_proofs.append({'version': version, 'record_layout': layout,
                                    'reference_table': table, 'target_table': french_table,
                                    'initializer_reference_address': initializer['low'],
                                    'initializer_target_address': START + initializer['low'] - low,
                                    'store_count': len(french_stores), 'callback_count': len(callbacks),
                                    'original_records': first, 'target_records': second,
                                    'final_store_delay_slot_offset': init_size - 4,
                                    'coefficient_reference_address': polynomial,
                                    'coefficient_target_address': data_map[polynomial],
                                    'coefficient_compared_prefix_bytes': 4})
        sequence_proofs.append({'version': version, 'source': SOURCE, 'source_start': low,
                                'target_start': START, 'sequence_bytes_with_alignment': end - low,
                                'uniqueness': unique_template(target, original.read(low, end - low), masks, START),
                                'data_address_count': len(data_map), 'direct_callee_count': len(transfer_map)})
    groups = {}
    for neighbor in neighbors:
        groups.setdefault(neighbor['target_address'], []).append(neighbor)
    for group in groups.values():
        require(len(group) == 3 and {n['version'] for n in group} == set(REFERENCES) and
                len({(n['reference_name'], n['reference_source'], n['size']) for n in group}) == 1,
                'Particle callee neighbors disagree across debug originals')
    require(sum(bool(f['corroboration']['direct_rooted_call_sites']) for f in functions) == 3,
            'Particle original rooted entries changed')
    for function in functions:
        if function['address'] in callbacks:
            function['corroboration']['registered_by_original_initializer'] = True
    return {'source': SOURCE, 'functions': functions, 'sequence_proofs': sequence_proofs,
            'registration_proofs': registration_proofs, 'call_neighbors': neighbors,
            'counts': {'functions': 30, 'code_bytes': 7544, 'source_units': 1,
                       'closed_return_bodies': 30, 'rooted_direct_call_entries': 3,
                       'registered_callback_entries': 26}}
