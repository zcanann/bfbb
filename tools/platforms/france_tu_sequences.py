"""Verify reviewed French whole-TU sequences using only retail originals.

This is a scoped sequence proof, not a general fuzzy-symbol promotion rule. It
preserves every opcode, register, arithmetic immediate and branch displacement.
Only direct transfers and proven LUI-derived address operands may vary. The
result establishes named extents, never source matches or recovered relocations.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from platforms.verify_reviewed import Original, REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow, rooted_graph
from platforms.ps2_source import canonical_linkages
from platforms.dwarf1 import iter_dies

SOURCE = 'SB/Core/x/xstransvc.cpp'
# Reviewed candidate placement. Every invocation independently checks the whole
# sequence's unique location, all three named originals and local boundaries.
START = 0x20ccd0
KIND = 'reviewed-whole-tu-sequence'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def words(body):
    return list(struct.unpack('<' + 'I' * (len(body) // 4), body))


def gpr_writes(w):
    """Conservative subset encountered between this TU's address halves."""
    op, rs, rt, rd, fn = w >> 26, w >> 21 & 31, w >> 16 & 31, w >> 11 & 31, w & 63
    if op == 0:
        if fn in (8, 12, 13, 15, 17, 19, 24, 25, 26, 27):
            return set()
        if fn in (0, 2, 3, 4, 6, 7, 9, 10, 11, 16, 18, 20, 22, 23,
                  32, 33, 34, 35, 36, 37, 38, 39, 42, 43, 44, 45, 46, 47,
                  56, 58, 59, 60, 62, 63):
            return {rd}
    if op in (8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 30,
              32, 33, 34, 35, 36, 37, 38, 39, 55):
        return {rt}
    if op in (2, 4, 5, 6, 7, 20, 21, 22, 23, 31, 40, 41, 42, 43, 44, 45, 46, 49, 57, 63):
        return set()
    if op == 3:
        return {31}
    if op == 1 and rt in (0, 1, 2, 3):
        return set()
    if op in (16, 17, 18) and rs in (0, 1, 2):
        return {rt}
    if op in (16, 17, 18) and rs in (4, 5, 6, 8):
        return set()
    # COP1 MOV.S has fmt=S, a reserved zero ft field and function=6.
    # Only its FPR destination changes; both address GPR halves stay live.
    if op == 17 and rs == 16 and rt == 0 and fn == 6:
        return set()
    raise ValueError(f'Unsupported GPR effects: {w:#x}')


