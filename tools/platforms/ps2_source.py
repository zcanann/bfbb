"""Compile genuine shared-source PS2 units and restore proven target call relocs."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]
PROFILE_PATH = ROOT / 'config/platforms/ps2-toolchain.json'


def load_profile() -> dict:
    return json.loads(PROFILE_PATH.read_text(encoding='utf-8'))


def version_define(executable_sha1: str) -> str:
    """Select one ordinary version macro from an authenticated original identity."""
    manifest = json.loads((ROOT / 'config/platforms/versions.json').read_text(encoding='utf-8'))
    versions = [version for version, record in manifest['versions'].items()
                if record.get('platform') == 'ps2'
                and record['executable']['sha1'] == executable_sha1]
    if len(versions) != 1 or not re.fullmatch(r'(?:SLUS|SLES)-[0-9]{5}', versions[0]):
        raise ValueError('Expected one known PS2 executable version for compilation')
    return 'VERSION_' + versions[0].replace('-', '_')


def profile_enabled(unit: dict, executable_sha1: str) -> bool:
    """Restrict profiles whose overload/relocation evidence is version-specific."""
    return executable_sha1 in unit.get('executable_sha1s', [executable_sha1])


def canonical_linkages(binary: bytes, metadata: dict) -> dict[int, str]:
    """Read MW's canonical linkage attribute without renaming ordinary targets."""
    from .dwarf1 import iter_dies
    result = {}
    for section in metadata.get('sections', []):
        if section['name'] != '.debug':
            continue
        data = binary[section['offset']:section['offset'] + section['size']]
        for _, tag, _, attrs in iter_dies(data):
            if tag in (6, 20) and attrs.get(18, 0) > attrs.get(17, 0) and 512 in attrs:
                address, linkage = attrs[17], attrs[512]
                if not isinstance(linkage, str) or (address in result and result[address] != linkage):
                    raise ValueError('Conflicting retail DWARF linkage identity')
                result[address] = linkage
    return result


def profile_function_key(unit: dict, function: dict, linkages: dict[int, str]) -> str:
    """Select overloads explicitly; retain human-name keys for existing profiles."""
    address = function.get('low', function.get('address'))
    if unit.get('selector') == 'name_address':
        # Stripped targets can have verified overloaded names but no DWARF linkage.
        # Their explicit profile aliases come from recorded reference provenance.
        if not isinstance(address, int):
            raise ValueError('Address selector requires a verified function address')
        return f"{function['name']}@{address:08x}"
    if unit.get('selector') != 'linkage_name':
        return function['name']
    if address in linkages:
        return linkages[address]
    raise ValueError('Canonical profile requires the original DWARF linkage attribute')


def validate_interleaved_pair(code: bytes, address: int, hi_offset: int, lo_offset: int,
                              register: int, validated_calls: set[int]) -> None:
    """Prove the observed straight-line LUI lifetime, including a call delay slot.

    This is deliberately not a general MIPS instruction decoder. Only the
    instructions present in the reviewed factory/constructor pairs are allowed;
    each has explicit GPR reads and writes. Unknown instructions fail closed.
    """
    if register == 0:
        raise ValueError('Address load cannot discard its result in $zero')
    if hi_offset:
        previous = struct.unpack_from('<I', code, hi_offset - 4)[0]
        opcode, rs = previous >> 26, (previous >> 21) & 31
        if (opcode in (1, 2, 3, 4, 5, 6, 7, 20, 21, 22, 23) or
                opcode == 0 and previous & 63 in (8, 9) or
                opcode in (16, 17, 18) and rs == 8):
            raise ValueError('Address high-half definition cannot be a control-transfer delay slot')
    for offset in range(hi_offset + 4, lo_offset, 4):
        word = struct.unpack_from('<I', code, offset)[0]
        opcode, rs, rt, rd = word >> 26, (word >> 21) & 31, (word >> 16) & 31, (word >> 11) & 31
        if word == 0:  # NOP
            reads, writes = set(), set()
        elif opcode == 15 and rs == 0:  # LUI
            reads, writes = set(), {rt}
        elif opcode in (9, 13):  # ADDIU, ORI
            reads, writes = {rs}, {rt}
        elif opcode == 0 and word & 63 == 45 and (word >> 6) & 31 == 0:  # DADDU
            reads, writes = {rs, rt}, {rd}
        elif opcode == 35:  # LW
            reads, writes = {rs}, {rt}
        elif opcode in (31, 43, 63):  # SQ, SW, SD
            reads, writes = {rs, rt}, set()
        elif opcode == 3 and offset == lo_offset - 4 and offset in validated_calls:
            # JAL writes $ra immediately; its callee executes after LO's delay slot.
            reads, writes = set(), {31}
        else:
            raise ValueError('Unsupported instruction between address halves')
        if register in reads or register in writes:
            raise ValueError('Address high-half register is used or clobbered before LO')
    # No direct edge may bypass the high-half definition. Reject even an edge
    # originating within the interval: those shapes require separate CFG proof.
    for offset in range(0, len(code), 4):
        word = struct.unpack_from('<I', code, offset)[0]
        opcode, rs = word >> 26, (word >> 21) & 31
        destination = None
        if opcode in (2, 3):
            destination = ((address + offset + 4) & 0xf0000000) | ((word & 0x03ffffff) << 2)
        elif (opcode in (1, 4, 5, 6, 7, 20, 21, 22, 23) or
              opcode in (16, 17, 18) and rs == 8):
            immediate = struct.unpack('<h', struct.pack('<H', word & 0xffff))[0]
            destination = address + offset + 4 + immediate * 4
        if destination is not None and address + hi_offset < destination <= address + lo_offset:
            raise ValueError('Control flow enters an address pair after its high-half definition')


