"""Shared report-v2 math for authenticated section coverage adapters.

Function scores remain authoritative; unresolved section bytes contribute no
matches or fuzzy score. No source-link completion is inferred. Zero-denominator
percentage fields are absent instead of implying progress.
"""
from __future__ import annotations

import math


def measure_functions(functions: list[dict], unresolved: int = 0, data: int = 0, units: int = 1, matched_data: int = 0) -> dict:
    if unresolved < 0 or not 0 <= matched_data <= data or units < 0:
        raise ValueError('Invalid section-report byte counts')
    if any(int(f['size']) <= 0 or not 0 <= f.get('fuzzy_match_percent', 0) <= 100 for f in functions):
        raise ValueError('Invalid section-report function size/score')
    code = sum(int(f['size']) for f in functions) + unresolved
    matched = sum(int(f['size']) for f in functions if f.get('fuzzy_match_percent', 0) == 100)
    matched_functions = sum(f.get('fuzzy_match_percent', 0) == 100 for f in functions)
    result = {'total_code': str(code), 'matched_code': str(matched), 'total_data': str(data),
              'matched_data': str(matched_data), 'total_functions': len(functions), 'matched_functions': matched_functions,
              'complete_code': '0', 'complete_data': '0', 'total_units': units, 'complete_units': 0}
    if code:
        result.update({'fuzzy_match_percent': sum(int(f['size']) * f.get('fuzzy_match_percent', 0) for f in functions) / code,
                       'matched_code_percent': matched / code * 100, 'complete_code_percent': 0.0})
    if data:
        result.update({'matched_data_percent': matched_data / data * 100, 'complete_data_percent': 0.0})
    if functions:
        result['matched_functions_percent'] = matched_functions / len(functions) * 100
    return result


COUNT_FIELDS = ('total_code', 'matched_code', 'total_data', 'matched_data',
                'total_functions', 'matched_functions', 'complete_code', 'complete_data',
                'total_units', 'complete_units')


def validate_report(report: dict) -> None:
    """Validate unit, category and total count/formula consistency."""
    if report.get('version') != 2:
        raise ValueError('Expected report v2')

    def check(measures: dict, functions: list[dict], units: int) -> None:
        code = int(measures['total_code'])
        unresolved = code - sum(int(f['size']) for f in functions)
        expected = measure_functions(functions, unresolved, int(measures['total_data']),
                                     units, int(measures['matched_data']))
        for key in COUNT_FIELDS:
            if int(measures.get(key, 0)) != int(expected[key]):
                raise ValueError(f'Inconsistent section-report {key}')
        for key in ('fuzzy_match_percent', 'matched_code_percent', 'matched_data_percent',
                    'matched_functions_percent', 'complete_code_percent', 'complete_data_percent'):
            if key not in expected:
                if key in measures:
                    raise ValueError(f'Zero-denominator section-report {key}')
            elif not math.isclose(measures.get(key, -1), expected[key], rel_tol=1e-7, abs_tol=1e-10):
                raise ValueError(f'Inconsistent section-report {key}')

    for unit in report['units']:
        if unit.get('metadata', {}).get('complete'):
            raise ValueError('Section coverage does not establish source-link completion')
        check(unit['measures'], unit.get('functions', []), 1)
    for measures, members in [(report['measures'], report['units'])] + [
            (category['measures'], [u for u in report['units'] if category['id'] in
                                   u.get('metadata', {}).get('progress_categories', [])])
            for category in report.get('categories', [])]:
        for key in COUNT_FIELDS:
            if int(measures.get(key, 0)) != sum(int(u['measures'].get(key, 0)) for u in members):
                raise ValueError(f'Section-report aggregate {key} differs from its units')
        check(measures, [f for u in members for f in u.get('functions', [])], len(members))
