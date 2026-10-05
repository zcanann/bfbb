"""Decode the DWARF1 records used by the PS2 retail debug sections.

Attribute/form encodings follow https://dwarfstd.org/doc/dwarf_1_1_0.pdf.
This reads metadata only; interpretation and ownership checks belong to callers.
"""

import struct

def iter_dies(debug: bytes):
    """Read the DWARF1 forms present in MW PS2 debug info using the stdlib."""
    offset = 0
    owner = None
    while offset < len(debug):
        if len(debug) - offset < 4:
            raise ValueError("Truncated DWARF1 record length")
        size = struct.unpack_from("<I", debug, offset)[0]
        if size < 4 or size > len(debug) - offset:
            raise ValueError("Invalid DWARF1 record length")
        end = offset + size
        if size < 8:
            offset = end
            continue
        tag = struct.unpack_from("<H", debug, offset + 4)[0]
        pos = offset + 6
        attrs = {}
        if tag == 0:
            offset = end
            continue
        while pos < end:
            if end - pos < 2:
                raise ValueError("Truncated DWARF1 attribute")
            attr = struct.unpack_from("<H", debug, pos)[0]
            pos += 2
            form, key = attr & 15, attr >> 4
            if form in (1, 2, 5, 6, 7):
                width = {1: 4, 2: 4, 5: 2, 6: 4, 7: 8}[form]
                if width > end - pos:
                    raise ValueError("Truncated DWARF1 scalar")
                value = int.from_bytes(debug[pos:pos + width], "little")
                pos += width
            elif form in (3, 4):
                width = 2 if form == 3 else 4
                if width > end - pos:
                    raise ValueError("Truncated DWARF1 block length")
                length = int.from_bytes(debug[pos:pos + width], "little")
                pos += width
                if length > end - pos:
                    raise ValueError("DWARF1 block exceeds record")
                value = bytes(debug[pos:pos + length])
                pos += length
            elif form == 8:
                terminator = debug.find(b"\0", pos, end)
                if terminator == -1:
                    raise ValueError("Unterminated DWARF1 string")
                value = debug[pos:terminator].decode("utf-8", errors="replace")
                pos = terminator + 1
            else:
                raise ValueError(f"Unsupported DWARF1 form {form}")
            attrs[key] = value
        if tag == 0x11:
            owner = attrs.get(3)
        yield offset, tag, owner, attrs
        offset = end
