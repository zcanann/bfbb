"""Compile full Xbox source units and compare explicitly reviewed linked leaves.

LTCG objects contain intermediate code. The comparison objects therefore contain
untouched final PE function bytes, independently bounded through decoded CFGs.
Host link support is never included in matching coverage or completion claims.
"""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile

from .coff import function_object

ROOT = Path(__file__).resolve().parents[2]
PROFILE_PATH = ROOT / 'config/platforms/xbox-toolchain.json'


def load_profile() -> dict:
    return json.loads(PROFILE_PATH.read_text(encoding='utf-8'))


def _tree_hash(directory: Path) -> str:
    digest = hashlib.sha256()
    files = sorted((p for p in directory.rglob('*') if p.is_file()),
                   key=lambda p: p.relative_to(directory).as_posix())
    if not files:
        raise ValueError(f'Empty compiler header tree: {directory}')
    for path in files:
        digest.update(path.relative_to(directory).as_posix().encode('utf-8') + b'\0' +
                      hashlib.sha256(path.read_bytes()).digest())
    return digest.hexdigest()


def _verify_compiler(directory: Path, profile: dict) -> None:
    for filename, expected in profile['compiler']['files_sha256'].items():
        if hashlib.sha256((directory / filename).read_bytes()).hexdigest() != expected:
            raise ValueError(f'Unexpected Xbox compiler/runtime file: {filename}')
    for dirname, expected in profile['compiler']['header_trees_sha256'].items():
        if _tree_hash(directory / dirname) != expected:
            raise ValueError(f'Unexpected Xbox compiler headers: {dirname}')


def _pe_text(path: Path) -> tuple[bytes, int]:
    """Read one file-backed i386 PE32 .text section, without changing its bytes."""
    data = path.read_bytes()
    if len(data) < 64 or data[:2] != b'MZ':
        raise ValueError('Linked source is not a DOS/PE image')
    pe = struct.unpack_from('<I', data, 60)[0]
    if pe + 24 > len(data) or data[pe:pe + 4] != b'PE\0\0':
        raise ValueError('Invalid linked PE header')
    machine, count, _, _, _, optional_size, _ = struct.unpack_from('<HHIIIHH', data, pe + 4)
    optional = pe + 24
    if (machine != 0x14c or optional_size < 96 or
            optional + optional_size + 40 * count > len(data) or
            struct.unpack_from('<H', data, optional)[0] != 0x10b):
        raise ValueError('Expected valid i386 PE32 linked source')
    image_base = struct.unpack_from('<I', data, optional + 28)[0]
    sections = [struct.unpack_from('<8sIIIIIIHHI', data, optional + optional_size + 40 * i)
                for i in range(count)]
    selected = [s for s in sections if s[0].rstrip(b'\0') == b'.text']
    if len(selected) != 1:
        raise ValueError('Expected exactly one linked .text section')
    section = selected[0]
    size = min(section[1], section[3])
    address = image_base + section[2]
    if (not size or section[4] + size > len(data) or address + size > 0x100000000 or
            section[9] & 0x20000020 != 0x20000020):
        raise ValueError('Invalid executable .text extent')
    return data[section[4]:section[4] + size], address


def _map_functions(path: Path) -> list[dict]:
    pattern = re.compile(r'^\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+'
                         r'([0-9a-fA-F]{8})\s+f\s+(\S+)\s*$', re.MULTILINE)
    functions = [{'name': name, 'address': int(address, 16), 'object': owner}
                 for name, address, owner in pattern.findall(path.read_text(encoding='utf-8'))]
    if not functions or len({f['name'] for f in functions}) != len(functions):
        raise ValueError('Empty or ambiguous linked MAP function names')
    return sorted(functions, key=lambda f: (f['address'], f['name']))


