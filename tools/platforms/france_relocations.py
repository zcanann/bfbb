#!/usr/bin/env python3
"""Corroborate France extents whose only changed bits encode direct transfers.

Regenerate against authenticated originals, or use --check to compare metadata.
Every changed J/JAL destination must preserve an internal offset or identify an
already trusted named callee. New callees become anchors only in later rounds,
so mutually supporting unresolved cycles cannot prove themselves. Whole bodies,
rooted entry witnesses and local bounds are checked; no source match is claimed.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from platforms.verify_reviewed import Original, REFERENCES, TARGET, require, verify
from platforms.france_corroborated import ControlFlow, rooted_graph, witness_path
from platforms.france_corroborated import generate as generate_exact


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def transfer_target(pc: int, word: int) -> int:
    require(word >> 26 in (2, 3), 'Only J/JAL address fields may be changed')
    return ((pc + 4) & 0xf0000000) | ((word & 0x3ffffff) << 2)


def template_matches(body: bytes, spans: list, cache: dict) -> list:
    words = struct.unpack('<' + 'I' * (len(body) // 4), body)
    fields = {i for i, word in enumerate(words) if word >> 26 in (2, 3)}
    if not fields:
        return []
    normalized = tuple(word & 0xfc000000 if i in fields else word for i, word in enumerate(words))
    if normalized in cache:
        return cache[normalized]
    runs, start = [], 0
    for index in sorted(fields) + [len(words)]:
        if index > start:
            runs.append((index - start, start))
        start = index + 1
    length, offset = max(runs, default=(0, 0))
    if length < 2:
        cache[normalized] = []
        return []
    needle = body[offset * 4:(offset + length) * 4]
    matches, attempts = [], 0
    for segment, span in spans:
        position = span.find(needle)
        while position >= 0:
            address = segment['address'] + position - offset * 4
            attempts += 1
            if attempts > 256:
                cache[normalized] = []
                return []
            if address % 4 == 0 and segment['address'] <= address and address + len(body) <= segment['address'] + segment['file_size']:
                relative = address - segment['address']
                raw = span[relative:relative + len(body)]
                other = struct.unpack('<' + 'I' * len(words), raw)
                if all((a & 0xfc000000) == (b & 0xfc000000) if i in fields else a == b
                       for i, (a, b) in enumerate(zip(words, other))):
                    matches.append((address, raw))
                    if len(matches) > 1:
                        cache[normalized] = []
                        return []
            position = span.find(needle, position + 1)
    cache[normalized] = matches
    return matches


def generate(manifest: Path, orig_dir: Path, registry_dir: Path) -> dict:
    # These named starting anchors are proved independently, not trusted merely
    # because a JSON record supplies a desired function name and byte hash.
    verify(manifest, orig_dir, registry_dir)
    exact = generate_exact(manifest, orig_dir, registry_dir / 'symbol-candidates.json',
                           registry_dir / 'reviewed-functions.json')
    require(exact == json.loads((registry_dir / 'corroborated-functions.json').read_text(encoding='utf-8')),
            'Existing exact-body corroboration differs from regenerated evidence')
    records = json.loads(manifest.read_text(encoding='utf-8'))['versions']
    originals = {v: Original(v, records[v], orig_dir) for v in (TARGET, *REFERENCES)}
    target = originals[TARGET]
    spans = [(s, target.read(s['address'], s['file_size'])) for s in target.loaded]
    words = {s['address'] + i * 4: word for s, body in spans
             for i, (word,) in enumerate(struct.iter_unpack('<I', body))}
    flow = ControlFlow(words)
    predecessors, entry_calls, _ = rooted_graph(flow, target.metadata['entry_point'])
    trusted = {}
    for filename, key in (('reviewed-functions.json', 'functions'),
                          ('corroborated-functions.json', 'functions'),
                          ('reviewed-call-targets.json', 'anchors')):
        for entry in json.loads((registry_dir / filename).read_text(encoding='utf-8'))[key]:
            identity = (entry['name'], entry['source'], entry['size'])
            require(entry['address'] not in trusted, 'Starting named anchors overlap by address')
            trusted[entry['address']] = {'identity': identity, 'registry': filename, 'round': 0}
    starting_count = len(trusted)
    raw_exact = defaultdict(set)
    for entry in json.loads((registry_dir / 'symbol-candidates.json').read_text(encoding='utf-8'))['candidates']:
        raw_exact[int(entry['french_address'], 16)].add((entry['name'], entry['source'], entry['size']))
    for address, entry in trusted.items():
        raw_exact[address].add(entry['identity'])
    found, cache = defaultdict(list), {}
    for version in REFERENCES:
        original = originals[version]
        for function in original.functions:
            size = function['high'] - function['low']
            if size < 32:
                continue
            reference_body = original.read(function['low'], size)
            matches = template_matches(reference_body, spans, cache)
            if len(matches) != 1:
                continue
            address, body = matches[0]
            changes = []
            reconstructed = bytearray(reference_body)
            for offset in range(0, size, 4):
                a = struct.unpack_from('<I', reference_body, offset)[0]
                b = struct.unpack_from('<I', body, offset)[0]
                if a == b:
                    continue
                require(a >> 26 == b >> 26 and a >> 26 in (2, 3), 'A non-transfer field changed')
                reference_target = transfer_target(function['low'] + offset, a)
                french_target = transfer_target(address + offset, b)
                struct.pack_into('<I', reconstructed, offset, (a & 0xfc000000) | ((french_target >> 2) & 0x3ffffff))
                named = original.by_address.get(reference_target)
                changes.append({'offset': offset, 'opcode': a >> 26,
                                'reference_target': reference_target, 'french_target': french_target,
                                'reference_callee': ({'name': named['name'], 'source': named['source'],
                                                      'size': named['high'] - named['low']} if named else None)})
            require(bytes(reconstructed) == body, 'Explicit transfer relocations do not reproduce every body byte')
            found[(address, size)].append({'version': version, 'executable_sha1': original.sha1,
                                          'name': function['name'], 'source': function['source'],
                                          'source_address': function['low'], 'reference_sha256': digest(reference_body),
                                          'transfer_differences': changes})
    matches = []
    ambiguous = 0
    for (address, size), proofs in sorted(found.items()):
        identities = {(p['name'], p['source']) for p in proofs}
        if len(identities) != 1:
            ambiguous += 1
            continue
        name, source = next(iter(identities))
        matches.append({'name': name, 'source': source, 'address': address, 'size': size,
                        'sha256': digest(target.read(address, size)), 'provenance': proofs})
    possible, excluded = [], Counter()
    for entry in matches:
        address, size = entry['address'], entry['size']
        if (entry['name'], entry['source'], size) in raw_exact[address]:
            excluded['already_exact_or_reviewed'] += 1
            continue
        bounds = flow.bounds(address, size)
        if not bounds['passes'] or not entry_calls[address]:
            excluded['unconfirmed_entry_or_local_bounds'] += 1
            continue
        other_ranges = [(f['address'], f['size']) for f in matches if f is not entry]
        other_ranges += [(a, f['identity'][2]) for a, f in trusted.items()]
        if any(address < a + s and a < address + size for a, s in other_ranges):
            excluded['overlapping_candidate'] += 1
            continue
        entry['corroboration'] = {'direct_entry_calls': sorted(entry_calls[address]), 'local_control_flow': bounds}
        possible.append(entry)
    def relations(entry):
        evidence = []
        for proof in entry['provenance']:
            version_evidence = []
            for change in proof['transfer_differences']:
                reference_target, french_target = change['reference_target'], change['french_target']
                if proof['source_address'] <= reference_target < proof['source_address'] + entry['size']:
                    if french_target != entry['address'] + reference_target - proof['source_address']:
                        return None
                    version_evidence.append({'offset': change['offset'], 'kind': 'same_internal_offset'})
                else:
                    named = change['reference_callee']
                    anchor = trusted.get(french_target)
                    if named is None or anchor is None or anchor['identity'] != (named['name'], named['source'], named['size']):
                        return None
                    version_evidence.append({'offset': change['offset'], 'kind': 'independently_named_callee',
                                             'anchor_registry': anchor['registry'], 'anchor_round': anchor['round']})
            evidence.append({'version': proof['version'], 'transfers': version_evidence})
        return evidence
    functions, rounds = [], []
    remaining = possible
    while True:
        batch = [(entry, relations(entry)) for entry in remaining]
        batch = [(entry, evidence) for entry, evidence in batch if evidence is not None]
        if not batch:
            break
        round_number = len(rounds) + 1
        for entry, evidence in batch:
            path = witness_path(predecessors, entry['corroboration']['direct_entry_calls'][0])
            entry['boundary_confirmation'] = True
            entry['confirmation_kind'] = 'machine-corroborated-explicit-transfers'
            entry['confirmation_round'] = round_number
            entry['corroboration']['acyclic_callee_anchors'] = evidence
            entry['corroboration']['entry_witness'] = {
                'entry_point': target.metadata['entry_point'], 'call_site': path[-1],
                'path_instruction_count': len(path),
                'path_sha256': digest(b''.join(struct.pack('<I', pc) for pc in path)),
                'scope': 'Static may-reach with returning calls, not observed execution.'}
            functions.append(entry)
        # Only after the complete batch was checked may its members anchor the
        # following round. Same-round cycles never bootstrap one another.
        for entry, _ in batch:
            trusted[entry['address']] = {'identity': (entry['name'], entry['source'], entry['size']),
                                        'registry': 'relocation-corroborated-functions.json', 'round': round_number}
        rounds.append({'round': round_number, 'functions': len(batch),
                       'code_bytes': sum(entry['size'] for entry, _ in batch)})
        selected = {entry['address'] for entry, _ in batch}
        remaining = [entry for entry in remaining if entry['address'] not in selected]
    functions.sort(key=lambda f: f['address'])
    return {
        'schema_version': 1, 'version': TARGET, 'executable_sha1': target.sha1,
        'status': 'machine-corroborated-explicit-transfer-extents', 'coverage_complete': False,
        'source_comparison_available': False, 'source_link_verified': False,
        'limitations': ['Only direct J/JAL destination bits may differ; every other bit and the complete extent agree.',
                       'Internal transfers retain exactly the same offset; external destinations require independently named anchors.',
                       'Named anchor rounds are acyclic; unresolved or mutually dependent identities stay excluded.',
                       'Static may-reach callers and local frame/return checks corroborate boundaries, not runtime execution.',
                       'This registry adds partial named extents, not matched source, recovered original relocations, or a retail link.'],
        'original_sha1s': {v: originals[v].sha1 for v in (TARGET, *REFERENCES)},
        'counts': {'starting_named_anchors': starting_count, 'unambiguous_template_matches': len(matches),
                   'ambiguous_identities': ambiguous, 'rooted_bounded_candidates': len(possible),
                   'unresolved_callee_candidates': len(remaining), 'functions': len(functions),
                   'code_bytes': sum(f['size'] for f in functions),
                   'source_units': len({f['source'] for f in functions})},
        'excluded_counts': dict(sorted(excluded.items())), 'anchor_rounds': rounds, 'functions': functions,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, default=ROOT / 'config/platforms/versions.json')
    parser.add_argument('--orig-dir', type=Path, default=ROOT / 'orig')
    parser.add_argument('--registry-dir', type=Path, default=ROOT / 'config/platforms/SLES-53623')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    output = args.output or args.registry_dir / 'relocation-corroborated-functions.json'
    try:
        document = generate(args.manifest, args.orig_dir, args.registry_dir)
        if args.check:
            require(json.loads(output.read_text(encoding='utf-8')) == document,
                    'Relocation corroboration differs; inspect original evidence before regeneration')
        else:
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_text(json.dumps(document, indent=2) + '\n', encoding='utf-8')
        counts = document['counts']
        print(f"{'Verified' if args.check else 'Wrote'} {counts['functions']} relocation-corroborated France extents "
              f"({counts['code_bytes']} bytes), {len(document['anchor_rounds'])} acyclic rounds; no source-match claim.")
    except (OSError, ValueError, KeyError, TypeError, struct.error) as error:
        parser.exit(1, f'error: {error}\n')


if __name__ == '__main__':
    main()
