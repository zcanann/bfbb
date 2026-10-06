"""Initializer-scoped particle callback addresses, separate from data globals.

The optional non-LTCG sidecar proves named source storage/relocation ownership;
only the ordinary /GL linked PE supplies compared bytes. Neither arbitrary RETs
nor arbitrary executable addresses are accepted as callback identities.
"""
from __future__ import annotations
import hashlib
from pathlib import Path
import struct
from collections import Counter

INITIALIZER = 'xParCmdInit()'
LINKAGE = '?xParCmdInit@@YAXXZ'
TABLE = 'xParCmd::sCmdInfo'
EMPTY = 'xParCmd::shared_empty_callback'
TABLE_SIZE = 35 * 12
EMPTY_ALIASES = {
    11: 'xParCmdScale3rdPolyReg_Update', 12: 'xParCmdTex_Update',
    19: 'xParCmdAlpha3rdPolyReg_Update', 24: 'xParCmdSmokeAlpha_Update',
    25: 'xParCmdScale_Update',
}

def require(value, message):
    if not value:
        raise ValueError('Particle initializer: ' + message)


def program(body: bytes):
    """Decode the complete constant-store program; no loads/calls/branches."""
    import capstone as cs
    from capstone.x86 import X86_OP_REG, X86_OP_MEM, X86_OP_IMM
    decoder = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
    decoder.detail = True
    instructions = list(decoder.disasm(body, 0))
    require(sum(i.size for i in instructions) == len(body) and instructions[-1].mnemonic == 'ret',
            'initializer is not a complete terminal-RET program')
    registers, stores, stack = {}, {}, []
    for i in instructions:
        operands = i.operands
        if i.mnemonic in ('push', 'pop'):
            require(len(operands) == 1 and operands[0].type == X86_OP_REG, 'non-register stack operation')
            reg = operands[0].reg
            if i.mnemonic == 'push':
                stack.append(reg)
            else:
                require(stack and stack.pop() == reg, 'unbalanced saved register')
                registers.pop(reg, None)
            continue
        if i.mnemonic == 'ret':
            require(i.address + i.size == len(body) and not stack and not operands, 'early/unbalanced return')
            continue
        if i.mnemonic == 'xor':
            require(len(operands) == 2 and operands[0].type == operands[1].type == X86_OP_REG and
                    operands[0].reg == operands[1].reg and operands[0].size == 4, 'nonzeroing arithmetic')
            registers[operands[0].reg] = 0
            continue
        require(i.mnemonic == 'mov' and len(operands) == 2 and
                operands[0].size == operands[1].size == 4, 'unsupported instruction')
        left, right = operands
        require(right.type == X86_OP_IMM or right.type == X86_OP_REG and right.reg in registers,
                'nonconstant stored value')
        value = (right.imm if right.type == X86_OP_IMM else registers[right.reg]) & 0xffffffff
        if left.type == X86_OP_REG:
            registers[left.reg] = value
        else:
            require(left.type == X86_OP_MEM and not left.mem.base and not left.mem.index and
                    left.mem.disp not in stores, 'nonabsolute or repeated store')
            stores[left.mem.disp] = value
    return instructions, stores


def check_slots(stores, table, registrations, addresses):
    require(len(registrations) == 27 and len({r['slot'] for r in registrations}) == 27,
            'registration inventory differs')
    expected = {}
    for row in registrations:
        slot = row['slot']
        require(isinstance(slot, int) and 0 <= slot < 35, 'invalid registration slot')
        expected[table + slot * 12] = slot
        expected[table + slot * 12 + 4] = row['asset_size']
        expected[table + slot * 12 + 8] = addresses[row['symbol']] if row['symbol'] else 0
    require(stores == expected, 'complete IDs, sizes, callback associations or store ownership differ')


