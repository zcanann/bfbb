"""Explicit original-only data bindings for partial Xbox source comparisons.

MSVC's absolute COFF symbols allocate no storage and produce no PE HIGHLOW.
This separate opt-in path verifies target preparation, the unresolved compiler
symbol, final absolute MAP ownership and each decoded data operand. It never
binds code, creates runtime stubs, or credits the external object's bytes.
"""
from __future__ import annotations
import hashlib
import json
from pathlib import Path
import re
import struct


def require(condition, message):
    if not condition:
        raise ValueError(message)


def load_bindings(output: Path, unit: dict, target_object: Path) -> dict:
    specs = unit.get('original_data_bindings', {})
    if not specs:
        return {}
    proof = json.loads((output / 'original-data-bindings.json').read_text(encoding='utf-8'))
    coverage = json.loads((output / 'coverage.json').read_text(encoding='utf-8'))
    require(proof.get('schema_version') == 1 and
            proof.get('executable_sha1') == coverage['executable_sha1'],
            'Original data proof belongs to another executable')
    require(proof['target_objects'].get(unit['target_unit']) ==
            hashlib.sha256(target_object.read_bytes()).hexdigest(),
            'Original data proof does not identify this target object')
    result, names = {}, set()
    for name, spec in specs.items():
        require(set(spec) == {'linkage_name', 'size', 'source'},
                'Original binding must declare only a data symbol, size and owner')
        anchor = proof['anchors'].get(name)
        require(anchor and anchor['kind'] == 'data' and
                anchor['size'] == spec['size'] and anchor['source'] == spec['source'] and
                isinstance(anchor['size'], int) and anchor['size'] > 0 and
                0 < anchor['address'] < anchor['address'] + anchor['size'] <= 0x100000000,
                'Original binding differs from independently reviewed object')
        linkage = spec['linkage_name']
        require(isinstance(linkage, str) and linkage and '\0' not in linkage and linkage not in names,
                'Missing or duplicate original data linkage name')
        names.add(linkage)
        require(not any(anchor['address'] < other['address'] + other['size'] and
                        other['address'] < anchor['address'] + anchor['size'] for other in result.values()),
                'Original data bindings overlap')
        result[name] = {**anchor, 'linkage_name': linkage}
    require(not (set(result) & set(unit.get('globals', {}))),
            'Original data cannot also be a reconstructed source global')
    return result


def binding_object(bindings: dict) -> bytes:
    require(bool(bindings), 'Empty original data binding object')
    strings, symbols = bytearray(4), bytearray()
    for anchor in bindings.values():
        name = anchor['linkage_name'].encode('utf-8') + b'\0'
        offset = len(strings)
        strings.extend(name)
        # IMAGE_SYM_ABSOLUTE, null (data) type, EXTERNAL. No sections or payload.
        symbols.extend(struct.pack('<IIIhHBB', 0, offset, anchor['address'], -1, 0, 2, 0))
    struct.pack_into('<I', strings, 0, len(strings))
    return (struct.pack('<HHIIIHH', 0x14c, 0, 0, 20, len(bindings), 0, 0) +
            bytes(symbols) + bytes(strings))


def verify_undefined(log: str, result: int, source_object: str, bindings: dict) -> None:
    """An unbound real LTCG link must fail on exactly these source data symbols."""
    require(result != 0, 'Original binding was not an undefined source dependency')
    errors = [line.strip() for line in log.splitlines() if ': error LNK' in line]
    expected = {a['linkage_name'] for a in bindings.values()}
    seen = set()
    for line in errors:
        require(line.startswith(source_object + ' : error LNK2001: unresolved external symbol '),
                'Unbound link has an unrelated error or wrong source owner')
        matches = [name for name in expected if line.endswith('(' + name + ')') or
                   line.endswith('symbol ' + name)]
        require(len(matches) == 1 and matches[0] not in seen,
                'Unbound link has an unexpected or repeated dependency')
        seen.add(matches[0])
    require(seen == expected and f'fatal error LNK1120: {len(expected)} unresolved externals' in log,
            'Unbound link does not prove exactly the declared original data dependencies')


def verify_map(map_path: Path, sections: list[dict], bindings: dict) -> dict:
    rows = re.findall(r'^\s*0000:([0-9a-fA-F]{8})\s+(\S+)\s+([0-9a-fA-F]{8})\s+<absolute>\s*$',
                      map_path.read_text(encoding='utf-8'), re.MULTILINE)
    result = {}
    for name, anchor in bindings.items():
        matches = [(int(offset, 16), int(address, 16)) for offset, symbol, address in rows
                   if symbol == anchor['linkage_name']]
        require(matches == [(anchor['address'], anchor['address'])],
                'Linked original data symbol is missing, ambiguous or not absolute')
        require(not any(anchor['address'] < s['address'] + s['size'] and
                        s['address'] < anchor['address'] + anchor['size'] for s in sections),
                'Original data assignment overlaps the linked source image')
        result[name] = {'address': anchor['address'], 'size': anchor['size']}
    return result


def expressions(body: bytes, templates: list[dict], anchors: dict, highlow: set[int]) -> list[dict]:
    """Discover only declared absolute memory operands, never calls or immediates."""
    import capstone as cs
    from .xbox_relocations import _address_field
    decoder = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
    decoder.detail = True
    instructions = list(decoder.disasm(body, 0))
    require(sum(i.size for i in instructions) == len(body), 'Undecodable externally bound source')
    result = []
    for template in templates:
        require(set(template) <= {'symbol', 'addend', 'field', 'prefix', 'count'} and
                template.get('field') == 'displacement' and template['symbol'] in anchors,
                'Original data expressions cannot specify offsets, calls or immediate values')
        anchor = anchors[template['symbol']]
        addend = template.get('addend', 0)
        require(isinstance(addend, int) and 0 <= addend < anchor['size'],
                'Original data addend lies outside its object')
        matches = []
        for instruction in instructions:
            operands = [op for op in instruction.operands if op.type == cs.x86.X86_OP_MEM]
            field = instruction.disp_offset
            offset = instruction.address + field
            if (not instruction.group(cs.CS_GRP_CALL) and not instruction.group(cs.CS_GRP_JUMP) and
                    len(operands) == 1 and operands[0].mem.base == 0 and operands[0].mem.index == 0 and
                    operands[0].size > 0 and addend + operands[0].size <= anchor['size'] and
                    offset not in highlow and _address_field(instruction, 'displacement') and
                    instruction.bytes[:field].hex() == template['prefix'] and
                    struct.unpack_from('<I', instruction.bytes, field)[0] == anchor['address'] + addend):
                matches.append({**template, 'offset': offset, 'instruction_offset': instruction.address})
        require(len(matches) == template.get('count', 1), 'Original data operand is missing or ambiguous')
        result.extend(matches)
    require(len({e['offset'] for e in result}) == len(result), 'Duplicate original data operand')
    # Every decoded reference into a bound object must be accounted for. This
    # catches changed source uses without mistaking unrelated integers for data.
    actual = set()
    for instruction in instructions:
        for op in instruction.operands:
            if op.type == cs.x86.X86_OP_MEM and not op.mem.base and not op.mem.index and any(
                    a['address'] <= op.mem.disp < a['address'] + a['size'] for a in anchors.values()):
                require(not instruction.group(cs.CS_GRP_CALL) and not instruction.group(cs.CS_GRP_JUMP),
                        'Original data binding cannot normalize indirect control flow')
                actual.add(instruction.address + instruction.disp_offset)
    require(actual == {e['offset'] for e in result}, 'Unaccounted original data reference')
    return sorted(result, key=lambda e: e['offset'])