def _leaf_extent(text: bytes, text_address: int, entry: int, bound: int) -> dict:
    """Follow actual source CFG; a neighboring MAP symbol is only a rejection bound.

    This deliberately supports only reviewed no-call leaves. Branches must be
    direct and local. Include interior alignment in the contiguous span, while
    excluding bytes following the last reachable instruction. No target is read.
    """
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_GRP_CALL, CS_GRP_JUMP, CS_GRP_RET
    from capstone.x86 import X86_OP_IMM

    if not text_address <= entry < bound <= text_address + len(text):
        raise ValueError('MAP function boundary lies outside linked .text')
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    pending, decoded, edges = [entry], {}, []
    while pending:
        address = pending.pop()
        if address in decoded:
            continue
        if not entry <= address < bound:
            raise ValueError('Source control flow exits the reviewed leaf boundary')
        offset = address - text_address
        instruction = next(decoder.disasm(text[offset:offset + 15], address, count=1), None)
        if instruction is None or address + instruction.size > bound:
            raise ValueError('Invalid source instruction extent')
        decoded[address] = instruction
        end = address + instruction.size
        if instruction.group(CS_GRP_CALL):
            raise ValueError('Calls are unsupported by this reviewed leaf profile')
        if instruction.group(CS_GRP_RET):
            continue
        if instruction.mnemonic in ('int3', 'ud2', 'hlt'):
            raise ValueError('Unexpected reachable trap in source leaf')
        if instruction.group(CS_GRP_JUMP):
            if len(instruction.operands) != 1 or instruction.operands[0].type != X86_OP_IMM:
                raise ValueError('Indirect source branches need explicit recovery')
            destination = instruction.operands[0].imm
            pending.append(destination)
            edges.append([address, destination])
            if instruction.mnemonic == 'jmp':
                continue
        pending.append(end)
        edges.append([address, end])
    ranges = sorted((i.address, i.address + i.size) for i in decoded.values())
    if any(left[1] > right[0] for left, right in zip(ranges, ranges[1:])):
        raise ValueError('Overlapping source instruction starts')
    if not any(i.group(CS_GRP_RET) for i in decoded.values()):
        raise ValueError('Source leaf has no reachable return')
    end = max(high for _, high in ranges)
    return {'address': entry, 'size': end - entry,
            'instruction_bytes': sum(high - low for low, high in ranges),
            'internal_gap_ranges': [[left[1], right[0]] for left, right in zip(ranges, ranges[1:])
                                    if left[1] < right[0]],
            'edges': edges}


def _extract_functions(executable: Path, map_path: Path, unit: dict) -> tuple[list[dict], list[dict]]:
    text, text_address = _pe_text(executable)
    functions = _map_functions(map_path)
    by_name = {f['name']: f for f in functions}
    addresses = sorted({f['address'] for f in functions if
                        text_address <= f['address'] < text_address + len(text)})
    output, evidence = [], []
    for canonical, linkage in unit['symbols'].items():
        if linkage not in by_name:
            raise ValueError(f'Reviewed source function absent from linked MAP: {linkage}')
        function = by_name[linkage]
        if function['object'].lower() != Path(unit['object']).name.lower():
            raise ValueError('Compared function did not originate in the real source TU')
        address = function['address']
        bound = next((a for a in addresses if a > address), text_address + len(text))
        extent = _leaf_extent(text, text_address, address, bound)
        offset = address - text_address
        body = text[offset:offset + extent['size']]
        output.append({'symbol': canonical, 'bytes': body})
        evidence.append({**extent, 'canonical_identifier': canonical, 'linkage_name': linkage,
                         'source_object': function['object'], 'sha256': hashlib.sha256(body).hexdigest()})
    return output, evidence


