#!/usr/bin/env python3
"""Append zero padding to a validated DOL without changing its loaded sections.

Usage: python tools/pad_dol.py build/GU4Y78/main.dol --alignment 128

The GU4Y78 compilation stores BFBB's DOL with 128-byte file alignment.
This only extends the file, preserving its header, sections, and existing zero
trailer. Already aligned files are unchanged. Malformed/truncated section
ranges or unexplained nonzero trailing data are rejected before writing.
"""

import argparse
import struct
from pathlib import Path


def pad_dol(path: Path, alignment: int) -> int:
    """Validate and append padding in place; return the number of bytes added."""
    if alignment <= 0:
        raise ValueError("alignment must be positive")
    with path.open("r+b") as dol:
        data = dol.read()
        if len(data) < 0x100:
            raise ValueError("DOL header is shorter than 0x100 bytes")
        header = struct.unpack_from(">64I", data)
        sections = []
        entry_is_text = False
        for index in range(18):
            offset, address, size = header[index], header[18 + index], header[36 + index]
            if not size:
                continue
            if offset < 0x100 or offset + size > len(data):
                raise ValueError(f"section {index} file range is outside the DOL payload")
            if address + size > 0x100000000:
                raise ValueError(f"section {index} address range overflows")
            sections.append((offset, offset + size))
            if index < 7 and address <= header[56] < address + size:
                entry_is_text = True
        if not sections or not entry_is_text:
            raise ValueError("DOL entry point is not within a text section")
        sections.sort()
        if any(left[1] > right[0] for left, right in zip(sections, sections[1:])):
            raise ValueError("DOL file sections overlap")
        if any(data[sections[-1][1]:]):
            raise ValueError("DOL has unexplained nonzero trailing bytes")
        padding = (-len(data)) % alignment
        if padding:
            dol.write(bytes(padding))
        return padding


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", type=Path, help="DOL file to pad in place")
    parser.add_argument("--alignment", type=int, required=True, help="positive file alignment")
    args = parser.parse_args()
    try:
        pad_dol(args.path, args.alignment)
    except (OSError, ValueError) as error:
        parser.error(str(error))


if __name__ == "__main__":
    main()
