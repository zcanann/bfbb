"""Write i386 COFF containers for independently bounded functions.

Callers own source identity, byte provenance and relocation recovery. Optional
DIR32/REL32 records carry named symbols and checked in-place addends; the
encoder does not infer relocations or modify caller-supplied payload bytes.
"""
import struct


def function_object(functions: list[dict]) -> bytes:
    if not functions or len(functions) > 32767:
        raise ValueError('COFF needs a nonempty supported function list')
    strings = bytearray(4)
    headers, payloads, symbols = [], [], []
    names = set()

    def encode(name: str) -> bytes:
        encoded = name.encode('utf-8')
        if not encoded or b'\0' in encoded:
            raise ValueError('Empty or NUL-containing COFF symbol')
        if len(encoded) <= 8:
            return encoded.ljust(8, b'\0')
        result = struct.pack('<II', 0, len(strings))
        strings.extend(encoded + b'\0')
        return result

    for index, function in enumerate(functions, 1):
        name, data = function['symbol'], function['bytes']
        if name in names or not data:
            raise ValueError('Empty or duplicate COFF function')
        names.add(name)
        symbols.append(struct.pack('<8sIhHBB', encode(name), 0, index, 0x20, 2, 1))
        symbols.append(struct.pack('<IIIIH', 0, len(data), 0, 0, 0))
    function_symbols = {f['symbol']: 2 * i for i, f in enumerate(functions)}
    external_names = sorted({r['symbol'] for f in functions for r in f.get('relocations', [])} - names)
    externals = {name: len(symbols) + i for i, name in enumerate(external_names)}
    for name in external_names:
        symbols.append(struct.pack('<8sIhHBB', encode(name), 0, 0, 0, 2, 0))
    offset = 20 + 40 * len(functions)
    for function in functions:
        data = function['bytes']
        relocations = sorted(function.get('relocations', []), key=lambda r: r['offset'])
        if len(relocations) >= 65535:
            raise ValueError('Too many COFF relocations')
        previous = -4
        encoded_relocations = []
        for relocation in relocations:
            at = relocation['offset']
            kind = relocation.get('type', 6)
            symbol = relocation['symbol']
            function_address = relocation.get('target_kind') == 'function'
            if (kind not in (6, 20) or relocation.get('target_kind') not in (None, 'function') or
                    function_address and (kind != 6 or relocation['addend'] != 0) or
                    kind == 6 and symbol in function_symbols and not function_address):
                raise ValueError('Unsupported COFF relocation or address/function conflict')
            if (not isinstance(at, int) or at < previous + 4 or at + 4 > len(data) or
                    struct.unpack_from('<I', data, at)[0] != relocation['addend']):
                raise ValueError('Invalid, overlapping or mismatched COFF addend')
            previous = at
            encoded_relocations.append(struct.pack('<IIH', at, function_symbols[symbol] if symbol in function_symbols else externals[symbol], kind))
        records = b''.join(encoded_relocations)
        headers.append(struct.pack('<8sIIIIIIHHI', b'.text', 0, 0, len(data), offset,
                                   offset + len(data) if records else 0, 0, len(relocations), 0, 0x60100020))
        payloads.extend((data, records))
        offset += len(data) + len(records)
    struct.pack_into('<I', strings, 0, len(strings))
    header = struct.pack('<HHIIIHH', 0x14c, len(functions), 0, offset, len(symbols), 0, 0)
    return header + b''.join(headers) + b''.join(payloads) + b''.join(symbols) + bytes(strings)
