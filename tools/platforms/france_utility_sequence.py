"""Original-only French utility TU and scoped seven-case dispatch/data contexts."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_tu_sequences import KIND, address_pair, compare, words, digest, unique_template, cstring, gpr_writes
from platforms.france_corroborated import ControlFlow
from platforms.france_update_cull_sequence import checked_identity
from platforms.ps2_source import canonical_linkages
from platforms.dwarf1 import iter_dies
SOURCE = 'SB/Core/x/xutil.cpp'
MISSING = [('xUtil_wtadjust', 280), ('BCDtoi', 52), ('strtosjis', 552),
           ('xUtil_crc_update', 484), ('xUtil_crc_init', 136),
           ('xUtil_idtag2string', 784), ('xUtilShutdown', 20), ('xUtilStartup', 48)]

def dispatches(original, address, size, name, *, specifications=None):
    """Only idtag's original seven-case JR may use distinct source/destination registers."""
    require(name == 'xUtil_idtag2string' and size == 784 and (specifications == ((156, 7),)),
        'Unreviewed utility dispatch scope')
    body = original.read(address, size)
    code = words(body)
    flow = ControlFlow({address + i * 4: w for (i, w) in enumerate(code)})
    proven = []
    for (offset, count) in specifications:
        (jr, load, add, low, shift) = (code[(offset - n) // 4] for n in (0, 4, 8, 12, 16))
        (index, base) = (jr >> 21 & 31, low >> 21 & 31)
        source_index = shift >> 16 & 31
        require(source_index == 5 and index == 4 and (base not in (0, 4, 5, 29, 31)),
            'Invalid bounded source index')
        require(index not in (0, 31) and base not in (0, index, 31) and (jr == index << 21 | 8),
            'Dispatch is not an ordinary index-register JR')
        require(code[offset // 4 + 1] == 0 and load == 35 << 26 | index << 21 | index << 16,
            'Dispatch must load its target then execute JR with NOP delay')
        require(add == index << 21 | base << 16 | index << 11 | 33 and low >> 26 == 9
            and (low >> 16 & 31 == base) and (shift == source_index << 16 | index << 11 | 2 << 6),
            'Dispatch table index/base dataflow changed')
        (hi, table) = address_pair(body, address, offset - 12)
        candidates = []
        for off in range(max(0, offset - 40), offset - 20, 4):
            (w, guard) = (code[off // 4], flow.instruction(address + off + 4))
            test = w >> 16 & 31
            if (w >> 26 == 11 and w >> 21 & 31 == source_index and (w & 65535 == count) and (test not in (0,
                source_index, index, base)) and (guard['op'] == 4) and (guard['rs'] == test)
                and (guard['rt'] == 0) and (address + offset + 8 <= guard['target'] < address + size)):
                candidates.append(off)
        require(len(candidates) == 1, 'Dispatch lacks one bounded unsigned index/default test')
        test_offset = candidates[0]
        for off in range(test_offset + 8, offset - 16, 4):
            require(flow.instruction(address + off)['kind'] == 'normal'
                and source_index not in gpr_writes(code[off // 4]),
                'Dispatch index changed between range test and scaling')
        require(test_offset + 8 <= hi < offset - 12, 'Table base does not originate inside checked dispatch')
        for off in range(size // 4):
            inst = flow.instruction(address + off * 4)
            destination = inst.get('target')
            require(destination is None
                or not address + test_offset + 4 <= destination <= address + offset + 4,
                'A direct edge bypasses dispatch index/base checks')
        targets = words(original.read(table, count * 4))
        require(all((address <= x < address + size and x % 4 == 0 for x in targets)),
            'Original dispatch table leaves its complete function')
        proven.append({'jump_offset': offset, 'index_count': count, 'test_offset': test_offset,
            'default_offset': flow.instruction(address + test_offset + 4)['target'] - address,
            'table_address': table, 'table_sha256': digest(original.read(table, count * 4)),
            'target_offsets': [x - address for x in targets]})
    for proof in proven:
        for other in proven:
            require(not any((proof['test_offset'] + 4 <= destination <= proof['jump_offset'] + 4 for destination in other['target_offsets'])), 'A table target bypasses another dispatch check')
    return proven

def arrays(o):
    """Read complete array dimensions and element types from original DWARF1."""
    sec = next((s for s in o.metadata['sections'] if s['name'] == '.debug'))
    rows = list(iter_dies(o.data[sec['offset']:sec['offset'] + sec['size']]))
    by = {off: (tag, owner, a) for (off, tag, owner, a) in rows}
    result = {}
    for (name, dims, ft, width) in [('g_crc32_table', (256,), 9, 4), ('ascii_table', (3, 2), 6, 2),
        ('ascii_k_table', (33,), 6, 2), ('buf', (6, 10), 1, 1)]:
        ds = [(off, a) for (off, tag, owner, a) in rows if tag in (7, 12) and owner.replace('\\',
            '/').endswith(SOURCE) and (a.get(3) == name)]
        require(len(ds) == 1, 'array declaration ambiguous')
        (off, a) = ds[0]
        loc = a[2]
        require(len(loc) == 5 and loc[0] == 3, 'array location missing')
        die = a[7]
        size = width
        chain = []
        for (i, count) in enumerate(dims):
            (tag, owner, attrs) = by[die]
            desc = attrs.get(10)
            prefix = bytes.fromhex('000a0000000000') + (count - 1).to_bytes(4, 'little') + b'\x08'
            require(tag == 1 and attrs[9] == 0 and desc.startswith(prefix), 'array dimensions differ')
            size *= count
            chain.append({'die': die, 'descriptor': desc.hex()})
            if i + 1 < len(dims):
                require(len(desc) == 18 and desc[12:14] == b'r\x00', 'nested array differs')
                die = int.from_bytes(desc[14:], 'little')
            else:
                require(desc[12:] == b'U\x00' + ft.to_bytes(2, 'little'), 'element type differs')
        result[int.from_bytes(loc[1:], 'little')] = {'name': name, 'dimensions': list(dims), 'element_type': ft,
            'size': size, 'declaration_die': off, 'type_chain': chain}
    return result

def byte_lookup(o, a, size, low):
    """Bound the observed table by a non-clobbered unsigned-byte index."""
    code = words(o.read(a, size))
    base = code[low // 4] >> 16 & 31
    adds = []
    for off in (low + 4, low + 8):
        w = code[off // 4]
        if w >> 26 == 0 and w & 63 == 33 and (w >> 21 & 31 == base):
            adds.append(off)
    require(len(adds) == 1, 'byte table add ambiguous')
    add = adds[0]
    w = code[add // 4]
    idx = w >> 16 & 31
    dest = w >> 11 & 31
    require(idx not in (0, 29, 31, base) and dest not in (0, 29, 31), 'invalid table registers')
    loads = [off for off in range(low - 8, add, 4) if code[off // 4] >> 26 == 36
        and code[off // 4] >> 16 & 31 == idx]
    require(len(loads) == 1, 'index is not single bounded LBU')
    load = loads[0]
    require(all((idx not in gpr_writes(code[off // 4]) for off in range(load + 4, add, 4))),
        'byte index clobbered')
    require(all((base not in gpr_writes(code[off // 4]) for off in range(low + 4, add, 4))),
        'byte table base clobbered')
    require(code[(add + 4) // 4] == 36 << 26 | dest << 21 | dest << 16, 'not bounded byte lookup')
    cf = ControlFlow({a + 4 * i: w for (i, w) in enumerate(code)})
    require(all((cf.instruction(a + off)['kind'] == 'normal' for off in range(load, add + 4, 4))),
        'byte lookup crosses control transfer')
    for off in range(0, size, 4):
        dst = cf.instruction(a + off).get('target')
        require(dst is None or not a + load < dst <= a + add, 'edge bypasses index definition')
    return {'index_load_offset': load, 'address_add_offset': add, 'maximum_index': 255,
        'observed_context_bytes': 256}

def impure_type(original):
    """Authenticate the existing animation pointer's original pointee type."""
    sec = next((s for s in original.metadata['sections'] if s['name'] == '.debug'))
    rows = list(iter_dies(original.data[sec['offset']:sec['offset'] + sec['size']]))
    by = {off: (tag, owner, a) for (off, tag, owner, a) in rows}
    candidates = [(off, a) for (off, tag, owner, a) in rows if tag == 7 and owner.replace('\\',
        '/').endswith('SB/Core/x/xAnim.cpp') and (a.get(3) == '_impure_ptr')]
    require(len(candidates) == 1, 'Original animation impure pointer declaration ambiguous')
    (off, attrs) = candidates[0]
    typ = attrs.get(8, b'')
    loc = attrs.get(2, b'')
    require(len(typ) == 5 and typ[0] == 1 and (len(loc) == 5) and (loc[0] == 3),
        'Original impure declaration is not an absolute ordinary pointer')
    die = int.from_bytes(typ[1:], 'little')
    (tag, owner, pointee) = by[die]
    require(tag == 2 and pointee.get(3) == '_reent' and (pointee.get(11) == 752),
        'Original runtime pointee type differs')
    return (int.from_bytes(loc[1:], 'little'), {'declaration_die': off, 'pointer_descriptor': typ.hex(),
        'pointee_die': die, 'pointee_name': '_reent', 'pointee_size': 752})

def within(binary, address, size, region_name):
    region = binary._stream_regions[region_name]
    require(region['address'] <= address and address + size <= region['address'] + region['size'],
        'Complete observed data range escapes ' + region_name)

def generate_unit(originals, registry_dir):
    known = {}
    # Existing animation records supply independent pointer witnesses, never utility
    # records generated by this module. Duplicate old anchors must agree exactly.
    for filename in ('corroborated-functions.json', 'relocation-corroborated-functions.json',
        'tu-corroborated-functions.json'):
        for f in json.loads((registry_dir / filename).read_text())['functions']:
            if f['source'] == SOURCE and f['address'] not in (2163360, 2163488, 2163584):
                continue
            require(f['address'] not in known or (known[f['address']]['name'],known[f['address']]['source'],
                known[f['address']]['size'],known[f['address']]['sha256']) == (f['name'],f['source'],
                f['size'],f['sha256']), 'Conflicting independent utility context')
            known[f['address']] = f
    functions = {}
    results = []
    t = originals[TARGET]
    for version in REFERENCES:
        o = originals[version]
        fs = sorted((f for f in o.functions if f['source'] == SOURCE), key=lambda f: f['low'])
        require(len(fs) == 11 and sum((f['high'] - f['low'] for f in fs)) == 2668, 'unit membership changed')
        neighbors = [f for f in known.values() if f['source'] == SOURCE]
        require(len(neighbors) == 3, 'expected three independent neighbors')
        shifts = set()
        for n in neighbors:
            ps = [p for p in n['provenance'] if p['version'] == version]
            require(len(ps) == 1, 'neighbor provenance missing')
            p = ps[0]
            require(p['executable_sha1'] == o.sha1 and digest(t.read(n['address'], n['size'])) == n['sha256']
                and (digest(o.read(p['source_address'], n['size'])) == p['reference_sha256']),
                'neighbor original hashes differ')
            checked_identity(o, t, p['source_address'], n['address'], known)
            shifts.add(n['address'] - p['source_address'])
        require(len(shifts) == 1, 'inconsistent placement')
        shift = shifts.pop()
        linkages = canonical_linkages(o.data, o.metadata)
        (lo, end) = (fs[0]['low'], fs[-1]['high'])
        start = lo + shift
        require(start == 2163072 and end - lo == 2736, 'Unexpected utility placement')
        require([(f['name'], f['high'] - f['low']) for f in fs if f['low'] + shift not in known] == MISSING,
                'Unexpected missing utility members')
        flow = ControlFlow({start + 4 * i: w for (i, w) in enumerate(words(t.read(start, end - lo + 16)))})
        rf = ControlFlow({lo + 4 * i: w for (i, w) in enumerate(words(o.read(lo, end - lo + 16)))})
        decl = arrays(o)
        masks = {}
        rows = []
        mapping = {}
        for (i, f) in enumerate(fs):
            a = f['low']
            b = a + shift
            size = f['high'] - a
            if i:
                gap = a - fs[i - 1]['high']
                require(0 <= gap < 16 and (not any(o.read(a - gap, gap))) and (not any(t.read(b - gap, gap))),
                    'alignment differs')
            (pairs, calls, mask) = compare(o, t, a, b, size)
            masks.update({a - lo + k: z for (k, z) in mask.items()})
            tables = []
            ref_tables = []
            if f['name'] == 'xUtil_idtag2string':
                tables = dispatches(t, b, size, f['name'], specifications=((156, 7),))
                ref_tables = dispatches(o, a, size, f['name'], specifications=((156, 7),))
                require(tables[0]['target_offsets'] == ref_tables[0]['target_offsets'],
                    'relative table targets differ')
            for (binary, base, cf, tbs) in [(o, a, rf, ref_tables), (t, b, flow, tables)]:
                require(cf.bounds(base, size,
                    resolved_indirect_jumps={base + q['jump_offset']: [base + x for x in q['target_offsets']] for q in tbs})['passes'], 'CFG not closed')
            for p in pairs:
                (ra, tb) = (p['reference_address'], p['target_address'])
                require(ra not in mapping or mapping[ra] == tb, 'data mapping inconsistent')
                mapping[ra] = tb
                if ra in decl:
                    proof = decl[ra]
                    n = proof['size']
                    if p['storage'] == 'zero_fill':
                        for (binary, addr) in [(o, ra), (t, tb)]:
                            region = binary._stream_regions['runtime_bss']
                            require(region['address'] <= addr
                                and addr + n <= region['address'] + region['size'], 'typed array outside BSS')
                    else:
                        within(o, ra, n, 'initialized_data')
                        within(t, tb, n, 'initialized_data')
                        require(o.read(ra, n) == t.read(tb, n), 'typed array initial payload differs')
                    p['typed_array'] = proof
                elif tables and ra == ref_tables[0]['table_address']:
                    require(tb == tables[0]['table_address'], 'dispatch pair mismatch')
                    p['scope'] = 'complete seven-entry bounded dispatch'
                elif f['name'] in ('BCDtoi', 'strtosjis'):
                    (left, right) = (cstring(o, ra), cstring(t, tb))
                    require(left == right, 'literal differs')
                    within(o, ra, len(left), 'initialized_data')
                    within(t, tb, len(right), 'initialized_data')
                    p['scope'] = 'unchanged complete string literal'
                    p['literal_sha256'] = digest(left)
                    p['literal_size_with_terminator'] = len(left)
                else:
                    require(f['name'] == 'xUtil_idtag2string' and p['storage'] == 'file_backed',
                        'unproved data pair')
                    p['byte_index'] = byte_lookup(o, a, size, p['lo_offset'])
                    require(byte_lookup(t, b, size, p['lo_offset']) == p['byte_index'],
                        'byte index dataflow differs')
                    within(o, ra, 256, 'initialized_data')
                    within(t, tb, 256, 'initialized_data')
                    require(o.read(ra, 256) == t.read(tb, 256), 'complete observed byte-table payload differs')
                    p['observed_table_sha256'] = digest(t.read(tb, 256))
                    p['scope'] = 'bounded byte-table context, no runtime identity or data extent promoted'
            for c in calls:
                (ra, tb) = (c['reference_address'], c['target_address'])
                if lo <= ra < end:
                    require(ra in {q['low'] for q in fs} and tb == ra + shift, 'internal call changes ownership')
                elif ra == tb and tb in (1160624, 1133520, 1154672, 1139152):
                    require(o.word(a + c['offset']) == t.word(b + c['offset']), 'caller literal transfer differs')
                    (cp, cc, cm) = compare(o, t, ra, tb, 64)
                    if cp:
                        require(not cc and len(cp) == 1 and (ra in (1160624, 1139152)),
                            'unreviewed runtime address changes')
                        pair = cp[0]
                        require((pair['hi_offset'], pair['lo_offset'], pair['opcode']) == ((0, 8,
                            35) if ra == 1160624 else (8, 32, 35)), 'runtime pointer load differs')
                        from platforms.france_tu_sequences import named_data
                        # These remain opaque entry contexts. Their callers retain
                        # the literal JAL words in the unique whole-TU comparison.
                        (impure, impure_proof) = impure_type(o)
                        require(impure == named_data(o, '_impure_ptr', 'SB/Core/x/xAnim.cpp'),
                            'Original pointer declaration identity differs')
                        require(pair['reference_address'] == impure,
                            'runtime operand not original named impure pointer')
                        witnesses = []
                        for rec in known.values():
                            if rec['source'] != 'SB/Core/x/xAnim.cpp':
                                continue
                            for pr in rec.get('provenance', []):
                                if pr['version'] != version:
                                    continue
                                if (any((q['reference_address'] == impure
                                    and q['target_address'] == pair['target_address'] for q in pr.get('data_address_operands', [])))):
                                    require(pr['executable_sha1'] == o.sha1
                                        and digest(o.read(pr['source_address'],
                                        rec['size'])) == pr['reference_sha256']
                                        and (digest(t.read(rec['address'], rec['size'])) == rec['sha256']),
                                        'independent animation pointer witness changed')
                                    (actual_pairs, _, _) = compare(o, t, pr['source_address'], rec['address'],
                                        rec['size'])
                                    require(any((q['reference_address'] == impure
                                        and q['target_address'] == pair['target_address'] for q in actual_pairs)), 'Actual animation body does not decode recorded pointer mapping')
                                    checked_identity(o, t, pr['source_address'], rec['address'], known)
                                    witnesses.append(rec['address'])
                        require(witnesses, 'runtime slot lacks independent original animation mapping')
                        require(o.word(impure) - impure == t.word(pair['target_address']) - pair['target_address'] == -752, 'existing runtime pointee relationship differs')
                        for (binary, slot) in [(o, impure), (t, pair['target_address'])]:
                            region = binary._stream_regions['initialized_data']
                            within(binary, slot, 4, 'initialized_data')
                            ptr = binary.word(slot)
                            require(region['address'] <= ptr
                                and ptr + 752 <= region['address'] + region['size'],
                                'existing runtime context outside initialized data')
                        c['independent_pointer_context'] = {'name': '_impure_ptr',
                            'source': 'SB/Core/x/xAnim.cpp', 'witnesses': witnesses,
                            'original_type': impure_proof, 'pair': pair, 'no_new_identity_or_extent': True}
                    else:
                        require(o.read(ra, 64) == t.read(tb, 64),
                            'opaque prefix differs without reviewed address change')
                    masks.pop(a - lo + c['offset'], None)
                    c['opaque_context_sha256'] = digest(t.read(tb, 64))
                    c['unmasked_no_identity_claim'] = True
                else:
                    original = o.by_address.get(ra)
                    record = known.get(tb)
                    require(original is not None and record is not None and ((original['name'],
                        original['source'], original['high'] - ra) == (record['name'], record['source'],
                        record['size'])), 'unknown named callee')
                    pr = next((p for p in record['provenance'] if p['version'] == version))
                    require(pr['source_address'] == ra and pr['executable_sha1'] == o.sha1
                        and (pr['reference_sha256'] == digest(o.read(ra, record['size'])))
                        and (record['sha256'] == digest(t.read(tb, record['size']))),
                        'callee original provenance differs')
            if b not in known:
                require(a in linkages, 'Original utility linkage missing')
                if b not in functions:
                    functions[b] = {'name': f['name'], 'source': SOURCE, 'address': b, 'size': size,
                        'sha256': digest(t.read(b, size)), 'boundary_confirmation': True,
                        'confirmation_kind': KIND, 'provenance': [],
                        'corroboration': {'local_control_flow': flow.bounds(b, size,
                        resolved_indirect_jumps={b + q['jump_offset']: [b + x for x in q['target_offsets']] for q in tables}), 'previously_confirmed_neighbor_entries': sorted((n['address'] for n in neighbors))}}
                rec = functions[b]
                require((rec['name'], rec['size']) == (f['name'], size),
                        'Original utility identities or extents disagree')
                require(not rec['provenance'] or rec['provenance'][0]['linkage_name'] == linkages[a],
                    'Original utility linkage differs')
                rec['provenance'].append({'version': version, 'executable_sha1': o.sha1, 'source_address': a,
                    'name': f['name'], 'source': SOURCE, 'linkage_name': linkages[a],
                    'reference_sha256': digest(o.read(a, size)), 'data_address_operands': pairs,
                    'direct_transfers': calls})
            rows.append({'name': f['name'], 'address': b, 'size': size, 'already_known': b in known,
                'pairs': pairs, 'calls': calls, 'tables': tables})
        require(len(set(mapping.values())) == len(mapping), 'distinct data targets collapse')
        unique = unique_template(t, o.read(lo, end - lo), masks, start)
        results.append({'version': version, 'source': SOURCE, 'source_start': lo, 'target_start': start,
            'complete_original_members': 11, 'previously_confirmed_members': 3, 'start': start,
            'span': end - lo, 'functions': rows, 'uniqueness': unique})
    require(len(functions) == 8 and sum((f['size'] for f in functions.values())) == 2356
        and all((len(f['provenance']) == 3 for f in functions.values())),
        'All eight utility members require three original proofs')
    return {'functions': sorted(functions.values(), key=lambda f: f['address']), 'sequence_proofs': results,
        'call_neighbors': [], 'counts': {'functions': 8, 'code_bytes': 2356, 'source_units': 1,
        'closed_return_bodies': 8, 'reviewed_utility_dispatches': 1}}
