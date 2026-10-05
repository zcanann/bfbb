"""Recover data extents only from explicit retail DWARF type byte sizes.

Fundamental types, pointers and array bounds are deliberately not inferred here.
Conflicting declarations stay unresolved, including alternate external views of
one object. This registry provides layout evidence, not matched-data progress.
"""
from collections import Counter
import json
from pathlib import Path


def write_extents(anchors: list[dict], declarations: dict, types: dict,
                  metadata: dict, functions: list[dict], output: Path) -> dict:
    extents, conflicts = [], []
    skipped = Counter()
    segments = [s for s in metadata['segments'] if s['type'] == 1 and s['memory_size']]
    for anchor in anchors:
        if anchor['kind'] != 'data_address':
            continue
        evidence = []
        for reference in anchor['references']:
            attrs = declarations[reference['die_offset']]
            type_offset = attrs.get(7)  # AT_user_def_type
            if type_offset is None:
                continue
            tag, size = types[type_offset]
            # Only concrete class/struct/union/enum types with AT_byte_size.
            if tag not in (2, 4, 0x13, 0x17) or size is None:
                continue
            if not isinstance(size, int) or size <= 0:
                continue
            evidence.append({'die_offset': reference['die_offset'],
                             'type_die_offset': type_offset, 'type_tag': tag,
                             'byte_size': size})
        identity = {k: anchor[k] for k in ('name', 'address', 'linkage_name')}
        sizes = sorted({entry['byte_size'] for entry in evidence})
        if not sizes:
            skipped['no_explicit_concrete_type_size'] += 1
            continue
        if len(sizes) != 1:
            conflicts.append({**identity, 'reason': 'Declarations disagree on type size',
                              'candidate_sizes': sizes, 'evidence': evidence})
            continue
        address, size = anchor['address'], sizes[0]
        owners = [s for s in segments if s['address'] <= address and
                  address + size <= s['address'] + s['memory_size']]
        if len(owners) != 1:
            raise ValueError('Explicit data extent lacks a unique loaded-memory owner')
        if any(address < f['high'] and f['low'] < address + size for f in functions):
            raise ValueError('Explicit data extent overlaps a known function')
        segment = owners[0]
        file_end = segment['address'] + segment['file_size']
        if address < file_end < address + size:
            raise ValueError('Explicit data extent crosses file-backed/zero-fill boundary')
        backed = address < file_end
        extents.append({**identity, 'size': size,
                        'storage': 'file_backed' if backed else 'zero_fill',
                        'file_offset': segment['offset'] + address - segment['address'] if backed else None,
                        'evidence': evidence})
    # Identical aliases are permissible, but no distinct overlapping range is
    # silently counted twice or assigned ownership by declaration order.
    ranges = sorted({(e['address'], e['size'], e['storage']) for e in extents})
    if any(a[0] + a[1] > b[0] for a, b in zip(ranges, ranges[1:])):
        raise ValueError('Distinct explicit data extents overlap')
    document = {
        'schema_version': 1, 'executable_sha1': metadata['sha1'],
        'provenance': 'Absolute DWARF declarations referencing concrete types with explicit AT_byte_size',
        'coverage_complete': False, 'eligible_for_progress': False,
        'definition_ownership_recovered': False,
        'limitations': ['References establish declared object extents, not definition TU ownership.',
                       'Fundamental, modified, pointer and array sizes are not inferred.',
                       'Conflicting declarations remain unresolved and excluded from measured extents.',
                       'Counts describe recovered layout; no source data matching or link is claimed.'],
        'counts': {'symbols': len(extents), 'unique_ranges': len(ranges),
                   'file_backed_bytes': sum(size for _, size, storage in ranges if storage == 'file_backed'),
                   'zero_fill_bytes': sum(size for _, size, storage in ranges if storage == 'zero_fill'),
                   'conflicting_symbols': len(conflicts)},
        'skipped': dict(sorted(skipped.items())),
        'extents': extents, 'conflicts': conflicts,
    }
    output.write_text(json.dumps(document, indent=2) + '\n', encoding='utf-8')
    return document
