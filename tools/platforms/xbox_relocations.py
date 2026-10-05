"""Restore only independently reviewed Xbox DIR32 address expressions.

PE HIGHLOW entries establish source operand provenance. Explicit MAP names and
addends disambiguate aliases, including an array end equal to another global.
No target bytes or addresses are used to identify source fields.
"""
from __future__ import annotations
import hashlib
import json
from pathlib import Path
import re
import struct


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def pe_provenance(path: Path) -> tuple[list[dict], set[int]]:
    data = path.read_bytes()
    require(len(data) >= 64 and data[:2] == b'MZ', 'Expected DOS/PE source image')
    pe = struct.unpack_from('<I', data, 60)[0]
    require(pe + 24 <= len(data) and data[pe:pe + 4] == b'PE\0\0', 'Invalid PE header')
    machine, count, _, _, _, optional_size, _ = struct.unpack_from('<HHIIIHH', data, pe + 4)
    optional = pe + 24
    require(machine == 0x14c and optional_size >= 144 and optional + optional_size + count * 40 <= len(data)
            and struct.unpack_from('<H', data, optional)[0] == 0x10b, 'Expected i386 PE32')
    base = struct.unpack_from('<I', data, optional + 28)[0]
    sections = []
    for i in range(count):
        s = struct.unpack_from('<8sIIIIIIHHI', data, optional + optional_size + i * 40)
        require(s[4] + s[3] <= len(data) and base + s[2] + s[1] <= 0x100000000, 'Invalid PE section')
        sections.append({'name': s[0].rstrip(b'\0').decode('ascii'), 'address': base + s[2],
                         'size': s[1], 'raw_size': s[3], 'offset': s[4], 'flags': s[9]})
    require(struct.unpack_from('<I', data, optional + 92)[0] >= 6, 'Missing PE relocation directory')
    rva, size = struct.unpack_from('<II', data, optional + 136)
    require(rva and size, 'Missing source PE base relocations')
    owners = [s for s in sections if s['address'] <= base + rva and base + rva + size <=
              s['address'] + min(s['size'], s['raw_size'])]
    require(len(owners) == 1, 'PE relocation directory is not file-backed')
    owner = owners[0]
    cursor = owner['offset'] + base + rva - owner['address']
    end, addresses = cursor + size, set()
    while cursor < end:
        require(cursor + 8 <= end, 'Truncated PE relocation block')
        page, length = struct.unpack_from('<II', data, cursor)
        require(length >= 8 and length % 2 == 0 and cursor + length <= end and page % 4096 == 0,
                'Invalid PE relocation block')
        for offset in range(cursor + 8, cursor + length, 2):
            item = struct.unpack_from('<H', data, offset)[0]
            kind = item >> 12
            if kind == 0:
                continue
            require(kind == 3, 'Only PE HIGHLOW source relocations are supported')
            address = base + page + (item & 4095)
            require(address not in addresses and any(s['address'] <= address and address + 4 <=
                    s['address'] + s['size'] for s in sections), 'Invalid or duplicate PE HIGHLOW')
            addresses.add(address)
        cursor += length
    return sections, addresses


def source_globals(map_path: Path, unit: dict, sections: list[dict], executable: Path | None = None) -> dict:
    pattern = re.compile(r'^\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+'
                         r'([0-9a-fA-F]{8})\s+(\S+)\s*$', re.MULTILINE)
    rows = [(name, int(address, 16), owner) for name, address, owner in
            pattern.findall(map_path.read_text(encoding='utf-8'))]
    result = {}
    for name, spec in unit.get('globals', {}).items():
        if 'definition_use' in spec:
            # LTCG omits file-static data from MAP. Resolve only a declared use
            # in its actual named source function, backed by PE HIGHLOW.
            from .xbox_source import _map_functions, _pe_text
            import capstone as cs
            require(executable is not None, 'Static source data requires linked PE')
            use = spec['definition_use']
            functions = _map_functions(map_path)
            selected = [f for f in functions if f['name'] == use['function']]
            require(len(selected) == 1, 'Missing static-data source function')
            function = selected[0]
            text, base = _pe_text(executable)
            bound = min([f['address'] for f in functions if f['address'] > function['address']] + [base + len(text)])
            decoder = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
            decoder.detail = True
            _, highlow = pe_provenance(executable)
            matches = []
            for instruction in decoder.disasm(text[function['address'] - base:bound - base], function['address']):
                field = instruction.disp_offset
                if (instruction.disp_size == 4 and field + 4 == instruction.size and
                        instruction.bytes[:field].hex() == use['prefix'] and instruction.address + field in highlow):
                    matches.append(struct.unpack_from('<I', instruction.bytes, field)[0])
            require(len(matches) == 1, 'Ambiguous source static-data operand')
            address, owner = matches[0], function['object']
        else:
            selected = [row for row in rows if row[0] == spec['linkage_name']]
            require(len(selected) == 1, f'Missing or ambiguous source global {name}')
            _, address, owner = selected[0]
        size = spec['size']
        require(owner.lower() == spec.get('object', Path(unit['object']).name).lower() and isinstance(size, int) and size > 0,
                'Source global has wrong object ownership or size')
        owners = [s for s in sections if s['address'] <= address and address + size <= s['address'] + s['size']
                  and not s['flags'] & 0x20000000 and
                  (bool(s['flags'] & 0x80000000) if 'value_hex' not in spec else not s['flags'] & 0x80000000)]
        require(len(owners) == 1, 'Source global range has incorrect data ownership')
        if 'value_hex' in spec:
            section = owners[0]
            offset = section['offset'] + address - section['address']
            require(executable is not None and address + size <= section['address'] + section['raw_size'] and
                    len(bytes.fromhex(spec['value_hex'])) == size and
                    executable.read_bytes()[offset:offset + size] == bytes.fromhex(spec['value_hex']),
                    'Source literal differs from independently reviewed value')
        require(not any(address < other < address + size for _, other, _ in rows),
                'Another MAP global lies inside the reviewed source range')
        result[name] = {'address': address, 'size': size}
    return result


