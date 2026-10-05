"""Recover absolute-address PS2 DWARF declarations without guessing object sizes.

A global-variable DIE can describe a function declaration in these MW binaries.
Only an unmodified subroutine type establishes that classification. Source files
are declaration references, not asserted definitions or translation-unit owners.
"""
from __future__ import annotations

from bisect import bisect_right
from collections import Counter
import json
from pathlib import Path

from .dwarf1 import iter_dies


def _source(path: str) -> str:
    path = path.replace('\\', '/')
    if len(path) > 2 and path[1:3] == ':/':
        path = path[3:]
    return path.lstrip('/')


def write_anchors(binary: bytes, metadata: dict, functions: list[dict], output: Path) -> dict:
    debug_sections = [s for s in metadata['sections'] if s['name'] == '.debug' and s['size']]
    if len(debug_sections) > 1:
        raise ValueError('Ambiguous PS2 debug section')
    type_tags, type_sizes, declarations = {}, {}, []
    if debug_sections:
        section = debug_sections[0]
        debug = binary[section['offset']:section['offset'] + section['size']]
        for offset, tag, source, attrs in iter_dies(debug):
            type_tags[offset] = tag
            type_sizes[offset] = (tag, attrs.get(11))
            if tag in (0x07, 0x0c):
                declarations.append((offset, tag, source, attrs))
    starts = [f['low'] for f in functions]
    segments = [s for s in metadata['segments'] if s['type'] == 1 and s['memory_size']]
    anchors, skipped = {}, Counter()
    for offset, tag, source, attrs in declarations:
        location = attrs.get(2)
        # DW_OP_addr with exactly one 32-bit operand. Register/stack locations
        # and composed expressions do not establish a static absolute anchor.
        if not isinstance(location, bytes) or len(location) != 5 or location[0] != 3:
            skipped['non_absolute_location'] += 1
            continue
        address = int.from_bytes(location[1:], 'little')
        owners = [s for s in segments if s['address'] <= address < s['address'] + s['memory_size']]
        if len(owners) != 1:
            skipped['outside_loaded_memory'] += 1
            continue
        name = attrs.get(3)
        if not isinstance(name, str) or not name or not source:
            raise ValueError('Absolute declaration lacks a name or source reference')
        direct_type = attrs.get(7)
        if direct_type is not None and direct_type not in type_tags:
            raise ValueError('Declaration references a missing type DIE')
        is_function = direct_type is not None and type_tags[direct_type] == 0x15
        kind = 'function_entry' if is_function else 'data_address'
        index = bisect_right(starts, address) - 1
        containing = functions[index] if index >= 0 and address < functions[index]['high'] else None
        if containing and (not is_function or address != containing['low']):
            raise ValueError('Declaration conflicts with a proven function interval')
        segment = owners[0]
        backed = address < segment['address'] + segment['file_size']
        if is_function and (not backed or address % 4):
            raise ValueError('Function declaration lacks an aligned file-backed entry')
        linkage = attrs.get(0x200)
        if linkage is not None and not isinstance(linkage, str):
            raise ValueError('Invalid DWARF linkage name')
        key = (address, kind, name, linkage)
        if key not in anchors:
            anchors[key] = {
                'kind': kind, 'name': name, 'address': address,
                'linkage_name': linkage,
                'storage': 'file_backed' if backed else 'zero_fill',
                'file_offset': segment['offset'] + address - segment['address'] if backed else None,
                'extent_known': bool(containing),
                'references': [],
            }
        reference = {'source': _source(source), 'die_offset': offset,
                     'declaration_kind': 'global' if tag == 7 else 'local_or_static'}
        if reference not in anchors[key]['references']:
            anchors[key]['references'].append(reference)
    values = sorted(anchors.values(), key=lambda a: (a['address'], a['kind'], a['name'], a['linkage_name'] or ''))
    for anchor in values:
        anchor['references'].sort(key=lambda r: (r['source'], r['die_offset']))
    document = {
        'schema_version': 1, 'executable_sha1': metadata['sha1'],
        'provenance': 'Retail DWARF1 declarations with a single absolute DW_OP_addr location',
        'coverage_complete': False, 'eligible_for_progress': False,
        'limitations': [
            'Data entries prove addresses, not sizes or complete object ownership.',
            'References identify declaring source files, not necessarily the definition owner.',
            'Function entries without a known extent do not add function bytes or matching progress.',
            'Zero addresses and declarations outside loaded memory are excluded.',
        ],
        'counts': dict(sorted(Counter(a['kind'] for a in values).items())),
        'skipped_declarations': dict(sorted(skipped.items())),
        'anchors': values,
    }
    output.write_text(json.dumps(document, indent=2) + '\n', encoding='utf-8')

    from .ps2_data import write_extents
    return write_extents(values, {offset: attrs for offset, _, _, attrs in declarations},
                  type_sizes, metadata, functions, output.with_name('data-extents.json'))