def expressions(body, instructions, anchors, fields=None):
    result = []
    for i in instructions:
        for kind, offset, size in [('displacement', i.disp_offset, i.disp_size),
                                   ('immediate', i.imm_offset, i.imm_size)]:
            at = i.address + offset
            if size != 4:
                continue
            value = struct.unpack_from('<I', body, at)[0]
            matches = [(name, a) for name, a in anchors.items() if
                       (a['kind'] == 'function' and value == a['address']) or
                       (a['kind'] == 'data' and a['address'] <= value < a['address'] + a['size'])]
            if fields is not None and at not in fields:
                require(not matches, 'known address lacks genuine PE HIGHLOW')
                continue
            if not matches:
                require(fields is None or at not in fields, 'unowned PE HIGHLOW')
                continue
            require(len(matches) == 1, 'ambiguous address role')
            name, anchor = matches[0]
            result.append({'offset': at, 'instruction_offset': i.address, 'field': kind,
                           'prefix': i.bytes[:offset].hex(), 'symbol': name,
                           'addend': value - anchor['address']})
    require(len(result) == 107, 'expected exactly 81 data and 26 function address fields')
    require(Counter(anchors[e['symbol']]['kind'] for e in result) == {'data': 81, 'function': 26},
            'address-role inventory differs')
    if fields is not None:
        require({e['offset'] for e in result} == fields, 'incomplete HIGHLOW coverage')
    return result


def original_anchors_from_text(text, base, function, reviewed, sections):
    proof = function.get('corroboration', {}).get('particle_initializer')
    if not proof:
        return {}
    require(function['canonical_identifier'] == INITIALIZER and function['size'] == 721,
            'unexpected original initializer')
    start = function['address'] - base
    require(0 <= start and start + function['size'] <= len(text), 'original bounds leave .text')
    body = text[start:start + function['size']]
    require(hashlib.sha256(body).hexdigest() == function['sha256'], 'original initializer hash differs')
    instructions, stores = program(body)
    table = proof['table_address']
    owners = [s for s in sections if s['name'] == '.data' and s['flags'] & 1 and
              s['virtual_address'] <= table and table + TABLE_SIZE <= s['virtual_address'] + s['virtual_size']]
    require(len(owners) == 1 and proof['table_size'] == TABLE_SIZE, 'original table ownership differs')
    anchors = {TABLE: {'address': table, 'size': TABLE_SIZE, 'kind': 'data'}}
    callbacks = {r['symbol'] for r in proof['registrations']} - {None, EMPTY}
    require(len(callbacks) == 21, 'live callback inventory differs')
    for name in callbacks:
        record = reviewed[name]
        cb = record['corroboration']['callback_registration']
        require(record['boundary_confirmation'] and record['identity_confirmation'] and
                cb['initializer']['address'] == function['address'] and
                cb['initializer']['size'] == function['size'] and cb['initializer']['sha256'] == function['sha256'] and
                cb['table_address'] == table and cb['slot_count'] == 35 and cb['slot_stride'] == 12,
                'callback is not independently reviewed for this original initializer')
        rows = [r for r in proof['registrations'] if r['symbol'] == name]
        require(len(rows) == 1 and rows[0]['slot'] == cb['command_type'] and
                rows[0]['asset_size'] == cb['asset_size'], 'callback slot identity differs')
        offset = record['address'] - base
        require(offset >= 0 and offset + record['size'] <= len(text) and
                hashlib.sha256(text[offset:offset + record['size']]).hexdigest() == record['sha256'],
                'reviewed original callback body differs')
        anchors[name] = {'address': record['address'], 'size': record['size'], 'kind': 'function'}
    empty = proof['empty_callback']
    require(empty['aliases'] == {str(k): v for k, v in EMPTY_ALIASES.items()} and
            {r['slot'] for r in proof['registrations'] if r['symbol'] == EMPTY} == set(EMPTY_ALIASES),
            'shared empty callback aliases differ')
    offset = empty['address'] - base
    require(0 <= offset < len(text) and text[offset:offset + 1] == b'\xc3' and empty['size'] == 1,
            'specific shared callback is not a closed one-byte RET')
    anchors[EMPTY] = {'address': empty['address'], 'size': 1, 'kind': 'function'}
    require(len({a['address'] for a in anchors.values()}) == len(anchors), 'original anchor collision')
    check_slots(stores, table, proof['registrations'], {k: a['address'] for k, a in anchors.items()})
    expected = expressions(body, instructions, anchors)
    require(function['address_expressions'] == expected, 'original relocation expressions differ')
    return anchors