def compile_units(output: Path, compilers: Path, wine: Path | None = None) -> list[dict]:
    """Build real source TUs through LTCG, then attach only reviewed leaf code.

    compilers contains the profile's compiler-id directory, with Bin/ and Include/.
    wine=None uses native Windows. Wine receives an isolated temporary prefix
    keyed by output path, so WSL does not put Unix server state on NTFS.
    Host support and omitted source functions never enter the comparison object.
    """
    import capstone

    output, compilers = Path(output).resolve(), Path(compilers).resolve()
    profile = load_profile()
    if capstone.__version__ != profile['analysis']['capstone_version']:
        raise ValueError('Xbox source extent extraction requires the pinned Capstone version')
    if wine is None and os.name != 'nt':
        raise ValueError('Xbox source linking on non-Windows hosts requires Wine')
    compiler = compilers / profile['compiler']['id']
    _verify_compiler(compiler, profile)
    config_path = output / 'objdiff.json'
    config = json.loads(config_path.read_text(encoding='utf-8'))
    available = {unit['name']: unit for unit in config['units']}
    units = [unit for unit in profile['units'] if unit['target_unit'] in available]
    # A failed refresh must not leave an older source object attached.
    for unit in units:
        available[unit['target_unit']].pop('base_path', None)
        (output / unit['comparison_object']).unlink(missing_ok=True)
    config_path.write_text(json.dumps(config, indent=2) + '\n', encoding='utf-8')
    environment = dict(os.environ)
    for key in list(environment):
        if key.upper() in ('CL', '_CL_', 'INCLUDE', 'LIB', 'LIBPATH'):
            del environment[key]
    if wine is not None:
        prefix_key = hashlib.sha256(str(output).encode('utf-8')).hexdigest()[:16]
        prefix = Path(tempfile.gettempdir()) / ('bfbb-xbox-wine-' + prefix_key)
        prefix.mkdir(mode=0o700, exist_ok=True)
        environment.update({'WINEPREFIX': str(prefix), 'WINEARCH': 'win32',
                            'WINEDEBUG': '-all', 'WINEDLLOVERRIDES': 'msvcr71,msvcp71=n'})

    def tool_path(path: Path) -> str:
        value = str(path.resolve())
        return 'Z:' + value.replace('/', '\\') if wine is not None else value

    records = []
    for unit in units:
        source_object = output / unit['object']
        build = source_object.parent
        build.mkdir(parents=True, exist_ok=True)
        commands = []

        def run(tool: str, args: list[str], label: str) -> None:
            executable = compiler / 'Bin' / tool
            command = ([str(Path(wine).resolve()), tool_path(executable)] if wine is not None
                       else [str(executable)]) + args
            commands.append({'label': label, 'command': command})
            # Wine's server can retain inherited pipes after the compiler exits.
            # Direct log files let subprocess wait only for the actual tool.
            with (build / f'{label}.log').open('w', encoding='utf-8') as log:
                result = subprocess.run(command, cwd=build, env=environment, stdout=log,
                                        stderr=subprocess.STDOUT)
            (build / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n', encoding='utf-8')
            if result.returncode:
                tail = (build / f'{label}.log').read_text(encoding='utf-8', errors='replace')[-8000:]
                raise RuntimeError(f'Xbox {label} failed ({result.returncode}); see '
                                   f'{build / (label + ".log")}\n{tail}')

        includes = ['/I' + tool_path(ROOT / p) for p in
                    ('include', 'src/SB/Core/xbox', 'src/SB/Core/x')]
        includes.append('/I' + tool_path(compiler / 'Include'))
        run('cl.exe', [*profile['flags'], *includes, '/Fo' + source_object.name,
                       tool_path(ROOT / unit['source'])], 'compile-source')
        support = profile['host_context']
        run('cl.exe', [*profile['flags'], *includes, '/Foentry.obj',
                       tool_path(ROOT / unit.get('host_entry', support['entry']))], 'compile-entry')
        run('cl.exe', [*profile['flags'], *includes, '/MD', '/Foxatof.obj',
                       tool_path(ROOT / support['xatof'])], 'compile-host-xatof')
        run('cl.exe', ['/nologo', '/c', '/O2', '/Fofltused.obj',
                       tool_path(ROOT / support['float_marker'])], 'compile-host-marker')
        run('lib.exe', ['/nologo', '/machine:x86', '/out:msvcrt.lib',
                        '/def:' + tool_path(ROOT / support['imports'])], 'create-host-imports')
        run('link.exe', ['/nologo', '/LTCG', '/NODEFAULTLIB', '/ENTRY:xbox_source_entry',
                         '/SUBSYSTEM:CONSOLE', '/MAP:source.map', '/FIXED:NO', '/OUT:source.exe',
                         source_object.name, 'entry.obj', 'xatof.obj', 'fltused.obj', 'msvcrt.lib'], 'link-source')
        functions, evidence = _extract_functions(build / 'source.exe', build / 'source.map', unit)
        comparison = output / unit['comparison_object']
        comparison.write_bytes(function_object(functions))
        (build / 'function-extents.json').write_text(json.dumps({
            'capstone_version': capstone.__version__, 'method': profile['analysis']['method'],
            'functions': evidence}, indent=2) + '\n', encoding='utf-8')
        record = available[unit['target_unit']]
        record['base_path'] = unit['comparison_object']
        record.setdefault('metadata', {}).update({'source_path': unit['source'], 'complete': False})
        records.append({'source': unit['source'], 'compiler_profile': profile['compiler']['id'],
                        'source_sha256': hashlib.sha256((ROOT / unit['source']).read_bytes()).hexdigest(),
                        'object_sha256': hashlib.sha256(source_object.read_bytes()).hexdigest(),
                        'linked_pe_sha256': hashlib.sha256((build / 'source.exe').read_bytes()).hexdigest(),
                        'comparison_object_sha256': hashlib.sha256(comparison.read_bytes()).hexdigest(),
                        'compared_function_count': len(functions), 'source_link_verified': False,
                        'complete_translation_unit': False, 'host_support_excluded': True})
    config_path.write_text(json.dumps(config, indent=2) + '\n', encoding='utf-8')
    return records
