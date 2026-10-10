"""Compile full Xbox source units and compare explicitly reviewed linked functions.

LTCG objects contain intermediate code. The comparison objects therefore contain
final PE function bytes, independently bounded through decoded CFGs, with only
explicitly reviewed address expressions restored from real PE HIGHLOW records
and direct transfers resolved against named functions in actual source/dependency
objects or a pinned runtime library.
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
    # MSVC also marks emitted inline functions `f i`; keep their actual object
    # owner and subject them to the same unique-name and decoded-CFG checks.
    pattern = re.compile(r'^\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+'
                         r'([0-9a-fA-F]{8})\s+f\s+(?:i\s+)?(\S+)\s*$', re.MULTILINE)
    functions = [{'name': name, 'address': int(address, 16), 'object': owner}
                 for name, address, owner in pattern.findall(path.read_text(encoding='utf-8'))]
    if not functions or len({f['name'] for f in functions}) != len(functions):
        raise ValueError('Empty or ambiguous linked MAP function names')
    return sorted(functions, key=lambda f: (f['address'], f['name']))


def _leaf_extent(text: bytes, text_address: int, entry: int, bound: int,
                 call_targets: dict[int, str] | None = None,
                 switch_table: dict | None = None) -> dict:
    """Follow actual source CFG; a neighboring MAP symbol is only a rejection bound.

    Calls default to rejected; opt-in E8 calls and E9 tail transfers must resolve
    to named MAP entries.
    Only their fallthrough is followed. Branches must be direct and local,
    except for an explicitly selected and independently checked finite switch. Include interior alignment in the contiguous span, while
    excluding bytes following the last reachable instruction. No target is read.
    """
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_GRP_CALL, CS_GRP_JUMP, CS_GRP_RET
    from capstone.x86 import X86_OP_IMM

    if not text_address <= entry < bound <= text_address + len(text):
        raise ValueError('MAP function boundary lies outside linked .text')
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    pending, decoded, edges, calls = [entry], {}, [], []
    tail_returns = 0
    tables = []
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
            if (instruction.size != 5 or instruction.bytes[0] != 0xe8 or
                    len(instruction.operands) != 1 or instruction.operands[0].type != X86_OP_IMM or
                    instruction.operands[0].imm not in (call_targets or {})):
                raise ValueError('Unknown or indirect source call in reviewed profile')
            calls.append({'offset': address - entry + 1,
                          'symbol': call_targets[instruction.operands[0].imm]})
        if instruction.group(CS_GRP_RET):
            continue
        if instruction.mnemonic in ('int3', 'ud2', 'hlt'):
            raise ValueError('Unexpected reachable trap in source leaf')
        if instruction.group(CS_GRP_JUMP):
            if len(instruction.operands) != 1 or instruction.operands[0].type != X86_OP_IMM:
                if switch_table is None:
                    raise ValueError('Indirect source branches need explicit recovery')
                from .xbox_switch import decode_switch
                table = decode_switch(text, text_address, entry, bound, instruction, switch_table)
                tables.append(table)
                for destination in table['entries']:
                    pending.append(destination)
                    edges.append([address, destination])
                continue
            destination = instruction.operands[0].imm
            if not entry <= destination < bound and instruction.bytes[0] == 0xe9 and destination in (call_targets or {}):
                calls.append({'offset': address - entry + 1, 'symbol': call_targets[destination], 'opcode': 0xe9})
                tail_returns += 1
                continue
            pending.append(destination)
            edges.append([address, destination])
            if instruction.mnemonic == 'jmp':
                continue
        pending.append(end)
        edges.append([address, end])
    ranges = sorted((i.address, i.address + i.size) for i in decoded.values())
    if any(left[1] > right[0] for left, right in zip(ranges, ranges[1:])):
        raise ValueError('Overlapping source instruction starts')
    if not tail_returns and not any(i.group(CS_GRP_RET) for i in decoded.values()):
        raise ValueError('Source leaf has no reachable return')
    end = max(high for _, high in ranges)
    from .xbox_switch import validate_switch_cfg
    validate_switch_cfg(tables, edges, decoded, end, switch_table)
    return {'address': entry, 'size': end - entry,
            'instruction_bytes': sum(high - low for low, high in ranges),
            'internal_gap_ranges': [[left[1], right[0]] for left, right in zip(ranges, ranges[1:])
                                    if left[1] < right[0]],
            'edges': edges, 'direct_calls': sorted(calls, key=lambda call: call['offset']),
            **({'switch_tables': tables} if tables else {})}


def _extract_functions(executable: Path, map_path: Path, unit: dict,
                       target_switches: dict | None = None,
                       original_data: dict | None = None) -> tuple[list[dict], list[dict]]:
    text, text_address = _pe_text(executable)
    functions = _map_functions(map_path)
    by_name = {f['name']: f for f in functions}
    addresses = sorted({f['address'] for f in functions if
                        text_address <= f['address'] < text_address + len(text)})
    from .xbox_relocations import pe_provenance, source_globals, normalize, discover_source_expressions
    sections, highlow = pe_provenance(executable)
    anchors = source_globals(map_path, unit, sections, executable)
    from .xbox_particle_initializer import source_anchors as initializer_source_anchors, INITIALIZER
    initializer_anchors, initializer_expressions = initializer_source_anchors(executable, map_path, unit)
    from .xbox_external_data import verify_map, expressions as external_expressions
    if bool(unit.get('original_data_bindings')) != bool(original_data):
        raise ValueError('Original data bindings lack authenticated target preparation')
    external_anchors = verify_map(map_path, sections, original_data or {})
    output, evidence = [], []
    for canonical, linkage in unit['symbols'].items():
        if linkage not in by_name:
            raise ValueError(f'Reviewed source function absent from linked MAP: {linkage}')
        function = by_name[linkage]
        if function['object'].lower() != Path(unit['object']).name.lower():
            raise ValueError('Compared function did not originate in the real source TU')
        address = function['address']
        bound = next((a for a in addresses if a > address), text_address + len(text))
        call_targets = {}
        for callee in unit.get('direct_calls', {}).get(canonical, []):
            dependency = unit.get('call_symbols', {}).get(callee)
            callee_linkage = dependency['linkage_name'] if dependency else unit['symbols'][callee]
            owner = dependency['object'] if dependency else Path(unit['object']).name
            target = by_name[callee_linkage]
            if (target['object'].lower() != owner.lower() or
                    target['address'] in call_targets or not text_address <= target['address'] < text_address + len(text)):
                raise ValueError('Ambiguous or foreign source call target')
            call_targets[target['address']] = callee
        extent = _leaf_extent(text, text_address, address, bound, call_targets,
                              unit.get('switch_tables', {}).get(canonical))
        from .xbox_switch import source_switch_anchors, verify_switch_correspondence
        if canonical in unit.get('switch_tables', {}):
            verify_switch_correspondence(extent, (target_switches or {}).get(canonical))
        switch_anchors = source_switch_anchors(extent.get('switch_tables', []), highlow, bound)
        if set(anchors) & set(switch_anchors):
            raise ValueError('Switch table aliases an ordinary data anchor')
        function_anchors = {**anchors, **switch_anchors}
        offset = address - text_address
        body = text[offset:offset + extent['size']]
        fields = {a - address for a in highlow if address <= a < address + len(body)}
        if canonical == INITIALIZER and unit.get('particle_initializer'):
            if set(function_anchors) & set(initializer_anchors):
                raise ValueError('Initializer anchors alias ordinary data/switch anchors')
            function_anchors = {**function_anchors, **initializer_anchors}
            expressions = initializer_expressions
        else:
            expressions = discover_source_expressions(body,
                unit.get('address_expressions', {}).get(canonical, []), function_anchors, fields)
        normalized, relocations = normalize(body, expressions, function_anchors, fields)
        if original_data:
            if set(function_anchors) & set(external_anchors):
                raise ValueError('Original data binding aliases a source or switch anchor')
            bound_expressions = external_expressions(body,
                unit.get('original_data_expressions', {}).get(canonical, []), external_anchors, fields)
            normalized, bound_relocations = normalize(normalized, bound_expressions, external_anchors)
            relocations = sorted(relocations + bound_relocations, key=lambda r: r['offset'])
            expressions += bound_expressions
            function_anchors = {**function_anchors, **external_anchors}
        if extent['direct_calls']:
            from .xbox_calls import normalize_calls
            normalized, call_relocations = normalize_calls(normalized, address, extent['direct_calls'],
                                                          {name: at for at, name in call_targets.items()})
            relocations = sorted(relocations + call_relocations, key=lambda r: r['offset'])
        output.append({'symbol': canonical, 'bytes': normalized, 'relocations': relocations})
        evidence.append({**extent, 'canonical_identifier': canonical, 'linkage_name': linkage,
                         'source_object': function['object'], 'sha256': hashlib.sha256(body).hexdigest(),
                         'address_expressions': expressions, 'source_globals': function_anchors,
                         'source_call_targets': {name: at for at, name in call_targets.items()},
                         'normalized_sha256': hashlib.sha256(normalized).hexdigest()})
    return output, evidence


def compile_units(output: Path, compilers: Path, wine: Path | None = None) -> list[dict]:
    """Build real source TUs through LTCG, then attach only reviewed function code.

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
    if any('_CIfmod' in unit.get('call_symbols', {}) for unit in units):
        from .xbox_runtime import verify_cifmod_vendor
        verify_cifmod_vendor(compiler / profile['static_runtime']['libraries']['libcmt']['path'])
    if any('__CIasin' in unit.get('call_symbols', {}) for unit in units):
        from .xbox_asin import verify_asin_vendor
        verify_asin_vendor(compiler / profile['static_runtime']['libraries']['libcmt']['path'])
    if any('__CIpow' in unit.get('call_symbols', {}) for unit in units):
        from .xbox_pow import verify_pow_vendor
        verify_pow_vendor(compiler / profile['static_runtime']['libraries']['libcmt']['path'])
    if any('malloc(size_t)' in unit.get('call_symbols', {}) for unit in units):
        from .xbox_malloc import verify_malloc_vendor
        verify_malloc_vendor(compiler / profile['static_runtime']['libraries']['libcmt']['path'])
    # A failed refresh must not leave an older source object attached.
    for unit in units:
        available[unit['target_unit']].pop('base_path', None)
        (output / unit['comparison_object']).unlink(missing_ok=True)
    config_path.write_text(json.dumps(config, indent=2) + '\n', encoding='utf-8')
    # The target preparation step regenerates this proof from the authenticated
    # original, then binds it to its actual comparison object. Never fall back
    # to profile-supplied or stale case mappings.
    checked_switches = {}
    if any(unit.get('switch_tables') for unit in units):
        checked_switches = json.loads((output / 'switch-tables.json').read_text(encoding='utf-8'))
        coverage = json.loads((output / 'coverage.json').read_text(encoding='utf-8'))
        if (checked_switches.get('schema_version') != 1 or
                checked_switches.get('executable_sha1') != coverage['executable_sha1']):
            raise ValueError('Switch proof belongs to another target preparation')
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
        from .xbox_external_data import load_bindings, binding_object, verify_undefined
        original_data = load_bindings(output, unit, output / available[unit['target_unit']]['target_path'])
        source_object = output / unit['object']
        build = source_object.parent
        build.mkdir(parents=True, exist_ok=True)
        commands = []

        def run(tool: str, args: list[str], label: str, prove_undefined: bool = False) -> None:
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
            if prove_undefined:
                verify_undefined((build / f'{label}.log').read_text(encoding='utf-8', errors='replace'),
                                 result.returncode, source_object.name, original_data)
                return
            if result.returncode:
                tail = (build / f'{label}.log').read_text(encoding='utf-8', errors='replace')[-8000:]
                raise RuntimeError(f'Xbox {label} failed ({result.returncode}); see '
                                   f'{build / (label + ".log")}\n{tail}')

        includes = ['/I' + tool_path(ROOT / p) for p in
                    ('include', 'src/SB/Core/xbox', 'src/SB/Core/x')]
        includes.append('/I' + tool_path(compiler / 'Include'))
        run('cl.exe', [*profile['flags'], *includes, '/Fo' + source_object.name,
                       tool_path(ROOT / unit['source'])], 'compile-source')
        if unit.get('particle_initializer'):
            # Symbol/relocation provenance only. This non-LTCG object is never
            # linked or scored; production objects/flags remain unchanged.
            run('cl.exe', [*[flag for flag in profile['flags'] if flag != '/GL'], *includes,
                           '/Foinitializer-symbols.obj', tool_path(ROOT / unit['source'])],
                'compile-initializer-symbol-evidence')
        dependency_objects, dependency_records = [], []
        for index, dependency in enumerate(unit.get('source_dependencies', [])):
            dependency_object = dependency['object']
            if Path(dependency_object).name != dependency_object or dependency_object in dependency_objects + [source_object.name]:
                raise ValueError('Invalid or repeated source dependency object name')
            run('cl.exe', [*profile['flags'], *includes, '/Fo' + dependency_object,
                           tool_path(ROOT / dependency['source'])], f'compile-dependency-{index}')
            dependency_objects.append(dependency_object)
            dependency_records.append({'source': dependency['source'],
                'source_sha256': hashlib.sha256((ROOT / dependency['source']).read_bytes()).hexdigest(),
                'object_sha256': hashlib.sha256((build / dependency_object).read_bytes()).hexdigest()})
        runtime_libraries = []
        for name in unit.get('runtime_libraries', []):
            spec = profile['static_runtime']['libraries'][name]
            library = compiler / spec['path']
            if hashlib.sha256(library.read_bytes()).hexdigest() != spec['sha256']:
                raise ValueError('Unexpected static compiler runtime library')
            runtime_libraries.append(tool_path(library))
        support = {**profile['host_context'], **unit.get('host_context', {})}
        run('cl.exe', [*profile['flags'], *includes, '/Foentry.obj',
                       tool_path(ROOT / unit.get('host_entry', support['entry']))], 'compile-entry')
        host_objects = []
        if support['xatof'] is not None:
            run('cl.exe', [*profile['flags'], *includes, '/MD', '/Foxatof.obj',
                           tool_path(ROOT / support['xatof'])], 'compile-host-xatof')
            host_objects.append('xatof.obj')
        run('cl.exe', ['/nologo', '/c', '/O2', '/Fofltused.obj',
                       tool_path(ROOT / support['float_marker'])], 'compile-host-marker')
        run('lib.exe', ['/nologo', '/machine:x86', '/out:msvcrt.lib',
                        '/def:' + tool_path(ROOT / support['imports'])], 'create-host-imports')
        link_args = ['/nologo', '/LTCG', '/NODEFAULTLIB', '/ENTRY:xbox_source_entry',
                     '/SUBSYSTEM:CONSOLE', '/MAP:source.map', '/FIXED:NO', '/OUT:source.exe',
                     source_object.name, *dependency_objects, 'entry.obj', *host_objects, 'fltused.obj',
                     'msvcrt.lib', *runtime_libraries]
        if original_data:
            run('link.exe', link_args, 'verify-undefined-data', prove_undefined=True)
            (build / 'original-data.obj').write_bytes(binding_object(original_data))
            link_args.append('original-data.obj')
        run('link.exe', link_args, 'link-source')
        target_switches = None
        if unit.get('switch_tables'):
            checked = checked_switches['units'][unit['target_unit']]
            target_object = output / available[unit['target_unit']]['target_path']
            if hashlib.sha256(target_object.read_bytes()).hexdigest() != checked['target_object_sha256']:
                raise ValueError('Switch proof does not identify the current target comparison object')
            target_switches = checked['functions']
            if set(target_switches) != set(unit['switch_tables']):
                raise ValueError('Source and original switch function inventories differ')
        functions, evidence = _extract_functions(build / 'source.exe', build / 'source.map', unit,
                                                 target_switches, original_data or None)
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
                        'complete_translation_unit': False, 'host_support_excluded': True,
                        **({'initializer_symbol_evidence': {
                            'object': str((source_object.parent / 'initializer-symbols.obj').relative_to(output)),
                            'sha256': hashlib.sha256((build / 'initializer-symbols.obj').read_bytes()).hexdigest(),
                            'command_label': 'compile-initializer-symbol-evidence',
                            'scope': 'Same whole TU with pinned compiler; only /GL omitted. Named storage/relocation evidence only; never linked or scored.'}}
                           if unit.get('particle_initializer') else {}),
                        **({'original_data_bindings': original_data, 'bound_storage_bytes_credited': 0,
                            'original_data_provenance': 'Fixed original addresses; no PE HIGHLOW; undefined-symbol and decoded-memory-operand proof'}
                           if original_data else {}),
                        **({'source_dependencies': dependency_records} if dependency_records else {}),
                        **({'runtime_libraries': {name: profile['static_runtime']['libraries'][name]
                            for name in unit['runtime_libraries']}} if runtime_libraries else {})})
    config_path.write_text(json.dumps(config, indent=2) + '\n', encoding='utf-8')
    return records