def original_anchors(original, function, reviewed):
    if not function.get('corroboration', {}).get('particle_initializer'):
        return {}
    section = original.section(function['address'], function['size'])
    require(section['name'] == '.text', 'initializer is not original code')
    return original_anchors_from_text(original.read(section['virtual_address'], min(section['raw_size'], section['virtual_size'])),
                                      section['virtual_address'], function, reviewed, original.metadata['sections'])


def verify_sidecar(path, spec, unit):
    """Validate named ordinary COFF storage; this object supplies no scored code."""
    data = Path(path).read_bytes()
    machine, count, _, symoff, nsym, optional, _ = struct.unpack_from('<HHIIIHH', data)
    require(machine == 0x14c and optional == 0 and symoff + nsym * 18 + 4 <= len(data), 'invalid sidecar COFF')
    strings = data[symoff + nsym * 18:]
    require(4 <= struct.unpack_from('<I', strings)[0] <= len(strings), 'invalid COFF string table')
    symbols, index = {}, 0
    while index < nsym:
        name, value, section, typ, storage, aux = struct.unpack_from('<8sIhHBB', data, symoff + index * 18)
        if name[:4] == b'\0' * 4:
            at = struct.unpack_from('<I', name, 4)[0]
            require(4 <= at < len(strings) and b'\0' in strings[at:], 'invalid symbol name')
            name = strings[at:strings.index(0, at)]
        symbols[index] = {'name': name.rstrip(b'\0').decode('ascii'), 'value': value,
                          'section': section, 'type': typ, 'storage': storage}
        index += 1 + aux
    def named(name):
        rows = [s for s in symbols.values() if s['name'] == name]
        require(len(rows) == 1, 'missing/ambiguous named COFF symbol ' + name)
        return rows[0]
    def section(s):
        require(1 <= s['section'] <= count and 20 + count * 40 <= len(data), 'invalid symbol section')
        return struct.unpack_from('<8sIIIIIIHHI', data, 20 + (s['section'] - 1) * 40)
    table = named('_sCmdInfo');header = section(table)
    require(table['storage'] == 3 and table['value'] == 0 and header[0].rstrip(b'\0') == b'.bss' and
            header[3] == TABLE_SIZE and not header[4] and header[9] & 0x80000000,
            'sidecar named static table allocation differs')
    require(all(s is table or s['section'] != table['section'] or
                s['name'] == '.bss' and s['storage'] == 3 and s['value'] == 0
                for s in symbols.values()), 'foreign symbol shares table allocation')
    function = named(LINKAGE);h = section(function)
    require(function['type'] == 0x20 and function['value'] == 0 and h[3] == 721 and h[9] & 0x20000000 and
            h[4] + h[3] <= len(data) and h[5] + h[7] * 10 <= len(data), 'invalid named initializer COFF extent')
    import capstone as cs
    decoded, _ = program(data[h[4]:h[4] + h[3]])
    fields = {}
    for instruction in decoded:
        for kind, offset, size in [('displacement', instruction.disp_offset, instruction.disp_size),
                                  ('immediate', instruction.imm_offset, instruction.imm_size)]:
            if size == 4:
                fields[instruction.address + offset] = (instruction, kind)
    relocs, seen = Counter(), set()
    for i in range(h[7]):
        offset, target, typ = struct.unpack_from('<IIH', data, h[5] + i * 10)
        require(offset + 4 <= h[3] and typ == 6 and target in symbols and
                offset in fields and offset not in seen, 'invalid/duplicate DIR32 initializer operand')
        seen.add(offset)
        instruction, kind = fields[offset]
        name = symbols[target]['name']
        require(instruction.mnemonic == 'mov' and len(instruction.operands) == 2,
                'address relocation is not a decoded MOV operand')
        if name == '_sCmdInfo':
            left = instruction.operands[0]
            require(kind == 'displacement' and left.type == cs.x86.X86_OP_MEM and left.size == 4 and
                    not left.mem.base and not left.mem.index, 'table relocation is not an absolute store destination')
        else:
            right = instruction.operands[1]
            require(kind == 'immediate' and right.type == cs.x86.X86_OP_IMM and right.size == 4,
                    'callback relocation is not a complete immediate field')
        relocs[name] += 1
    expected = Counter({'_sCmdInfo': 81})
    for row in spec['registrations']:
        if row['symbol'] == EMPTY:
            name = EMPTY_ALIASES[row['slot']]
            linkage = '?' + name + '@@YAXPAUxParCmd@@PAUxParGroup@@M@Z'
        elif row['symbol']:
            linkage = unit['symbols'][row['symbol']]
        else:
            continue
        expected[linkage] += 1
    require(relocs == expected, 'sidecar actual named callback/data relocation ownership differs')
    return {'sha256': hashlib.sha256(data).hexdigest(), 'table_symbol': '_sCmdInfo', 'table_size': TABLE_SIZE,
            'method': 'same complete source, pinned compiler; non-GL symbol evidence only; no scored bytes'}


