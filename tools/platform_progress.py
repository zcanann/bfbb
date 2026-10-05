#!/usr/bin/env python3
"""Verify PS2/Xbox discs, extract private originals, and generate objdiff baselines."""
from __future__ import annotations
import argparse
import hashlib
import importlib
import io
import json
import os
from pathlib import Path
import subprocess
import tempfile
import zlib
import urllib.request
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / 'config/platforms/versions.json'


def write_json(path: Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')


def disc_hashes(path: Path) -> dict:
    md5, sha1, crc = hashlib.md5(), hashlib.sha1(), 0
    size = 0
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(8 * 1024 * 1024), b''):
            md5.update(block)
            sha1.update(block)
            crc = zlib.crc32(block, crc)
            size += len(block)
    return dict(size=size, crc32=f'{crc:08x}', md5=md5.hexdigest(), sha1=sha1.hexdigest())


def verify_executable(path: Path, version: dict) -> dict:
    expected = version['executable']['sha1']
    actual = hashlib.sha1(path.read_bytes()).hexdigest()
    if actual != expected:
        raise ValueError(f'{path}: executable SHA-1 {actual}, expected {expected}')
    module = importlib.import_module('platforms.' + version['platform'])
    return (module.inspect_elf if version['platform'] == 'ps2' else module.inspect_xbe)(path)


def verify_references(versions: dict) -> None:
    """Recheck committed expected hashes against public Redump metadata only."""
    archives = {}
    for key, version in versions.items():
        url = version['reference']['dat_url']
        if url not in archives:
            with urllib.request.urlopen(url, timeout=60) as response:
                archive = zipfile.ZipFile(io.BytesIO(response.read()))
            members = [name for name in archive.namelist() if name.endswith('.dat')]
            if len(members) != 1:
                raise ValueError(f'{url}: expected exactly one DAT')
            archives[url] = ET.fromstring(archive.read(members[0]))
        records = [rom for rom in archives[url].iter('rom')
                   if rom.get('sha1', '').lower() == version['disc']['sha1']]
        if not any({'size':int(rom.get('size', 0)),
                    'crc32':rom.get('crc', '').lower(),
                    'md5':rom.get('md5', '').lower(),
                    'sha1':rom.get('sha1', '').lower()} == version['disc'] for rom in records):
            raise ValueError(f'{key}: expected disc hashes absent from {url}')
        print(f'{key}: official Redump size/CRC32/MD5/SHA-1 confirmed', flush=True)


def extract(args, versions: dict) -> None:
    selected = {k:v for k,v in versions.items() if not args.version or k == args.version}
    found = set()
    candidates = [args.iso] if args.iso else sorted(args.iso_dir.rglob('*.iso'))
    for path in candidates:
        if path.stat().st_size not in {v['disc']['size'] for v in selected.values()}:
            continue
        possible = {k:v for k,v in versions.items() if v['disc']['size'] == path.stat().st_size}
        if not possible:
            continue
        print(f'Hashing {path.name}', flush=True)
        measured = disc_hashes(path)
        matches = [(k,v) for k,v in possible.items() if v['disc'] == measured]
        if not matches:
            raise ValueError(f'{path.name}: full-disc hashes do not match a registered Redump image')
        key, version = matches[0]
        if key not in selected:
            continue
        output = args.orig_dir / key / Path(version['executable']['path']).name
        output.parent.mkdir(parents=True, exist_ok=True)
        module = importlib.import_module('platforms.' + version['platform'])
        # Validate the exact extracted bytes before replacing any existing original.
        with tempfile.TemporaryDirectory(prefix='.extract-', dir=output.parent) as temporary:
            candidate = Path(temporary) / output.name
            module.extract_executable(path, candidate)
            metadata = verify_executable(candidate, version)
            os.replace(candidate, output)
        metadata.pop('path', None)
        write_json(args.build_dir / key / 'original-verification.json', {
            'version':key, 'disc_verified':True, 'disc':measured,
            'reference':version['reference'], 'executable':metadata,
        })
        found.add(key)
        print(f'{key}: Redump size/CRC32/MD5/SHA-1 and extracted executable verified', flush=True)
    missing = (set(selected) - found) if not args.iso or args.version else (set() if found else {'requested ISO'})
    if missing:
        raise ValueError('No verified ISO found for: ' + ', '.join(sorted(missing)))


