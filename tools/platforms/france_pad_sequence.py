"""Original-only French pad sequence, typed arrays, and unpromoted call context."""
from __future__ import annotations

import json

from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow
from platforms.france_tu_sequences import (
    KIND, address_pair, compare, digest, gpr_writes, unique_template, words,
)
from platforms.france_update_cull_sequence import checked_identity
from platforms.ps2_source import canonical_linkages
from platforms.dwarf1 import iter_dies
from platforms.ps2_type_layouts import aggregate_layouts

SOURCE = 'SB/Core/x/xPad.cpp'
START = 0x1f3950
MEMBERS = [('xPadAddRumble', 448), ('xPadDestroyRumbleChain', 136),
           ('xPadKill', 8), ('xPadNormalizeAnalog', 688), ('xPadUpdate', 1672),
           ('xPadRumbleEnable', 212), ('xPadEnable', 272), ('xPadInit', 100)]
OPAQUE = (1133248, 1149960)
BYTE_OPERANDS = {(168, 172, 36), (180, 188, 36), (228, 232, 36), (240, 248, 36),
                 (280, 296, 36), (304, 308, 36), (316, 320, 36), (328, 332, 36),
                 (340, 344, 36), (352, 356, 36), (364, 376, 36), (384, 388, 36),
                 (396, 408, 36), (416, 420, 36), (428, 432, 36), (440, 444, 36),
                 (452, 464, 36), (468, 476, 36), (480, 488, 40), (492, 496, 40),
                 (500, 504, 36), (520, 552, 36)}