def source_anchors(executable, map_path, unit):
    spec = unit.get('particle_initializer')
    if not spec:
        return {}, []
    from .xbox_source import _pe_text, _map_functions, _leaf_extent
    from .xbox_relocations import pe_provenance
    verify_sidecar(Path(executable).with_name('initializer-symbols.obj'), spec, unit)
    text, base = _pe_text(executable)
    maps = _map_functions(map_path)
    byname = {m['name']: m for m in maps}
    owner = Path(unit['object']).name
    require(unit['symbols'].get(INITIALIZER) == LINKAGE and byname[LINKAGE]['object'] == owner == 'xParCmd.obj',
            'initializer is not actual complete particle source TU')
    addresses = sorted({m['address'] for m in maps})
    def extent(linkage, calls):
        f = byname[linkage]
        require(f['object'] == owner, 'callback owner is not actual particle TU')
        bound = next((a for a in addresses if a > f['address']), base + len(text))
        return _leaf_extent(text, base, f['address'], bound, calls)
    anchors = {}
    for name in {r['symbol'] for r in spec['registrations']} - {None, EMPTY}:
        linkage = unit['symbols'][name]
        calls = {}
        for callee in unit.get('direct_calls', {}).get(name, []):
            dep = unit.get('call_symbols', {}).get(callee)
            target = byname[dep['linkage_name'] if dep else unit['symbols'][callee]]
            require(target['object'] == (dep['object'] if dep else owner), 'callback callee owner differs')
            calls[target['address']] = callee
        e = extent(linkage, calls)
        anchors[name] = {'address': e['address'], 'size': e['size'], 'kind': 'function'}
    aliases = []
    for slot, name in EMPTY_ALIASES.items():
        linkage = '?' + name + '@@YAXPAUxParCmd@@PAUxParGroup@@M@Z'
        e = extent(linkage, {})
        require(e['size'] == 1 and text[e['address'] - base:e['address'] - base + 1] == b'\xc3',
                'specific named empty source alias is not a closed RET')
        aliases.append(e['address'])
    require(len(set(aliases)) == 1, 'declared shared empty source aliases do not fold together')
    anchors[EMPTY] = {'address': aliases[0], 'size': 1, 'kind': 'function'}
    require(len({a['address'] for a in anchors.values()}) == len(anchors), 'source callback collision')
    e = extent(LINKAGE, {})
    body = text[e['address'] - base:e['address'] - base + e['size']]
    instructions, stores = program(body)
    # Derive the base independently from every declared ID/size/callback triple,
    # not min(destination), an original address, or a production code offset.
    candidates = {destination - row['slot'] * 12 for row in spec['registrations']
                  for destination, value in stores.items() if value == row['slot']}
    valid = []
    for table in candidates:
        try:
            check_slots(stores, table, spec['registrations'], {k: a['address'] for k, a in anchors.items()})
        except ValueError:
            continue
        valid.append(table)
    require(len(valid) == 1, 'source registration tuples do not establish exactly one table base')
    table = valid[0]
    sections, highlow = pe_provenance(executable)
    require(len([s for s in sections if s['address'] <= table and table + TABLE_SIZE <= s['address'] + s['size']
                 and s['flags'] & 0x80000000 and not s['flags'] & 0x20000000]) == 1,
            'source table is not one writable nonexecutable allocation range')
    anchors[TABLE] = {'address': table, 'size': TABLE_SIZE, 'kind': 'data'}
    fields = {at - e['address'] for at in highlow if e['address'] <= at < e['address'] + e['size']}
    return anchors, expressions(body, instructions, anchors, fields)
