"""Export original Xbox .text byte coverage without inventing function symbols.

The untouched objdiff baseline supplies every function score. Authenticated
original sections supply the byte denominator. Unresolved .text ranges contribute
zero matches and zero functions. This is a report-v2 aggregation adapter, not an
objdiff match result or a reconstructed executable link.
"""
from __future__ import annotations

from copy import deepcopy
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from platforms.section_report import measure_functions, validate_report


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def _json(path: Path) -> dict:
    return json.loads(path.read_text(encoding='utf-8'))


def _coff(path: Path) -> tuple[list[dict], dict[str, bytes]]:
    """Read the simple comparison COFF containers; reject unsupported layouts."""
    data = path.read_bytes()
    require(len(data) >= 20, 'Truncated comparison COFF')
    machine, count, _, symoff, symcount, optional, _ = struct.unpack_from('<HHIIIHH', data)
    require(machine == 0x14c and optional == 0 and 20 + 40 * count <= len(data), 'Expected i386 COFF')
    sections = []
    for index in range(count):
        name, _, _, size, offset, _, _, relocs, _, flags = struct.unpack_from('<8sIIIIIIHHI', data, 20 + 40 * index)
        require(not relocs and offset + size <= len(data), 'Unsupported relocations or truncated COFF section')
        sections.append({'name': name.rstrip(b'\0').decode('ascii'), 'bytes': data[offset:offset + size], 'flags': flags})
    functions = {}
    if not symcount:
        return sections, functions
    strings = symoff + 18 * symcount
    require(symoff >= 20 + 40 * count and strings + 4 <= len(data), 'Invalid COFF symbol table')
    string_size = struct.unpack_from('<I', data, strings)[0]
    require(string_size >= 4 and strings + string_size <= len(data), 'Invalid COFF strings')
    index = 0
    while index < symcount:
        name, value, section, kind, storage, aux = struct.unpack_from('<8sIhHBB', data, symoff + 18 * index)
        require(index + aux < symcount, 'Invalid auxiliary symbol count')
        if name[:4] == b'\0' * 4:
            offset = struct.unpack_from('<I', name, 4)[0]
            require(4 <= offset < string_size, 'Invalid COFF symbol name')
            end = data.find(b'\0', strings + offset, strings + string_size)
            require(end != -1, 'Unterminated COFF symbol')
            label = data[strings + offset:end].decode('utf-8')
        else:
            label = name.rstrip(b'\0').decode('utf-8')
        require(kind == 0x20 and storage == 2 and aux == 1 and 1 <= section <= len(sections),
                'Expected explicit comparison function symbol')
        size = struct.unpack_from('<I', data, symoff + 18 * (index + 1) + 4)[0]
        payload = sections[section - 1]['bytes']
        require(label not in functions and size > 0 and value + size <= len(payload), 'Invalid function extent')
        functions[label] = payload[value:value + size]
        index += aux + 1
    return sections, functions


