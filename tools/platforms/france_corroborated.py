#!/usr/bin/env python3
"""Corroborate France exact-byte candidates with independent static entry paths.

Run with --orig-dir orig, then use --check to regenerate and compare the registry.
This authenticates all four originals, rechecks named DWARF identities and unique
whole-body equality, and requires a JAL entry witness reached from the ELF entry.
Calls may return; conditional paths are statically possible, not observed runtime
execution. Unresolved indirect jumps stop. Function-local checks require closed
branches, valid delay slots, a final return, balanced SP paths, recognizable RA
save/load slots for calling functions, and zero-only unreachable/alignment words.

The machine-corroborated registry remains separate from manually reviewed ranges.
Neither byte equality nor these bounds assert source matching, complete coverage,
or a retail link. This command never writes symbols.json or report artifacts.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict, deque
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from platforms.verify_reviewed import Original, REFERENCES, TARGET, require

NORMAL = {8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 28, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 49, 51, 54, 55, 57, 62, 63}
CONTROL = {'jump', 'call', 'branch', 'return', 'indirect_jump', 'indirect_call'}

class ControlFlow:
    """Conservative R5900 control-flow decoding; unresolved transfers stop."""

    def __init__(self, words):
        self.words = words

    def instruction(self, pc):
        """Classify control transfers; arithmetic categories are not a full ISA validator."""
        w = self.words.get(pc)
        if w is None:
            return {'kind': 'invalid'}
        op = w >> 26
        rs = w >> 21 & 31
        rt = w >> 16 & 31
        fn = w & 63
        imm = (w & 65535) - (65536 if w & 32768 else 0)
        d = {'kind': 'normal', 'word': w, 'op': op, 'rs': rs, 'rt': rt, 'imm': imm}
        if op in (2, 3):
            d.update(kind='jump' if op == 2 else 'call', target=((pc + 4) & 0xf0000000) | ((w & 0x3ffffff) << 2))
        elif op in (4, 5, 6, 7, 20, 21, 22, 23):
            d.update(kind='branch', target=pc + 4 + 4 * imm, likely=op >= 20)
        elif op == 1:
            if rt in (0, 1, 2, 3, 16, 17, 18, 19):
                d.update(kind='branch', target=pc + 4 + 4 * imm, likely=rt in (2, 3, 18, 19), link=rt >= 16)
            elif rt not in (8, 9, 10, 11, 12, 14, 24, 25):
                d['kind'] = 'invalid'
        elif op == 0:
            if fn == 8:
                d.update(kind='return' if rs == 31 else 'indirect_jump')
            elif fn == 9:
                d.update(kind='indirect_call' if w >> 11 & 31 == 31 else 'indirect_jump')
            elif fn == 13:
                d.update(kind='trap')
            elif fn not in {0, 2, 3, 4, 6, 7, 10, 11, 12, 15, 16, 17, 18, 19, 20, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 54, 56, 58, 59, 60, 62, 63}:
                d['kind'] = 'invalid'
        elif op in (16, 17, 18):
            if rs == 8:
                d.update(kind='branch', target=pc + 4 + 4 * imm, likely=bool(rt & 2))
            elif op == 16 and rs == 16 and (fn in (24, 31)):
                d['kind'] = 'indirect_jump'
            elif rs not in (0, 1, 2, 4, 5, 6) and rs < 16:
                d['kind'] = 'invalid'
        elif op not in NORMAL:
            d['kind'] = 'invalid'
        if d['kind'] == 'branch':
            if op in (4, 20) and rs == rt:
                d['taken'] = True
            elif op in (5, 21) and rs == rt:
                d['taken'] = False
            elif op in (6, 22) and rs == 0:
                d['taken'] = True
            elif op in (7, 23) and rs == 0:
                d['taken'] = False
            elif op == 1 and rs == 0:
                d['taken'] = bool(rt & 1)
        return d

    def valid_delay(self, pc):
        """Do not follow transfers with a trapping or control-transfer delay slot."""
        return self.instruction(pc + 4)['kind'] in ('normal',)

    def successors(self, pc, d):
        k = d['kind']
        if k == 'normal':
            return [pc + 4]
        if k == 'jump':
            return [d['target']] if self.valid_delay(pc) else []
        if k in ('call', 'indirect_call'):
            return ([d['target'], pc + 8] if k == 'call' else [pc + 8]) if self.valid_delay(pc) else []
        if k == 'branch':
            if not self.valid_delay(pc):
                # Likely branches annul BREAK on their non-taken path.
                return [pc + 8] if d.get('likely') and d.get('taken') is not True and (self.instruction(pc + 4)['kind'] == 'trap') else []
            return ([d['target']] if d.get('taken') is not False else []) + ([pc + 8] if d.get('taken') is not True or d.get('link') else [])
        return []

    def bounds(self, start, size, *, resolved_indirect_jumps=None):
        """Check independently identified bounds, optionally following proven tables.

        A caller supplying resolved_indirect_jumps must first prove the actual
        index bound, table address, load/JR dataflow and complete successor set.
        Ordinary callers still stop at every unresolved indirect jump.
        """
        end = start + size
        resolved_indirect_jumps = resolved_indirect_jumps or {}
        for pc, destinations in resolved_indirect_jumps.items():
            require(start <= pc < end and self.instruction(pc)['kind'] == 'indirect_jump' and
                    self.valid_delay(pc) and destinations and
                    all(start <= target < end and target % 4 == 0 for target in destinations),
                    'Invalid explicitly resolved local dispatch')
        todo = deque([(start, 0)])
        states = {}
        covered = set()
        reasons = set()
        returns = []
        calls = []
        branches = []
        saves = []
        loads = []
        allocations = []

        def step(pc, sp):
            d = self.instruction(pc)
            w = d.get('word', 0)
            op = d.get('op')
            rs = d.get('rs')
            rt = d.get('rt')
            imm = d.get('imm', 0)
            if op in (9, 25) and rs == rt == 29:
                sp += imm
                allocations.append((pc, imm))
            elif op in (8, 10, 11, 12, 13, 14, 15, 24, 26, 27, 30, 32, 33, 34, 35, 36, 37, 38, 39, 55) and rt == 29 or (op == 0 and w >> 11 & 31 == 29 and (w & 63 not in (8, 9, 12, 13, 15, 17, 19, 24, 25, 26, 27))):
                reasons.add('unmodeled_sp_write')
            elif (op == 28 and (w >> 11) & 31 == 29 or
                  op in (16, 17, 18) and rs in (0, 1, 2) and rt == 29):
                reasons.add('unmodeled_sp_write')
            if rs == 29 and rt == 31:
                if op in (43, 63):
                    saves.append((pc, sp + imm))
                if op in (35, 55):
                    loads.append((pc, sp + imm))
            return sp
        while todo:
            (pc, sp) = todo.popleft()
            if not start <= pc < end:
                reasons.add('local_edge_outside_extent')
                continue
            if pc in states:
                if states[pc] != sp:
                    reasons.add('inconsistent_sp_at_join')
                continue
            states[pc] = sp
            covered.add(pc)
            d = self.instruction(pc)
            kind = d['kind']
            if kind in ('invalid', 'trap'):
                reasons.add('undecoded_or_trapping_instruction')
                continue
            if kind == 'indirect_jump':
                if pc in resolved_indirect_jumps:
                    covered.add(pc + 4)
                    after = step(pc + 4, sp)
                    for target in resolved_indirect_jumps[pc]:
                        branches.append((pc, target))
                        todo.append((target, after))
                else:
                    reasons.add('unresolved_indirect_jump')
                continue
            if kind == 'normal':
                todo.append((pc + 4, step(pc, sp)))
                continue
            if pc + 4 >= end or not self.valid_delay(pc):
                reasons.add('invalid_delay_slot')
                continue
            covered.add(pc + 4)
            after = step(pc + 4, sp)
            if kind == 'return':
                returns.append(pc)
                if after != 0:
                    reasons.add('return_sp_not_balanced')
            elif kind in ('call', 'indirect_call'):
                calls.append(pc)
                todo.append((pc + 8, after))
            elif kind == 'jump':
                todo.append((d['target'], after))
            elif kind == 'branch':
                if d.get('link'):
                    reasons.add('conditional_link_branch')
                branches.append((pc, d['target']))
                if d.get('taken') is not False:
                    todo.append((d['target'], after))
                if d.get('taken') is not True:
                    todo.append((pc + 8, sp if d.get('likely') else after))
        if not returns:
            reasons.add('no_return')
        if end - 8 not in returns:
            reasons.add('extent_not_terminal_return_delay')
        uncovered = [p for p in range(start, end, 4) if p not in covered]
        if any((self.words[p] != 0 for p in uncovered)):
            reasons.add('unreachable_nonpadding_words')
        if calls:
            slots = {slot for (_, slot) in saves} & {slot for (_, slot) in loads}
            if not slots:
                reasons.add('calling_function_without_ra_save_restore')
        if any((not start <= target < end for (_, target) in branches)):
            reasons.add('external_conditional_branch')
        pad = -end % 16
        if any((self.words.get(p) != 0 for p in range(end, end + pad, 4))):
            reasons.add('following_alignment_not_zero')
        return {'passes': not reasons, 'reasons': sorted(reasons), 'returns': sorted(set(returns)), 'direct_or_indirect_calls': sorted(set(calls)), 'branches': [list(x) for x in sorted(set(branches))], 'stack_adjustments': [list(x) for x in sorted(set(allocations))], 'ra_saves': [list(x) for x in sorted(set(saves))], 'ra_loads': [list(x) for x in sorted(set(loads))], 'unreachable_zero_words': uncovered, 'following_zero_alignment': pad}

def rooted_graph(flow: ControlFlow, entry: int) -> tuple[dict, dict, dict]:
    """Return first-path predecessors and direct JALs in an entry-rooted may-CFG."""
    predecessors = {entry: None}
    pending = deque([entry])
    calls = defaultdict(list)
    stops = Counter()
    while pending:
        pc = pending.popleft()
        decoded = flow.instruction(pc)
        if decoded['kind'] == 'call' and flow.valid_delay(pc):
            calls[decoded['target']].append(pc)
        successors = flow.successors(pc, decoded)
        if not successors:
            stops[decoded['kind']] += 1
        for address in successors:
            if address not in flow.words:
                stops['unmapped_transfer'] += 1
            elif address not in predecessors:
                predecessors[address] = pc
                pending.append(address)
    return predecessors, calls, dict(sorted(stops.items()))


def witness_path(predecessors: dict, address: int) -> list[int]:
    path = []
    while address is not None:
        path.append(address)
        address = predecessors[address]
    return list(reversed(path))


def verify_candidates(registry: dict, originals: dict) -> None:
    """Recheck all identities, including omitted/conflicting reference aliases."""
    target = originals[TARGET]
    require(registry['version'] == TARGET and registry['executable_sha1'] == target.sha1,
            'Candidate original identity differs')
    references = defaultdict(list)
    for version in REFERENCES:
        original = originals[version]
        for function in original.functions:
            size = function['high'] - function['low']
            if size >= 32:
                body = original.read(function['low'], size)
                references[(size, hashlib.sha256(body).hexdigest())].append((version, function))
    candidates = registry['candidates']
    require(registry['candidate_count'] == len(candidates) and
            registry['candidate_byte_count'] == sum(c['size'] for c in candidates),
            'Candidate registry totals differ')
    previous_end = 0
    for candidate in candidates:
        address = int(candidate['french_address'], 16)
        size = candidate['size']
        require(address >= previous_end and address % 4 == size % 4 == 0 and size >= 32,
                'Candidates overlap, are unordered, or have invalid extents')
        previous_end = address + size
        body = target.read(address, size)
        digest = hashlib.sha256(body).hexdigest()
        require(digest == candidate['exact_bytes_sha256'], 'Candidate whole-body hash differs')
        occurrences = []
        for segment in target.loaded:
            span = target.read(segment['address'], segment['file_size'])
            offset = span.find(body)
            while offset >= 0:
                occurrences.append(segment['address'] + offset)
                offset = span.find(body, offset + 1)
        require(occurrences == [address], 'Candidate bytes are not unique in the loaded image')
        matches = references[(size, digest)]
        require(matches and {(f['name'], f['source']) for _, f in matches} ==
                {(candidate['name'], candidate['source'])}, 'Conflicting named reference identity')
        recorded = {(p['version'], int(p['source_address'], 16), p['name'], p['source'])
                    for p in candidate['provenance']}
        actual = {(v, f['low'], f['name'], f['source']) for v, f in matches}
        require(recorded == actual and len(recorded) == len(candidate['provenance']),
                'Candidate provenance omits or duplicates a matching named reference')
        for version, function in matches:
            require(originals[version].read(function['low'], size) == body,
                    'Candidate differs from the complete named reference body')


def generate(manifest: Path, orig_dir: Path, candidate_path: Path, manual_path: Path) -> dict:
    versions = json.loads(manifest.read_text(encoding='utf-8'))['versions']
    originals = {v: Original(v, versions[v], orig_dir) for v in (TARGET, *REFERENCES)}
    target = originals[TARGET]
    registry = json.loads(candidate_path.read_text(encoding='utf-8'))
    verify_candidates(registry, originals)
    manual = json.loads(manual_path.read_text(encoding='utf-8'))
    require(manual['version'] == TARGET and manual['executable_sha1'] == target.sha1,
            'Manual registry original identity differs')
    for entry in manual['functions']:
        require(hashlib.sha256(target.read(entry['address'], entry['size'])).hexdigest() == entry['sha256']
                and entry['boundary_confirmation'] is True, 'Manual range hash/confirmation differs')
    words = {s['address'] + index * 4: word for s in target.loaded
             for index, (word,) in enumerate(struct.iter_unpack('<I', target.read(s['address'], s['file_size'])))}
    flow = ControlFlow(words)
    entry = target.metadata['entry_point']
    predecessors, calls, stops = rooted_graph(flow, entry)
    functions = []
    exclusions = []
    manual_duplicates = []
    reasons = Counter()
    witness_count = 0
    bounded_count = 0
    for candidate in registry['candidates']:
        address, size = int(candidate['french_address'], 16), candidate['size']
        bounds = flow.bounds(address, size)
        witnesses = calls[address]
        witness_count += bool(witnesses)
        bounded_count += bounds['passes']
        failed = list(bounds['reasons'])
        if not witnesses:
            failed.append('no_entry_rooted_jal_witness')
        if failed:
            for reason in failed:
                reasons[reason] += 1
            exclusions.append({'name': candidate['name'], 'source': candidate['source'],
                               'address': address, 'size': size, 'reasons': failed})
            continue
        overlaps = [f for f in manual['functions']
                    if address < f['address'] + f['size'] and f['address'] < address + size]
        if overlaps:
            require(len(overlaps) == 1 and overlaps[0]['address'] == address and
                    overlaps[0]['size'] == size and overlaps[0]['name'] == candidate['name'] and
                    overlaps[0]['source'] == candidate['source'], 'Machine/manual ranges conflict')
            manual_duplicates.append({'name': candidate['name'], 'source': candidate['source'],
                                      'address': address, 'size': size})
            continue
        caller = witnesses[0]
        path = witness_path(predecessors, caller)
        path_counts = Counter(flow.instruction(pc)['kind'] for pc in path)
        proofs = []
        for proof in candidate['provenance']:
            proofs.append({**proof, 'source_address': int(proof['source_address'], 16),
                           'executable_sha1': originals[proof['version']].sha1,
                           'reference_sha256': candidate['exact_bytes_sha256']})
        functions.append({
            'name': candidate['name'], 'source': candidate['source'], 'address': address, 'size': size,
            'sha256': candidate['exact_bytes_sha256'], 'boundary_confirmation': True,
            'confirmation_kind': 'machine-corroborated-static-cfg',
            'confirmation_method': 'Unique complete named-DWARF body equality plus an entry-rooted direct JAL witness and closed local control-flow/frame/return/alignment checks.',
            'provenance': proofs,
            'corroboration': {
                'entry_witness': {
                    'entry_point': entry, 'call_site': caller, 'target': address,
                    'path_instruction_count': len(path),
                    'path_sha256': hashlib.sha256(b''.join(struct.pack('<I', pc) for pc in path)).hexdigest(),
                    'path_instruction_kinds': dict(sorted(path_counts.items())),
                    'scope': 'Static may-reach path with returning-call continuations; no runtime execution claim.'},
                'direct_entry_calls': sorted(witnesses), 'local_control_flow': bounds,
            },
        })
    return {
        'schema_version': 1, 'version': TARGET, 'executable_sha1': target.sha1,
        'status': 'machine-corroborated-named-extents', 'coverage_complete': False,
        'source_comparison_available': False, 'source_link_verified': False,
        'method': 'Authenticated full-reference byte equality, unique identity, rooted JAL entry and closed local CFG; deterministic regeneration required.',
        'limitations': [
            'Coverage is partial and excludes unresolved indirect entry paths and uncertain local bounds.',
            'Entry paths are static may-reach witnesses, with calls permitted to return; no execution is claimed.',
            'Instruction decoding models control flow and SP adjustments, not complete program semantics.',
            'RA checks establish recognizable saved/restored stack slots, not a general ABI proof.',
            'Names and source identities come from reference DWARF, not a recovered France symbol table.',
            'Identical bytes and corroborated extents do not establish source matching or retail relinking.',
            'Manually reviewed identical ranges are excluded to avoid double counting.'],
        'original_sha1s': {v: originals[v].sha1 for v in (TARGET, *REFERENCES)},
        'counts': {'input_candidates': len(registry['candidates']), 'locally_bounded': bounded_count,
                   'rooted_jal_candidates': witness_count, 'manual_duplicates': len(manual_duplicates),
                   'functions': len(functions), 'code_bytes': sum(f['size'] for f in functions),
                   'source_units': len({f['source'] for f in functions}),
                   'reference_multiplicity': dict(sorted(Counter(str(len(f['provenance'])) for f in functions).items()))},
        'entry_cfg': {'entry_point': entry, 'reachable_instruction_heads': len(predecessors),
                      'stop_reasons': stops},
        'exclusion_counts': dict(sorted(reasons.items())), 'manual_duplicates': manual_duplicates,
        'functions': functions, 'excluded_candidates': exclusions,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, default=ROOT / 'config/platforms/versions.json')
    parser.add_argument('--orig-dir', type=Path, default=ROOT / 'orig')
    parser.add_argument('--candidates', type=Path, default=ROOT / 'config/platforms/SLES-53623/symbol-candidates.json')
    parser.add_argument('--manual', type=Path, default=ROOT / 'config/platforms/SLES-53623/reviewed-functions.json')
    parser.add_argument('--output', type=Path, default=ROOT / 'config/platforms/SLES-53623/corroborated-functions.json')
    parser.add_argument('--check', action='store_true', help='Recompute all original evidence and require identical metadata')
    args = parser.parse_args()
    try:
        document = generate(args.manifest, args.orig_dir, args.candidates, args.manual)
        if args.check:
            require(json.loads(args.output.read_text(encoding='utf-8')) == document,
                    'Corroborated registry differs; inspect evidence before explicit regeneration')
        else:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(json.dumps(document, indent=2) + '\n', encoding='utf-8')
        counts = document['counts']
        print(f"{'Verified' if args.check else 'Wrote'} {counts['functions']} machine-corroborated France extents "
              f"({counts['code_bytes']} bytes); {counts['manual_duplicates']} manual duplicates excluded; "
              'partial named coverage, no source-match or retail-link claim.')
    except (OSError, ValueError, KeyError, TypeError, struct.error) as error:
        parser.exit(1, f'error: {error}\n')


if __name__ == '__main__':
    main()