def normalize(body: bytes, expressions: list[dict], anchors: dict,
              source_highlow: set[int] | None = None) -> tuple[bytes, list[dict]]:
    """Check every declared address field, then emit its actual COFF addend.

    source_highlow contains offsets in this function, never target offsets.
    Substituting each independently named symbol address must invert exactly.
    """
    import capstone as cs
    decoder = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
    decoder.detail = True
    instructions = {i.address: i for i in decoder.disasm(body, 0)}
    require(sum(i.size for i in instructions.values()) == len(body), 'Undecodable address-expression body')
    fields = [e['offset'] for e in expressions]
    require(len(set(fields)) == len(fields), 'Duplicate address expression')
    require(all(a + 4 <= b for a, b in zip(sorted(fields), sorted(fields)[1:])), 'Overlapping address fields')
    if source_highlow is not None:
        require(set(fields) == source_highlow, 'Source address expressions do not cover exactly the PE HIGHLOW fields')
    normalized, inverse, relocations = bytearray(body), bytearray(body), []
    for expression in expressions:
        offset, symbol, addend = expression['offset'], expression['symbol'], expression.get('addend', 0)
        instruction = instructions.get(expression['instruction_offset'])
        require(instruction is not None and symbol in anchors, 'Missing instruction or reviewed data anchor')
        field = expression['field']
        require(field in ('displacement', 'immediate'), 'Unsupported address operand')
        field_offset = instruction.disp_offset if field == 'displacement' else instruction.imm_offset
        field_size = instruction.disp_size if field == 'displacement' else instruction.imm_size
        require(field_size == 4 and instruction.address + field_offset == offset and
                instruction.bytes[:field_offset].hex() == expression['prefix'] and
                field_offset + 4 == instruction.size, 'Address expression does not describe the actual instruction operand')
        anchor = anchors[symbol]
        require(isinstance(addend, int) and 0 <= addend <= anchor['size'], 'Address addend outside reviewed object/end')
        value = anchor['address'] + addend
        require(value <= 0xffffffff and struct.unpack_from('<I', body, offset)[0] == value,
                'Address operand does not equal its independently named symbol plus addend')
        struct.pack_into('<I', normalized, offset, addend)
        struct.pack_into('<I', inverse, offset, anchor['address'] + struct.unpack_from('<I', normalized, offset)[0])
        relocations.append({'offset': offset, 'symbol': symbol, 'addend': addend})
    require(bytes(inverse) == body, 'Address-expression inverse changed original bytes')
    return bytes(normalized), relocations


def discover_source_expressions(body: bytes, templates: list[dict], anchors: dict,
                                highlow: set[int]) -> list[dict]:
    """Resolve semantic operands from source instructions, not original offsets."""
    import capstone as cs
    decoder = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
    decoder.detail = True
    instructions = list(decoder.disasm(body, 0))
    require(sum(i.size for i in instructions) == len(body), 'Undecodable source address expressions')
    result = []
    for template in templates:
        require('offset' not in template and 'instruction_offset' not in template,
                'Source expression discovery must not use target instruction offsets')
        symbol, addend, field = template['symbol'], template.get('addend', 0), template['field']
        require(symbol in anchors and field in ('displacement', 'immediate'), 'Unsupported source expression')
        matches = []
        for instruction in instructions:
            at = instruction.disp_offset if field == 'displacement' else instruction.imm_offset
            size = instruction.disp_size if field == 'displacement' else instruction.imm_size
            offset = instruction.address + at
            if (size == 4 and offset in highlow and at + 4 == instruction.size and
                    instruction.bytes[:at].hex() == template['prefix'] and
                    struct.unpack_from('<I', body, offset)[0] == anchors[symbol]['address'] + addend):
                matches.append({**template, 'offset': offset, 'instruction_offset': instruction.address})
        require(len(matches) == template.get('count', 1), 'Source address expression is missing or ambiguous')
        result.extend(matches)
    require(len({e['offset'] for e in result}) == len(result) and {e['offset'] for e in result} == highlow,
            'Source expressions do not uniquely cover actual PE HIGHLOW fields')
    return sorted(result, key=lambda e: e['offset'])


