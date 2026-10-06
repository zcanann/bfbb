"""Original-only French entity-motion sequences and explicit dispatch evidence."""
from __future__ import annotations
from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow, rooted_graph
from platforms.france_tu_sequences import (KIND, address_pair, compare, cstring, digest, gpr_writes, memory_kind, unique_template, words)
from platforms.ps2_source import canonical_linkages

SOURCE = 'SB/Core/x/xEntMotion.cpp'
DISPATCHES = {'xEntMotionDebugDraw': ((0x48, 6),),
              'xEntMotionDebugWrite': ((0x3c, 6), (0x17c, 6), (0x59c, 8))}
DIAMONDS = (0xdc, 0x4b8, 0x530, 0x6b0, 0x85c)


def dispatches(original, address, size, name, *, specifications=None):
    body = original.read(address, size)
    code = words(body)
    flow = ControlFlow({address + i * 4: w for i, w in enumerate(code)})
    proven = []
    for offset, count in (DISPATCHES.get(name, ()) if specifications is None else specifications):
        jr, load, add, low, shift = (code[(offset - n) // 4] for n in (0, 4, 8, 12, 16))
        index, base = jr >> 21 & 31, low >> 21 & 31
        require(index not in (0, 31) and base not in (0, index, 31) and jr == index << 21 | 8,
                'Dispatch is not an ordinary index-register JR')
        require(code[offset // 4 + 1] == 0 and load == (35 << 26 | index << 21 | index << 16),
                'Dispatch must load its target then execute JR with NOP delay')
        require(add == (index << 21 | base << 16 | index << 11 | 33) and
                low >> 26 == 9 and low >> 16 & 31 == base and
                shift == (index << 16 | index << 11 | 2 << 6),
                'Dispatch table index/base dataflow changed')
        hi, table = address_pair(body, address, offset - 12)
        candidates = []
        for off in range(max(0, offset - 40), offset - 20, 4):
            w, guard = code[off // 4], flow.instruction(address + off + 4)
            test = w >> 16 & 31
            if (w >> 26 == 11 and w >> 21 & 31 == index and w & 65535 == count and test not in (0, index, base) and
                    guard['op'] == 4 and guard['rs'] == test and guard['rt'] == 0 and
                    address + offset + 8 <= guard['target'] < address + size):
                candidates.append(off)
        require(len(candidates) == 1, 'Dispatch lacks one bounded unsigned index/default test')
        test_offset = candidates[0]
        for off in range(test_offset + 8, offset - 16, 4):
            require(flow.instruction(address + off)['kind'] == 'normal' and index not in gpr_writes(code[off // 4]),
                    'Dispatch index changed between range test and scaling')
        require(test_offset + 8 <= hi < offset - 12, 'Table base does not originate inside checked dispatch')
        for off in range(size // 4):
            inst = flow.instruction(address + off * 4)
            destination = inst.get('target')
            require(destination is None or not address + test_offset + 4 <= destination <= address + offset + 4,
                    'A direct edge bypasses dispatch index/base checks')
        targets = words(original.read(table, count * 4))
        require(all(address <= x < address + size and x % 4 == 0 for x in targets),
                'Original dispatch table leaves its complete function')
        proven.append({'jump_offset': offset, 'index_count': count, 'test_offset': test_offset,
                       'default_offset': flow.instruction(address + test_offset + 4)['target'] - address,
                       'table_address': table, 'table_sha256': digest(original.read(table, count * 4)),
                       'target_offsets': [x - address for x in targets]})
    for proof in proven:
        for other in proven:
            require(not any(proof['test_offset'] + 4 <= destination <= proof['jump_offset'] + 4
                            for destination in other['target_offsets']), 'A table target bypasses another dispatch check')
    return proven


def diamond_pair(body, address, offset, *, allow_return_store=False):
    if offset not in DIAMONDS:
        return address_pair(body, address, offset, allow_return_store=allow_return_store)
    code = words(body)
    flow = ControlFlow({address + i * 4: w for i, w in enumerate(code)})
    branch = flow.instruction(address + offset - 20)
    skip = flow.instruction(address + offset - 8)
    high, alternate, low = code[(offset - 16) // 4], code[(offset - 4) // 4], code[offset // 4]
    register = low >> 21 & 31
    require(register not in (0, 31) and low >> 26 == 9 and low >> 16 & 31 == register and
            high >> 26 == 15 and high >> 21 & 31 == 0 and high >> 16 & 31 == register and
            code[(offset - 12) // 4] == high and alternate >> 16 == low >> 16,
            'String diamond does not preserve both actual LUI/ADDIU definitions')
    require(branch['op'] in (4, 5) and branch.get('taken') is None and branch['target'] == address + offset and
            skip['kind'] == 'branch' and skip.get('taken') is True and not skip.get('likely') and
            skip['target'] == address + offset + 4, 'String diamond branch paths changed')
    for pc in range(address, address + len(body), 4):
        inst = flow.instruction(pc)
        destination = inst.get('target')
        if destination is not None and address + offset - 16 <= destination <= address + offset:
            require(pc == address + offset - 20 and destination == address + offset,
                    'A direct edge bypasses a string diamond reaching definition')
    immediate = (low & 65535) - (65536 if low & 32768 else 0)
    return offset - 16, (((high & 65535) << 16) + immediate) & 0xffffffff


def generate_unit(originals):
    target = originals[TARGET]
    start = 0x1d73c0
    flow = ControlFlow({s['address'] + i * 4: w for s in target.loaded
                        for i, w in enumerate(words(target.read(s['address'], s['file_size'])))})
    _, rooted_calls, _ = rooted_graph(flow, target.metadata['entry_point'])
    functions, sequences, data_proofs, neighbors = [], [], [], []
    for version in REFERENCES:
        original = originals[version]
        members = sorted((f for f in original.functions if f['source'] == SOURCE), key=lambda f: f['low'])
        require(len(members) == 20 and sum(f['high'] - f['low'] for f in members) == 14616,
                'Reviewed motion membership changed')
        low, end = members[0]['low'], members[-1]['high']
        by_address = {f['low']: f for f in members}
        linkages = canonical_linkages(original.data, original.metadata)
        reference_flow = ControlFlow({low + i * 4: w for i, w in
                                     enumerate(words(original.read(low, end - low + 16)))})
        masks, all_pairs, transfer_map, table_pairs = {}, [], {}, {}
        for index, function in enumerate(members):
            a, size, name = function['low'], function['high'] - function['low'], function['name']
            b = start + a - low
            require(a % 16 == b % 16 == 0 and a in linkages, 'Motion alignment/linkage missing')
            if index:
                gap = a - members[index - 1]['high']
                require(0 <= gap < 16 and not any(original.read(a - gap, gap)) and
                        not any(target.read(b - gap, gap)), 'Non-padding interrupts the complete motion TU')
            first, second = dispatches(original, a, size, name), dispatches(target, b, size, name)
            shape = lambda proof: {k: v for k, v in proof.items() if k not in ('table_address', 'table_sha256')}
            require([shape(p) for p in first] == [shape(p) for p in second], 'Original motion dispatch relationships differ')
            for left, right in zip(first, second):
                table_pairs[left['table_address']] = right['table_address']
                if name == 'xEntMotionDebugWrite':
                    for diamond in DIAMONDS:
                        require(not any(diamond - 16 <= x <= diamond for x in right['target_offsets']),
                                'Table target bypasses a string reaching definition')
            local = flow.bounds(b, size, resolved_indirect_jumps={b + p['jump_offset']:
                                [b + x for x in p['target_offsets']] for p in second})
            ref_bounds = reference_flow.bounds(a, size, resolved_indirect_jumps={a + p['jump_offset']:
                                [a + x for x in p['target_offsets']] for p in first})
            require(local['passes'] and ref_bounds['passes'], 'Motion local original CFG does not close')
            resolver = diamond_pair if name == 'xEntMotionDebugWrite' else None
            pairs, calls, mask = compare(original, target, a, b, size, address_resolver=resolver)
            masks.update({a - low + off: value for off, value in mask.items()})
            all_pairs.extend(pairs)
            for call in calls:
                c, d = call['reference_address'], call['target_address']
                require(c not in transfer_map or transfer_map[c] == d, 'Motion callee relationship changed')
                transfer_map[c] = d
            proof = {'version': version, 'executable_sha1': original.sha1, 'source_address': a,
                     'name': name, 'source': SOURCE, 'linkage_name': linkages[a],
                     'reference_sha256': digest(original.read(a, size)), 'data_address_operands': pairs,
                     'direct_transfers': calls, 'dispatch_tables': first}
            if name == 'xEntMotionDebugWrite':
                proof['reviewed_string_diamond_offsets'] = list(DIAMONDS)
            if version == REFERENCES[0]:
                functions.append({'name': name, 'source': SOURCE, 'address': b, 'size': size,
                                  'sha256': digest(target.read(b, size)), 'boundary_confirmation': True,
                                  'confirmation_kind': KIND, 'provenance': [proof],
                                  'corroboration': {'local_control_flow': local, 'dispatch_tables': second,
                                                   'direct_rooted_call_sites': sorted(rooted_calls[b])}})
            else:
                entry = functions[index]
                require((entry['name'], entry['address'], entry['size'], entry['provenance'][0]['linkage_name']) ==
                        (name, b, size, linkages[a]), 'Motion original ordered identities disagree')
                entry['provenance'].append(proof)
        data_map, literal_proofs = {}, []
        for pair in all_pairs:
            a, b = pair['reference_address'], pair['target_address']
            require(a not in data_map or data_map[a] == b, 'Motion data mapping is inconsistent')
            if a in data_map:
                continue
            data_map[a] = b
            if a in table_pairs:
                require(b == table_pairs[a], 'Motion table operand does not reach the proven table')
            elif low <= a < end:
                require(a in by_address and by_address[a]['name'] == 'xEntMotionDebugCB' and b == start + a - low,
                        'Motion callback operand is not the corresponding original entry')
            else:
                for binary, address in ((original, a), (target, b)):
                    region = binary._stream_regions['initialized_data']
                    require(region['address'] <= address < region['address'] + region['size'],
                            'Motion literal is outside initialized data')
                first, second = cstring(original, a), cstring(target, b)
                require(first == second, 'Motion original literal bytes differ')
                literal_proofs.append({'reference_address': a, 'target_address': b,
                                      'size_with_terminator': len(first), 'sha256': digest(first)})
        require(len(data_map) == 89 and len(literal_proofs) == 84 and len(table_pairs) == 4 and
                len(set(data_map.values())) == len(data_map), 'Motion data inventory/bijection changed')
        require(len(transfer_map) == 38 and len(set(transfer_map.values())) == 38,
                'Motion call inventory/bijection changed')
        for a, b in sorted(transfer_map.items()):
            if low <= a < end:
                require(a in by_address and b == start + a - low, 'Motion internal call changed ownership')
                continue
            neighbor = original.by_address.get(a)
            size = min(32, neighbor['high'] - a) if neighbor else 32
            pairs, calls, _ = compare(original, target, a, b, size)
            neighbors.append({'version': version, 'reference_address': a, 'target_address': b,
                              'reference_name': neighbor['name'] if neighbor else None,
                              'reference_source': neighbor['source'] if neighbor else None,
                              'compared_entry_prefix_bytes': size, 'target_sha256': digest(target.read(b, size)),
                              'reference_sha256': digest(original.read(a, size)),
                              'data_address_operands': pairs, 'direct_transfers': calls,
                              'promoted_as_named_anchor': False})
        sequences.append({'version': version, 'source': SOURCE, 'source_start': low, 'target_start': start,
                          'sequence_bytes_with_alignment': end - low,
                          'uniqueness': unique_template(target, original.read(low, end - low), masks, start),
                          'data_address_count': len(data_map), 'direct_callee_count': len(transfer_map)})
        data_proofs.append({'version': version, 'source': SOURCE, 'literals': literal_proofs,
                            'table_addresses': [{'reference_address': a, 'target_address': b}
                                                for a, b in sorted(table_pairs.items())]})
    groups = {}
    for neighbor in neighbors:
        groups.setdefault(neighbor['target_address'], []).append(neighbor)
    for entries in groups.values():
        require({e['version'] for e in entries} == set(REFERENCES) and len(entries) == 3 and
                len({(e['reference_name'], e['reference_source'], e['compared_entry_prefix_bytes']) for e in entries}) == 1,
                'Motion original call-neighbor identities differ')
    require(sum(bool(f['corroboration']['direct_rooted_call_sites']) for f in functions) == 5,
            'Motion rooted entry witness inventory changed')
    return {'functions': functions, 'sequence_proofs': sequences, 'data_proofs': data_proofs, 'call_neighbors': neighbors,
            'counts': {'functions': 20, 'code_bytes': 14616, 'source_units': 1, 'rooted_direct_call_entries': 5,
                       'closed_return_bodies': 20, 'reviewed_dispatch_tables': 4, 'reviewed_string_diamonds': 5}}