def export_report(build_dir: Path) -> dict:
    """Write section-report.json plus explanatory section-coverage.json.

    Read existing authenticated production outputs without changing baseline,
    comparison objects or objdiff config. Code-unit padded section inventories
    are omitted; real original ranges remain in splits.json and the byte ledger.
    """
    build = Path(build_dir).resolve()
    destination = build / 'section-report.json'
    sidecar = build / 'section-coverage.json'
    destination.unlink(missing_ok=True)
    sidecar.unlink(missing_ok=True)
    baseline_path = build / 'baseline-report.json'
    baseline = _json(baseline_path)
    config, splits, symbols, coverage = (_json(build / name) for name in
        ('objdiff.json', 'splits.json', 'symbols.json', 'coverage.json'))
    require(baseline.get('version') == 2, 'Expected objdiff report v2')
    original_hash = coverage['executable_sha1']
    require(splits['executable_sha1'] == symbols['executable_sha1'] == original_hash, 'Output original identities differ')
    versions = _json(ROOT / 'config/platforms/versions.json')['versions']
    identities = [name for name, record in versions.items() if record['platform'] == 'xbox'
                  and record['executable']['sha1'] == original_hash]
    require(len(identities) == 1, 'Unregistered or ambiguous original')
    committed = _json(ROOT / 'config/platforms' / identities[0] / 'splits.json')
    require(splits['sections'] == committed['sections'], 'Original section metadata differs from committed identity')
    inventory, inventory_functions = _coff(build / 'target.obj')
    require(not inventory_functions, 'Original inventory must not fabricate function symbols')
    by_section = {s['name']: s for s in inventory}
    require(len(by_section) == len(inventory) and set(by_section) == {'.text', '.rdata', '.data'}, 'Unexpected inventory sections')
    original_sections = {s['name']: s for s in splits['sections']}
    for name, section in by_section.items():
        expected = original_sections[name]
        require(len(section['bytes']) == expected['raw_size'] and
                hashlib.sha1(section['bytes']).hexdigest() == expected['sha1'], 'Original section bytes differ')
    text = by_section['.text']['bytes']
    text_base = original_sections['.text']['virtual_address']
    rows = sorted(symbols['symbols'], key=lambda f: f['address'])
    registry = {r['canonical_identifier']: r for r in rows}
    require(len(registry) == len(rows), 'Duplicate registered function identity')
    cursor, unresolved, ledger = text_base, [], []
    for row in rows:
        start, end = row['address'], row['address'] + row['size']
        require(cursor <= start < end <= text_base + len(text), 'Overlapping or invalid original function ranges')
        if cursor < start:
            unresolved.append((cursor, start))
            ledger.append(('unresolved', cursor, start - cursor))
        ledger.append(('function', start, end - start))
        cursor = end
    if cursor < text_base + len(text):
        unresolved.append((cursor, text_base + len(text)))
        ledger.append(('unresolved', cursor, text_base + len(text) - cursor))
    require(sum(size for _, _, size in ledger) == len(text), 'Original .text partition does not cover every byte once')
    require([(r['kind'], r['address'], r['size']) for r in splits['text_ranges']] ==
            [('unclassified' if kind == 'unresolved' else kind, address, size) for kind, address, size in ledger],
            'Symbols and generated split partition differ')
    configurations = {u['name']: u for u in config['units']}
    require(len(configurations) == len(config['units']), 'Duplicate objdiff units')
    all_functions, report_units, seen = [], [], set()
    for unit in baseline['units']:
        functions = unit.get('functions', [])
        require(not unit.get('metadata', {}).get('complete') and
                not any(int(unit.get('measures', {}).get(k, 0)) for k in ('complete_code', 'complete_data', 'complete_units')),
                'Unverified source-link completion in baseline')
        if not functions:
            require(not int(unit.get('measures', {}).get('matched_data', 0)), 'Data matching is outside this exporter scope')
            continue
        description = configurations[unit['name']]
        _, targets = _coff(build / description['target_path'])
        bases = _coff(build / description['base_path'])[1] if description.get('base_path') else {}
        require(set(targets) == {f['name'] for f in functions}, 'Reported functions differ from actual target object')
        for function in functions:
            name = function['name']
            require(name in registry and name not in seen, 'Missing or repeated original function identity')
            seen.add(name)
            row = registry[name]
            size, score = int(function['size']), function.get('fuzzy_match_percent', 0)
            require(size == row['size'] and 0 <= score <= 100, 'Function size or score differs')
            offset = row['address'] - text_base
            require(targets[name] == text[offset:offset + size], 'Function target bytes differ from authenticated .text')
            require(not score or name in bases, 'Nonzero match without actual source function')
            if score == 100:
                require(bases[name] == targets[name], 'Exact matched function differs from source bytes')
        measured = measure_functions(functions)
        for key in ('total_code', 'matched_code', 'total_functions', 'matched_functions'):
            require(int(unit['measures'].get(key, 0)) == int(measured[key]), 'Baseline function measure sum differs')
        updated = deepcopy(unit)
        updated.pop('sections', None)  # objdiff's combined COFF sections add non-retail padding.
        updated['measures'] = measured
        updated['metadata'] = {**updated.get('metadata', {}), 'complete': False,
                               'progress_categories': ['original_text', 'known_functions']}
        report_units.append(updated)
        all_functions.extend(deepcopy(functions))
    require(seen == set(registry), 'Some registered functions are absent from the actual comparison')
    for key in ('total_code', 'matched_code', 'total_functions', 'matched_functions'):
        require(int(baseline['measures'].get(key, 0)) == int(measure_functions(all_functions)[key]), 'Baseline aggregate differs')
    missing = sum(end - start for start, end in unresolved)
    report_units.append({'name': 'unresolved/original.text', 'measures': measure_functions([], unresolved=missing),
                         'sections': [{'name': '.text', 'size': str(end - start),
                                       'fuzzy_match_percent': 0.0, 'metadata': {'virtual_address': str(start)}}
                                      for start, end in unresolved], 'functions': [],
                         'metadata': {'complete': False, 'auto_generated': True, 'progress_categories': ['original_text']}})
    data_size = sum(len(by_section[name]['bytes']) for name in ('.rdata', '.data'))
    report_units.append({'name': 'inventory/original_data', 'measures': measure_functions([], data=data_size),
                         'sections': [{'name': name, 'size': str(len(by_section[name]['bytes'])),
                                       'fuzzy_match_percent': 0.0,
                                       'metadata': {'virtual_address': str(original_sections[name]['virtual_address'])}}
                                      for name in ('.rdata', '.data')], 'functions': [],
                         'metadata': {'complete': False, 'auto_generated': True, 'progress_categories': ['original_data']}})
    code_units = len(report_units) - 1
    report = {'version': 2, 'units': report_units,
              'measures': measure_functions(all_functions, unresolved=missing, data=data_size, units=len(report_units)),
              'categories': [
                  {'id': 'original_text', 'name': 'Main .text (mixed SDK sections excluded)',
                   'measures': measure_functions(all_functions, unresolved=missing, units=code_units)},
                  {'id': 'known_functions', 'name': 'Known functions only (function count incomplete)',
                   'measures': measure_functions(all_functions, units=code_units - 1)},
                  {'id': 'original_data', 'name': 'Initialized .rdata/.data',
                   'measures': measure_functions([], data=data_size)}]}
    require(int(report['measures']['total_code']) == len(text), 'Section byte denominator differs')
    validate_report(report)
    explanation = {'schema_version': 1, 'version': identities[0], 'executable_sha1': original_hash,
                   'scope': 'Full original .text byte denominator, including alignment and embedded tables; mixed SDK sections excluded.',
                   'aggregation': 'Report-v2 section coverage adapter over unchanged objdiff per-function scores; not stock objdiff aggregate measures.',
                   'baseline_report_sha256': hashlib.sha256(baseline_path.read_bytes()).hexdigest(),
                   'partition_sha256': hashlib.sha256(json.dumps(ledger, separators=(',', ':')).encode()).hexdigest(),
                   'text_bytes': len(text), 'known_function_bytes': len(text) - missing, 'unresolved_text_bytes': missing,
                   'known_function_count': len(all_functions), 'whole_game_function_count': None,
                   'function_coverage_complete': False, 'retail_link_verified': False,
                   'formula': 'total_code = disjoint known original function sizes + unmatched original .text gaps; matches and fuzzy contributions are from actual objdiff functions only.',
                   'limitations': ['Function counts cover recovered functions only, not the entire executable.',
                                   'Unknown .text ranges add no functions, no fuzzy contribution and no matches.',
                                   'Comparison-container alignment is excluded; original internal alignment remains.',
                                   'Source link completion is zero; SDK mixed sections and virtual zero-fill are outside this report.'],
                   'upstream_reference': 'https://github.com/encounter/objdiff/blob/3cebee67667d440fa0fe1b64026c35db53ac40d6/objdiff-cli/src/cmd/report.rs#L288'}
    destination.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    sidecar.write_text(json.dumps(explanation, indent=2) + '\n', encoding='utf-8')
    return explanation


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build_dir', type=Path)
    args = parser.parse_args()
    print(json.dumps(export_report(args.build_dir), indent=2))
