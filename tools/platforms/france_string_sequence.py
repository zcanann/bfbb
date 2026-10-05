"""Original-only French string completion with bounded dispatch and call context."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow
from platforms.france_tu_sequences import KIND, compare, digest, unique_template, words
from platforms.france_motion_sequence import dispatches
from platforms.ps2_source import canonical_linkages

SOURCE = 'SB/Core/x/xString.cpp'
START = 2155296
MEMBERS = [('find_char', 1648), ('atox', 200), ('icompare', 192), ('imemcmp', 104),
           ('xStrParseFloatList', 524), ('xStrupr', 88), ('xStricmp', 256),
           ('xStrTokBuffer', 336), ('xStrTok', 320), ('xStrHashCat', 88),
           ('xStrHash', 104), ('xStrHash', 88)]


def math_tail_context(original, target, reference, destination, known):
    members = sorted((f for f in original.functions if f['source'] == 'SB/Core/x/xMath.cpp'), key=lambda f: f['low'])
    first = next(i for i, f in enumerate(members) if f['name'] == 'xurand')
    members = members[first:]
    require([(f['name'], f['high'] - f['low']) for f in members] ==
            [('xurand', 88), ('xrand', 32), ('xsrand', 8), ('xatof', 8), ('xMathExit', 24), ('xMathInit', 32)],
            'Original math-tail context membership changed')
    require(members[3]['low'] == reference, 'String call does not reference original xatof')
    shift = destination - reference
    low, end = members[0]['low'], members[-1]['high']
    start = low + shift
    require(end - low == 224, 'Math-tail context span changed')
    flow = ControlFlow({start + 4 * i: w for i, w in enumerate(words(target.read(start, end - low + 16)))})
    ref_flow = ControlFlow({low + 4 * i: w for i, w in enumerate(words(original.read(low, end - low + 16)))})
    masks, rows = {}, []
    entries = {f['low'] for f in members}
    for i, f in enumerate(members):
        a, size = f['low'], f['high'] - f['low']
        b = a + shift
        require(a % 16 == b % 16 == 0, 'Math-tail context entry alignment changed')
        if i:
            gap = a - members[i - 1]['high']
            require(0 <= gap < 16 and not any(original.read(a - gap, gap)) and not any(target.read(b - gap, gap)),
                    'Math-tail context alignment changed')
        pairs, calls, mask = compare(original, target, a, b, size)
        require(not pairs, 'Math-tail context has unreviewed data-address changes')
        masks.update({a - low + off: value for off, value in mask.items()})
        if f['name'] in ('xurand', 'xrand', 'xMathInit'):
            neighbor = known.get(b)
            require(neighbor is not None and neighbor['boundary_confirmation'] and
                    (neighbor['name'], neighbor['source'], neighbor['size']) == (f['name'], f['source'], size) and
                    digest(target.read(b, size)) == neighbor['sha256'], 'Independent math-tail neighbor is absent')
            refs = [p for p in neighbor['provenance'] if p['version'] == original.version]
            require(len(refs) == 1 and refs[0]['source_address'] == a and refs[0]['executable_sha1'] == original.sha1 and
                    refs[0]['reference_sha256'] == digest(original.read(a, size)), 'Math-tail neighbor original provenance differs')
        row = {'name': f['name'], 'reference_address': a, 'address': b, 'size': size,
               'sha256': digest(target.read(b, size)), 'context_only': True}
        if f['name'] == 'xatof':
            code = words(original.read(a, size))
            require(size == 8 and code[0] >> 26 == 2 and code[1] == 0 and
                    original.read(a, size) == target.read(b, size), 'xatof context is not the same literal J/NOP tail')
            runtime = ((a + 4) & 0xf0000000) | ((code[0] & 0x3ffffff) << 2)
            require(original.read(runtime, 64) == target.read(runtime, 64), 'Opaque math-tail destination prefix differs')
            masks.pop(a - low, None)
            row.update(opaque_destination=runtime, opaque_context_bytes=64,
                       opaque_context_sha256=digest(target.read(runtime, 64)), transfer_word_unmasked=True)
        else:
            require(flow.bounds(b, size)['passes'] and ref_flow.bounds(a, size)['passes'], 'Math-tail strict CFG proof fails')
        for call in calls:
            if f['name'] == 'xatof':
                continue
            ra, tb = call['reference_address'], call['target_address']
            require(ra in entries and tb == ra + shift, 'Math-tail context has an unconfirmed external callee')
        rows.append(row)
    return {'members': rows, 'uniqueness': unique_template(target, original.read(low, end - low), masks, start),
            'no_progress_or_new_anchor': True}


def generate_unit(originals, registry_dir):
    target = originals[TARGET]
    known = {}
    for filename in ('corroborated-functions.json', 'relocation-corroborated-functions.json'):
        doc = json.loads((registry_dir / filename).read_text())
        require(doc['executable_sha1'] == target.sha1, 'String neighbor registry targets another executable')
        for f in doc['functions']:
            require(f['address'] not in known, 'Duplicate independent string neighbor')
            known[f['address']] = f
    neighbors = sorted((f for f in known.values() if f['source'] == SOURCE), key=lambda f: f['address'])
    require(len(neighbors) == 8 and sum(f['size'] for f in neighbors) == 1472, 'Expected eight independent string neighbors')
    expected = {(f['address'], f['name'], f['size']) for f in neighbors}
    for f in neighbors:
        require(f['boundary_confirmation'] and digest(target.read(f['address'], f['size'])) == f['sha256'],
                'Established string neighbor bytes changed')
    functions, sequences, contexts, table_proofs = {}, [], [], []
    for version in REFERENCES:
        original = originals[version]
        members = sorted((f for f in original.functions if f['source'] == SOURCE), key=lambda f: f['low'])
        require([(f['name'], f['high'] - f['low']) for f in members] == MEMBERS, 'Original string full membership changed')
        low, end = members[0]['low'], members[-1]['high']
        require(end - low == 3992, 'Original complete string placement changed')
        missing = [f for f in members if START + f['low'] - low not in known]
        require(len(missing) == 4 and sum(f['high'] - f['low'] for f in missing) == 2476, 'Unexpected missing string members')
        require({(START + f['low'] - low, f['name'], f['high'] - f['low']) for f in members if f not in missing} == expected,
                'All eight string neighbor placements must agree')
        flow = ControlFlow({START + 4 * i: w for i, w in enumerate(words(target.read(START, end - low + 16)))})
        ref_flow = ControlFlow({low + 4 * i: w for i, w in enumerate(words(original.read(low, end - low + 16)))})
        linkages = canonical_linkages(original.data, original.metadata)
        entries = {f['low'] for f in members}
        masks = {}
        external_calls = 0
        for i, f in enumerate(members):
            a, size, name = f['low'], f['high'] - f['low'], f['name']
            b = START + a - low
            require(a % 16 == b % 16 == 0 and a in linkages, 'String alignment or original linkage missing')
            if i:
                gap = a - members[i - 1]['high']
                require(0 <= gap < 16 and not any(original.read(a - gap, gap)) and not any(target.read(b - gap, gap)),
                        'String alignment contains nonzero or excessive bytes')
            pairs, calls, mask = compare(original, target, a, b, size)
            masks.update({a - low + off: value for off, value in mask.items()})
            tables, ref_tables = [], []
            if name == 'find_char':
                tables = dispatches(target, b, size, name, specifications=((64, 12),))
                ref_tables = dispatches(original, a, size, name, specifications=((64, 12),))
                require(len(tables) == len(ref_tables) == len(pairs) == 1, 'find_char requires one reviewed dispatch table')
                shape = lambda t: {k: v for k, v in t.items() if k not in ('table_address', 'table_sha256')}
                require(shape(tables[0]) == shape(ref_tables[0]), 'Original string dispatch dataflow/relative targets differ')
                require(pairs[0]['reference_address'] == ref_tables[0]['table_address'] and
                        pairs[0]['target_address'] == tables[0]['table_address'], 'Changed string pointer is not the complete dispatch table')
                table_proofs.append({'version': version, 'reference': ref_tables[0], 'target': tables[0]})
            else:
                require(not pairs, 'String has an unreviewed data-address change')
            bounds = flow.bounds(b, size, resolved_indirect_jumps={b + t['jump_offset']: [b + off for off in t['target_offsets']] for t in tables})
            rb = ref_flow.bounds(a, size, resolved_indirect_jumps={a + t['jump_offset']: [a + off for off in t['target_offsets']] for t in ref_tables})
            require(bounds['passes'] and rb['passes'], 'String strict CFG/frame proof fails')
            for call in calls:
                ra, tb = call['reference_address'], call['target_address']
                if low <= ra < end:
                    require(ra in entries and tb == START + ra - low, 'String internal call changes member ownership')
                else:
                    require(name == 'xStrParseFloatList' and call['opcode'] == 3, 'Unexpected string external transfer')
                    context = math_tail_context(original, target, ra, tb, known)
                    contexts.append({'version': version, 'function_address': b, **call, 'math_tail_context': context,
                                     'promoted_as_named_anchor': False})
                    external_calls += 1
            if f not in missing:
                continue
            if b not in functions:
                functions[b] = {'name': name, 'source': SOURCE, 'address': b, 'size': size,
                                'sha256': digest(target.read(b, size)), 'boundary_confirmation': True,
                                'confirmation_kind': KIND, 'provenance': [],
                                'corroboration': {'local_control_flow': bounds, 'dispatch_tables': tables,
                                                  'previously_confirmed_neighbor_entries': [n['address'] for n in neighbors]}}
            record = functions[b]
            require(record['size'] == size and (not record['provenance'] or record['provenance'][0]['linkage_name'] == linkages[a]),
                    'String original identities disagree')
            record['provenance'].append({'version': version, 'executable_sha1': original.sha1, 'source_address': a,
                                         'name': name, 'source': SOURCE, 'linkage_name': linkages[a],
                                         'reference_sha256': digest(original.read(a, size)),
                                         'data_address_operands': pairs, 'direct_transfers': calls, 'dispatch_tables': ref_tables})
        require(external_calls == 1, 'Expected exactly one string external call')
        sequences.append({'version': version, 'source': SOURCE, 'source_start': low, 'target_start': START,
                          'complete_original_members': 12, 'previously_confirmed_members': 8,
                          'sequence_bytes_with_alignment': end - low,
                          'uniqueness': unique_template(target, original.read(low, end - low), masks, START)})
    require(len(functions) == 4 and all(len(f['provenance']) == 3 for f in functions.values()), 'Four string functions need all three originals')
    return {'functions': sorted(functions.values(), key=lambda f: f['address']), 'sequence_proofs': sequences,
            'call_neighbors': contexts, 'data_proofs': table_proofs,
            'counts': {'functions': 4, 'code_bytes': 2476, 'source_units': 1, 'closed_return_bodies': 4,
                       'reviewed_string_dispatch_tables': 1}}