def original_anchors(path: Path, executable_sha1: str, sections: list[dict]) -> dict:
    document = json.loads(path.read_text(encoding='utf-8'))
    require(document['executable_sha1'] == executable_sha1 and document['coverage_complete'] is False,
            'Data anchors identify another original or overclaim coverage')
    result = {}
    for anchor in document['anchors']:
        name, address, size = anchor['name'], anchor['address'], anchor['size']
        require(name and name not in result and size > 0 and anchor['identity_confirmation'] is True,
                'Duplicate or unreviewed data anchor')
        require(sum(s['virtual_address'] <= address and address + size <= s['virtual_address'] + s['virtual_size']
                    and (bool(s['flags'] & 1) if 'value_hex' not in anchor else s['name'] == '.rdata') for s in sections) == 1, 'Original anchor lacks unique writable mapped ownership')
        result[name] = {'address': address, 'size': size}
    return result


def reconstruct(body: bytes, relocations: list[dict], anchors: dict) -> bytes:
    result = bytearray(body)
    for relocation in relocations:
        offset, name = relocation['offset'], relocation['symbol']
        require(name in anchors and 0 <= offset <= len(body) - 4, 'Invalid COFF address reconstruction')
        addend = struct.unpack_from('<I', body, offset)[0]
        require(addend <= anchors[name]['size'], 'COFF addend outside reviewed object/end')
        struct.pack_into('<I', result, offset, anchors[name]['address'] + addend)
    return bytes(result)


def verify_original_anchors(path: Path, original) -> dict:
    """Recheck the reviewed pool/head semantic anchor using actual original code."""
    import capstone as cs
    anchors = original_anchors(path, original.sha1, original.metadata['sections'])
    document = json.loads(path.read_text(encoding='utf-8'))
    proof = document['pool_loop']
    raw = original.read(proof['address'], proof['size'])
    require(hashlib.sha256(raw).hexdigest() == proof['sha256'], 'Reviewed pool-loop bytes differ')
    pool, head = anchors[proof['pool']], anchors[proof['head']]
    require(proof['stride'] == 96 and proof['count'] == 2000 and head['size'] == 4 and
            pool['size'] == proof['stride'] * proof['count'] and
            pool['address'] + pool['size'] == proof['end'], 'Particle pool extent equation differs')
    require(pool['address'] + pool['size'] <= head['address'] or head['address'] + 4 <= pool['address'],
            'Reviewed particle globals overlap')
    decoder = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
    instructions = [(i.address - proof['address'], i.mnemonic, i.op_str)
                    for i in decoder.disasm(raw, proof['address'])]
    start = proof['address']
    expected = [(0, 'mov', f"ecx, dword ptr [{head['address']:#x}]"),
                (6, 'mov', f"eax, {pool['address']:#x}"), (11, 'xor', 'edx, edx'),
                (13, 'lea', 'ecx, [ecx]'), (16, 'cmp', 'ecx, edx'),
                (18, 'mov', 'dword ptr [eax], edx'), (20, 'mov', 'dword ptr [eax + 4], edx'),
                (23, 'je', f'{start + 30:#x}'), (25, 'mov', 'dword ptr [ecx + 4], eax'),
                (28, 'mov', 'dword ptr [eax], ecx'), (30, 'mov', 'ecx, eax'),
                (32, 'add', 'eax, 0x60'), (35, 'cmp', f"eax, {proof['end']:#x}"),
                (40, 'jl', f'{start + 16:#x}'),
                (42, 'mov', f"dword ptr [{head['address']:#x}], ecx"), (48, 'ret', '')]
    require(instructions == expected, 'Original loop no longer establishes the reviewed particle data anchors')
    for proof in document.get('instruction_witnesses', []):
        original.check_hash(proof)
        actual = [(i.address - proof['address'], i.mnemonic, i.op_str)
                  for i in decoder.disasm(original.read(proof['address'], proof['size']), proof['address'])]
        require(actual == [tuple(i) for i in proof['instructions']], 'Reviewed data-anchor instruction witness differs')
    for anchor in document['anchors']:
        if 'value_hex' in anchor:
            value = bytes.fromhex(anchor['value_hex'])
            require(len(value) == anchor['size'] and original.read(anchor['address'], anchor['size']) == value,
                    'Original literal value differs')
    return anchors