def prepare_functions(functions: list[dict], binary: bytes, segments: list[dict],
                      executable_sha1: str, reviewed_targets: Path | None = None, *,
                      metadata: dict | None = None, address_anchors: list[dict] | None = None) -> list[dict]:
    """Restore named, independently validated direct-transfer relocations in known units.

    No source object is consulted when reconstructing the target. Reapplying each
    recovered absolute destination must reproduce the retail instruction exactly.
    """
    profile = load_profile()
    from .ps2_report import _source_name
    linkages = canonical_linkages(binary, metadata or {})
    callees = {}
    for function in functions:
        key = (_source_name(function['source']), function['name'])
        callees.setdefault(key, set()).add(function['low'])
        # Address-qualified aliases distinguish independently verified overloads.
        callees.setdefault((key[0], f"{function['name']}@{function['low']:08x}"), set()).add(function['low'])
        if function['low'] in linkages:
            callees.setdefault((key[0], linkages[function['low']]), set()).add(function['low'])
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
        if not profile_enabled(unit, executable_sha1):
            continue
        members = [f for f in functions if _source_name(f['source']) == unit['target_unit']]
        if not members:
            continue
        by_name = {profile_function_key(unit, f, linkages): f for f in members}
        if len(by_name) != len(members) or set(by_name) != set(unit['symbols']):
            raise ValueError('Validated source profile does not cover the target unit exactly')
        retail_code = {name: bytes(function['bytes']) for name, function in by_name.items()}
        for name, function in by_name.items():
            function['linkage_name'] = unit['symbols'][name]
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
            expected_opcode = call.get('opcode', 3)
            if expected_opcode not in (2, 3):
                raise ValueError('Direct-transfer profile requires J or JAL')
            interior = call.get('interior_offset', 0)
            if 'interior_offset' in call:
                # Handwritten kernels can use J to one of their own labels.
                # The original function identity and complete extent establish
                # the symbol base; the original operand establishes the label.
                # This does not permit aliases for arbitrary interior callees.
                if (type(interior) is not int or interior <= 0 or interior % 4 or
                        interior >= len(retail_code[call['function']]) or
                        function.get('high') != function['low'] + len(retail_code[call['function']]) or
                        expected_opcode != 2 or destination != function['low'] or
                        _source_name(call['target_source']) != unit['target_unit'] or
                        call['symbol'] != unit['symbols'][call['function']]):
                    raise ValueError('Internal J requires an aligned label within its own original function')
                destination += interior
            if original >> 26 != expected_opcode or decoded != destination:
                raise ValueError('Retail direct transfer does not reach the independently identified target')
            opcode = original & 0xfc000000
            if opcode | ((destination >> 2) & 0x03ffffff) != original:
                raise ValueError('Restored relocation fails inverse reconstruction')
            # ELF REL stores the byte addend divided by four in the J field.
            struct.pack_into('<I', code, offset, opcode | (interior >> 2))
            function['bytes'] = bytes(code)
            function.setdefault('relocations', []).append({'offset':offset, 'symbol':call['symbol']})
            restored.append({'function':function['name'], 'address':address,
                             'type':'R_MIPS_26', 'symbol':call['symbol'], 'target_address':destination,
                             'original_instruction':original, 'inverse_reconstruction_verified':True,
                             **({'symbol_address': function['low'], 'addend': interior} if interior else {})})
        for pair in unit.get('address_pairs', []):
            function = by_name[pair['function']]
            if 'target_data' in pair:
                if ('target_source' in pair or 'target_function' in pair or
                        not any(s['name'] == '.debug' and s['size'] for s in (metadata or {}).get('sections', []))):
                    raise ValueError('Data address pairs require original DWARF declarations')
                destinations = {a['address'] for a in (address_anchors or [])
                                if a['kind'] == 'data_address' and a['name'] == pair['target_data']
                                and a.get('references')}
            else:
                key = (pair['target_source'], pair['target_function'])
                destinations = callees.get(key, set())
            if len(destinations) != 1:
                raise ValueError('Address pair lacks one independently named destination')
            destination = next(iter(destinations))
            lo_opcode = pair.get('lo_opcode', 9)
            addend = pair.get('addend', 0)
            if (type(lo_opcode) is not int or lo_opcode not in (9, 49, 57) or
                    type(addend) is not int or addend < 0 or addend > 0x7fff or addend % 4):
                raise ValueError('Unsupported address-pair opcode or component addend')
            if lo_opcode == 9 and addend:
                raise ValueError('ADDIU address pairs require a zero addend')
            if lo_opcode != 9 and 'target_data' not in pair:
                raise ValueError('Floating load/store address pairs require named data')
            symbol_address = destination
            destination += addend
            if lo_opcode != 9 and not any(
                    segment['type'] == 1 and segment['address'] <= destination and
                    destination + 4 <= segment['address'] + segment['memory_size']
                    for segment in (metadata or {}).get('segments', [])):
                raise ValueError('Data component address is outside original memory')
            hi_offset, lo_offset = pair['hi_offset'], pair['lo_offset']
            code = bytearray(function['bytes'])
            if any(offset < 0 or offset % 4 or offset + 4 > len(code)
                   for offset in (hi_offset, lo_offset)) or hi_offset >= lo_offset:
                raise ValueError('Address pair exceeds its function')
            hi = struct.unpack_from('<I', code, hi_offset)[0]
            lo = struct.unpack_from('<I', code, lo_offset)[0]
            register = (hi >> 16) & 31
            signed_low = struct.unpack('<h', struct.pack('<H', lo & 0xffff))[0]
            if (hi >> 26 != 15 or (hi >> 21) & 31 or register == 0 or lo >> 26 != lo_opcode or
                    (lo >> 21) & 31 != register or
                    (lo_opcode == 9 and (lo >> 16) & 31 != register) or
                    (((hi & 0xffff) << 16) + signed_low) & 0xffffffff != destination):
                raise ValueError('Retail address pair does not load the named destination')
            if lo_opcode != 9 and lo_offset != hi_offset + 4:
                raise ValueError('Floating load/store address pairs must be adjacent')
            if lo_offset != hi_offset + 4 or lo_opcode != 9:
                if lo_offset != hi_offset + 4 and pair.get('interleaved') is not True:
                    raise ValueError('Nonadjacent address pairs require additional data-flow proof')
                validated_calls = {call['offset'] for call in unit['calls']
                                   if call['function'] == pair['function']}
                validate_interleaved_pair(retail_code[pair['function']], function['low'], hi_offset, lo_offset,
                                          register, validated_calls)
            for offset, word, kind, immediate in (
                    (hi_offset, hi, 5, ((destination + 0x8000) >> 16) & 0xffff),
                    (lo_offset, lo, 6, destination & 0xffff)):
                normalized = word & 0xffff0000
                if normalized | immediate != word:
                    raise ValueError('Address relocation fails inverse reconstruction')
                # REL addends live in the instruction fields. Preserve the
                # component displacement while removing the linked symbol base.
                implicit_addend = ((addend + 0x8000) >> 16) if kind == 5 else addend
                struct.pack_into('<I', code, offset, normalized | (implicit_addend & 0xffff))
                function.setdefault('relocations', []).append(
                    {'offset': offset, 'symbol': pair['symbol'], 'type': kind})
                restored.append({'function': function['name'], 'address': function['low'] + offset,
                                 'type': 'R_MIPS_HI16' if kind == 5 else 'R_MIPS_LO16',
                                 'symbol': pair['symbol'], 'target_address': destination,
                                 'original_instruction': word, 'inverse_reconstruction_verified': True,
                                 **({'symbol_address': symbol_address, 'addend': addend} if addend else {})})
            function['bytes'] = bytes(code)
        for relocation in unit.get('gp_relocations', []):
            # Retail .reginfo establishes GP; DWARF independently establishes
            # the data address. No source-object field is used to infer either.
            reginfo = [s for s in (metadata or {}).get('sections', [])
                       if s['name'] == '.reginfo' and s['size'] == 24]
            if len(reginfo) != 1:
                raise ValueError('GP relocation requires one ELF .reginfo record')
            gp = struct.unpack_from('<I', binary, reginfo[0]['offset'] + 20)[0]
            owner = relocation.get('target_source')
            data_linkage = relocation.get('target_linkage')
            if data_linkage is not None and (not isinstance(data_linkage, str) or not data_linkage):
                raise ValueError('GP data linkage must be a nonempty original identifier')
            addresses = {a['address'] for a in (address_anchors or [])
                         if a['kind'] == 'data_address' and a['name'] == relocation['target_name']
                         and (data_linkage is None or a.get('linkage_name') == data_linkage)
                         and (owner is None or any(_source_name(reference['source']) == owner
                              for reference in a.get('references', [])))}
            if len(addresses) != 1:
                raise ValueError('GP relocation lacks one independently named data address')
            destination = next(iter(addresses))
            function = by_name[relocation['function']]
            offset = relocation['offset']
            code = bytearray(function['bytes'])
            if offset < 0 or offset % 4 or offset + 4 > len(code):
                raise ValueError('GP relocation exceeds its function')
            original = struct.unpack_from('<I', code, offset)[0]
            immediate = struct.unpack('<h', struct.pack('<H', original & 0xffff))[0]
            if (original >> 26 != relocation['opcode'] or (original >> 21) & 31 != 28 or
                    gp + immediate != destination):
                raise ValueError('Retail GP access does not reference the independently named data')
            if not -32768 <= destination - gp < 32768:
                raise ValueError('Named data lies outside GP-relative signed range')
            normalized = original & 0xffff0000
            if normalized | ((destination - gp) & 0xffff) != original:
                raise ValueError('GP relocation fails inverse reconstruction')
            struct.pack_into('<I', code, offset, normalized)
            function['bytes'] = bytes(code)
            function.setdefault('relocations', []).append(
                {'offset': offset, 'symbol': relocation['symbol'], 'type': 7})
            restored.append({'function': function['name'], 'address': function['low'] + offset,
                             'type': 'R_MIPS_GPREL16', 'symbol': relocation['symbol'],
                             'target_address': destination, 'gp': gp,
                             'original_instruction': original, 'inverse_reconstruction_verified': True})
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
    executable_sha1 = json.loads((output / 'coverage.json').read_text(encoding='utf-8'))['executable_sha1']
    define = version_define(executable_sha1)
    results = []
    for unit in profile['units']:
        if not profile_enabled(unit, executable_sha1) or unit['target_unit'] not in available:
            continue
        target = output / unit['object']
        target.parent.mkdir(parents=True, exist_ok=True)
        target.unlink(missing_ok=True)
        flags = list(profile['flags'])
        if 'inline' in unit:
            # A unit-level inlining mode where the original evidently differed.
            flags[flags.index('-inline') + 1] = unit['inline']
        command = [str(wibo.resolve()), '-C', str(ROOT), str(compiler.resolve()), *flags,
                   '-D' + define + '=1']
        for include in ('include', 'src/SB/Core/p2', 'src/SB/Core/x', 'src/SB/Game'):
            command.extend(['-i', str(ROOT / include)])
        command.extend(['-o', str(target.resolve()), str(ROOT / unit['source'])])
        subprocess.run(command, cwd=ROOT, check=True)
        record = available[unit['target_unit']]
        record['base_path'] = unit['object']
        record['metadata']['source_path'] = unit['source']
        # Exact function comparison is not proof of whole-executable linking.
        record['metadata']['complete'] = False
        results.append({'source':unit['source'], 'compiler_profile':profile['compiler']['id'],
                        'version_define':define,
                        'object_sha256':hashlib.sha256(target.read_bytes()).hexdigest(),
                        'source_link_verified':False})
    config_path.write_text(json.dumps(config, indent=2) + '\n', encoding='utf-8')
    return results
