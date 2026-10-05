"""Scoped original-only French Common/Standard NPC goal sequence proof.

Every target byte and original name comes from authenticated retail originals.
Vtable descriptors describe data arrays, not inferred NPC class layouts.
"""
from __future__ import annotations

from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow, rooted_graph
from platforms.france_tu_sequences import KIND, compare, digest, gpr_writes, unique_template, words
from platforms.ps2_source import canonical_linkages

COMMON = 'SB/Game/zNPCGoalCommon.cpp'
STANDARD = 'SB/Game/zNPCGoalStd.cpp'
UNITS = {COMMON: (0x2cb190, 5, 940), STANDARD: (0x2c62c0, 54, 17944)}


def goal_gpr_writes(word):
    # Decoded COP1 fmt=S arithmetic operates only on FPRs/FP accumulator.
    # In addition to the previously observed math operations, these windows
    # contain SUB.S (1) and MOV.S (6); MFC1/MTC1 have different fmt values.
    if word >> 26 == 17 and word >> 21 & 31 == 16 and word & 63 in (1, 2, 6, 7, 24, 25, 26, 28, 29):
        return set()
    return gpr_writes(word)


def goal_pair(body, address, offset, *, allow_return_store=False):
    code = words(body)
    low = code[offset // 4]
    register = low >> 21 & 31
    require(low >> 26 in (9, 35, 43, 49, 57) and register not in (0, 31),
            'Goal address consumer lacks an ordinary base register')
    high_offset = None
    for off in range(offset - 4, -1, -4):
        word = code[off // 4]
        if register in goal_gpr_writes(word):
            require(word >> 26 == 15 and word >> 21 & 31 == 0, 'Goal address register is not produced by LUI')
            high_offset = off
            break
    require(high_offset is not None, 'Goal address lacks a reaching LUI')
    flow = ControlFlow({address + i * 4: w for i, w in enumerate(code)})
    if high_offset >= 4:
        previous = flow.instruction(address + high_offset - 4)
        require(previous['kind'] == 'normal' or previous['kind'] == 'branch' and not previous.get('likely'),
                'Goal LUI has an unproved delay-slot lifetime')
    for off in range(high_offset + 4, offset, 4):
        inst = flow.instruction(address + off)
        require(inst['kind'] in ('normal', 'branch') or inst['kind'] == 'call' and off == offset - 4,
                'Transfer interrupts goal address lifetime')
    for off in range(0, len(body), 4):
        inst = flow.instruction(address + off)
        destination = inst.get('target')
        if destination is not None and address + high_offset < destination <= address + offset:
            require(off == high_offset - 4 and inst['kind'] == 'branch', 'Edge bypasses goal address producer')
    immediate = (low & 65535) - (65536 if low & 32768 else 0)
    return high_offset, (((code[high_offset // 4] & 65535) << 16) + immediate) & 0xffffffff


def goal_bounds(flow, address, size, name, source, mapped_members):
    result = flow.bounds(address, size)
    if result['passes']:
        return result
    tails = {('Enter', 8): ('Enter', 216), ('Resume', 16): ('Resume', 224), ('Enter', 32): ('Enter', 216)}
    if source == STANDARD and (name, size) in tails:
        terminal = flow.instruction(address + size - 8)
        callee = mapped_members.get(terminal.get('target'))
        require(callee is not None and callee['source'] == COMMON and
                (callee['name'], callee['size']) == tails[(name, size)], 'Goal tail is not a complete Common member')
        require(result['reasons'] == ['extent_not_terminal_return_delay', 'local_edge_outside_extent', 'no_return'] and
                not any(result[k] for k in ('returns', 'direct_or_indirect_calls', 'stack_adjustments',
                                           'ra_saves', 'ra_loads', 'unreachable_zero_words')),
                'Goal tail is not a completely covered frame-free leaf')
        require(terminal['kind'] == 'jump' and flow.valid_delay(address + size - 8) and
                flow.bounds(terminal['target'], callee['size'])['passes'], 'Common tail target does not independently close')
        for pc in range(address, address + size, 4):
            require(31 not in goal_gpr_writes(flow.words[pc]), 'Goal tail clobbers its inherited return address')
            instruction = flow.instruction(pc)
            if instruction['kind'] == 'jump' and not address <= instruction['target'] < address + size:
                require(pc == address + size - 8, 'Another jump leaves goal tail')
        return {**result, 'passes': True, 'reasons': [], 'boundary_kind': 'reviewed-goal-tail-to-closed-common-body',
                'tail_target_address': terminal['target'], 'tail_target_name': callee['name'],
                'tail_target_size': callee['size'], 'terminal_instruction_offset': size - 8}
    require(source == STANDARD and name == 'MoveAutoSmooth' and size == 1956 and
            result['reasons'] == ['unreachable_nonpadding_words'], 'Unreviewed goal boundary exception')
    unreachable = [pc - address for pc in result['unreachable_zero_words']]
    require(unreachable == [0x6c, 0x104, 0x108, 0x10c, 0x384, 0x55c, 0x6ac], 'Goal dead-island inventory changed')
    require([off for off in unreachable if flow.words[address + off]] == [0x104, 0x108] and
            flow.words[address + 0x104] == 0xc4600004 and flow.words[address + 0x108] == 0xe4000000,
            'Goal dead island is not the observed original scalar load/store')
    # BEQ zero,zero -> +0x110 executes its +0x100 store delay slot and skips
    # +0x104..+0x10c. No decoded local edge enters the preserved dead island.
    branch = flow.instruction(address + 0xfc)
    require(branch['kind'] == 'branch' and branch.get('taken') is True and not branch.get('likely') and
            branch['target'] == address + 0x110 and flow.valid_delay(address + 0xfc),
            'Goal dead island is not skipped by the original unconditional branch')
    for pc in range(address, address + size, 4):
        inst = flow.instruction(pc)
        require(not address + 0x104 <= inst.get('target', 0) < address + 0x110,
                'Direct entry bypasses goal dead-island exclusion')
    return {**result, 'passes': True, 'reasons': [], 'boundary_kind': 'reviewed-original-unreachable-scalar-island',
            'preserved_dead_word_offsets': [0x104, 0x108], 'skipping_branch_offset': 0xfc,
            'rejoin_offset': 0x110}



def no_external_direct_entry(original, flow, begin, end):
    # Inspect the actual authenticated CPU section, including callers outside
    # these units. Only real decoded direct transfers contribute entry targets.
    if not hasattr(original, '_goal_direct_targets'):
        region = original._stream_regions['cpu_text']
        targets = set()
        for pc in range(region['address'], region['address'] + region['size'], 4):
            word = flow.words[pc]
            if word >> 26 in (1, 2, 3, 4, 5, 6, 7, 17, 20, 21, 22, 23):
                destination = flow.instruction(pc).get('target')
                if destination is not None:
                    targets.add(destination)
        original._goal_direct_targets = targets
    require(not any(pc in original._goal_direct_targets for pc in range(begin, end, 4)),
            'Original CPU direct transfer enters goal dead island')


def original_data(original):
    from platforms.dwarf1 import iter_dies
    from platforms.ps2_type_layouts import aggregate_layouts
    section = next(s for s in original.metadata['sections'] if s['name'] == '.debug' and s['size'])
    debug = original.data[section['offset']:section['offset'] + section['size']]
    rows = list(iter_dies(debug)); by = {off: (tag, owner, attrs) for off, tag, owner, attrs in rows}
    layouts = aggregate_layouts(debug, STANDARD, {'zGlobals', 'zPlayerGlobals', 'zEnt', 'xEnt', 'xVec3'})
    require({n: r['size'] for n, r in layouts.items()} ==
            {'zGlobals': 8272, 'zPlayerGlobals': 6464, 'zEnt': 212, 'xEnt': 208, 'xVec3': 12}, 'Goal aggregate sizes changed')
    def member(owner, name, offset, child=None, pointer=False):
        matches = [m for m in layouts[owner]['members'] if m['name'] == name and m['offset'] == offset]
        require(len(matches) == 1, 'Goal aggregate member missing')
        m = matches[0]
        if child:
            ref = m['type_attributes'].get('7')
            require(ref in by and by[ref][2].get(3) == child and by[ref][2].get(11) == layouts[child]['size'],
                    'Goal aggregate path type changed')
        if pointer:
            typ = m['type_attributes'].get('8')
            require(isinstance(typ, str) and len(bytes.fromhex(typ)) == 5 and typ.startswith('01'),
                    'Goal aggregate field is not an original pointer')
        return {'owner': owner, 'member': name, 'offset': offset, 'type_attributes': m['type_attributes']}
    ent_die = layouts['zEnt']['die_offset']
    bases = [a for off, tag, owner, a in rows if ent_die < off < by[ent_die][2][1] and tag == 0x1c]
    require(len(bases) == 1 and bases[0].get(7) == layouts['xEnt']['die_offset'] and
            bases[0].get(2) == bytes.fromhex('040000000007'), 'Goal player zEnt does not inherit xEnt at zero')
    ent_base = {'owner': 'zEnt', 'base': 'xEnt', 'offset': 0, 'type_die': bases[0][7]}
    paths = [[member('zGlobals', 'player', 0x700, 'zPlayerGlobals'),
              member('zPlayerGlobals', 'ent', 0, 'zEnt'), ent_base, member('xEnt', 'model', 0x24, pointer=True)],
             [member('zGlobals', 'sceneCur', 0x2048, pointer=True)]]
    require([(m['name'], m['offset'], m['type_attributes'].get('5')) for m in layouts['xVec3']['members']] ==
            [('x', 0, 14), ('y', 4, 14), ('z', 8, 14)], 'Goal vector components changed')
    declarations = {}
    for off, tag, owner, attrs in rows:
        loc = attrs.get(2); name = attrs.get(3)
        if tag in (7, 12) and owner.replace('\\', '/').endswith(STANDARD) and isinstance(loc, bytes) and len(loc) == 5 and loc[0] == 3:
            address = int.from_bytes(loc[1:], 'little')
            if address:
                require(name not in declarations, 'Goal original data declaration ambiguous')
                declarations[name] = (off, attrs, address)
    records = {}
    for name, type_name in [('g_O3', 'xVec3'), ('g_Z3', 'xVec3'), ('globals', 'zGlobals')]:
        off, attrs, address = declarations[name]
        require(attrs.get(7) == layouts[type_name]['die_offset'], 'Goal named global type differs')
        records[name] = {'reference_address': address, 'type': type_name, 'size': layouts[type_name]['size'], 'declaration_die': off}
    off, attrs, address = declarations['ds2_min']
    require(attrs.get(5) == 14 and 7 not in attrs, 'Goal threshold is not original single precision')
    records['ds2_min'] = {'reference_address': address, 'fundamental_type': 14, 'size': 4, 'declaration_die': off}
    tables = {}
    for name, (off, attrs, address) in declarations.items():
        if not name.startswith('__vt__'):
            continue
        die = attrs.get(7); tag, owner, typ = by[die]
        size = 44 if name == '__vt__5xGoal' else 52
        require(tag == 0x13 and typ.get(11) == size and typ.get(3, '').startswith('@anon'),
                'Goal vtable does not have its complete original anonymous descriptor')
        # No NPC class size/layout is inferred from this data-only descriptor.
        tables[name] = {'reference_address': address, 'size': size, 'declaration_die': off,
                        'type_die': die, 'type_tag': tag, 'type_name': typ[3]}
    require(len(tables) == 15, 'Goal original vtable inventory changed')
    return records, tables, paths, {n: {'size': r['size'], 'die_offset': r['die_offset']} for n, r in layouts.items()}


def storage(original, address, size, region_name):
    region = original._stream_regions[region_name]
    require(region['address'] <= address and address + size <= region['address'] + region['size'],
            'Goal data exceeds authenticated original storage')


def generate_unit(originals):
    target = originals[TARGET]
    flow = ControlFlow({s['address'] + i * 4: w for s in target.loaded
                        for i, w in enumerate(words(target.read(s['address'], s['file_size'])))})
    _, rooted_calls, _ = rooted_graph(flow, target.metadata['entry_point'])
    functions, sequences, data_proofs, neighbors = [], [], [], []
    reference_shapes = None; identities = {}; cross_data = None
    for version in REFERENCES:
        original = originals[version]
        linkages = canonical_linkages(original.data, original.metadata)
        ref_flow = ControlFlow({s['address'] + i * 4: w for s in original.loaded
                               for i, w in enumerate(words(original.read(s['address'], s['file_size'])))})
        groups = {source: sorted((f for f in original.functions if f['source'] == source), key=lambda f: f['low']) for source in UNITS}
        member_map = {}; target_map = {}
        for source, members in groups.items():
            start, count, total = UNITS[source]
            require(len(members) == count and sum(f['high'] - f['low'] for f in members) == total, 'Goal original TU membership changed')
            for f in members:
                a = f['low']; b = start + a - members[0]['low']; size = f['high'] - a
                require(a in linkages and a % 16 == b % 16 == 0, 'Goal original entry linkage/alignment missing')
                member_map[a] = {**f, 'size': size, 'target_address': b}
                target_map[b] = {**f, 'size': size}
        pair_map, roles, callees = {}, {}, {}; version_functions = []; contexts = []
        for source, members in groups.items():
            start, count, total = UNITS[source]; low, end = members[0]['low'], members[-1]['high']; masks = {}; headers = []
            for index, f in enumerate(members):
                a = f['low']; b = member_map[a]['target_address']; size = f['high'] - a; name = f['name']
                if index:
                    previous = members[index - 1]; gap = a - previous['high']
                    if gap < 16:
                        require(gap >= 0 and not any(original.read(a-gap, gap)) and not any(target.read(b-gap, gap)),
                                'Nonzero bytes interrupt goal sequence')
                    else:
                        expected = ('PreCalc', 'Resume', 24, 'SB/Core/x/xBehaveMgr.h', 'Notice') if source == COMMON else (
                            'UseDefaultAnims', 'LoopCountSet', 20, 'SB/Game/zNPCTypeCommon.h', 'AnimPick')
                        require((previous['name'], name, gap) == expected[:3], 'Unexpected goal sequence interleave')
                        header = original.by_address.get(a - 16)
                        require(header and (header['source'], header['name'], header['high']-(a-16)) == (*expected[3:], 8) and
                                a-16 in linkages and not any(original.read(a-gap, gap-16)) and not any(target.read(b-gap, gap-16)) and
                                not any(original.read(a-8, 8)) and not any(target.read(b-8, 8)) and
                                original.read(a-16, 8) == target.read(b-16, 8) and ref_flow.bounds(a-16, 8)['passes'] and flow.bounds(b-16, 8)['passes'],
                                'Goal header interleave lacks exact original identity/bounds')
                        context = {'reference_address': a-16, 'target_address': b-16, 'source': header['source'],
                                   'name': header['name'], 'linkage_name': linkages[a-16], 'size': 8,
                                   'sha256': digest(original.read(a-16, 8)), 'promoted_as_named_anchor': False,
                                   'included_in_source_unit': False}
                        headers.append(context); contexts.append(context)
                pairs, calls, mask = compare(original, target, a, b, size, address_resolver=goal_pair)
                masks.update({a-low+off: value for off, value in mask.items()})
                for p in pairs:
                    c, d = p['reference_address'], p['target_address']
                    require(c not in pair_map or pair_map[c]['target_address'] == d, 'Goal data correspondence inconsistent')
                    pair_map[c] = p; roles.setdefault(c, set()).add(p['opcode'])
                for c in calls:
                    x, y = c['reference_address'], c['target_address']
                    require(x not in callees or callees[x] == y, 'Goal callee correspondence inconsistent'); callees[x] = y
                rb = goal_bounds(ref_flow, a, size, name, source, member_map)
                tb = goal_bounds(flow, b, size, name, source, target_map)
                require(rb['passes'] and tb['passes'], 'Goal original body bounds do not close')
                if name == 'MoveAutoSmooth':
                    no_external_direct_entry(original, ref_flow, a+0x104, a+0x110)
                    no_external_direct_entry(target, flow, b+0x104, b+0x110)
                    require(not any(a+0x104 <= x['low'] < a+0x110 for x in original.functions), 'Named entry enters goal dead island')
                    require(original.read(a+0x104, 12) == target.read(b+0x104, 12), 'Goal preserved dead bytes changed')
                proof = {'version': version, 'executable_sha1': original.sha1, 'source_address': a,
                         'name': name, 'source': source, 'linkage_name': linkages[a],
                         'reference_sha256': digest(original.read(a,size)), 'data_address_operands': pairs, 'direct_transfers': calls}
                if not ref_flow.bounds(a,size)['passes']:
                    proof['original_boundary_exception'] = rb
                shape = (source, name, b, size, linkages[a]); version_functions.append(shape)
                if version == REFERENCES[0]:
                    identities[(source, b)] = len(functions)
                    functions.append({'name': name, 'source': source, 'address': b, 'size': size,
                                      'sha256': digest(target.read(b,size)), 'boundary_confirmation': True,
                                      'confirmation_kind': KIND, 'provenance': [proof],
                                      'corroboration': {'local_control_flow': tb, 'direct_rooted_call_sites': sorted(rooted_calls[b])}})
                else:
                    functions[identities[(source,b)]]['provenance'].append(proof)
            require(len(headers) == 1, 'Goal header interleave inventory changed')
            sequences.append({'version': version, 'source': source, 'source_start': low, 'target_start': start,
                              'sequence_bytes_with_alignment': end-low,
                              'uniqueness': unique_template(target, original.read(low,end-low), masks, start),
                              'immutable_interleaved_context': headers[0]})
        require(reference_shapes is None or reference_shapes == version_functions, 'Goal ordered identities disagree across originals')
        reference_shapes = version_functions
        global_records, tables, paths, layouts = original_data(original); accounted = set(); globals_proof = []
        for name, record in global_records.items():
            a = record['reference_address']; size = record['size']
            offsets = (0,4,8) if name == 'g_O3' else (0x724,0x2048) if name == 'globals' else (0,)
            bases = {pair_map[a+off]['target_address']-off for off in offsets}
            require(len(bases) == 1, 'Goal aggregate components do not share a target base'); b = bases.pop()
            expected = {49} if name == 'g_O3' else {35} if name == 'globals' else {9} if name == 'g_Z3' else {43,49}
            require(all(roles[a+off] == expected for off in offsets), 'Goal global operand role changed')
            region = 'runtime_bss' if name == 'globals' else 'initialized_data'
            storage(original,a,size,region); storage(target,b,size,region)
            item = {**record, 'target_address':b, 'component_offsets':list(offsets), 'opcodes':sorted(expected)}
            if name == 'globals':
                item['original_component_paths'] = paths
            else:
                require(original.read(a,size) == target.read(b,size), 'Complete goal global payload changed')
                item['sha256'] = digest(original.read(a,size))
            globals_proof.append(item); accounted.update(a+off for off in offsets)
        table_records = []; table_methods = {}; method_records = []
        methods = ['Clear','Enter','Exit','Suspend','Resume','PreCalc','EvalRules','Process','SysEvent','NPCMessage','CollReview']
        for name, record in sorted(tables.items()):
            a = record['reference_address']; size = record['size']; b = pair_map[a]['target_address']
            require(roles[a] == {9}, 'Goal table is not loaded as a data pointer')
            storage(original,a,size,'initialized_data'); storage(target,b,size,'initialized_data')
            aa, bb = words(original.read(a,size)), words(target.read(b,size)); entries = []
            for index, (x,y) in enumerate(zip(aa,bb)):
                if index < 2 or name == '__vt__5xGoal' and index == 2:
                    require(x == y == 0, 'Goal vtable null prefix changed'); continue
                member = original.by_address.get(x)
                require(member is not None and member['name'] == methods[index-2] and x in linkages,
                        'Goal vtable slot lacks its original named method identity')
                require(x not in table_methods or table_methods[x] == y, 'Goal vtable method correspondence inconsistent')
                table_methods[x] = y
                entries.append({'offset':index*4,'reference_address':x,'target_address':y,
                                'source':member['source'],'name':member['name'],'linkage_name':linkages[x],
                                'size':member['high']-x})
            table_records.append({**record,'name':name,'target_address':b,'entries':entries,
                                  'reference_sha256':digest(original.read(a,size)), 'target_sha256':digest(target.read(b,size))})
            accounted.add(a)
        require(accounted == set(pair_map) and len(pair_map) == 22 and
                len({p['target_address'] for p in pair_map.values()}) == 22, 'Goal complete data inventory/bijection changed')
        require(len(table_methods) == 56 and len(set(table_methods.values())) == 56, 'Goal complete table-method inventory changed')
        for a,b in sorted(table_methods.items()):
            f = original.by_address[a]; size = f['high']-a
            if a in member_map:
                require(member_map[a]['target_address'] == b, 'Goal vtable method does not enter its complete matched sequence body')
            else:
                pairs,calls,_ = compare(original,target,a,b,size,address_resolver=goal_pair)
                require(not pairs, 'External goal method has unaccounted absolute data')
                rb, tb = ref_flow.bounds(a,size), flow.bounds(b,size)
                extra_tail = None
                if not rb['passes'] or not tb['passes']:
                    require((f['source'],f['name'],size) == ('SB/Game/zNPCGoalCommon.h','Clear',8) and
                            len(calls)==1 and calls[0]['offset']==0 and calls[0]['opcode']==2 and
                            original.word(a+4)==target.word(b+4)==0xa4800048 and
                            rb['reasons']==tb['reasons']==['extent_not_terminal_return_delay','local_edge_outside_extent','no_return'],
                            'External goal table method has an unreviewed boundary')
                    c,d=calls[0]['reference_address'],calls[0]['target_address']; cf=original.by_address.get(c)
                    require(cf and (cf['source'],cf['name'],cf['high']-c)==('SB/Core/x/xBehaviour.cpp','Clear',8) and
                            original.read(c,8)==target.read(d,8) and ref_flow.bounds(c,8)['passes'] and flow.bounds(d,8)['passes'],
                            'Header Clear tail target does not independently close with identical bytes')
                    extra_tail={'reference_address':c,'target_address':d,'source':cf['source'],'name':cf['name'],
                                'size':8,'sha256':digest(original.read(c,8)),'promoted_as_named_anchor':False}
                for c in calls:
                    x,y=c['reference_address'],c['target_address']
                    require(x not in callees or callees[x]==y,'Goal table-method call correspondence changed');callees[x]=y
                method_records.append({'reference_address':a,'target_address':b,'source':f['source'],'name':f['name'],
                                       'linkage_name':linkages[a],'size':size,'reference_sha256':digest(original.read(a,size)),
                                       'target_sha256':digest(target.read(b,size)),'direct_transfers':calls,
                                       'complete_original_tail_context':extra_tail,
                                       'promoted_as_named_anchor':False,'included_in_source_unit':False})
            require(a not in callees or callees[a] == b, 'Goal direct/table call mappings disagree')
        require(len(method_records)==11,'External complete goal table-method count changed')
        require(len(set(callees.values())) == len(callees), 'Distinct goal callees collapse')
        context_map={c['reference_address']:c['target_address'] for c in contexts}
        for a,b in sorted(callees.items()):
            if a in member_map:
                require(member_map[a]['target_address']==b, 'Goal internal call changed member identity'); continue
            if a in context_map:
                require(context_map[a]==b,'Goal header context call changed identity'); continue
            require(not any(fs[0]['low'] <= a < fs[-1]['high'] for fs in groups.values()),
                    'Goal internal call enters an unowned interior')
            f=original.by_address.get(a);size=min(32,f['high']-a) if f else 32
            pairs,calls,_=compare(original,target,a,b,size,address_resolver=goal_pair)
            neighbors.append({'version':version,'reference_address':a,'target_address':b,
                              'reference_name':f['name'] if f else None,'reference_source':f['source'] if f else None,
                              'compared_entry_prefix_bytes':size,'reference_sha256':digest(original.read(a,size)),
                              'target_sha256':digest(target.read(b,size)),'data_address_operands':pairs,
                              'direct_transfers':calls,'promoted_as_named_anchor':False})
        shape = {'globals':[(g['type'] if 'type'in g else g['fundamental_type'],g['target_address'],g['size'],g['component_offsets']) for g in globals_proof],
                 'tables':[(t['name'],t['target_address'],t['size'],[(e['offset'],e['source'],e['linkage_name'],e['target_address'],e['size']) for e in t['entries']]) for t in table_records]}
        require(cross_data is None or cross_data==shape,'Goal original data/method identities disagree across regions');cross_data=shape
        data_proofs.append({'version':version,'source_units':list(UNITS),'original_aggregate_layouts':layouts,
                            'typed_globals':globals_proof,'complete_original_vtables':table_records,
                            'complete_external_table_methods':method_records,
                            'no_man_land_scope':'Original vtable descriptor and actual entries only; no inferred class layout or missing method.'})
    groups={}
    for n in neighbors:groups.setdefault(n['target_address'],[]).append(n)
    for entries in groups.values():
        require(len(entries)==3 and {e['version'] for e in entries}==set(REFERENCES) and
                len({(e['reference_name'],e['reference_source'],e['compared_entry_prefix_bytes']) for e in entries})==1,
                'Goal external-neighbor identities disagree')
    return {'functions':functions,'sequence_proofs':sequences,'data_proofs':data_proofs,'call_neighbors':neighbors,
            'counts':{'functions':59,'code_bytes':18884,'source_units':2,'closed_return_bodies':55,
                      'reviewed_leaf_tail_functions':3,'reviewed_dead_islands':1,'original_vtables':15,
                      'complete_external_table_methods':11,'header_context_bodies_not_promoted':2}}
