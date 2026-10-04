"""Extract a PS2 boot ELF from ISO9660 and inspect its original metadata.

This module does not compile, relink, or measure decompilation progress. Original
bytes stay in the caller-selected output; returned metadata contains no payload.
"""

from __future__ import annotations

import hashlib
from pathlib import Path
import re
import struct
from typing import BinaryIO

SECTOR_SIZE = 2048


def _read_at(stream: BinaryIO, offset: int, size: int, limit: int) -> bytes:
    if offset < 0 or size < 0 or offset > limit or size > limit - offset:
        raise ValueError(f"File range outside input: offset={offset}, size={size}")
    stream.seek(offset)
    data = stream.read(size)
    if len(data) != size:
        raise ValueError("Truncated input")
    return data


def _both_endian(data: bytes, offset: int, size: int) -> int:
    little = int.from_bytes(data[offset:offset + size], "little")
    big = int.from_bytes(data[offset + size:offset + 2 * size], "big")
    if little != big:
        raise ValueError("Inconsistent ISO9660 dual-endian field")
    return little


def _record(data: bytes) -> dict:
    if len(data) < 34 or data[0] != len(data) or 33 + data[32] > len(data):
        raise ValueError("Malformed ISO9660 directory record")
    if data[1] or data[26] or data[27] or data[25] & 0x80:
        raise ValueError("Extended, interleaved, or multi-extent ISO entries are unsupported")
    if _both_endian(data, 28, 2) != 1:
        raise ValueError("Multi-volume ISO entries are unsupported")
    return {
        "name": data[33:33 + data[32]].decode("ascii"),
        "extent": _both_endian(data, 2, 4),
        "size": _both_endian(data, 10, 4),
        "directory": bool(data[25] & 2),
    }


