"""Combine PS2 function comparisons with independently verified CPU/data regions.

Original functions retain objdiff scores. Unknown CPU ranges are unmatched bytes,
not invented functions. VU upload packets are counted as host data storage.
"""
from __future__ import annotations
from copy import deepcopy
import hashlib
import json
from pathlib import Path

from .section_report import measure_functions, validate_report

ROOT = Path(__file__).resolve().parents[2]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding='utf-8'))


def export_report(build_dir: Path, executable: Path) -> dict:
    from .ps2_layout import generate_layouts
    build = Path(build_dir)
    for name in ('section-report.json', 'section-coverage.json'):
        (build / name).unlink(missing_ok=True)
    baseline_path = build / 'baseline-report.json'
    baseline = read_json(baseline_path)
    coverage, symbols, config = (read_json(build / n) for n in
                                 ('coverage.json', 'symbols.json', 'objdiff.json'))
    layouts = generate_layouts(Path(executable).resolve().parent.parent, ROOT / 'config/platforms')
    matches = [v for v, item in layouts.items() if item['executable_sha1'] == coverage['executable_sha1']]
    require(len(matches) == 1, 'Missing or ambiguous verified PS2 layout')
    version = matches[0]
    layout = layouts[version]
    committed_dir = ROOT / 'config/platforms' / version
    require(layout == read_json(committed_dir / 'region-layout.json'), 'PS2 layout changed from reviewed registry')
    require(symbols == read_json(committed_dir / 'symbols.json'), 'PS2 symbols differ from verified registry')
    require(symbols['executable_sha1'] == layout['executable_sha1'], 'PS2 original identities differ')
    require(layout['classification_complete'] and baseline.get('version') == 2, 'Incomplete layout or unsupported report')
    regions = {r['name']: r for r in layout['regions']}
    require(set(regions) == {'cpu_text', 'vu_packets', 'initialized_data', 'runtime_bss'}, 'Unexpected PS2 regions')
    cpu = regions['cpu_text']
    require(cpu['classification'] == 'code', 'CPU region is not code')
    profile = read_json(ROOT / 'config/platforms/ps2-toolchain.json')
    from .ps2 import inspect_elf
    from .ps2_source import canonical_linkages, profile_function_key, profile_enabled
    linkages = canonical_linkages(Path(executable).read_bytes(), inspect_elf(Path(executable)))
    profiles = {u['target_unit']: u for u in profile['units']
                if profile_enabled(u, coverage['executable_sha1'])}
    expected, gaps = {}, []
    cursor = cpu['address']
    for row in sorted(symbols['symbols'], key=lambda r: r['address']):
        start, end = row['address'], row['address'] + row['size']
        require(cursor <= start < end <= cpu['address'] + cpu['size'], 'PS2 functions overlap or leave CPU region')
        if cursor < start:
            gaps.append((cursor, start))
        cursor = end
        source = row['source']
        unit_profile = profiles.get(source, {})
        selector = profile_function_key(unit_profile, row, linkages)
        name = unit_profile.get('symbols', {}).get(selector, f"{row['name']}@{start:08x}")
        require((source, name) not in expected, 'Duplicate original PS2 function identity')
        expected[(source, name)] = row
    if cursor < cpu['address'] + cpu['size']:
        gaps.append((cursor, cpu['address'] + cpu['size']))
    missing = sum(end - start for start, end in gaps)
    require(missing + sum(r['size'] for r in symbols['symbols']) == cpu['size'], 'Incomplete CPU byte partition')
    configurations = {u['name']: u for u in config['units']}
    require(len(configurations) == len(config['units']), 'Duplicate PS2 comparison unit')
    units, all_functions, seen = [], [], set()
    for unit in baseline['units']:
        functions = unit.get('functions', [])
        require(unit['name'] in configurations, 'Unknown comparison unit')
        description = configurations[unit['name']]
        require(not unit.get('metadata', {}).get('complete') and
                not any(int(unit['measures'].get(k, 0)) for k in
                        ('complete_code', 'complete_data', 'complete_units', 'matched_data')),
                'Source completion/data matching is outside current PS2 exporter scope')
        for function in functions:
            identity = (unit['name'], function['name'])
            require(identity in expected and identity not in seen, 'Missing or repeated PS2 function')
            seen.add(identity)
            require(int(function['size']) == expected[identity]['size'], 'PS2 function byte count differs')
            if function.get('fuzzy_match_percent', 0):
                require(bool(description.get('base_path')) and (build / description['base_path']).is_file(),
                        'Nonzero comparison without a source object')
        measured = measure_functions(functions)
        for key in ('total_code', 'matched_code', 'total_functions', 'matched_functions'):
            require(int(unit['measures'].get(key, 0)) == int(measured[key]), 'PS2 baseline function sum differs')
        updated = deepcopy(unit)
        updated.pop('sections', None)
        updated['measures'] = measured
        updated['metadata'] = {**updated.get('metadata', {}), 'complete': False,
                               'progress_categories': ['cpu_text', 'known_functions']}
        units.append(updated)
        all_functions.extend(deepcopy(functions))
    require(seen == set(expected), 'Verified PS2 functions missing from comparison')
    for key in ('total_code', 'matched_code', 'total_functions', 'matched_functions'):
        require(int(baseline['measures'].get(key, 0)) == int(measure_functions(all_functions)[key]),
                'PS2 baseline aggregate differs')
    known_units = len(units)
    units.append({'name': 'unresolved/cpu_text', 'functions': [],
                  'measures': measure_functions([], unresolved=missing),
                  'sections': [{'name': 'cpu_text', 'size': str(end - start), 'fuzzy_match_percent': 0.0,
                                'metadata': {'virtual_address': str(start)}} for start, end in gaps],
                  'metadata': {'complete': False, 'auto_generated': True, 'progress_categories': ['cpu_text']}})
    categories = [
        {'id': 'cpu_text', 'name': 'CPU text (including unresolved functions and alignment)',
         'measures': measure_functions(all_functions, unresolved=missing, units=known_units + 1)},
        {'id': 'known_functions', 'name': 'Known functions only (function count incomplete)',
         'measures': measure_functions(all_functions, units=known_units)}]
    for name, title in [('vu_packets', 'VU upload packets (host data storage)'),
                        ('initialized_data', 'Initialized CPU data'), ('runtime_bss', 'Runtime zero-fill data')]:
        region = regions[name]
        require(region['classification'] == ('bss' if name == 'runtime_bss' else 'data'), 'Unexpected data classification')
        measured = measure_functions([], data=region['size'])
        units.append({'name': f'inventory/{name}', 'measures': measured, 'functions': [],
                      'sections': [{'name': name, 'size': str(region['size']), 'fuzzy_match_percent': 0.0,
                                    'metadata': {'virtual_address': str(region['address'])}}],
                      'metadata': {'complete': False, 'auto_generated': True, 'progress_categories': [name]}})
        categories.append({'id': name, 'name': title, 'measures': deepcopy(measured)})
    data_size = sum(regions[name]['size'] for name in ('vu_packets', 'initialized_data', 'runtime_bss'))
    report = {'version': 2, 'units': units, 'categories': categories,
              'measures': measure_functions(all_functions, unresolved=missing, data=data_size, units=len(units))}
    validate_report(report)
    explanation = {'schema_version': 1, 'version': version, 'executable_sha1': layout['executable_sha1'],
                   'scope': 'Verified CPU text, VU packet storage, initialized CPU data and runtime BSS.',
                   'aggregation': 'Report-v2 region coverage adapter over unchanged objdiff per-function scores.',
                   'baseline_report_sha256': hashlib.sha256(baseline_path.read_bytes()).hexdigest(),
                   'text_bytes': cpu['size'], 'known_function_bytes': cpu['size'] - missing,
                   'unresolved_text_bytes': missing, 'known_function_count': len(all_functions),
                   'whole_game_function_count': None, 'function_coverage_complete': False,
                   'retail_link_verified': False,
                   'limitations': ['Function counts include recovered bounds only; unresolved CPU bytes add no functions or matches.',
                                   'CPU text includes original alignment and possible inline literals.',
                                   'VU packets are host storage, not matched R5900 functions.',
                                   'Source comparison uses independently restored relocations; complete retail linking is not established.']}
    for name, document in [('section-report.json', report), ('section-coverage.json', explanation),
                           ('region-layout.json', layout)]:
        (build / name).write_text(json.dumps(document, indent=2) + '\n', encoding='utf-8')
    return explanation
