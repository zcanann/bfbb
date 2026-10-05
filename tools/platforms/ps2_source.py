"""Compile genuine shared-source PS2 units and restore proven target call relocs."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]
PROFILE_PATH = ROOT / 'config/platforms/ps2-toolchain.json'


def load_profile() -> dict:
    return json.loads(PROFILE_PATH.read_text(encoding='utf-8'))


def prepare_functions(functions: list[dict], binary: bytes, segments: list[dict],
                      executable_sha1: str, reviewed_targets: Path | None = None) -> list[dict]:
    """Restore only named, independently validated JAL relocations in known units.

    No source object is consulted when reconstructing the target. Reapplying each
    recovered absolute destination must reproduce the retail instruction exactly.
    """
    profile = load_profile()
    from .ps2_report import _source_name
    callees = {}
    for function in functions:
        key = (_source_name(function['source']), function['name'])
        callees.setdefault(key, set()).add(function['low'])
    if reviewed_targets is not None:
        document = json.loads(Path(reviewed_targets).read_text(encoding='utf-8'))
        if document['executable_sha1'] != executable_sha1:
            raise ValueError('Reviewed call targets identify another executable')
        for anchor in document['anchors']:
            if not anchor.get('identity_confirmation'):
                raise ValueError('Unconfirmed call target cannot supply a relocation')
            address, size = anchor['address'], anchor['size']
            spans = [s for s in segments if s['address'] <= address and address + size <= s['address'] + s['file_size']]
            if size <= 0 or len(spans) != 1:
                raise ValueError('Call-target extent is not file-backed')
            offset = spans[0]['offset'] + address - spans[0]['address']
            if hashlib.sha256(binary[offset:offset + size]).hexdigest() != anchor['sha256']:
                raise ValueError('Reviewed call-target bytes differ')
            callees.setdefault((anchor['source'], anchor['name']), set()).add(address)
    restored = []
    for unit in profile['units']:
        members = [f for f in functions if _source_name(f['source']) == unit['target_unit']]
        if not members:
            continue
        by_name = {f['name']: f for f in members}
        if len(by_name) != len(members) or set(by_name) != set(unit['symbols']):
            raise ValueError('Validated source profile does not cover the target unit exactly')
        for name, linkage in unit['symbols'].items():
            by_name[name]['linkage_name'] = linkage
        for call in unit['calls']:
            function = by_name[call['function']]
            key = (call['target_source'], call['target_function'])
            if key not in callees or len(callees[key]) != 1:
                raise ValueError(f'No proven call-target identity for {key}')
            destination = next(iter(callees[key]))
            offset = call['offset']
            code = bytearray(function['bytes'])
            if offset < 0 or offset % 4 or offset + 4 > len(code):
                raise ValueError('Call relocation exceeds its function')
            original = struct.unpack_from('<I', code, offset)[0]
            address = function['low'] + offset
            decoded = ((address + 4) & 0xf0000000) | ((original & 0x03ffffff) << 2)
            if original >> 26 != 3 or decoded != destination:
                raise ValueError('Retail JAL does not call the independently identified target')
            opcode = original & 0xfc000000
            if opcode | ((destination >> 2) & 0x03ffffff) != original:
                raise ValueError('Restored relocation fails inverse reconstruction')
            struct.pack_into('<I', code, offset, opcode)
            function['bytes'] = bytes(code)
            function.setdefault('relocations', []).append({'offset':offset, 'symbol':call['symbol']})
            restored.append({'function':function['name'], 'address':address,
                             'type':'R_MIPS_26', 'symbol':call['symbol'], 'target_address':destination,
                             'original_instruction':original, 'inverse_reconstruction_verified':True})
    return restored


def compile_units(output: Path, compilers: Path, wibo: Path) -> list[dict]:
    profile = load_profile()
    compiler = compilers / profile['compiler']['id'] / 'mwccps2.exe'
    for path, expected in ((compiler, profile['compiler']['compiler_sha256']),
                           (wibo, profile['runtime']['sha256'])):
        if hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            raise ValueError(f'Unexpected PS2 toolchain binary: {path.name}')
    config_path = output / 'objdiff.json'
    if not config_path.is_file():
        return []
    config = json.loads(config_path.read_text(encoding='utf-8'))
    available = {unit['name']:unit for unit in config['units']}
    results = []
    for unit in profile['units']:
        if unit['target_unit'] not in available:
            continue
        target = output / unit['object']
        target.parent.mkdir(parents=True, exist_ok=True)
        target.unlink(missing_ok=True)
        command = [str(wibo.resolve()), '-C', str(ROOT), str(compiler.resolve()), *profile['flags']]
        for include in ('include', 'src/SB/Core/p2', 'src/SB/Core/x'):
            command.extend(['-i', str(ROOT / include)])
        command.extend(['-o', str(target.resolve()), str(ROOT / unit['source'])])
        subprocess.run(command, cwd=ROOT, check=True)
        record = available[unit['target_unit']]
        record['base_path'] = unit['object']
        record['metadata']['source_path'] = unit['source']
        # Exact function comparison is not proof of whole-executable linking.
        record['metadata']['complete'] = False
        results.append({'source':unit['source'], 'compiler_profile':profile['compiler']['id'],
                        'object_sha256':hashlib.sha256(target.read_bytes()).hexdigest(),
                        'source_link_verified':False})
    config_path.write_text(json.dumps(config, indent=2) + '\n', encoding='utf-8')
    return results
