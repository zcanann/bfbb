"""Extract original Xbox executables without modifying their bytes.

XDVDFS layout: https://github.com/XboxDev/extract-xiso
XBE layout: https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/master/src/common/xbe/Xbe.h

Only executable metadata and hashes are returned. Certificate keys and binary
payloads are deliberately omitted. No external tools or packages are required.
"""

from __future__ import annotations

import hashlib
import os
from pathlib import Path
import struct
import tempfile
from typing import BinaryIO


_SECTOR_SIZE = 2048
_MEDIA_MAGIC = b"MICROSOFT*XBOX*MEDIA"
# Bare XISO, original Xbox XGD1, and the other offsets supported by extract-xiso.
_PARTITION_OFFSETS = (0, 0x18300000, 0x0FD90000, 0x02080000)


def _read_at(stream: BinaryIO, offset: int, size: int, file_size: int) -> bytes:
    if offset < 0 or size < 0 or offset + size > file_size:
        raise ValueError(f"Read outside image bounds: offset={offset}, size={size}")
    stream.seek(offset)
    data = stream.read(size)
    if len(data) != size:
        raise ValueError("Image was truncated while reading")
    return data


def _find_executable(stream: BinaryIO, iso_size: int) -> dict:
    partition = None
    for candidate in _PARTITION_OFFSETS:
        offset = candidate + 0x10000
        if offset + _SECTOR_SIZE > iso_size:
            continue
        descriptor = _read_at(stream, offset, _SECTOR_SIZE, iso_size)
        if descriptor[:20] == _MEDIA_MAGIC and descriptor[-20:] == _MEDIA_MAGIC:
            partition = candidate
            break
    if partition is None:
        raise ValueError("No supported XDVDFS volume descriptor found")

    root_sector, root_size = struct.unpack_from("<II", descriptor, 20)
    root_offset = partition + root_sector * _SECTOR_SIZE
    if root_size < 14 or root_offset + root_size > iso_size:
        raise ValueError("Invalid XDVDFS root directory bounds")

    # XDVDFS directories are binary trees. Child offsets are four-byte units,
    # relative to the directory start. The Xbox boot executable is in the root.
    pending = [0]
    visited = set()
    matches = []
    while pending:
        offset = pending.pop()
        if offset in visited:
            raise ValueError("Repeated node in XDVDFS root directory")
        visited.add(offset)
        if offset + 14 > root_size:
            raise ValueError("XDVDFS directory entry exceeds directory bounds")
        entry = _read_at(stream, root_offset + offset, 14, iso_size)
        left, right, sector, size, attributes, name_size = struct.unpack("<HHIIBB", entry)
        if name_size == 0 or offset + 14 + name_size > root_size:
            raise ValueError("Invalid XDVDFS directory entry name")
        name = _read_at(stream, root_offset + offset + 14, name_size, iso_size)
        for child in (left, right):
            if child:
                pending.append(child * 4)
        if name.lower() == b"default.xbe":
            if attributes & 0x10:
                raise ValueError("default.xbe is a directory")
            absolute_offset = partition + sector * _SECTOR_SIZE
            if size < 0x178 or absolute_offset + size > iso_size:
                raise ValueError("Invalid default.xbe file bounds")
            matches.append({
                "filesystem_path": name.decode("ascii"),
                "file_sector": sector,
                "file_offset": absolute_offset,
                "file_size": size,
            })
    if len(matches) != 1:
        raise ValueError(f"Expected one root default.xbe, found {len(matches)}")
    return {"iso_size": iso_size, "partition_offset": partition, **matches[0]}


