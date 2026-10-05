"""Identify the original x87 asin wrapper from the pinned vendor object.

Only its closed 20-byte callable wrapper enters target coverage. The complete
203-byte vendor container, including the alternate stack entry at +20, is
identity context. No CRT source code receives matching credit.
"""
from __future__ import annotations

import hashlib
from pathlib import Path
import struct

from .xbox_runtime import require

VENDOR = {
    'archive_sha256': '780aa4cbe614efeeb72e3bb9538230f5cd37caeb0adc5d70c346b0dda19ba3a5',
    'member': '..\\build\\intel\\mt_obj\\asin.obj',
    'member_sha256': '07f2643570eacea16628bffae63f04bf468c871663d6381e170c7dbc4b5d8048',
    'symbol': '__CIasin', 'section': 1, 'symbol_offset': 0,
    'vendor_container_size': 203, 'wrapper_size': 20, 'alternate_entry_offset': 20,
    'normalized_sha256': '6bcc9cd28fea3819b00478234d26f82fe8f73bfaeb274b18a3191e39ad386143',
    'relocations': [[7, '__checkTOS_withFB', 20], [25, '__fload_withFB', 20],
                    [45, '__load_CW', 20], [72, '___fastflag', 6], [79, '__fast_exit', 20],
                    [90, '_NAME_', 6], [95, '__math_exit', 20], [128, '__piby2', 6],
                    [139, '__convertTOStoQNaN', 20], [163, '__indefinite', 6],
                    [174, '___fastflag', 6], [181, '__fast_exit', 20],
                    [192, '_NAME_', 6], [197, '__startOneArgErrorHandling', 20]],
}


def verify_asin_vendor(library: Path) -> dict:
    """Authenticate actual COFF names, auxiliary extent and relocation fields."""
    archive = library.read_bytes()
    require(hashlib.sha256(archive).hexdigest() == VENDOR['archive_sha256'] and
            archive[:8] == b'!<arch>\n', 'Unexpected asin vendor archive')
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
            'Missing or changed vendor asin object')
    body = selected[0]
    machine, count, _, symbols_at, symbol_count, optional, _ = struct.unpack_from('<HHIIIHH', body)
    require(machine == 0x14c and count == 4 and optional == 0, 'Unexpected asin COFF layout')
    strings_at, symbols, index = symbols_at + symbol_count * 18, {}, 0
    while index < symbol_count:
        raw, value, section, kind, storage, aux = struct.unpack_from('<8sIhHBB', body, symbols_at + index * 18)
        if raw[:4] == b'\0' * 4:
            start = strings_at + struct.unpack_from('<I', raw, 4)[0]
            name = body[start:body.index(b'\0', start)].decode('ascii')
        else:
            name = raw.rstrip(b'\0').decode('ascii')
        symbols[index] = {'name': name, 'value': value, 'section': section, 'kind': kind,
                          'storage': storage, 'aux': aux,
                          'size': struct.unpack_from('<I', body, symbols_at + (index + 1) * 18 + 4)[0]
                          if aux else None}
        index += 1 + aux
    require([s for s in symbols.values() if s['name'] == '__CIasin'] ==
            [{'name': '__CIasin', 'value': 0, 'section': 1, 'kind': 32, 'storage': 2, 'aux': 1, 'size': 203}],
            'Vendor asin identity/container extent differs')
    require([s for s in symbols.values() if s['name'] == '_asin'] ==
            [{'name': '_asin', 'value': 20, 'section': 1, 'kind': 0, 'storage': 2, 'aux': 0, 'size': None}],
            'Vendor alternate asin entry differs')
    section = struct.unpack_from('<8sIIIIIIHHI', body, 20)
    require(section[0].rstrip(b'\0') == b'.text' and section[3] >= 203, 'Vendor asin code section differs')
    code = bytearray(body[section[4]:section[4] + 203])
    relocations = []
    for i in range(section[7]):
        offset, target, kind = struct.unpack_from('<IIH', body, section[5] + i * 10)
        if offset < 203:
            require(offset + 4 <= 203 and code[offset:offset + 4] == bytes(4), 'Unexpected asin vendor addend')
            relocations.append([offset, symbols[target]['name'], kind])
    require(relocations == VENDOR['relocations'] and
            hashlib.sha256(code).hexdigest() == VENDOR['normalized_sha256'], 'Vendor asin code/relocations differ')
    return VENDOR


def verify_asin_original(original, function: dict) -> None:
    """Keep both entries intact and compare only actual vendor relocation fields."""
    proof = function['corroboration']['runtime_asin']
    require(function['canonical_identifier'] == '__CIasin' and function['size'] == 20 and
            function['source'] == 'Runtime/MSVC/asin.obj' and
            function['source_comparison_available'] is False and proof['vendor'] == VENDOR,
            'Unsupported asin runtime record')
    context = proof['vendor_container']
    original.check_hash(context)
    require(context['address'] == function['address'] and context['size'] == 203 and
            proof['alternate_entry'] == function['address'] + 20 and
            original.section(context['address'], 203)['name'] == '.text', 'Asin container/secondary entry differs')
    raw = bytearray(original.read(context['address'], 203))
    targets = {}
    for offset, name, kind in VENDOR['relocations']:
        value = struct.unpack_from('<I', raw, offset)[0]
        destination = ((context['address'] + offset + 4 + struct.unpack_from('<i', raw, offset)[0]) & 0xffffffff
                       if kind == 20 else value)
        if name in targets:
            require(targets[name] == destination, 'Asin repeated vendor symbol has inconsistent targets')
        targets[name] = destination
        if kind == 20:
            require(original.section(destination, 1)['name'] == '.text', 'Asin runtime transfer leaves original text')
        else:
            owners = [s for s in original.metadata['sections'] if s['virtual_address'] <= destination and
                      destination + 4 <= s['virtual_address'] + s['virtual_size']]
            require(len(owners) == 1 and owners[0]['name'] in ('.rdata', '.data'), 'Asin data operand has no unique owner')
        raw[offset:offset + 4] = bytes(4)
    require(hashlib.sha256(raw).hexdigest() == VENDOR['normalized_sha256'],
            'Original asin differs outside genuine vendor relocation fields')
    # The final NUL of the vendor name occupies original loader zero-fill.
    name_at = targets['_NAME_']
    section = original.section(name_at, 4)
    require(section['name'] == '.data' and original.read(name_at, 4) == b'asin' and
            name_at + 4 == section['virtual_address'] + section['raw_size'] and
            name_at + 5 <= section['virtual_address'] + section['virtual_size'], 'Original asin name/zero terminator differs')
    require(raw[:6] == bytes.fromhex('83ec0cdd1424') and raw[6] == 0xe8 and raw[11] == 0xe8 and
            struct.unpack_from('<i', raw, 12)[0] == 13 and raw[16:20] == bytes.fromhex('83c40cc3'),
            'Original asin wrapper does not return before the alternate stack entry')

