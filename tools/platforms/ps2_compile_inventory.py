#!/usr/bin/env python3
"""Compile a bounded inventory of real PS2 translation units, without updating progress.

Run under Linux/WSL with the authenticated compiler and wibo from ps2-toolchain.json.
Only source files with original DWARF function ownership are selected. Compilation
success and emitted functions are diagnostics, never source-match or link claims.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import struct
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from platforms.verify_reviewed import Original


def emitted_functions(path: Path) -> list[dict]:
    """Read defined ELF32 little-endian MIPS STT_FUNC records, including local ones."""
    data = path.read_bytes()
    if len(data) < 52 or data[:6] != b'\x7fELF\x01\x01' or struct.unpack_from('<H', data, 18)[0] != 8:
        raise ValueError('Compiler output is not ELF32 little-endian MIPS')
    shoff = struct.unpack_from('<I', data, 32)[0]
    entsize, count = struct.unpack_from('<HH', data, 46)
    if entsize != 40 or not count or shoff + count * entsize > len(data):
        raise ValueError('Invalid output section table')
    sections = [struct.unpack_from('<10I', data, shoff + i * 40) for i in range(count)]
    result = []
    for section in sections:
        if section[1] != 2:
            continue
        offset, size, link, stride = section[4], section[5], section[6], section[9]
        if stride != 16 or size % stride or offset + size > len(data) or link >= count:
            raise ValueError('Invalid output symbol table')
        strings = sections[link]
        if strings[4] + strings[5] > len(data):
            raise ValueError('Invalid output string table')
        names = data[strings[4]:strings[4] + strings[5]]
        for at in range(offset, offset + size, stride):
            name, value, length, info, _, owner = struct.unpack_from('<IIIBBH', data, at)
            if info & 15 != 2 or not 0 < owner < count:
                continue
            end = names.find(b'\0', name)
            if name >= len(names) or end < 0 or value + length > sections[owner][5]:
                raise ValueError('Invalid output function symbol')
            result.append({'name': names[name:end].decode('utf-8', errors='replace'),
                           'size': length, 'section_index': owner, 'section_offset': value})
    return result


def run_command(command: list[str], log: Path, timeout: float) -> tuple[int, bool]:
    """Bound both compiler execution and its process group; keep full diagnostic logs."""
    with log.open('wb') as stream:
        process = subprocess.Popen(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT,
                                   start_new_session=os.name != 'nt')
        try:
            return process.wait(timeout=timeout), False
        except subprocess.TimeoutExpired:
            if os.name != 'nt':
                os.killpg(process.pid, signal.SIGKILL)
            else:
                process.kill()
            process.wait()
            return process.returncode, True


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--version', default='SLUS-20680')
    parser.add_argument('--orig-dir', type=Path, default=ROOT / 'orig')
    parser.add_argument('--compilers', type=Path, required=True)
    parser.add_argument('--wibo', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--max-units', type=int, default=40)
    parser.add_argument('--timeout', type=float, default=90)
    parser.add_argument('--unit', action='append', default=[], help='Exact DWARF source path; repeatable')
    parser.add_argument('--include', action='append', default=[], help='Additional existing repo-relative include directory')
    args = parser.parse_args()
    if not 1 <= args.max_units <= 200 or not 0 < args.timeout <= 600:
        parser.error('max-units must be 1..200 and timeout must be greater than 0 and at most 600 seconds')
    profile = json.loads((ROOT / 'config/platforms/ps2-toolchain.json').read_text())
    manifest = json.loads((ROOT / 'config/platforms/versions.json').read_text())['versions']
    if args.version not in manifest:
        parser.error('Unknown version')
    original = Original(args.version, manifest[args.version], args.orig_dir.resolve())
    if not any(s['name'] == '.debug' and s['size'] for s in original.metadata['sections']):
        parser.error('This inventory requires an original with DWARF source ownership')
    compiler = args.compilers.resolve() / profile['compiler']['id'] / 'mwccps2.exe'
    wibo = args.wibo.resolve()
    for path, expected in ((compiler, profile['compiler']['compiler_sha256']),
                           (wibo, profile['runtime']['sha256'])):
        if hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            raise ValueError(f'Unexpected tool binary: {path.name}')
    includes = []
    for directory in ['include', 'src/SB/Core/p2', 'src/SB/Core/x', 'src/SB/Game', *args.include]:
        path = (ROOT / directory).resolve()
        path.relative_to(ROOT)
        if not path.is_dir():
            raise ValueError(f'Missing include directory: {directory}')
        if path not in includes:
            includes.append(path)
    grouped = defaultdict(list)
    for function in original.functions:
        grouped[function['source']].append(function)
    covered = {unit['target_unit'] for unit in profile['units']}
    units = []
    for name, functions in grouped.items():
        source = (ROOT / 'src' / name).resolve()
        source.relative_to(ROOT / 'src')
        if not name.endswith('.cpp') or not source.is_file():
            continue
        if args.unit and name not in args.unit:
            continue
        if not args.unit and name in covered:
            continue
        units.append({'unit': name, 'source': source.relative_to(ROOT).as_posix(),
                      'original_functions': len(functions),
                      'original_code_bytes': sum(f['high'] - f['low'] for f in functions)})
    if args.unit and set(args.unit) != {u['unit'] for u in units}:
        parser.error('Requested unit lacks original owned functions or an existing .cpp source')
    units.sort(key=lambda unit: (unit['original_code_bytes'], unit['unit']))
    units = units[:args.max_units]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    document = {'schema_version': 1, 'version': args.version, 'executable_sha1': original.sha1,
                'compiler_sha256': profile['compiler']['compiler_sha256'],
                'runtime_sha256': profile['runtime']['sha256'], 'flags': profile['flags'],
                'includes': [p.relative_to(ROOT).as_posix() for p in includes],
                'eligible_for_progress': False, 'source_match_verified': False,
                'scope': 'Whole-source compilation only; no score, symbol or profile changes',
                'planned_units': len(units), 'results': []}
    report = output / 'compile-inventory.json'
    report.write_text(json.dumps(document, indent=2) + '\n')
    for unit in units:
        relative = Path(unit['unit'])
        obj = output / 'objects' / relative.with_suffix('.o')
        log = output / 'logs' / relative.with_suffix('.log')
        obj.parent.mkdir(parents=True, exist_ok=True)
        log.parent.mkdir(parents=True, exist_ok=True)
        obj.unlink(missing_ok=True)
        command = [str(wibo), '-C', str(ROOT), str(compiler), *profile['flags']]
        for include in includes:
            command += ['-i', str(include)]
        command += ['-o', str(obj), str(ROOT / unit['source'])]
        started = time.monotonic()
        returncode, timed_out = run_command(command, log, args.timeout)
        text = log.read_text(errors='replace')
        lines = text.splitlines()
        errors = [' | '.join(lines[max(0, i - 1):i + 4]) for i, line in enumerate(lines)
                  if '#   Error:' in line]
        success = returncode == 0 and obj.is_file() and not timed_out
        entry = {**unit, 'returncode': returncode, 'timed_out': timed_out, 'compiled': success,
                 'seconds': round(time.monotonic() - started, 3), 'command': command,
                 'log': log.relative_to(output).as_posix(),
                 'missing_headers': sorted(set(re.findall("the file '([^']+)' cannot be opened", text))),
                 'error_count': len(errors), 'first_errors': errors[:4], 'emitted_functions': []}
        if success:
            entry['emitted_functions'] = emitted_functions(obj)
            entry['object'] = obj.relative_to(output).as_posix()
            entry['object_sha256'] = hashlib.sha256(obj.read_bytes()).hexdigest()
        else:
            obj.unlink(missing_ok=True)
        document['results'].append(entry)
        document['missing_header_counts'] = dict(Counter(h for x in document['results'] for h in x['missing_headers']).most_common())
        document['compiled_units'] = sum(x['compiled'] for x in document['results'])
        report.write_text(json.dumps(document, indent=2) + '\n')
        print(f"{unit['unit']}: {'compiled' if success else 'timeout' if timed_out else 'blocked'}", flush=True)


if __name__ == '__main__':
    main()
