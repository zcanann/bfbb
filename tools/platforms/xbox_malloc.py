"""Pinned CRT provenance for the reviewed Xbox malloc entry, with no source credit.

The two complete named vendor functions are compared only across their real
COFF relocation operands. The original RW default allocator slot corroborates
ownership; neither the delegate nor its callees becomes game-source coverage.
"""
from __future__ import annotations

import hashlib
from pathlib import Path
import struct

from .xbox_runtime import require

VENDOR = {
    'archive_sha256': '780aa4cbe614efeeb72e3bb9538230f5cd37caeb0adc5d70c346b0dda19ba3a5',
    'member': 'build\\intel\\mt_obj\\malloc.obj',
    'member_sha256': '682ee05b92be1eab3545eb7b87bf0fdc7a8d0e669b86f7b84e3e8a113e984abb',
    'functions': [
        {'symbol': '_malloc', 'section': 10, 'size': 18,
         'relocations': [[2, '__newmode', 6], [11, '__nh_malloc', 20]]},
        {'symbol': '__nh_malloc', 'section': 7, 'size': 44,
         'relocations': [[12, '__heap_alloc', 20], [32, '__callnewh', 20]]},
    ],
}
CODE = {
    '_malloc': bytes.fromhex('ff3500000000ff742408e8000000005959c3'),
    '__nh_malloc': bytes.fromhex(
        '837c2404e07722ff742404e80000000085c0597516394424087410ff742404e80000000085c05975de33c0c3'),
}


def verify_malloc_vendor(library: Path) -> dict:
    """Read the actual pinned archive, including its named symbol extents/relocs."""
    archive = library.read_bytes()
    require(hashlib.sha256(archive).hexdigest() == VENDOR['archive_sha256'] and
            archive[:8] == b'!<arch>\n', 'Unexpected malloc vendor archive')
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
            'Missing or changed vendor malloc object')
    body = selected[0]
    machine, sections, _, symbols_at, count, optional, _ = struct.unpack_from('<HHIIIHH', body)
    require(machine == 0x14c and sections >= 10 and optional == 0, 'Unexpected vendor COFF layout')
    strings_at, symbols, index = symbols_at + count * 18, {}, 0
    while index < count:
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
    for spec in VENDOR['functions']:
        matches = [s for s in symbols.values() if s['name'] == spec['symbol']]
        require(matches == [{'name': spec['symbol'], 'value': 0, 'section': spec['section'],
                             'kind': 0x20, 'storage': 2, 'aux': 1, 'size': spec['size']}],
                'Vendor malloc function identity/extent differs')
        section = struct.unpack_from('<8sIIIIIIHHI', body, 20 + (spec['section'] - 1) * 40)
        require(section[0].rstrip(b'\0') == b'.text' and
                body[section[4]:section[4] + spec['size']] == CODE[spec['symbol']],
                'Vendor malloc instructions differ')
        relocations = []
        for i in range(section[7]):
            offset, target, kind = struct.unpack_from('<IIH', body, section[5] + i * 10)
            if offset < spec['size']:
                relocations.append([offset, symbols[target]['name'], kind])
        require(relocations == spec['relocations'], 'Vendor malloc relocations differ')
    return VENDOR


def verify_malloc_original(original, function: dict) -> None:
    """Check complete original bodies, call relationship and default RW slot."""
    proof = function['corroboration']['runtime_malloc']
    require(function['canonical_identifier'] == 'malloc(size_t)' and function['size'] == 18 and
            function['source'] == 'Runtime/MSVC/malloc.obj' and
            function['source_comparison_available'] is False and proof['vendor'] == VENDOR,
            'Unsupported runtime malloc record')
    delegate, registration = proof['delegate'], proof['rw_registration']
    original.check_hash(delegate)
    original.check_hash(registration)
    require(delegate['size'] == 44 and registration['size'] == 105 and
            original.section(delegate['address'], delegate['size'])['name'] == '.text' and
            original.section(registration['address'], registration['size'])['name'] == '.text',
            'Malloc delegate or RW registration ownership differs')
    for spec, address in zip(VENDOR['functions'], (function['address'], delegate['address'])):
        raw = bytearray(original.read(address, spec['size']))
        for offset, _, kind in spec['relocations']:
            if kind == 20:
                require(raw[offset - 1] == 0xe8, 'Malloc vendor CALL boundary differs')
                destination = address + offset + 4 + struct.unpack_from('<i', raw, offset)[0]
                require(original.section(destination, 1)['name'] == '.text', 'Malloc callee leaves text')
                if spec['symbol'] == '_malloc':
                    require(destination == delegate['address'], 'Original malloc does not call its named delegate')
            else:
                require(spec['symbol'] == '_malloc' and offset == 2 and kind == 6 and
                        raw[:2] == b'\xff\x35', 'Unsupported malloc data operand')
                data_address = struct.unpack_from('<I', raw, offset)[0]
                owners = [section for section in original.metadata['sections'] if
                          section['virtual_address'] <= data_address and data_address + 4 <=
                          section['virtual_address'] + section['virtual_size']]
                require(len(owners) == 1 and owners[0]['name'] == '.data',
                        'Original malloc newmode operand has no unique data owner')
            raw[offset:offset + 4] = b'\0' * 4
        require(bytes(raw) == CODE[spec['symbol']], 'Original malloc differs outside genuine vendor relocations')
    instruction = original.read(registration['address'] + 59, 10)
    require(instruction[:2] == b'\xc7\x05' and
            struct.unpack_from('<I', instruction, 2)[0] == proof['rw_malloc_slot'] and
            struct.unpack_from('<I', instruction, 6)[0] == function['address'],
            'RW default allocator slot does not install the named malloc entry')
