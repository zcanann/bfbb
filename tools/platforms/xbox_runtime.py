"""Verify the specifically reviewed Microsoft x87 fmod dispatch entry.

This is not a general tail-call boundary rule. The original descriptor names
fmod and points to its FPREM implementation; pinned vendor COFF independently
supplies the ten-byte __CIfmod symbol extent and its two named relocations.
Runtime code receives no game-source credit.
"""
from __future__ import annotations

import hashlib
from pathlib import Path
import struct

VENDOR = {
    'archive_sha256': '780aa4cbe614efeeb72e3bb9538230f5cd37caeb0adc5d70c346b0dda19ba3a5',
    'member': '..\\build\\intel\\mt_obj\\87fmod.obj',
    'member_sha256': 'a1837d7b2935ef335a4f7d484c868cdda07bd1e94568ecf5507734aa55d88552',
    'symbol': '__CIfmod',
    'section': '.text',
    'symbol_offset': 10,
    'function_size': 10,
    'relocations': [[1, '__OP_FMODjmptab', 6], [6, '__cintrindisp2', 20]],
}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def verify_cifmod_vendor(library: Path) -> dict:
    """Replay provenance from the actual pinned, private CRT archive."""
    archive = library.read_bytes()
    require(hashlib.sha256(archive).hexdigest() == VENDOR['archive_sha256'] and
            archive[:8] == b'!<arch>\n', 'Unexpected CIfmod vendor archive')
    cursor, strings, selected = 8, b'', []
    while cursor < len(archive):
        header = archive[cursor:cursor + 60]
        require(len(header) == 60 and header[58:] == b'`\n', 'Invalid CRT archive member')
        size = int(header[48:58])
        require(cursor + 60 + size <= len(archive), 'Truncated CRT archive member')
        name = header[:16].decode('ascii').strip()
        body = archive[cursor + 60:cursor + 60 + size]
        if name == '//':
            strings = body
        elif name.startswith('/') and name[1:].isdigit():
            offset = int(name[1:])
            require(offset < len(strings), 'Invalid CRT member name offset')
            name = strings[offset:strings.index(b'\0', offset)].decode('ascii')
        if name == VENDOR['member']:
            selected.append(body)
        cursor += 60 + size + size % 2
    require(len(selected) == 1 and hashlib.sha256(selected[0]).hexdigest() == VENDOR['member_sha256'],
            'Missing or changed vendor 87fmod object')
    body = selected[0]
    machine, sections, _, symbols_at, symbol_count, optional, _ = struct.unpack_from('<HHIIIHH', body)
    require(machine == 0x14c and sections >= 2 and optional == 0, 'Unexpected vendor COFF layout')
    strings_at = symbols_at + symbol_count * 18
    symbols, index = {}, 0
    while index < symbol_count:
        raw, value, section, kind, storage, aux = struct.unpack_from('<8sIhHBB', body, symbols_at + index * 18)
        if raw[:4] == b'\0' * 4:
            start = strings_at + struct.unpack_from('<I', raw, 4)[0]
            name = body[start:body.index(b'\0', start)].decode('ascii')
        else:
            name = raw.rstrip(b'\0').decode('ascii')
        symbols[index] = {'name': name, 'value': value, 'section': section, 'kind': kind,
                          'storage': storage, 'aux': aux,
                          'function_size': struct.unpack_from('<I', body, symbols_at + (index + 1) * 18 + 4)[0]
                          if aux else None}
        index += 1 + aux
    symbol = next(s for s in symbols.values() if s['name'] == VENDOR['symbol'])
    require(symbol == {'name': '__CIfmod', 'value': 10, 'section': 1, 'kind': 0x20,
                       'storage': 2, 'aux': 1, 'function_size': 10}, 'Vendor function extent differs')
    section = struct.unpack_from('<8sIIIIIIHHI', body, 20)
    require(section[0].rstrip(b'\0') == b'.text', 'Vendor code section differs')
    code = body[section[4] + 10:section[4] + 20]
    require(code == bytes.fromhex('ba00000000e900000000'), 'Vendor dispatch instructions differ')
    relocations = []
    for i in range(section[7]):
        offset, target, kind = struct.unpack_from('<IIH', body, section[5] + i * 10)
        if 10 <= offset < 20:
            relocations.append([offset - 10, symbols[target]['name'], kind])
    require(relocations == VENDOR['relocations'], 'Vendor dispatch relocations differ')
    return VENDOR


def verify_cifmod_dispatch(original, function: dict) -> None:
    """Check this original's named descriptor, tail entry and real FPREM body."""
    proof = function['corroboration']['runtime_dispatch']
    require(function['canonical_identifier'] == '_CIfmod' and function['size'] == 10 and
            function['source'] == 'Runtime/MSVC/87fmod.obj' and
            function['source_comparison_available'] is False and proof['vendor'] == VENDOR,
            'Unsupported runtime-dispatch record')
    raw = original.read(function['address'], 10)
    descriptor, implementation, dispatcher = (proof[key] for key in ('descriptor', 'implementation', 'dispatcher'))
    for record in (descriptor, implementation, dispatcher):
        original.check_hash(record)
    require(raw[0] == 0xba and raw[5] == 0xe9 and
            struct.unpack_from('<I', raw, 1)[0] == descriptor['address'] and
            function['address'] + 10 + struct.unpack_from('<i', raw, 6)[0] == dispatcher['address'],
            'Original CIfmod descriptor/dispatcher operands differ')
    require(descriptor['size'] == 20 and
            original.read(descriptor['address'], 16) == bytes.fromhex('04666d6f640000000000000000021600') and
            struct.unpack('<I', original.read(descriptor['address'] + 16, 4))[0] == implementation['address'],
            'Original descriptor does not identify the reviewed fmod implementation')
    require(implementation['size'] == 14 and
            original.read(implementation['address'], 14) == bytes.fromhex('d9c9d9f89bdfe09b9e7af7ddd9c3') and
            original.section(implementation['address'], 14)['name'] == '.text' and
            original.section(dispatcher['address'], dispatcher['size'])['name'] == '.text',
            'Original x87 FPREM loop or dispatcher ownership differs')