def _directory(stream: BinaryIO, record: dict, limit: int) -> list[dict]:
    if not record["directory"]:
        raise ValueError("Expected an ISO9660 directory")
    if record["size"] > 16 * 1024 * 1024:
        raise ValueError("ISO9660 directory exceeds supported size")
    data = _read_at(stream, record["extent"] * SECTOR_SIZE, record["size"], limit)
    result = []
    offset = 0
    while offset < len(data):
        size = data[offset]
        if not size:
            offset = (offset // SECTOR_SIZE + 1) * SECTOR_SIZE
            continue
        if size > SECTOR_SIZE - offset % SECTOR_SIZE or offset + size > len(data):
            raise ValueError("ISO9660 directory record crosses its boundary")
        entry = _record(data[offset:offset + size])
        if entry["name"] not in ("\x00", "\x01"):
            result.append(entry)
        offset += size
    return result


def _lookup(stream: BinaryIO, root: dict, path: str, limit: int) -> dict:
    parts = path.replace("\\", "/").strip("/").split("/")
    if not parts or any(part in ("", ".", "..") for part in parts):
        raise ValueError("Invalid ISO9660 boot path")
    current = root
    for part in parts:
        matches = [entry for entry in _directory(stream, current, limit)
                   if entry["name"].upper() == part.upper() or
                   (";" not in part and entry["name"].split(";", 1)[0].upper() == part.upper())]
        if len(matches) != 1:
            raise ValueError(f"ISO9660 path component is missing or ambiguous: {part}")
        current = matches[0]
    if current["directory"]:
        raise ValueError("Boot path identifies a directory")
    return current


def _cstring(data: bytes, offset: int) -> str:
    if offset < 0 or offset >= len(data):
        raise ValueError("ELF string offset outside string table")
    end = data.find(b"\0", offset)
    if end == -1:
        raise ValueError("Unterminated ELF string")
    return data[offset:end].decode("utf-8", errors="replace")


def inspect_elf(path: Path) -> dict:
    """Return hashes, load/section layout, compiler strings, and debug presence."""
    path = Path(path)
    size = path.stat().st_size
    sha1, sha256 = hashlib.sha1(), hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            sha1.update(chunk)
            sha256.update(chunk)
        header = _read_at(stream, 0, 52, size)
        if header[:7] != b"\x7fELF\x01\x01\x01":
            raise ValueError("Expected a little-endian ELF32 executable")
        fields = struct.unpack("<16sHHIIIIIHHHHHH", header)
        (_, elf_type, machine, version, entry, phoff, shoff, flags,
         ehsize, phentsize, phnum, shentsize, shnum, shstrndx) = fields
        if elf_type != 2 or machine != 8 or version != 1 or ehsize != 52:
            raise ValueError("Expected an executable ELF32 MIPS header")
        if shnum == 0 or shentsize != 40 or shstrndx >= shnum:
            raise ValueError("Missing or unsupported ELF section table")
        raw_sections = _read_at(stream, shoff, shnum * shentsize, size)
        section_headers = list(struct.iter_unpack("<IIIIIIIIII", raw_sections))
        strings = section_headers[shstrndx]
        if strings[1] != 3:
            raise ValueError("ELF section-name table is not STRTAB")
        names = _read_at(stream, strings[4], strings[5], size)
        sections = []
        comments = []
        for index, section in enumerate(section_headers):
            name, kind, attributes, address, offset, length, link, info, align, entsize = section
            name = _cstring(names, name)
            if kind != 8 and (offset > size or length > size - offset):
                raise ValueError(f"ELF section {index} exceeds input")
            sections.append({
                "index": index, "name": name, "type": kind, "flags": attributes,
                "address": address, "offset": offset, "size": length,
                "alignment": align,
            })
            if name == ".comment":
                if length > 1024 * 1024:
                    raise ValueError("ELF compiler comment exceeds supported size")
                comment = _read_at(stream, offset, length, size)
                comments.extend(value.decode("utf-8", errors="replace")
                                for value in comment.split(b"\0") if value)
        segments = []
        if phnum:
            if phentsize != 32 or phnum == 0xffff:
                raise ValueError("Unsupported ELF program-header table")
            raw_programs = _read_at(stream, phoff, phnum * phentsize, size)
            for p in struct.iter_unpack("<IIIIIIII", raw_programs):
                kind, offset, address, physical, filesz, memsz, attributes, align = p
                if offset > size or filesz > size - offset or filesz > memsz:
                    raise ValueError("ELF segment exceeds its file or memory range")
                segments.append({
                    "type": kind, "offset": offset, "address": address,
                    "physical_address": physical, "file_size": filesz,
                    "memory_size": memsz, "flags": attributes, "alignment": align,
                })
    debug_sections = [s["name"] for s in sections if s["size"] and
                      (s["name"] in (".debug", ".line", ".mdebug", ".stab") or
                       s["name"].startswith(".debug_"))]
    return {
        "path": str(path), "size": size, "sha1": sha1.hexdigest(),
        "sha256": sha256.hexdigest(), "format": "elf32-mips-little",
        "entry_point": entry, "flags": flags, "sections": sections,
        "segments": segments, "compiler_comments": comments,
        "has_debug_info": bool(debug_sections), "debug_sections": debug_sections,
    }


def extract_executable(iso_path: Path, output_path: Path) -> dict:
    """Extract SYSTEM.CNF's BOOT2 ELF, without altering or publishing the ISO."""
    iso_path, output_path = Path(iso_path), Path(output_path)
    if iso_path.resolve() == output_path.resolve():
        raise ValueError("Output must not overwrite the original ISO")
    iso_size = iso_path.stat().st_size
    with iso_path.open("rb") as stream:
        pvd = None
        for sector in range(16, 80):
            descriptor = _read_at(stream, sector * SECTOR_SIZE, SECTOR_SIZE, iso_size)
            if descriptor[1:7] != b"CD001\x01":
                raise ValueError("Expected an ISO9660 volume descriptor")
            if descriptor[0] == 1:
                pvd = descriptor
                break
            if descriptor[0] == 255:
                break
        if pvd is None:
            raise ValueError("ISO9660 primary volume descriptor not found")
        if _both_endian(pvd, 128, 2) != SECTOR_SIZE:
            raise ValueError("Only 2048-byte ISO9660 logical blocks are supported")
        limit = _both_endian(pvd, 80, 4) * SECTOR_SIZE
        if limit > iso_size:
            raise ValueError("ISO9660 declared volume exceeds input")
        root = _record(pvd[156:156 + pvd[156]])
        cnf_record = _lookup(stream, root, "SYSTEM.CNF;1", limit)
        if cnf_record["size"] > 65536:
            raise ValueError("SYSTEM.CNF exceeds supported size")
        cnf = _read_at(stream, cnf_record["extent"] * SECTOR_SIZE,
                       cnf_record["size"], limit).decode("ascii")
        boot_lines = re.findall(r"^\s*BOOT2\s*=\s*cdrom0:\\?([^\r\n]+)",
                                cnf, flags=re.IGNORECASE | re.MULTILINE)
        if len(boot_lines) != 1:
            raise ValueError("SYSTEM.CNF must contain exactly one cdrom0 BOOT2 entry")
        boot_path = boot_lines[0].strip().replace("\\", "/").lstrip("/")
        executable = _lookup(stream, root, boot_path, limit)
        # Reject a wrong BOOT2 payload before creating an output file.
        offset = executable["extent"] * SECTOR_SIZE
        if executable["size"] < 52 or _read_at(stream, offset, 7, limit) != b"\x7fELF\x01\x01\x01":
            raise ValueError("BOOT2 does not identify a little-endian ELF32 file")
        if offset > limit or executable["size"] > limit - offset:
            raise ValueError("Boot executable exceeds ISO9660 volume")
        output_path.parent.mkdir(parents=True, exist_ok=True)
        # A sibling temporary permits inspection before replacing an existing output.
        import tempfile
        temporary = None
        try:
            with tempfile.NamedTemporaryFile(dir=output_path.parent, prefix=output_path.name + ".",
                                             suffix=".tmp", delete=False) as output:
                temporary = Path(output.name)
                stream.seek(offset)
                remaining = executable["size"]
                while remaining:
                    chunk = stream.read(min(1024 * 1024, remaining))
                    if not chunk:
                        raise ValueError("Truncated boot executable")
                    output.write(chunk)
                    remaining -= len(chunk)
            metadata = inspect_elf(temporary)
            temporary.replace(output_path)
        finally:
            if temporary is not None and temporary.exists():
                temporary.unlink()
    metadata.update({
        "path": str(output_path), "iso_path": str(iso_path), "iso_size": iso_size,
        "iso_volume_id": pvd[40:72].decode("ascii").strip(),
        "boot_path": boot_path, "system_cnf": cnf,
    })
    return metadata
