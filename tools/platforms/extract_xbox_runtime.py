#!/usr/bin/env python3
"""Extract the pinned Microsoft static CRT from its public signed toolkit archive.

Never executes the installer. The archive hash binds the Microsoft Authenticode
signature checked during review; the extracted library has a second pinned hash.
Requires a full 7-Zip executable supporting MSI/Compound and CAB (7z or 7zz).
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def extract(archive: Path, compilers: Path, sevenzip: str, profile_path: Path) -> Path:
    profile = json.loads(profile_path.read_text(encoding='utf-8'))
    runtime = profile['static_runtime']
    data = archive.read_bytes()
    if hashlib.sha256(data).hexdigest() != runtime['archive_sha256']:
        raise ValueError('Unexpected Microsoft toolkit archive hash')
    member = b'Microsoft Visual C++ Toolkit 2003.msi'
    at = data.find(member + b'\0')
    if at < 0 or data.find(member + b'\0', at + 1) >= 0 or at + 312 > len(data):
        raise ValueError('Missing or ambiguous toolkit MSI directory entry')
    flags, _, size = struct.unpack_from('<III', data, at + 260)
    if flags != 2 or size != 7351296 or at + 312 + size > len(data):
        raise ValueError('Unexpected signed InstallShield member layout')
    # InstallShield filename-key nibble transform, full-member mode (flag 2).
    # Format reference: ISx.c gen_key/decode_byte/extract_encrypted_files, revision
    # 098e866fa5341db4424d3831d40943c01b88aefe of the public ISx extractor.
    magic = (0x13, 0x35, 0x86, 0x07)
    key = bytes(value ^ magic[index % 4] for index, value in enumerate(member))
    encoded = data[at + 312:at + 312 + size]
    decoded = bytes((~(key[index % len(key)] ^ ((value << 4) | (value >> 4)))) & 255
                    for index, value in enumerate(encoded))
    if not decoded.startswith(bytes.fromhex('d0cf11e0a1b11ae1')):
        raise ValueError('Extracted member is not MSI Compound storage')
    spec = runtime['libraries']['libcmt']
    with tempfile.TemporaryDirectory(prefix='bfbb-xbox-crt-') as temp:
        scratch = Path(temp)
        installer = scratch / 'toolkit.msi'
        installer.write_bytes(decoded)
        def unpack(path: Path, member_name: str, output: Path) -> None:
            result = subprocess.run([sevenzip, 'x', str(path), member_name,
                                     '-o' + str(output), '-y'], capture_output=True, text=True)
            if result.returncode:
                raise ValueError('Static runtime extraction failed: ' + result.stdout + result.stderr)
        unpack(installer, 'Data1.cab', scratch / 'msi')
        # The MSI File table maps this exact cabinet identifier to libcmt.lib.
        cabinet_member = '_52534B1E423F4D6BAF1F10C5B396DE4D'
        unpack(scratch / 'msi/Data1.cab', cabinet_member, scratch / 'cab')
        library = scratch / 'cab' / cabinet_member
        if hashlib.sha256(library.read_bytes()).hexdigest() != spec['sha256']:
            raise ValueError('Extracted static CRT library hash differs')
        relative = Path(spec['path'])
        if relative.is_absolute() or '..' in relative.parts:
            raise ValueError('Invalid compiler runtime path')
        destination = compilers / profile['compiler']['id'] / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(library, destination)
    return destination


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive', type=Path)
    parser.add_argument('--compilers', required=True, type=Path)
    parser.add_argument('--sevenzip', default='7z')
    parser.add_argument('--profile', type=Path, default=ROOT / 'config/platforms/xbox-toolchain.json')
    args = parser.parse_args()
    try:
        print(extract(args.archive, args.compilers, args.sevenzip, args.profile))
    except (OSError, ValueError, KeyError, struct.error) as error:
        parser.exit(1, f'error: {error}\n')


if __name__ == '__main__':
    main()