def inspect_xbe(path: Path) -> dict:
    """Read XBE section, library, certificate metadata and whole-file hashes."""
    path = Path(path)
    data = path.read_bytes()
    if len(data) < 0x178 or data[:4] != b"XBEH":
        raise ValueError("Not a complete XBE executable")

    def unpack(fmt: str, offset: int):
        if offset < 0 or offset + struct.calcsize(fmt) > len(data):
            raise ValueError("XBE structure exceeds file bounds")
        return struct.unpack_from(fmt, data, offset)

    def u32(offset: int) -> int:
        return unpack("<I", offset)[0]

    base = u32(0x104)
    header_size = u32(0x108)
    if header_size < 0x178 or header_size > len(data):
        raise ValueError("Invalid XBE header size")

    def header_offset(address: int, size: int = 1) -> int:
        offset = address - base
        if offset < 0 or offset + size > header_size:
            raise ValueError("XBE header pointer exceeds header bounds")
        return offset

    def header_string(address: int) -> str | None:
        if not address:
            return None
        offset = header_offset(address)
        end = data.find(b"\0", offset, header_size)
        if end < 0:
            raise ValueError("Unterminated XBE header string")
        return data[offset:end].decode("ascii", errors="replace")

    sections = []
    section_count = u32(0x11C)
    section_table = header_offset(u32(0x120), section_count * 56)
    for index in range(section_count):
        fields = unpack("<9I20s", section_table + index * 56)
        flags, address, virtual_size, raw_offset, raw_size, name_address = fields[:6]
        if raw_offset + raw_size > len(data) or address + virtual_size > 0x100000000:
            raise ValueError(f"Invalid XBE section bounds at index {index}")
        sections.append({
            "name": header_string(name_address),
            "flags": flags,
            "virtual_address": address,
            "virtual_size": virtual_size,
            "raw_offset": raw_offset,
            "raw_size": raw_size,
            "sha1": hashlib.sha1(data[raw_offset:raw_offset + raw_size]).hexdigest(),
        })

    libraries = []
    library_count = u32(0x160)
    if library_count:
        library_table = header_offset(u32(0x164), library_count * 16)
        for index in range(library_count):
            name, major, minor, build, flags = unpack("<8s4H", library_table + index * 16)
            libraries.append({
                "name": name.rstrip(b"\0").decode("ascii", errors="replace"),
                "major": major,
                "minor": minor,
                "build": build,
                "qfe": flags & 0x1FFF,
                "approval": (flags >> 13) & 3,
                "debug": bool(flags & 0x8000),
            })

    certificate = None
    if u32(0x118):
        cert = header_offset(u32(0x118), 0xB0)
        cert_size = u32(cert)
        if cert_size < 0xB0:
            raise ValueError("XBE certificate is too small")
        header_offset(u32(0x118), cert_size)
        certificate = {
            "size": cert_size,
            "timestamp": u32(cert + 4),
            "title_id": u32(cert + 8),
            "title_name": data[cert + 12:cert + 92].decode("utf-16le").rstrip("\0"),
            "allowed_media": u32(cert + 0x9C),
            "game_region": u32(cert + 0xA0),
            "game_ratings": u32(cert + 0xA4),
            "disc_number": u32(cert + 0xA8),
            "version": u32(cert + 0xAC),
        }

    encoded_entry = u32(0x128)
    candidates = []
    for kind, key in (("retail", 0xA8FC57AB), ("debug", 0x94859D4B)):
        address = encoded_entry ^ key
        if any(section["flags"] & 4 and
               section["virtual_address"] <= address <
               section["virtual_address"] + section["virtual_size"]
               for section in sections):
            candidates.append((kind, address))
    image_type, entry_point = candidates[0] if len(candidates) == 1 else ("unknown", None)

    return {
        "format": "XBE",
        "architecture": "i386",
        "size": len(data),
        "sha1": hashlib.sha1(data).hexdigest(),
        "sha256": hashlib.sha256(data).hexdigest(),
        "base_address": base,
        "header_size": header_size,
        "image_size": u32(0x10C),
        "image_type": image_type,
        "entry_point": entry_point,
        "timestamp": u32(0x114),
        "pe_timestamp": u32(0x148),
        "debug_path": header_string(u32(0x14C)),
        "debug_filename": header_string(u32(0x150)),
        "certificate": certificate,
        "sections": sections,
        "libraries": libraries,
    }


def extract_executable(iso_path: Path, output_path: Path) -> dict:
    """Extract the root default.xbe atomically, preserving every original byte.

    The ISO is opened read-only. File location is obtained from its XDVDFS
    directory, never from a title-specific sector constant. Existing output is
    replaced only after the complete extracted executable passes inspection.
    """
    iso_path, output_path = Path(iso_path), Path(output_path)
    if iso_path.resolve() == output_path.resolve() or (
        output_path.exists() and os.path.samefile(iso_path, output_path)
    ):
        raise ValueError("Executable output must not overwrite the source ISO")

    temporary_path = None
    try:
        with iso_path.open("rb") as source:
            metadata = _find_executable(source, os.fstat(source.fileno()).st_size)
            output_path.parent.mkdir(parents=True, exist_ok=True)
            with tempfile.NamedTemporaryFile(
                mode="wb", prefix=f".{output_path.name}.", suffix=".tmp",
                dir=output_path.parent, delete=False,
            ) as destination:
                temporary_path = Path(destination.name)
                source.seek(metadata["file_offset"])
                remaining = metadata["file_size"]
                while remaining:
                    chunk = source.read(min(1024 * 1024, remaining))
                    if not chunk:
                        raise ValueError("ISO was truncated while extracting default.xbe")
                    destination.write(chunk)
                    remaining -= len(chunk)
        executable = inspect_xbe(temporary_path)
        os.replace(temporary_path, output_path)
        temporary_path = None
        return {**metadata, "executable": executable}
    finally:
        if temporary_path is not None:
            temporary_path.unlink(missing_ok=True)