def address_pair(body, address, offset, *, allow_return_store=False):
    """Find a non-clobbered LUI reaching this actual ADDIU/LW/SW operand.

    Calls may occur only immediately before LO: its delay slot executes before
    the callee. Direct edges cannot bypass LUI, except a branch whose own delay
    slot is that LUI (both taken edges execute it). No indirect transfers pass.
    A separate opt-in permits a final JR $ra followed immediately by a low SW
    address consumer, with a non-$ra address register. Strict callers reject it.
    """
    code = words(body)
    low = code[offset // 4]
    register = low >> 21 & 31
    require(low >> 26 in (9, 35, 43) and register != 0, f'Not an eligible address operand: {address:#x}+{offset:#x}: {low:#x}')
    high_offset = None
    for off in range(offset - 4, -1, -4):
        word = code[off // 4]
        if register in gpr_writes(word):
            require(word >> 26 == 15 and word >> 21 & 31 == 0,
                    'Address register is not produced by LUI')
            high_offset = off
            break
    require(high_offset is not None, 'Address operand lacks LUI definition')
    flow = ControlFlow({address + i * 4: w for i, w in enumerate(code)})
    if high_offset >= 4:
        previous = flow.instruction(address + high_offset - 4)
        require(previous['kind'] == 'normal' or
                (previous['kind'] == 'branch' and not previous.get('likely')),
                'LUI in an annulled or call delay slot needs a separate lifetime proof')
    for off in range(high_offset + 4, offset, 4):
        instruction = flow.instruction(address + off)
        require(instruction['kind'] in ('normal', 'branch') or
                (instruction['kind'] == 'call' and off == offset - 4 and register != 31) or
                (allow_return_store and instruction['word'] == 0x03e00008 and
                 off == offset - 4 and offset == len(body) - 4 and low >> 26 == 43 and register != 31),
                'Call or unsupported transfer interrupts address lifetime')
    for off in range(0, len(body), 4):
        instruction = flow.instruction(address + off)
        destination = instruction.get('target')
        if destination is not None and address + high_offset < destination <= address + offset:
            require(off == high_offset - 4 and instruction['kind'] == 'branch',
                    'A direct edge bypasses the address definition')
    high = code[high_offset // 4]
    immediate = (low & 65535) - (65536 if low & 32768 else 0)
    return high_offset, (((high & 65535) << 16) + immediate) & 0xffffffff


def memory_kind(original, address):
    owners = [s for s in original.loaded if
              s['address'] <= address < s['address'] + s['file_size']]
    if len(owners) == 1:
        return 'file_backed'
    # France's stripped ELF omits p_memsz for BSS. Reuse the existing independent
    # startup SQ-zero loop and writable-section end proof, not a guessed extent.
    if not hasattr(original, '_stream_bss'):
        from types import SimpleNamespace
        from platforms.ps2_layout import startup_bss
        require(len(original.loaded) == 1, 'Expected one merged PS2 load')
        load = original.loaded[0]
        adapter = SimpleNamespace(word=original.word, version=original.version,
                                  metadata=original.metadata, load=load, named={},
                                  start=load['address'], end=load['address'] + load['file_size'])
        original._stream_bss = startup_bss(adapter)
    bss = original._stream_bss
    require(bss['address'] <= address < bss['end'], 'Data address lacks original storage')
    return 'zero_fill'


def compare(reference, target, a, b, size, *, allow_return_store=False, address_resolver=None):
    """Return explicit exceptions; all other actual instruction bits stay equal.

    A scoped address_resolver may prove a specific original reaching-definition
    pattern; the default remains the strict linear LUI lifetime checker.
    """
    first, second = reference.read(a, size), target.read(b, size)
    resolve = address_resolver or address_pair
    pairs, transfers, masks, high_fields = [], [], {}, set()
    for index, (x, y) in enumerate(zip(words(first), words(second))):
        off = index * 4
        if x >> 26 in (2, 3):
            require(x >> 26 == y >> 26, 'Direct transfer opcode changed')
            source_dest = ((a + off + 4) & 0xf0000000) | ((x & 0x3ffffff) << 2)
            target_dest = ((b + off + 4) & 0xf0000000) | ((y & 0x3ffffff) << 2)
            # A direct target must be executable, aligned, original file-backed code.
            for original, destination in ((reference, source_dest), (target, target_dest)):
                region = original._stream_regions['cpu_text']
                require(destination % 4 == 0 and region['address'] <= destination < region['address'] + region['size'],
                        'Transfer target is outside independently recovered CPU text')
            transfers.append({'offset': off, 'opcode': x >> 26,
                              'reference_address': source_dest, 'target_address': target_dest})
            masks[off] = 0xfc000000
        elif x != y and x >> 26 == y >> 26 == 15:
            require(x >> 16 == y >> 16, 'LUI register changed')
            high_fields.add(off)
            masks[off] = 0xffff0000
        elif x != y:
            require(x >> 16 == y >> 16, 'Non-address instruction bits changed')
            hi_a, data_a = resolve(first, a, off, allow_return_store=allow_return_store)
            hi_b, data_b = resolve(second, b, off, allow_return_store=allow_return_store)
            require(hi_a == hi_b, 'Address producer position changed')
            kind = memory_kind(reference, data_a)
            require(kind == memory_kind(target, data_b), 'Data storage class changed')
            pairs.append({'hi_offset': hi_a, 'lo_offset': off, 'opcode': x >> 26,
                          'reference_address': data_a, 'target_address': data_b, 'storage': kind})
            masks[off] = 0xffff0000
    require(high_fields <= {p['hi_offset'] for p in pairs},
            'Changed LUI lacks a checked data-address consumer')
    return pairs, transfers, masks


def named_data(original, name, source_owner=SOURCE):
    section = next(s for s in original.metadata['sections'] if s['name'] == '.debug' and s['size'])
    data = original.data[section['offset']:section['offset'] + section['size']]
    addresses = set()
    for _, tag, source, attrs in iter_dies(data):
        location = attrs.get(2)
        if (tag in (7, 12) and source.replace('\\', '/').endswith(source_owner) and attrs.get(3) == name and
                isinstance(location, bytes) and len(location) == 5 and location[0] == 3):
            addresses.add(int.from_bytes(location[1:], 'little'))
    require(len(addresses) == 1, 'Expected one authentic named streaming global')
    return addresses.pop()


def cstring(original, address):
    data = bytearray()
    for off in range(256):
        value = original.read(address + off, 1)
        data.extend(value)
        if value == b'\0':
            return bytes(data)
    raise ValueError('Unbounded streaming literal')


def unique_template(target, body, masks, address):
    """Search original loaded spans, not compiled code or a guessed-size slice."""
    code = words(body)
    runs, start = [], 0
    for off in sorted(masks) + [len(body)]:
        if off > start:
            runs.append((off - start, start))
        start = off + 4
    length, anchor = max(runs)
    require(length >= 32, 'Whole-TU template lacks a useful unmodified anchor')
    needle, hits = body[anchor:anchor + length], []
    for segment in target.loaded:
        span = target.read(segment['address'], segment['file_size'])
        position = span.find(needle)
        while position >= 0:
            begin = position - anchor
            if begin >= 0 and begin % 4 == 0 and begin + len(body) <= len(span):
                other = words(span[begin:begin + len(body)])
                if all((x & masks.get(i * 4, 0xffffffff)) == (y & masks.get(i * 4, 0xffffffff))
                       for i, (x, y) in enumerate(zip(code, other))):
                    hits.append(segment['address'] + begin)
            position = span.find(needle, position + 1)
    require(hits == [address], 'Whole-TU instruction template is not uniquely located')
    return {'matching_addresses': hits, 'unchanged_anchor_offset': anchor,
            'unchanged_anchor_bytes': length, 'masked_instruction_count': len(masks)}


def generate(manifest: Path, orig_dir: Path, registry_dir: Path) -> dict:
    records = json.loads(manifest.read_text(encoding='utf-8'))['versions']
    originals = {v: Original(v, records[v], orig_dir) for v in (*REFERENCES, TARGET)}
    target = originals[TARGET]
    from platforms.ps2_layout import generate_layouts
    layouts = generate_layouts(orig_dir, registry_dir.parent)
    for version, original in originals.items():
        original._stream_regions = {r['name']: r for r in layouts[version]['regions']}
    spans = [(s, target.read(s['address'], s['file_size'])) for s in target.loaded]
    flow = ControlFlow({s['address'] + i * 4: w for s, body in spans
                        for i, w in enumerate(words(body))})
    _, entry_calls, _ = rooted_graph(flow, target.metadata['entry_point'])
    functions, sequence_proofs, call_neighbors = [], [], []
    for version in REFERENCES:
        original = originals[version]
        members = sorted((f for f in original.functions if f['source'] == SOURCE), key=lambda f: f['low'])
        require(len(members) == 19 and sum(f['high'] - f['low'] for f in members) == 5584,
                'Reviewed original streaming TU membership changed')
        low, end = members[0]['low'], members[-1]['high']
        linkages = canonical_linkages(original.data, original.metadata)
        masks, all_pairs, all_calls = {}, [], []
        for index, function in enumerate(members):
            a, size = function['low'], function['high'] - function['low']
            b = START + a - low
            require(a % 16 == b % 16 == 0 and a in linkages, 'Original entry alignment/linkage missing')
            if index:
                gap = a - members[index - 1]['high']
                require(0 <= gap < 16 and not any(original.read(a - gap, gap)) and
                        not any(target.read(b - gap, gap)), 'Non-padding data or unrelated code interrupts TU sequence')
            pairs, calls, mask = compare(original, target, a, b, size)
            masks.update({a - low + off: value for off, value in mask.items()})
            all_pairs.extend(pairs)
            all_calls.extend(calls)
            local = flow.bounds(b, size)
            if size == 8:
                require(index == 3 and function['name'] == 'xST_xAssetID_HIPFullPath' and
                        len(calls) == 1 and calls[0]['opcode'] == 2 and calls[0]['offset'] == 0 and
                        calls[0]['reference_address'] == members[index - 1]['low'] and
                        calls[0]['target_address'] == START + members[index - 1]['low'] - low and
                        flow.valid_delay(b) and not any(target.read(b + size, 8)),
                        'Expected the reviewed eight-byte overload tail wrapper')
                local = {'passes': True, 'kind': 'direct-tail-wrapper-to-previous-confirmed-body',
                         'target': calls[0]['target_address'], 'delay_instruction': target.word(b + 4),
                         'following_zero_alignment': 8}
            else:
                require(local['passes'], f'{function["name"]}: local original CFG does not close')
            proof = {'version': version, 'executable_sha1': original.sha1, 'source_address': a,
                     'name': function['name'], 'source': SOURCE, 'linkage_name': linkages[a],
                     'reference_sha256': digest(original.read(a, size)),
                     'data_address_operands': pairs, 'direct_transfers': calls}
            if version == REFERENCES[0]:
                functions.append({'name': function['name'], 'source': SOURCE, 'address': b, 'size': size,
                                  'sha256': digest(target.read(b, size)), 'boundary_confirmation': True,
                                  'confirmation_kind': KIND, 'provenance': [proof],
                                  'corroboration': {'local_control_flow': local,
                                                   'direct_rooted_call_sites': sorted(entry_calls[b])}})
            else:
                entry = functions[index]
                require((entry['name'], entry['address'], entry['size'], entry['provenance'][0]['linkage_name']) ==
                        (function['name'], b, size, linkages[a]), 'Original ordered identities or extents disagree')
                entry['provenance'].append(proof)
        # Absolute data relationships are checked independently of C objects.
        state = named_data(original, 'g_xstdata')
        destinations = {p['target_address'] for p in all_pairs if p['reference_address'] == state}
        require(len(destinations) == 1, 'No unique observed French streaming-state address')
        french_state = destinations.pop()
        data_map = {}
        for pair in all_pairs:
            a, b = pair['reference_address'], pair['target_address']
            require(a not in data_map or data_map[a] == b, 'One reference data address mapped inconsistently')
            data_map[a] = b
            if pair['storage'] == 'zero_fill':
                require(a - state == b - french_state, 'Streaming zero-fill relative layout differs')
            else:
                for binary, address in ((original, a), (target, b)):
                    region = binary._stream_regions['initialized_data']
                    require(region['address'] <= address < region['address'] + region['size'],
                            'Streaming literal is outside initialized data')
                require(cstring(original, a) == cstring(target, b), 'Original streaming string bytes differ')
        require(len(set(data_map.values())) == len(data_map), 'Distinct data addresses were collapsed')
        # Call-site correspondence must be one-to-one. Neighbors corroborate the
        # sequence but do not become new named relocation anchors or code units.
        transfer_map = {}
        for call in all_calls:
            a, b = call['reference_address'], call['target_address']
            require(a not in transfer_map or transfer_map[a] == b, 'Callee relationship changed between callers')
            transfer_map[a] = b
        require(len(set(transfer_map.values())) == len(transfer_map), 'Distinct callees collapsed')
        for a, b in sorted(transfer_map.items()):
            if low <= a < end:
                require(a in {f['low'] for f in members} and b == START + a - low,
                        'Internal TU transfer changed ownership or offset')
                continue
            neighbor = original.by_address.get(a)
            if neighbor:
                size = neighbor['high'] - a
                pairs, calls, _ = compare(original, target, a, b, size)
                call_neighbors.append({'version': version, 'reference_address': a, 'target_address': b,
                                       'reference_name': neighbor['name'], 'reference_source': neighbor['source'],
                                       'size': size, 'reference_sha256': digest(original.read(a, size)),
                                       'target_sha256': digest(target.read(b, size)),
                                       'address_operand_count': len(pairs), 'direct_transfer_count': len(calls),
                                       'promoted_as_named_anchor': False})
            else:
                pairs, calls, _ = compare(original, target, a, b, 32)
                call_neighbors.append({'version': version, 'reference_address': a, 'target_address': b,
                                       'compared_entry_prefix_bytes': 32, 'sha256': digest(target.read(b, 32)),
                                       'data_address_operands': pairs, 'direct_transfers': calls,
                                       'promoted_as_named_anchor': False})
        uniqueness = unique_template(target, original.read(low, end - low), masks, START)
        sequence_proofs.append({'version': version, 'source_start': low, 'target_start': START,
                                'sequence_bytes_with_alignment': end - low, 'uniqueness': uniqueness,
                                'data_address_count': len(data_map), 'direct_callee_count': len(transfer_map),
                                'named_reference_state_address': state, 'observed_target_state_address': french_state})
    neighbor_groups = {}
    for neighbor in call_neighbors:
        neighbor_groups.setdefault(neighbor['target_address'], []).append(neighbor)
    for neighbors in neighbor_groups.values():
        require({n['version'] for n in neighbors} == set(REFERENCES) and len(neighbors) == 3,
                'External call neighborhood differs across debug originals')
        identities = {(n.get('reference_name'), n.get('reference_source'), n.get('size')) for n in neighbors}
        require(len(identities) == 1, 'Debug originals disagree on a call-neighbor identity')
    require(len([f for f in functions if f['corroboration']['direct_rooted_call_sites']]) == 17,
            'Reviewed sequence entry witnesses changed')
    document = {'schema_version': 1, 'version': TARGET, 'executable_sha1': target.sha1,
            'coverage_complete': False, 'source_comparison_available': False, 'source_link_verified': False,
            'status': 'reviewed-original-whole-tu-sequence',
            'original_sha1s': {v: originals[v].sha1 for v in (*REFERENCES, TARGET)},
            'method': 'Unique whole named-DWARF TU sequence in three originals; explicit address-producer and call relationships, unchanged non-address bits, closed local bounds and zero alignment.',
            'limitations': ['Only the reviewed streaming, particle-command, animation, entity-motion, math, collision, entity, clump-collision, spline and Common/Standard NPC goal TUs are eligible; this is not an automatic fuzzy-symbol promotion rule.',
                           'Names and ownership come from debug-reference DWARF, not a recovered France symbol table.',
                           'External call neighbors corroborate structure but are not independently promoted named relocation anchors.',
                           'Seventeen streaming entries have rooted direct-call witnesses; the remaining overload is reached by its verified tail wrapper, while shutdown also uses the unique complete streaming TU sequence.',
                           'Data operands preserve observed original relationships; no new data symbol sizes or public binary data are invented.',
                           'No compiled candidate is consulted. Named extents do not imply a source match or complete retail link.'],
            'counts': {'functions': len(functions), 'code_bytes': sum(f['size'] for f in functions),
                       'source_units': 1, 'rooted_direct_call_entries': 17, 'closed_return_bodies': 18,
                       'reviewed_tail_wrappers': 1},
            'sequence_proofs': sequence_proofs, 'call_neighbors': call_neighbors, 'functions': functions}
    from platforms.france_particle_sequence import generate_unit
    particles = generate_unit(originals)
    document['functions'].extend(particles['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(particles['sequence_proofs'])
    document['call_neighbors'].extend(particles['call_neighbors'])
    document['particle_registration_proofs'] = particles['registration_proofs']
    for key, value in particles['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Particle callback identities additionally use all 81 concrete stores in the original initializer; no compiled table or guessed record layout is used.')
    from platforms.france_animation_sequence import generate_unit as generate_animation
    animation = generate_animation(originals)
    document['functions'].extend(animation['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(animation['sequence_proofs'])
    document['call_neighbors'].extend(animation['call_neighbors'])
    document['animation_data_proofs'] = animation['data_proofs']
    for key, value in animation['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Animation uses two explicitly reviewed frame-free leaf tails and three dead copies of executed delay-slot instructions; the shared strict CFG checker is unchanged.')
    from platforms.france_motion_sequence import generate_unit as generate_motion
    motion = generate_motion(originals)
    document['functions'].extend(motion['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(motion['sequence_proofs'])
    document['call_neighbors'].extend(motion['call_neighbors'])
    document['motion_data_proofs'] = motion['data_proofs']
    for key, value in motion['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Motion explicitly validates four original bounded dispatch tables and five string-address diamonds; all other indirect jumps and address patterns remain strict.')
    from platforms.france_math_sequence import generate_unit as generate_math
    math = generate_math(originals)
    document['functions'].extend(math['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(math['sequence_proofs'])
    document['call_neighbors'].extend(math['call_neighbors'])
    document['math_data_proofs'] = math['data_proofs']
    for key, value in math['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Math uses three reviewed leaf tails and explicit float-memory addresses for six original DWARF-named aggregates; unknown COP1 operations are not assumed harmless.')
    from platforms.france_collision_sequence import generate_unit as generate_collision
    collision = generate_collision(originals)
    document['functions'].extend(collision['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(collision['sequence_proofs'])
    document['call_neighbors'].extend(collision['call_neighbors'])
    document['collision_data_proofs'] = collision['data_proofs']
    for key, value in collision['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Collision uses two explicitly closed platform leaf-tail callees, six complete callback entries and original DWARF-typed global components; external entry prefixes are not promoted.')
    from platforms.france_entity_sequence import generate_unit as generate_entity
    entity = generate_entity(originals)
    document['functions'].extend(entity['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(entity['sequence_proofs'])
    document['call_neighbors'].extend(entity['call_neighbors'])
    document['entity_data_proofs'] = entity['data_proofs']
    for key, value in entity['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Entity verifies one exact pointer-return leaf, original typed arrays and one unnamed immutable interleaved leaf as context only; that context is not assigned source ownership or promoted.')
    from platforms.france_geometry_sequence import generate_unit as generate_geometry
    geometry = generate_geometry(originals)
    document['functions'].extend(geometry['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(geometry['sequence_proofs'])
    document['call_neighbors'].extend(geometry['call_neighbors'])
    document['geometry_data_proofs'] = geometry['data_proofs']
    for key, value in geometry['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Clump collision and splines retain strict CFG checks; original array descriptors prove two coefficient matrices and observed allocator-array slots without inferring a RenderWare struct layout.')
    from platforms.france_goal_sequence import generate_unit as generate_goals
    goals = generate_goals(originals)
    document['functions'].extend(goals['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(goals['sequence_proofs'])
    document['call_neighbors'].extend(goals['call_neighbors'])
    document['goal_data_proofs'] = goals['data_proofs']
    for key, value in goals['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Common/Standard goal identity uses unique complete sequences in three named originals plus full original vtables/globals. Rooted direct-call witnesses are empty; header interleaves and external table methods remain nonpromoted context. No missing NoManLand class layout or method is inferred.')
    from platforms.france_grid_sequence import generate_unit as generate_grid
    grid = generate_grid(originals, registry_dir)
    document['functions'].extend(grid['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(grid['sequence_proofs'])
    document['call_neighbors'].extend(grid['call_neighbors'])
    document['grid_data_proofs'] = grid['data_proofs']
    for key, value in grid['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Grid adds only xGridCheckPosition. Its eleven previously confirmed neighbors remain unchanged; all twelve original members form a unique sequence. The complete signed-int offs[4][3][2] array and four named calls are independently checked; external entry prefixes remain context only.')
    from platforms.france_quickcull_sequence import generate_unit as generate_quickcull
    quickcull = generate_quickcull(originals, registry_dir)
    document['functions'].extend(quickcull['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(quickcull['sequence_proofs'])
    document['call_neighbors'].extend(quickcull['call_neighbors'])
    for key, value in quickcull['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('QuickCull adds only two Init overloads using all eleven original members and nine independently confirmed neighbors. The 28-byte wrapper is exactly six argument loads and a terminal J to the adjacent strictly closed initializer; generic CFG checks remain strict.')
    from platforms.france_skb_sequence import generate_unit as generate_skb
    skb = generate_skb(originals, registry_dir)
    document['functions'].extend(skb['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(skb['sequence_proofs'])
    document['skb_data_proofs'] = skb['data_proofs']
    for key, value in skb['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('SKB adds Duration and Eval using the complete four-member sequence and two independently confirmed neighbors. All bodies satisfy the unchanged strict CFG checks; original DWARF and all 96 bytes prove the sole changed slerpPolynomial address.')
    from platforms.france_ffx_sequence import generate_unit as generate_ffx
    ffx = generate_ffx(originals, registry_dir)
    document['functions'].extend(ffx['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(ffx['sequence_proofs'])
    document['call_neighbors'].extend(ffx['call_neighbors'])
    for key, value in ffx['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('FFX adds seven members using all fourteen original functions and seven independently confirmed neighbors. Allocator context decodes original JALs from at least two other source units; unchanged opaque math calls remain unmasked and unnamed. Strict CFG checks remain unchanged.')
    from platforms.france_imath3_sequence import generate_unit as generate_imath3
    imath3 = generate_imath3(originals, registry_dir)
    document['functions'].extend(imath3['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(imath3['sequence_proofs'])
    document['call_neighbors'].extend(imath3['call_neighbors'])
    for key, value in imath3['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Platform geometry adds nine members using all fifteen original functions and six independently confirmed neighbors. Two opaque runtime calls retain literal unmasked JAL words and identical context only; no new callee identity, extent, or CFG exception is introduced.')
    from platforms.france_string_sequence import generate_unit as generate_strings
    strings = generate_strings(originals, registry_dir)
    document['functions'].extend(strings['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(strings['sequence_proofs'])
    document['call_neighbors'].extend(strings['call_neighbors'])
    document['string_dispatch_proofs'] = strings['data_proofs']
    for key, value in strings['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Strings adds four members from the complete twelve-member sequence, preserving eight prior neighbors. One exact twelve-entry dispatch uses the existing strict recognizer. The sole xatof call is corroborated by a complete six-member original math-tail sequence with three independent neighbors; its literal runtime tail remains unmasked context only, not a new anchor or progress extent.')
    from platforms.france_par_sequence import generate_unit as generate_par
    addition = generate_par(originals, registry_dir)
    document['functions'].extend(addition['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(addition['sequence_proofs'])
    document['call_neighbors'].extend(addition['call_neighbors'])
    document['par_data_proofs'] = addition['data_proofs']
    for key, value in addition['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Particle initialization adds one member from the complete four-function sequence. The original 2000-element xPar array descriptor bounds the sole changed pool address inside authentic BSS; no data extent is promoted.')
    from platforms.france_update_cull_sequence import generate_unit as generate_update_cull
    addition = generate_update_cull(originals, registry_dir)
    document['functions'].extend(addition['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(addition['sequence_proofs'])
    document['call_neighbors'].extend(addition['call_neighbors'])
    document['update_cull_data_proofs'] = addition['data_proofs']
    for key, value in addition['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Update culling adds three members from all seven original functions and four independent neighbors. The complete group-accessor/count context and independently reviewed allocator neighborhood corroborate calls and pointer slots; opaque runtime transfer words remain unmasked. No context becomes a new identity or extent.')
    from platforms.france_bound_sequence import generate_unit as generate_bound
    addition = generate_bound(originals, registry_dir)
    document['functions'].extend(addition['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(addition['sequence_proofs'])
    document['call_neighbors'].extend(addition['call_neighbors'])
    document['bound_data_proofs'] = addition['data_proofs']
    for key, value in addition['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Bounds adds six members from all nine original functions and three independent neighbors. The complete platform-geometry dependency is independently replayed for external calls; original DWARF proves the 60-byte quick-cull global, with no data extent promotion.')
    from platforms.france_scalar_math_sequence import generate_unit as generate_scalar_math
    scalar_math = generate_scalar_math(originals, registry_dir)
    document['functions'].extend(scalar_math['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(scalar_math['sequence_proofs'])
    document['call_neighbors'].extend(scalar_math['call_neighbors'])
    for key, value in scalar_math['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Scalar math adds nine members from all eighteen original functions and nine independent neighbors. Only the exact 60-byte EndPoints leaf tail and eight-byte literal xatof J/NOP have scoped boundary rules; generic CFG checks stay unchanged. Opaque runtime transfer words remain unmasked, unnamed context, and the two overloaded acceleration functions retain distinct original linkage identities.')
    from platforms.france_utility_sequence import generate_unit as generate_utility
    utility = generate_utility(originals, registry_dir)
    document['functions'].extend(utility['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(utility['sequence_proofs'])
    for key, value in utility['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Utility adds eight members from all eleven original functions and three independent neighbors. Its exact seven-entry dispatch is scoped locally; unsigned-byte lookup dataflow bounds twelve observed table reads. Original typed arrays and independently witnessed animation pointer operands corroborate changed addresses. Opaque runtime caller words remain unmasked, and no runtime identity or data extent is promoted.')
    from platforms.france_camera_tweak_sequence import generate_unit as generate_camera_tweak
    camera_tweak = generate_camera_tweak(originals, registry_dir)
    document['functions'].extend(camera_tweak['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(camera_tweak['sequence_proofs'])
    document['call_neighbors'].extend(camera_tweak['call_neighbors'])
    document['camera_tweak_data_proofs'] = camera_tweak['data_proofs']
    for key, value in camera_tweak['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Camera tweak adds eleven members from the complete twelve-function sequence and the existing Reset neighbor. Original member types prove the eight-element state array and two look aggregates. Only exact eight-byte J/NOP wrappers to independently verified, strictly closed base Save/Load functions receive scoped tail handling; opaque runtime calls retain literal unmasked words and identical 64-byte context, with no new identity or extent.')
    from platforms.france_pad_sequence import generate_unit as generate_pad
    pad = generate_pad(originals, registry_dir)
    document['functions'].extend(pad['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(pad['sequence_proofs'])
    document['call_neighbors'].extend(pad['call_neighbors'])
    document['pad_data_proofs'] = pad['data_proofs']
    for key, value in pad['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Pad adds seven members from the complete eight-function sequence and the existing NormalizeAnalog neighbor. Original typed arrays prove four pad records and thirty-two rumble records. Only the exact eight-byte Kill J/NOP to an independently compared empty-return leaf receives scoped tail handling. Complete external platform contexts remain unpromoted; locally enumerated byte-address consumers preserve strict LUI lifetimes, and opaque runtime calls retain literal unmasked words and identical 64-byte context.')
    from platforms.france_hangable_sequence import generate_unit as generate_hangable
    hangable = generate_hangable(originals, registry_dir)
    document['functions'].extend(hangable['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(hangable['sequence_proofs'])
    document['call_neighbors'].extend(hangable['call_neighbors'])
    document['hangable_data_proofs'] = hangable['data_proofs']
    for key, value in hangable['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Hangable adds ten members from all eleven original functions and the existing Setup neighbor. Original typed globals, the complete circle initializer, four strings and independently verified culling callbacks corroborate changed operands. Only exact eight-byte Load/Save J/NOP chains into independently verified base bodies receive scoped tail handling; external entry prefixes remain unpromoted context, and generic CFG checks remain unchanged.')
    from platforms.france_group_sequence import generate_unit as generate_group
    group = generate_group(originals, registry_dir)
    document['functions'].extend(group['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(group['sequence_proofs'])
    document['call_neighbors'].extend(group['call_neighbors'])
    document['group_data_proofs'] = group['data_proofs']
    for key, value in group['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Group adds eight members from the complete nine-function sequence and the existing item-accessor neighbor. Only exact Load/Save J/NOP wrappers into independently verified base bodies receive scoped tail handling. The event wrapper remains complete unpromoted original context; generic boundary checks remain unchanged.')
    from platforms.france_dutchman_tweaks import generate_unit as generate_dutchman
    dutchman = generate_dutchman(originals, registry_dir)
    document['functions'].extend(dutchman['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(dutchman['sequence_proofs'])
    document['call_neighbors'].extend(dutchman['call_neighbors'])
    document['dutchman_data_proofs'] = dutchman['data_proofs']
    for key, value in dutchman['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Dutchman proves one unique complete parameter-registration caller and three complete named helpers, not the whole translation unit. All four bodies pass strict closure checks in all three originals. Complete parameter strings and known helper callees corroborate all changed operands; one unchanged runtime prefix remains unpromoted. The distinct caller-callee-cluster kind preserves this narrower proof scope.')
    from platforms.france_event_sequence import generate_unit as generate_event
    event = generate_event(originals, registry_dir)
    document['functions'].extend(event['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(event['sequence_proofs'])
    document['call_neighbors'].extend(event['call_neighbors'])
    document['event_data_proofs'] = event['data_proofs']
    for key, value in event['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Event adds all ten original overloads rooted in a freshly regenerated group caller. Five strictly closed bodies and five exact register-only shuffle tails preserve complete original control flow. External callees are independently known; group regeneration excludes these dependent event entries so the original context cannot become a circular anchor.')
    from platforms.france_kingjelly_tweaks import generate_unit as generate_kingjelly
    kingjelly = generate_kingjelly(originals, registry_dir)
    document['functions'].extend(kingjelly['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(kingjelly['sequence_proofs'])
    document['call_neighbors'].extend(kingjelly['call_neighbors'])
    document['kingjelly_data_proofs'] = kingjelly['data_proofs']
    for key, value in kingjelly['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('King Jelly proves one unique complete parameter-registration caller and its complete float-list helper. Three-version names, bounds, strict returns, all instruction bits and complete strings agree. The earlier parameter helpers are independently regenerated; the complete original color helper remains unpromoted context. No whole translation-unit or data extent is claimed.')
    from platforms.france_sb2_tweaks import generate_unit as generate_sb2
    sb2 = generate_sb2(originals, registry_dir)
    document['functions'].extend(sb2['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(sb2['sequence_proofs'])
    document['sb2_data_proofs'] = sb2['data_proofs']
    for key, value in sb2['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('SB2 proves one unique complete parameter-registration body with all three original ownership and strict return witnesses. Complete strings, independently regenerated parameter helpers and original typed sound-array layouts corroborate every changed operand. Array element/member offsets are preserved; no whole translation-unit or data extent is claimed.')
    from platforms.france_plankton_tweaks import generate_unit as generate_plankton
    plankton = generate_plankton(originals, registry_dir)
    document['functions'].extend(plankton['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(plankton['sequence_proofs'])
    document['plankton_data_proofs'] = plankton['data_proofs']
    for key, value in plankton['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Plankton proves one unique complete parameter-registration body in all three originals, preserving strict returns, complete strings, independently regenerated parameter helpers and original typed sound arrays. Array element/member offsets remain exact; no whole translation-unit or data extent is claimed.')
    from platforms.france_cruise_tweaks import generate_unit as generate_cruise
    cruise = generate_cruise(originals, registry_dir)
    document['functions'].extend(cruise['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(cruise['sequence_proofs'])
    document['cruise_data_proofs'] = cruise['data_proofs']
    for key, value in cruise['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('CruiseBubble proves one unique complete parameter-registration caller in all three originals. Strict returns, all non-address instruction bits, complete strings and independently regenerated parameter helpers agree. The previously confirmed bounded hash entry remains an independently checked callee, with no duplicate extent or whole translation-unit claim.')
    from platforms.france_prawn_tweaks import generate_unit as generate_prawn
    prawn = generate_prawn(originals, registry_dir)
    document['functions'].extend(prawn['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(prawn['sequence_proofs'])
    document['prawn_data_proofs'] = prawn['data_proofs']
    for key, value in prawn['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Prawn proves one unique complete parameter-registration caller in all three originals. Strict original/target returns, all instruction bits, complete strings including repeated empty literals, independently regenerated parameter helpers and original typed sound arrays corroborate the body. No whole translation-unit or data extent is claimed.')
    from platforms.france_player_animation_tables import generate_unit as generate_player_tables
    player_tables = generate_player_tables(originals, registry_dir)
    document['functions'].extend(player_tables['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(player_tables['sequence_proofs'])
    document['player_animation_table_data_proofs'] = player_tables['data_proofs']
    for key, value in player_tables['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Player animation tables adds five unique complete builders using all three original ownership and strict-return witnesses. Complete strings, independently confirmed animation callees and complete callback contexts corroborate every changed operand, including German pointer relocations. The main builder remains excluded pending insertion-helper and callback evidence; no whole translation-unit or data extent is claimed.')
    from platforms.france_cruise_animation import generate_unit as generate_cruise_animation
    cruise_animation = generate_cruise_animation(originals, registry_dir)
    document['functions'].extend(cruise_animation['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(cruise_animation['sequence_proofs'])
    document['cruise_animation_data_proofs'] = cruise_animation['data_proofs']
    for key, value in cruise_animation['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Cruise animation adds three complete functions rooted in the independently regenerated parameter-registration anchor. Complete insertion strings, typed fields/tables, callback identity and full original runtime contexts corroborate all operands. The existing parameter body is preserved, and no whole translation-unit or data extent is claimed.')
    from platforms.france_sound_listeners import generate_unit as generate_sound_listeners
    sound_listeners = generate_sound_listeners(originals, registry_dir)
    document['functions'].extend(sound_listeners['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(sound_listeners['sequence_proofs'])
    document['sound_listeners_data_proofs'] = sound_listeners['data_proofs']
    for key, value in sound_listeners['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Sound listeners adds nine unique complete listener, voice-query and delayed-insertion bodies using all three originals. Strict control flow, the complete internal call and original typed sound arrays corroborate every changed operand. No registry identities are consumed, and no whole translation-unit or data extent is claimed.')
    from platforms.france_sound_playback import generate_unit as generate_sound_playback
    sound_playback = generate_sound_playback(originals, registry_dir)
    document['functions'].extend(sound_playback['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(sound_playback['sequence_proofs'])
    document['sound_playback_data_proofs'] = sound_playback['data_proofs']
    for key, value in sound_playback['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Sound playback adds nine complete bodies with all three original ownership and strict-control-flow witnesses. Typed objects, complete diagnostic strings, independently proved callees and literal opaque runtime contexts corroborate every operand. Vector identity uses the unique complete two-member sequence; one scoped runtime leaf and one complete flush template retain explicitly checked sizes and operand inventories. Two HIS bodies remain coverage-only. No opaque runtime identity, whole playback translation-unit or data extent is claimed.')
    from platforms.france_goalrobo_streaks import generate_unit as generate_goalrobo_streaks
    goalrobo_streaks = generate_goalrobo_streaks(originals, registry_dir)
    document['functions'].extend(goalrobo_streaks['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(goalrobo_streaks['sequence_proofs'])
    document['goalrobo_streaks_data_proofs'] = goalrobo_streaks['data_proofs']
    for key, value in goalrobo_streaks['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Robotic streaks adds four unique complete robotic callers and effects consumers using all three originals. Both full effects consumers agree on the original typed nested streak-array base, and every caller transfer reaches the complete update helper. No registry identities are consumed, and no whole translation-unit or data extent is claimed.')
    from platforms.france_goalrobo_motion import generate_unit as generate_goalrobo_motion
    goalrobo_motion = generate_goalrobo_motion(originals, registry_dir)
    document['functions'].extend(goalrobo_motion['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(goalrobo_motion['sequence_proofs'])
    document['goalrobo_motion_data_proofs'] = goalrobo_motion['data_proofs']
    for key, value in goalrobo_motion['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Robotic motion adds thirty-one unique complete original motion helpers and robotic consumers. Complete literal helper bodies and exactly thirty-four proved JAL destinations close every transfer; no changed data operand is accepted. Eight fixed independently proved destinations fence later recovery dependencies. No whole translation-unit or data extent is claimed.')
    from platforms.france_goalrobo_models import generate_unit as generate_goalrobo_models
    goalrobo_models = generate_goalrobo_models(originals, registry_dir)
    document['functions'].extend(goalrobo_models['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(goalrobo_models['sequence_proofs'])
    document['goalrobo_models_data_proofs'] = goalrobo_models['data_proofs']
    for key, value in goalrobo_models['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Robotic models adds nine unique complete original model helpers and robotic consumers. The complete find/show/hide helper graph and exactly nineteen proved JAL destinations close every transfer; no changed data operand is accepted. Three fixed independently proved destinations fence later recovery dependencies. No whole translation-unit or data extent is claimed.')
    from platforms.france_goalrobo_facing import generate_unit as generate_goalrobo_facing
    goalrobo_facing = generate_goalrobo_facing(originals, registry_dir)
    document['functions'].extend(goalrobo_facing['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(goalrobo_facing['sequence_proofs'])
    document['goalrobo_facing_data_proofs'] = goalrobo_facing['data_proofs']
    for key, value in goalrobo_facing['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Robotic facing adds five unique complete original facing helpers and robotic consumers. Exactly sixteen original JALs, six fixed independent identities and one literal unnamed runtime context close every transfer. One complete forty-byte helper uses an exhaustive scoped twenty-byte seed with exactly one proved call operand masked; generic uniqueness thresholds are unchanged. No runtime identity, whole translation-unit or data extent is claimed.')
    from platforms.france_npc_helpers import generate_unit as generate_npc_helpers
    npc_helpers = generate_npc_helpers(originals, registry_dir)
    document['functions'].extend(npc_helpers['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(npc_helpers['sequence_proofs'])
    document['npc_helpers_data_proofs'] = npc_helpers['data_proofs']
    for key, value in npc_helpers['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('NPC helpers adds thirty-one unique complete original navigation, collision and robotic bodies. Exactly thirty-four JAL destinations follow an acyclic complete-body graph or nineteen fixed independent identities. No changed data operand, whole translation-unit or data extent is claimed.')
    from platforms.france_npc_player_model import generate_unit as generate_npc_player_model
    npc_player_model = generate_npc_player_model(originals, registry_dir)
    document['functions'].extend(npc_player_model['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(npc_player_model['sequence_proofs'])
    document['npc_player_model_data_proofs'] = npc_player_model['data_proofs']
    for key, value in npc_player_model['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('NPC player-model adds eight unique complete original consumers. Nine data operands follow the original typed globals.player.ent.model path and an independent complete CalcNewDir anchor; nineteen JALs close through complete identities or two literal unnamed runtime contexts. No runtime identity, whole translation-unit or data extent is claimed.')
    from platforms.france_npc_vector_constants import generate_unit as generate_npc_vectors
    npc_vectors = generate_npc_vectors(originals, registry_dir)
    document['functions'].extend(npc_vectors['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(npc_vectors['sequence_proofs'])
    document['npc_vectors_data_proofs'] = npc_vectors['data_proofs']
    for key, value in npc_vectors['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('NPC vectors adds ten unique complete original consumers. Thirty-three data operands follow complete typed vector objects, scalar members, an independently anchored player-model field and one original local float; thirty-three JALs close through complete identities or literal unnamed runtime contexts. Twelve fixed dependencies preserve evidence across later recoveries. No runtime identity, whole translation-unit or data extent is claimed.')
    from platforms.france_npc_animation_tables import generate_unit as generate_npc_animation
    npc_animation = generate_npc_animation(originals, registry_dir)
    document['functions'].extend(npc_animation['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(npc_animation['sequence_proofs'])
    document['npc_animation_data_proofs'] = npc_animation['data_proofs']
    for key, value in npc_animation['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('NPC animation adds seventeen complete original Robot builders in one unique contiguous 9884-byte cluster. All original member boundaries, ordering and zero alignment gaps are checked; 257 typed table/string/callback operands and 149 JALs are inventoried in each reference. Complete array-copy coverage includes call delay-slot stores. Small members are identified through the complete cluster and original DWARF order, not individual short seeds. No whole translation-unit or data extent is claimed.')
    from platforms.france_npc_common_animation import generate_unit as generate_npc_common_animation
    npc_common_animation = generate_npc_common_animation(originals, registry_dir)
    document['functions'].extend(npc_common_animation['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(npc_common_animation['sequence_proofs'])
    document['npc_common_animation_data_proofs'] = npc_common_animation['data_proofs']
    for key, value in npc_common_animation['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Common NPC animation adds two complete original builders in an independently unique 400-byte cluster with 392 code bytes. Original member ordering, bounds and zero alignment gaps are checked separately from the Robot cluster. Complete typed names, literals, callback identity and animation callees corroborate every operand. No whole translation-unit or data extent is claimed.')
    from platforms.france_npc_boss_animation import generate_unit as generate_npc_boss_animation
    npc_boss_animation = generate_npc_boss_animation(originals, registry_dir)
    document['functions'].extend(npc_boss_animation['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(npc_boss_animation['sequence_proofs'])
    document['npc_boss_animation_data_proofs'] = npc_boss_animation['data_proofs']
    for key, value in npc_boss_animation['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Boss NPC animation adds five individually unique complete original builders. All original owner/boundary identities, 279 typed table/string/callback/initializer operands and 143 JALs are checked in each reference, using five fixed complete dependencies. Shared initializer owner selection is constrained by each authenticated function source and preserves earlier Robot proof output. No whole translation-unit or data extent is claimed.')
    from platforms.france_npc_remaining_animation import generate_unit as generate_npc_remaining_animation
    npc_remaining_animation = generate_npc_remaining_animation(originals, registry_dir)
    document['functions'].extend(npc_remaining_animation['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(npc_remaining_animation['sequence_proofs'])
    document['npc_remaining_animation_data_proofs'] = npc_remaining_animation['data_proofs']
    for key, value in npc_remaining_animation['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Additional NPC animation adds five individually unique complete original builders. All original owners, bounds, 128 typed operands and 69 JALs are checked through five fixed complete dependencies. One scoped Sandy initializer verifies its complete bounded three-iteration copy prefix and final word; generic straight-line checks remain unchanged. No whole translation-unit or data extent is claimed.')
    from platforms.france_npc_townsfolk_animation import generate_unit as generate_npc_townsfolk_animation
    npc_townsfolk_animation = generate_npc_townsfolk_animation(originals, registry_dir)
    document['functions'].extend(npc_townsfolk_animation['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(npc_townsfolk_animation['sequence_proofs'])
    document['npc_townsfolk_animation_data_proofs'] = npc_townsfolk_animation['data_proofs']
    for key, value in npc_townsfolk_animation['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Townsfolk animation adds nine complete original functions in two independently unique Ambient/Villager clusters. All original member orders, bounds, zero gaps, 74 typed operands and 62 transfers are checked. Three exact null-argument tail wrappers reach complete local builders; two scoped copy prefixes preserve the generic checker. No whole translation-unit or data extent is claimed.')
    from platforms.france_imodel_kernels import generate_unit as generate_imodel_kernels
    imodel_kernels = generate_imodel_kernels(originals, registry_dir)
    document['functions'].extend(imodel_kernels['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(imodel_kernels['sequence_proofs'])
    document['imodel_kernels_data_proofs'] = imodel_kernels['data_proofs']
    for key, value in imodel_kernels['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Model leaves add four independently unique complete original functions. Typed material arrays and an independently anchored camera frustum prove all three address pairs; all 42 Cull vector instruction words match raw. Original ownership, bounds, zero alignment and strict control flow are checked in all three references. No SDK callee, whole translation-unit or data extent is claimed.')
    from platforms.france_npc_particle_kernels import generate_unit as generate_npc_particle_kernels
    npc_particle_kernels = generate_npc_particle_kernels(originals, registry_dir)
    document['functions'].extend(npc_particle_kernels['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(npc_particle_kernels['sequence_proofs'])
    document['npc_particle_kernels_data_proofs'] = npc_particle_kernels['data_proofs']
    document['npc_particle_kernels_call_proofs'] = npc_particle_kernels['call_proofs']
    for key, value in npc_particle_kernels['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Particle and hazard kernels add nine independently unique complete original functions. Complete fixed xurand and streak callees and a typed full zero vector prove every changed operand. Original ownership, bounds, zero alignment and strict control flow are checked in all three references. Duplicate builders remain unpromoted. No whole translation-unit or data extent is claimed.')
    from platforms.france_boss_goal_kernels import generate_unit as generate_boss_goal_kernels
    boss_goal_kernels = generate_boss_goal_kernels(originals, registry_dir)
    document['functions'].extend(boss_goal_kernels['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(boss_goal_kernels['sequence_proofs'])
    document['boss_goal_kernels_data_proofs'] = boss_goal_kernels['data_proofs']
    document['boss_goal_kernels_call_proofs'] = boss_goal_kernels['call_proofs']
    for key, value in boss_goal_kernels['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Boss goal kernels add 24 independently unique complete Sandy and Patrick bodies. Eight fixed dependencies prove 41 calls and seven typed player-model operands. Original owners, bounds, alignment and strict control flow are checked in all three references. Duplicate exits and bone-position consumers remain unpromoted. No whole translation-unit or data extent is claimed.')
    from platforms.france_sound_wrappers import generate_unit as generate_sound_wrappers
    sound_wrappers = generate_sound_wrappers(originals, registry_dir)
    document['functions'].extend(sound_wrappers['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(sound_wrappers['sequence_proofs'])
    document['sound_wrappers_data_proofs'] = sound_wrappers['data_proofs']
    for key, value in sound_wrappers['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Sound wrappers add three complete original entries in one independently unique 1520-byte cluster with an already-proved 1376-byte anchor. Tiny tail wrappers have exact complete argument, terminal jump and delay-slot inventories, preserve SP/RA, and mask only the fixed known callee. All three original owners, bounds and alignment gaps are checked. No whole translation-unit or data extent is claimed.')
    from platforms.france_sound_controls import generate_unit as generate_sound_controls
    sound_controls = generate_sound_controls(originals, registry_dir)
    document['functions'].extend(sound_controls['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(sound_controls['sequence_proofs'])
    document['sound_controls_data_proofs'] = sound_controls['data_proofs']
    document['sound_controls_call_proofs'] = sound_controls['call_proofs']
    for key, value in sound_controls['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Sound controls add eight independently unique complete original bodies. Typed sound-global fields and four fixed complete HIS callees corroborate every changed operand. Original declarations, member paths, ownership, bounds, zero padding and strict control flow are checked in all three references. No whole translation-unit, data extent or new runtime context is claimed.')
    from platforms.france_sound_stop_wrapper import generate_unit as generate_sound_stop_wrapper
    sound_stop_wrapper = generate_sound_stop_wrapper(originals, registry_dir)
    document['functions'].extend(sound_stop_wrapper['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(sound_stop_wrapper['sequence_proofs'])
    document['sound_stop_wrapper_data_proofs'] = sound_stop_wrapper['data_proofs']
    for key, value in sound_stop_wrapper['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Sound stop wrapper adds one complete eight-byte tail within an independently unique 176-byte cluster of three complete authenticated original members. Original ownership, member order, bounds, zero gaps and complete known anchors/callee are rechecked in all three references. Only the fixed iSndStop terminal jump target is masked; generic short-seed uniqueness rules are unchanged.')
    from platforms.france_sandy_clothesline import generate_unit as generate_sandy_clothesline
    sandy_clothesline = generate_sandy_clothesline(originals, registry_dir)
    document['functions'].extend(sandy_clothesline['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(sandy_clothesline['sequence_proofs'])
    document['sandy_clothesline_data_proofs'] = sandy_clothesline['data_proofs']
    document['sandy_clothesline_call_proofs'] = sandy_clothesline['call_proofs']
    for key, value in sandy_clothesline['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Sandy clothesline adds one independently unique complete original goal. Twenty-five fixed complete calls and 35 typed spring-object, bounded sound-array and complete-string operands are checked in all three references. Original ownership, bounds, alignment and strict control flow remain required. No whole translation-unit or data extent is claimed.')
    from platforms.france_sandy_damage_effect import generate_unit as generate_sandy_damage_effect
    sandy_damage_effect = generate_sandy_damage_effect(originals, registry_dir)
    document['functions'].extend(sandy_damage_effect['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(sandy_damage_effect['sequence_proofs'])
    document['sandy_damage_effect_data_proofs'] = sandy_damage_effect['data_proofs']
    for key, value in sandy_damage_effect['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Sandy damage effect adds one independently unique complete call-free original body. The full four-element BDE record array and model RGB/Next member paths are checked through original declarations, array bounds and types in all three references. Original ownership, bounds, alignment and strict control flow are preserved. No data extent or whole translation-unit is claimed.')
    from platforms.france_sandy_sit import generate_unit as generate_sandy_sit
    sandy_sit = generate_sandy_sit(originals, registry_dir)
    document['functions'].extend(sandy_sit['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(sandy_sit['sequence_proofs'])
    document['sandy_sit_data_proofs'] = sandy_sit['data_proofs']
    document['sandy_sit_call_proofs'] = sandy_sit['call_proofs']
    for key, value in sandy_sit['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Sandy sit adds one independently unique complete original goal. Fourteen fixed complete calls and typed player carry/model paths, complete bone/sound arrays and strings corroborate every changed operand in all three references. Original ownership, bounds, alignment and strict control flow remain required. No whole translation-unit or data extent is claimed.')
    from platforms.france_sandy_control_callbacks import generate_unit as generate_sandy_control_callbacks
    sandy_control_callbacks = generate_sandy_control_callbacks(originals, registry_dir)
    document['functions'].extend(sandy_control_callbacks['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(sandy_control_callbacks['sequence_proofs'])
    document['sandy_control_callbacks_data_proofs'] = sandy_control_callbacks['data_proofs']
    document['sandy_control_callbacks_call_proofs'] = sandy_control_callbacks['call_proofs']
    for key, value in sandy_control_callbacks['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Sandy control callbacks add four independently unique complete original bodies. Typed original ControlOff and model-pointer paths and fixed complete CalcNewDir/AnimTimeRemain dependencies corroborate every operand in all three references. Original ownership, bounds, zero alignment and strict control flow are rechecked. No whole translation-unit or data extent is claimed.')
    from platforms.france_sandy_leap import generate_unit as generate_sandy_leap
    sandy_leap = generate_sandy_leap(originals, registry_dir)
    document['functions'].extend(sandy_leap['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(sandy_leap['sequence_proofs'])
    document['sandy_leap_data_proofs'] = sandy_leap['data_proofs']
    document['sandy_leap_call_proofs'] = sandy_leap['call_proofs']
    for key, value in sandy_leap['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Sandy Leap adds one independently unique complete 484-byte original tail body. Two typed model-pointer loads and a fixed complete Common Enter callee corroborate every operand. The scoped verifier preserves SP/RA and checks local COP1 encodings, full body coverage and terminal J; generic boundary/unknown-opcode rules remain unchanged. All three original identities, extents and padding are checked.')
    from platforms.france_npc_particle_emitters import generate_unit as generate_npc_particle_emitters
    npc_particle_emitters = generate_npc_particle_emitters(originals, registry_dir)
    document['functions'].extend(npc_particle_emitters['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(npc_particle_emitters['sequence_proofs'])
    document['npc_particle_emitters_data_proofs'] = npc_particle_emitters['data_proofs']
    document['npc_particle_emitters_call_proofs'] = npc_particle_emitters['call_proofs']
    for key, value in npc_particle_emitters['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Particle emitters add twelve complete original members in two independently unique ordered clusters. Full typed management/parameter arrays, fixed complete ConfigPar callees and twelve literal calls to one unnamed 64-byte runtime context corroborate every operand. All original member orders, bounds, padding and strict CFGs are checked in three references. No short-seed exception, runtime identity, whole translation-unit or data extent is claimed.')
    from platforms.france_hazard_splashes import generate_unit as generate_hazard_splashes
    hazard_splashes = generate_hazard_splashes(originals, registry_dir)
    document['functions'].extend(hazard_splashes['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(hazard_splashes['sequence_proofs'])
    document['hazard_splashes_data_proofs'] = hazard_splashes['data_proofs']
    document['hazard_splashes_call_proofs'] = hazard_splashes['call_proofs']
    for key, value in hazard_splashes['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Hazard splashes add two independently unique complete original bodies and one complete perpendicular helper identified in a unique 264-byte anchored cluster. All calls reach fixed complete identities; no data operands change. The helper tail uses only its independently proved Normalize callee. Original ownership, bounds, zero gaps and strict CFGs are checked in all three references without weak individual seeds.')
    from platforms.france_hazard_oil_splash import generate_unit as generate_hazard_oil_splash
    hazard_oil_splash = generate_hazard_oil_splash(originals, registry_dir)
    document['functions'].extend(hazard_oil_splash['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(hazard_oil_splash['sequence_proofs'])
    document['hazard_oil_splash_data_proofs'] = hazard_oil_splash['data_proofs']
    document['hazard_oil_splash_call_proofs'] = hazard_oil_splash['call_proofs']
    for key, value in hazard_oil_splash['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('Hazard OilSplash adds one complete original body through stronger full-template uniqueness. Only six inventoried JAL target fields are reconstructed from independently checked complete callees; all other original bits remain literal and exhaustive search uses zero masks. No target body bytes are copied. Original ownership, extents, padding and closed CFG remain required in all three references; unbound ambiguity is retained as negative evidence.')
    from platforms.france_npc_smooth import generate_unit as generate_npc_smooth
    npc_smooth = generate_npc_smooth(originals, registry_dir)
    document['functions'].extend(npc_smooth['functions'])
    document['functions'].sort(key=lambda function: function['address'])
    document['sequence_proofs'].extend(npc_smooth['sequence_proofs'])
    document['npc_smooth_data_proofs'] = npc_smooth['data_proofs']
    for key, value in npc_smooth['counts'].items():
        document['counts'][key] = document['counts'].get(key, 0) + value
    document['limitations'].append('NPC smoothing adds one independently unique complete 508-byte original call-free leaf. Full typed sample and coefficient arrays and signed 32-bit init ownership, complete BSS ranges, original reginfo and unclobbered startup GP setup corroborate all operands in three references. The two GP words remain literal; only three inventoried address fields are reconstructed. Closed control flow, full uniqueness and zero padding remain required. No whole translation-unit, data extent or new data anchor is claimed.')
    return document


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, default=ROOT / 'config/platforms/versions.json')
    parser.add_argument('--orig-dir', type=Path, default=ROOT / 'orig')
    parser.add_argument('--registry-dir', type=Path, default=ROOT / 'config/platforms/SLES-53623')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    output = args.output or args.registry_dir / 'tu-corroborated-functions.json'
    result = generate(args.manifest, args.orig_dir, args.registry_dir)
    if args.check:
        require(json.loads(output.read_text(encoding='utf-8')) == result, 'TU corroboration differs')
    else:
        output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(result['counts'], sort_keys=True))


if __name__ == '__main__':
    main()