def report(args, versions: dict) -> None:
    for key, version in versions.items():
        if args.version and key != args.version:
            continue
        path = args.orig_dir / key / Path(version['executable']['path']).name
        output = args.build_dir / key
        output.mkdir(parents=True, exist_ok=True)
        # Do not retain a previously successful report after a failed refresh.
        for name in ('report.json', 'baseline-report.json', 'progress.json', 'coverage.json', 'objdiff.json'):
            (output / name).unlink(missing_ok=True)
        metadata = verify_executable(path, version)
        metadata.pop('path', None)
        status = {'version':key, 'platform':version['platform'],
                  'original_verified':True, 'executable':metadata,
                  'source_build_verified':False, 'publish_matching_report':False, 'report_status':'pending-function-boundaries'}
        if args.command == 'report':
            backend = importlib.import_module('platforms.' + version['platform'] + '_report')
            options = {}
            reviewed = ROOT / 'config/platforms' / key / 'reviewed-functions.json'
            if reviewed.is_file():
                options['reviewed_functions'] = reviewed
            reviewed_calls = reviewed.with_name('reviewed-call-targets.json')
            if version['platform'] == 'ps2' and reviewed_calls.is_file():
                options['reviewed_call_targets'] = reviewed_calls
            reviewed_data = reviewed.with_name('reviewed-data-anchors.json')
            if version['platform'] == 'ps2' and reviewed_data.is_file():
                options['reviewed_data_anchors'] = reviewed_data
            coverage = backend.prepare_report(path, output, **options)
            status['coverage'] = coverage
            if hasattr(backend, 'verify_registries'):
                backend.verify_registries(output, ROOT / 'config/platforms' / key)
            compiled = []
            if version['platform'] == 'ps2' and args.ps2_compilers:
                from platforms.ps2_source import compile_units
                if args.wibo is None:
                    raise ValueError('--wibo is required with --ps2-compilers')
                compiled = compile_units(output, args.ps2_compilers, args.wibo)
            if version['platform'] == 'xbox' and args.xbox_compilers:
                from platforms.xbox_source import compile_units
                compiled = compile_units(output, args.xbox_compilers, args.wine)
            if compiled:
                status['compiled_units'] = compiled
                coverage['source_compilation_available'] = True
                coverage['source_comparison_available'] = True
                coverage['status'] = 'partial-source-comparison'
                write_json(output / 'coverage.json', coverage)
            if (output / 'objdiff.json').is_file():
                subprocess.run([str(args.objdiff.resolve()), 'report', 'generate',
                                '-p', str(output.resolve()), '-o', str((output / 'baseline-report.json').resolve())], check=True)
                measured = json.loads((output / 'baseline-report.json').read_text())
                if (not compiled and int(measured.get('measures', {}).get('matched_code', 0))) or int(measured.get('measures', {}).get('complete_code', 0)):
                    raise ValueError('Target-only bootstrap unexpectedly reports matched or linked code')
                status['report_status'] = ('partial-target-only-baseline' if int(measured['measures'].get('total_code', 0)) else 'pending-function-boundaries')
                if compiled:
                    status['report_status'] = 'partial-source-comparison'
                status['publish_matching_report'] = False
                if int(measured['measures'].get('total_code', 0)):
                    status['measures'] = measured['measures']
        write_json(output / 'progress.json', status)
        summary = f"{key}: original verified; {status['report_status']}; full executable build pending"
        coverage = status.get('coverage', {})
        if coverage.get('known_code_bytes'):
            summary += f"; {coverage['function_count']:,} recovered functions / {coverage['known_code_bytes']:,} code bytes (partial coverage)"
        if status.get('compiled_units'):
            summary += f"; {len(status['compiled_units'])} source TU compiled, {status['measures'].get('matched_code', 0)} code bytes matched"
        print(summary, flush=True)
        if os.environ.get('GITHUB_STEP_SUMMARY'):
            with open(os.environ['GITHUB_STEP_SUMMARY'], 'a', encoding='utf-8') as stream:
                stream.write(summary + '\n\n')


def main() -> None:
    versions = json.loads(MANIFEST.read_text(encoding='utf-8'))['versions']
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=['extract', 'verify', 'report', 'verify-references'])
    parser.add_argument('--version', choices=versions)
    source = parser.add_mutually_exclusive_group()
    source.add_argument('--iso', type=Path)
    source.add_argument('--iso-dir', type=Path, default=ROOT / 'orig')
    parser.add_argument('--orig-dir', type=Path, default=ROOT / 'orig')
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build')
    parser.add_argument('--ps2-compilers', type=Path)
    parser.add_argument('--wibo', type=Path)
    parser.add_argument('--xbox-compilers', type=Path)
    parser.add_argument('--wine', type=Path)
    parser.add_argument('--objdiff', type=Path, default=ROOT / 'build/tools' / ('objdiff-cli.exe' if os.name == 'nt' else 'objdiff-cli'))
    args = parser.parse_args()
    try:
        if args.command == 'verify-references':
            verify_references({k:v for k,v in versions.items() if not args.version or k == args.version})
        elif args.command == 'extract':
            extract(args, versions)
        else:
            report(args, versions)
    except (ValueError, OSError, RuntimeError, subprocess.CalledProcessError) as error:
        parser.exit(1, f'error: {error}\n')


if __name__ == '__main__':
    main()