def pad_context_pair(body, address, offset, *, allow_return_store=False):
    """Support only the actual byte accesses in the 868-byte iPadUpdate context.

    This is the strict ordinary LUI lifetime check for the locally enumerated
    LBU/SB consumers. It never promotes platform functions or data identities.
    """
    code = words(body)
    low = code[offset // 4]
    if low >> 26 not in (36, 40):
        return address_pair(body, address, offset, allow_return_store=allow_return_store)
    require(len(body) == 868, 'Byte-address context is restricted to iPadUpdate')
    register = low >> 21 & 31
    require(register != 0, 'Byte access lacks an address register')
    high_offset = None
    for off in range(offset - 4, -1, -4):
        word = code[off // 4]
        if register in gpr_writes(word):
            require(word >> 26 == 15 and word >> 21 & 31 == 0,
                    'Byte address register is not produced by LUI')
            high_offset = off
            break
    require(high_offset is not None, 'Byte address lacks a LUI definition')
    require((high_offset, offset, low >> 26) in BYTE_OPERANDS,
            'Unreviewed iPadUpdate byte-address producer/consumer pair')
    flow = ControlFlow({address + i * 4: w for i, w in enumerate(code)})
    if high_offset >= 4:
        previous = flow.instruction(address + high_offset - 4)
        require(previous['kind'] == 'normal' or
                (previous['kind'] == 'branch' and not previous.get('likely')),
                'Byte address LUI has an unsupported delay-slot lifetime')
    for off in range(high_offset + 4, offset, 4):
        instruction = flow.instruction(address + off)
        require(instruction['kind'] in ('normal', 'branch') or
                (instruction['kind'] == 'call' and off == offset - 4 and register != 31),
                'A transfer interrupts the byte-address lifetime')
    for off in range(0, len(body), 4):
        instruction = flow.instruction(address + off)
        destination = instruction.get('target')
        if destination is not None and address + high_offset < destination <= address + offset:
            require(off == high_offset - 4 and instruction['kind'] == 'branch',
                    'A direct edge bypasses the byte-address definition')
    high = code[high_offset // 4]
    immediate = (low & 65535) - (65536 if low & 32768 else 0)
    return high_offset, (((high & 65535) << 16) + immediate) & 0xffffffff


def typed_arrays(original):
    section = next(s for s in original.metadata['sections'] if s['name'] == '.debug' and s['size'])
    debug = original.data[section['offset']:section['offset'] + section['size']]
    dies = list(iter_dies(debug))
    layouts = aggregate_layouts(debug, SOURCE, {'_tagxPad', '_tagxRumble'})
    pad_fields = {m['name']: m['offset'] for m in layouts['_tagxPad']['members']}
    rumble_fields = {m['name']: m['offset'] for m in layouts['_tagxRumble']['members']}
    require(pad_fields['rumble_head'] == 68 and rumble_fields['next'] == 8,
            'Pad interior address is not the original rumble_head.next member')
    index = {off: (tag, owner, attrs) for off, tag, owner, attrs in dies}
    result = []
    for name, count, element_name, element_size in (
            ('mPad', 4, '_tagxPad', 328), ('mRumbleList', 32, '_tagxRumble', 16)):
        declarations = [(off, attrs) for off, tag, owner, attrs in dies if tag in (7, 12) and
                        owner.replace('\\', '/').endswith(SOURCE) and attrs.get(3) == name]
        require(len(declarations) == 1, 'Original pad array declaration is ambiguous')
        die, attrs = declarations[0]
        location = attrs.get(2)
        require(isinstance(location, bytes) and len(location) == 5 and location[0] == 3,
                'Original pad array lacks an absolute address')
        require(attrs.get(7) in index, 'Original pad array type is missing')
        tag, _, typ = index[attrs[7]]
        desc = typ.get(10)
        prefix = bytes.fromhex('000a0000000000') + (count - 1).to_bytes(4, 'little') + bytes.fromhex('087200')
        require(tag == 1 and typ.get(9) == 0 and isinstance(desc, bytes) and len(desc) == 18 and
                desc[:14] == prefix, 'Pad array does not have its original element count')
        element_die = int.from_bytes(desc[-4:], 'little')
        require(element_die in index, 'Original pad array element type is missing')
        tag, _, element = index[element_die]
        require(tag == 2 and element.get(3) == element_name and element.get(11) == element_size,
                'Original pad array element identity or size changed')
        result.append({'name': name, 'reference_address': int.from_bytes(location[1:], 'little'),
                       'declaration_die': die, 'type_die': attrs[7], 'element_die': element_die,
                       'element_name': element_name, 'element_bytes': element_size,
                       'elements': count, 'size': count * element_size, 'descriptor': desc.hex()})
    return result


def kill_tail(original, target, reference, address, calls):
    require(len(calls) == 1 and calls[0]['offset'] == 0 and calls[0]['opcode'] == 2,
            'Pad kill is not the original sole terminal jump')
    call = calls[0]
    callee = original.by_address.get(call['reference_address'])
    require(callee is not None and (callee['name'], callee['source'], callee['high'] - callee['low']) ==
            ('iPadKill', 'SB/Core/p2/iPad.cpp', 8), 'Pad kill tail changed original callee')
    for binary, entry, destination in ((original, reference, call['reference_address']),
                                       (target, address, call['target_address'])):
        require(binary.word(entry) >> 26 == 2 and binary.word(entry + 4) == 0,
                'Pad kill wrapper is not J/NOP')
        require(binary.read(destination, 8) == bytes.fromhex('0800e00300000000'),
                'Pad kill callee is not the complete original JR RA/NOP leaf')
        flow = ControlFlow({destination + 4 * i: w for i, w in
                            enumerate(words(binary.read(destination, 24)))})
        require(flow.bounds(destination, 8)['passes'], 'Pad kill context does not strictly close')
    return {'passes': True, 'boundary_kind': 'reviewed-pad-kill-j-nop-to-empty-return-leaf',
            'target_context_address': call['target_address'], 'target_context_bytes': 8,
            'callee_identity_promoted': False, 'unchanged_sp_ra': True}


def generate_unit(originals, registry_dir):
    target = originals[TARGET]
    known = {}
    for path in sorted(registry_dir.glob('*functions.json')):
        for f in json.loads(path.read_text()).get('functions', []):
            if f['source'] == SOURCE and f['name'] != 'xPadNormalizeAnalog':
                continue
            if f['address'] in known:
                require(all(known[f['address']][k] == f[k] for k in ('name', 'source', 'size', 'sha256')),
                        'Conflicting independent pad context')
            known[f['address']] = f
    anchors = [f for f in known.values() if f['source'] == SOURCE]
    require(len(anchors) == 1 and (anchors[0]['name'], anchors[0]['address'], anchors[0]['size']) ==
            ('xPadNormalizeAnalog', 0x1f3bb0, 688), 'Expected the independently confirmed NormalizeAnalog neighbor')
    functions, sequences, data_proofs, contexts = {}, [], [], []
    for version in REFERENCES:
        original = originals[version]
        members = sorted((f for f in original.functions if f['source'] == SOURCE), key=lambda f: f['low'])
        require([(f['name'], f['high'] - f['low']) for f in members] == MEMBERS,
                'Complete original pad membership changed')
        low, end = members[0]['low'], members[-1]['high']
        require(end - low == 3572, 'Original pad sequence span changed')
        shift = START - low
        neighbor = next(f for f in members if f['name'] == 'xPadNormalizeAnalog')
        checked_identity(original, target, neighbor['low'], neighbor['low'] + shift, known)
        arrays = typed_arrays(original)
        inside = {f['low']: f for f in members}
        linkages = canonical_linkages(original.data, original.metadata)
        masks, mapping, pairmap, callees = {}, {}, {}, {}
        for i, f in enumerate(members):
            a, b, size, name = f['low'], f['low'] + shift, f['high'] - f['low'], f['name']
            require(a in linkages and a % 16 == b % 16 == 0, 'Original pad linkage/alignment changed')
            if i:
                gap = a - members[i - 1]['high']
                require(0 <= gap < 16 and not any(original.read(a - gap, gap)) and
                        not any(target.read(b - gap, gap)), 'Pad alignment bytes changed')
            pairs, calls, mask = compare(original, target, a, b, size)
            bounds = None
            for binary, entry in ((original, a), (target, b)):
                flow = ControlFlow({entry + 4 * j: w for j, w in enumerate(words(binary.read(entry, size + 16)))})
                bounds = flow.bounds(entry, size)
                if not bounds['passes']:
                    require(name == 'xPadKill' and size == 8, 'Unreviewed pad control-flow boundary')
                    bounds = kill_tail(original, target, a, b, calls)
            for pair in pairs:
                ra, tb = pair['reference_address'], pair['target_address']
                require(ra not in pairmap or pairmap[ra] == tb, 'Pad data operand mapping is inconsistent')
                pairmap[ra] = tb
                owners = [g for g in arrays if g['reference_address'] <= ra < g['reference_address'] + g['size']]
                require(len(owners) == 1 and pair['opcode'] == 9 and pair['storage'] == 'zero_fill',
                        'Changed pad data operand lacks original typed-array ownership')
                g = owners[0]
                offset = ra - g['reference_address']
                require(offset == 0 or (g['name'] == 'mPad' and offset == 76),
                        'Unreviewed original pad array interior address')
                base = tb - offset
                require(g['name'] not in mapping or mapping[g['name']] == base, 'Pad array bases disagree')
                mapping[g['name']] = base
                for binary, start in ((original, g['reference_address']), (target, base)):
                    region = binary._stream_regions['runtime_bss']
                    require(region['address'] <= start and start + g['size'] <= region['address'] + region['size'],
                            'Complete original pad array escapes runtime BSS')
            for call in calls:
                ra, tb = call['reference_address'], call['target_address']
                require(ra not in callees or callees[ra] == tb, 'Pad callee relationships disagree')
                callees[ra] = tb
                if ra in inside:
                    require(tb == ra + shift, 'Pad internal call changes original member ownership')
                elif tb in known:
                    if any(p['version'] == version for p in known[tb]['provenance']):
                        checked_identity(original, target, ra, tb, known)
                    else:
                        ref, entry = original.by_address.get(ra), known[tb]
                        require(ref is not None and entry['boundary_confirmation'] and
                                (ref['name'], ref['source'], ref['high'] - ra) ==
                                (entry['name'], entry['source'], entry['size']) and
                                digest(target.read(tb, entry['size'])) == entry['sha256'],
                                'Independent pad callee identity changed')
                elif ra not in original.by_address:
                    require(ra == tb and tb in OPAQUE and call['opcode'] == 3 and
                            original.word(a + call['offset']) == target.word(b + call['offset']) and
                            original.read(ra, 64) == target.read(tb, 64), 'Opaque runtime literal/context changed')
                    mask.pop(call['offset'], None)
                    call.update(opaque_context_bytes=64, opaque_context_sha256=digest(target.read(tb, 64)),
                                caller_word_unmasked=True, no_identity_or_extent_claim=True)
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
                        record['provenance'][0]['linkage_name'] == linkages[a]), 'Original pad identities disagree')
                record['provenance'].append({'version': version, 'executable_sha1': original.sha1,
                    'source_address': a, 'name': name, 'source': SOURCE, 'linkage_name': linkages[a],
                    'reference_sha256': digest(original.read(a, size)),
                    'data_address_operands': pairs, 'direct_transfers': calls})
        require(len(mapping) == 2 and len(set(mapping.values())) == 2 and
                len(set(pairmap.values())) == len(pairmap) and len(set(callees.values())) == len(callees),
                'Distinct pad data or callees collapse')
        require(mapping['mRumbleList'] + 512 == mapping['mPad'] and
                arrays[1]['reference_address'] + 512 == arrays[0]['reference_address'],
                'Original adjacent pad-array layout changed')
        for ra, tb in sorted(callees.items()):
            if ra in inside or ra not in original.by_address:
                continue
            if tb in known and all(any(p['version'] == v for p in known[tb]['provenance']) for v in REFERENCES):
                continue
            callee = original.by_address[ra]
            size = callee['high'] - ra
            require(tb in known or callee['source'] == 'SB/Core/p2/iPad.cpp' or
                    (callee['source'], callee['name'], size) in
                    (('SB/Game/zMenu.cpp', 'zMenuRunning', 8),
                     ('SB/Game/zScene.cpp', 'zScene_ScreenAdjustMode', 8)),
                    'Unreviewed external pad context owner')
            pairs, calls, _ = compare(original, target, ra, tb, size, address_resolver=pad_context_pair)
            contexts.append({'version': version, 'reference_address': ra, 'target_address': tb,
                'reference_name': callee['name'], 'reference_source': callee['source'], 'size': size,
                'reference_sha256': digest(original.read(ra, size)), 'target_sha256': digest(target.read(tb, size)),
                'data_address_operands': pairs, 'direct_transfers': calls,
                'promoted_as_named_anchor': False, 'target_extent_claimed': False})
        sequences.append({'version': version, 'source': SOURCE, 'source_start': low, 'target_start': START,
            'complete_original_members': 8, 'previously_confirmed_members': 1,
            'sequence_bytes_with_alignment': end - low,
            'uniqueness': unique_template(target, original.read(low, end - low), masks, START)})
        data_proofs.append({'version': version,
            'typed_globals': [{**g, 'target_address': mapping[g['name']]} for g in arrays],
            'data_extents_promoted': False})
    groups = {}
    for context in contexts:
        groups.setdefault(context['target_address'], []).append(context)
    require(all(len(group) == 3 and {c['version'] for c in group} == set(REFERENCES) and
                len({(c['reference_name'], c['reference_source'], c['size']) for c in group}) == 1
                for group in groups.values()), 'Original pad call contexts disagree')
    require(len(functions) == 7 and sum(f['size'] for f in functions.values()) == 2848 and
            all(len(f['provenance']) == 3 for f in functions.values()),
            'All seven added pad members require three original witnesses')
    return {'functions': sorted(functions.values(), key=lambda f: f['address']),
            'sequence_proofs': sequences, 'call_neighbors': contexts, 'data_proofs': data_proofs,
            'counts': {'functions': 7, 'code_bytes': 2848, 'source_units': 1,
                       'closed_return_bodies': 6, 'reviewed_pad_kill_tails': 1}}
