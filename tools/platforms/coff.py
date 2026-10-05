"""Write i386 COFF function containers for independently bounded code.

The payloads are untouched; this is not a relocatable reconstruction. Explicit
function auxiliary sizes follow Microsoft's PE/COFF specification. Callers own
source identity, byte provenance, and any necessary relocation recovery.
"""
import struct


def function_object(functions: list[dict]) -> bytes:
    if not functions or len(functions) > 32767:
        raise ValueError('COFF needs a nonempty supported function list')
    strings = bytearray(4)
    headers, payloads, symbols = [], [], []
    offset = 20 + 40 * len(functions)
    names = set()
    for index, function in enumerate(functions, 1):
        name = function['symbol']
        data = function['bytes']
        if not name or name in names or not data:
            raise ValueError('Empty or duplicate COFF function')
        names.add(name)
        encoded = name.encode('utf-8')
        if b'\0' in encoded:
            raise ValueError('NUL in COFF function name')
        if len(encoded) <= 8:
            symbol_name = encoded.ljust(8, b'\0')
        else:
            symbol_name = struct.pack('<II', 0, len(strings))
            strings.extend(encoded + b'\0')
        # One .text contribution per function; byte alignment inserts no code.
        headers.append(struct.pack('<8sIIIIIIHHI', b'.text', 0, 0, len(data),
                                   offset, 0, 0, 0, 0, 0x60100020))
        payloads.append(data)
        offset += len(data)
        symbols.append(struct.pack('<8sIhHBB', symbol_name, 0, index, 0x20, 2, 1))
        symbols.append(struct.pack('<IIIIH', 0, len(data), 0, 0, 0))
    struct.pack_into('<I', strings, 0, len(strings))
    header = struct.pack('<HHIIIHH', 0x14c, len(functions), 0, offset,
                         len(symbols), 0, 0)
    return header + b''.join(headers) + b''.join(payloads) + b''.join(symbols) + bytes(strings)
