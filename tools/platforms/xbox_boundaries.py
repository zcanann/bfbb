"""Recheck conservative anonymous Xbox function bounds against original bytes.

Ghidra proposes entry/range seeds. Capstone independently follows each CFG and
checks direct caller evidence. These are anonymous machine-code extents, not
original symbols, recovered source identities, or translation-unit splits.
"""
from __future__ import annotations

import argparse
from bisect import bisect_right
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from platforms.xbox import inspect_xbe

CAPSTONE_VERSION = '5.0.7'
METHOD = 'closed-contiguous-direct-caller-v1'
REGISTRY = 'verified-anonymous-functions.json'
VERSIONS = ('XBOX-US', 'XBOX-EU')


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def metadata_hash(data: bytes) -> str:
    # Git newline conversion must not change proof identity across Windows/Linux.
    canonical = json.dumps(json.loads(data), sort_keys=True, separators=(',', ':')).encode('utf-8')
    return hashlib.sha256(canonical).hexdigest()


def generate(version: str, orig_dir: Path, config_dir: Path) -> dict:
    import capstone as cs
    from capstone.x86 import X86_OP_IMM

    require(cs.__version__ == CAPSTONE_VERSION, 'Capstone 5.0.7 is required')
    version_record = json.loads((config_dir / 'versions.json').read_text())['versions'][version]
    executable = orig_dir / version / Path(version_record['executable']['path']).name
    original = executable.read_bytes()
    original_hash = hashlib.sha1(original).hexdigest()
    require(original_hash == version_record['executable']['sha1'], 'Original executable hash differs')
    metadata = inspect_xbe(executable)
    require(metadata['sha1'] == original_hash, 'Original changed while being inspected')
    section = next(s for s in metadata['sections'] if s['name'] == '.text')
    base = section['virtual_address']
    text = original[section['raw_offset']:section['raw_offset'] + section['raw_size']]
    directory = config_dir / version
    candidate_path = directory / 'symbol-candidates.json'
    candidate_bytes = candidate_path.read_bytes()
    candidates = json.loads(candidate_bytes)
    require(candidates['executable_sha1'] == original_hash, 'Candidates target another original')
    provenance = candidates['provenance']
    require(provenance['tool'] == 'Ghidra' and provenance['version'] == '11.0.1_PUBLIC',
            'Unexpected candidate analyzer/version')
    require(provenance['analysis_completed'] and not provenance['analysis_timeout_occurred'],
            'Candidate analysis did not finish')
    decoder = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
    decoder.detail = True
    rows = {}
    transfers = []
    all_ranges = []
    for candidate in candidates['functions']:
        entry, ranges = candidate['entry'], candidate['ranges']
        require(entry not in rows and bool(ranges), 'Duplicate entry or empty candidate')
        require(ranges == sorted(ranges), 'Unsorted candidate ranges')
        require(any(low <= entry < high for low, high in ranges), 'Entry outside candidate')
        payload = b''
        for low, high in ranges:
            require(base <= low < high <= base + len(text), 'Candidate outside original .text')
            payload += text[low - base:high - base]
            all_ranges.append((low, high, entry))
            instructions = list(decoder.disasm(text[low - base:high - base], low))
            require(sum(i.size for i in instructions) == high - low,
                    f'Candidate range is not completely decoded at {low:#x}')
            for instruction in instructions:
                if (instruction.group(cs.CS_GRP_CALL) or instruction.group(cs.CS_GRP_JUMP)
                        or instruction.mnemonic.startswith('loop')):
                    if len(instruction.operands) == 1 and instruction.operands[0].type == X86_OP_IMM:
                        transfers.append({'owner': entry, 'address': instruction.address,
                                          'target': instruction.operands[0].imm,
                                          'kind': 'call' if instruction.group(cs.CS_GRP_CALL) else 'branch'})
        require(len(payload) == candidate['body_bytes'] == candidate['instruction_bytes'],
                'Candidate byte count differs')
        require(hashlib.sha1(payload).hexdigest() == candidate['body_sha1'],
                'Candidate body hash differs from original')

        def containing_end(address: int) -> int | None:
            return next((high for low, high in ranges if low <= address < high), None)

        pending, decoded, calls, returns = [entry], {}, [], []
        reason = None
        while pending and reason is None:
            address = pending.pop()
            if address in decoded:
                continue
            limit = containing_end(address)
            if limit is None:
                reason = 'branch_or_fallthrough_outside_body'
                break
            instruction = next(decoder.disasm(text[address - base:address - base + 15], address, count=1), None)
            if instruction is None or address + instruction.size > limit:
                reason = 'invalid_or_cross_range_instruction'
                break
            decoded[address] = instruction
            if instruction.group(cs.CS_GRP_RET):
                returns.append(address)
                continue
            if instruction.mnemonic in ('int3', 'ud2', 'hlt', 'int', 'iret', 'iretd', 'sysenter', 'sysexit'):
                reason = 'trap_or_privileged_exit'
                break
            if instruction.group(cs.CS_GRP_CALL) and instruction.operands[0].type == X86_OP_IMM:
                target = instruction.operands[0].imm
                calls.append([address, target])
                if containing_end(target) is not None:
                    reason = 'internal_call'
                    break
            if instruction.group(cs.CS_GRP_JUMP) or instruction.mnemonic.startswith('loop'):
                if len(instruction.operands) != 1 or instruction.operands[0].type != X86_OP_IMM:
                    reason = 'indirect_branch'
                    break
                pending.append(instruction.operands[0].imm)
                if instruction.mnemonic == 'jmp':
                    continue
            pending.append(address + instruction.size)
        decoded_ranges = sorted((a, a + i.size) for a, i in decoded.items())
        if reason is None and any(a[1] > b[0] for a, b in zip(decoded_ranges, decoded_ranges[1:])):
            reason = 'overlapping_decode'
        if reason is None and sum(i.size for i in decoded.values()) != len(payload):
            reason = 'unreachable_candidate_bytes'
        if reason is None and not returns:
            reason = 'no_return'
        rows[entry] = {'entry': entry, 'ranges': ranges, 'body_bytes': len(payload),
                       'status': reason or 'closed_cfg', 'calls': calls,
                       'returns': returns, 'instruction_count': len(decoded)}
    all_ranges.sort()
    require(all(a[1] <= b[0] for a, b in zip(all_ranges, all_ranges[1:])),
            'Candidate ranges overlap')
    incoming = defaultdict(list)
    for entry, row in rows.items():
        if row['status'] == 'closed_cfg':
            for address, target in row['calls']:
                incoming[target].append({'caller_entry': entry, 'call_address': address})
    range_starts = [r[0] for r in all_ranges]
    interior = defaultdict(list)
    for transfer in transfers:
        index = bisect_right(range_starts, transfer['target']) - 1
        if index < 0:
            continue
        low, high, owner = all_ranges[index]
        if (low <= transfer['target'] < high and owner != transfer['owner']
                and transfer['target'] != owner):
            interior[owner].append(transfer)
    manual_path = directory / 'reviewed-functions.json'
    manual_bytes = manual_path.read_bytes()
    manual = json.loads(manual_bytes)
    require(manual['executable_sha1'] == original_hash, 'Manual registry targets another original')
    manual_ranges = []
    for function in manual['functions']:
        low, high = function['address'], function['address'] + function['size']
        require(base <= low < high <= base + len(text), 'Manual extent outside .text')
        require(hashlib.sha256(text[low - base:high - base]).hexdigest() == function['sha256'],
                'Manual extent hash differs')
        manual_ranges.append((low, high))
    functions, conflicts, deduplicated = [], [], []
    for entry, row in sorted(rows.items()):
        if row['status'] != 'closed_cfg' or len(row['ranges']) != 1 or not incoming[entry]:
            continue
        low, high = row['ranges'][0]
        require(low == entry, 'Contiguous closed body does not start at entry')
        if interior[entry]:
            conflicts.append({'address': entry, 'size': high - low,
                              'reason': 'foreign_interior_transfer', 'transfers': interior[entry]})
            continue
        if any(low < end and start < high for start, end in manual_ranges):
            deduplicated.append({'address': entry, 'size': high - low, 'reason': 'manual_extent_precedence'})
            continue
        callers = {}
        for witness in sorted(incoming[entry], key=lambda x: (x['caller_entry'], x['call_address'])):
            callers.setdefault(witness['caller_entry'], witness)
        functions.append({'address': entry, 'size': high - low,
                          'canonical_identifier': f'anon_{entry:08x}', 'original_name': None,
                          'source': None, 'sha256': hashlib.sha256(text[low - base:high - base]).hexdigest(),
                          'instruction_count': row['instruction_count'], 'returns': row['returns'],
                          'direct_call_count': len(incoming[entry]), 'distinct_caller_count': len(callers),
                          'direct_call_witnesses': list(callers.values())[:2]})
    return {'schema_version': 1, 'version': version, 'executable_sha1': original_hash,
            'status': 'machine_verified_anonymous_extents', 'coverage_complete': False,
            'source_identities_recovered': False, 'translation_unit_ownership_recovered': False,
            'retail_link_verified': False,
            'provenance': {'method': METHOD, 'capstone_version': CAPSTONE_VERSION,
                           'candidate_analyzer': 'Ghidra 11.0.1_PUBLIC',
                           'registry_hash_format': 'SHA256 of UTF-8 JSON with sorted keys and compact separators',
                           'candidate_registry_sha256': metadata_hash(candidate_bytes),
                           'manual_registry_sha256': metadata_hash(manual_bytes),
                           'validation_command': f'python tools/platforms/xbox_boundaries.py --version {version}',
                           'constraints': ['Contiguous body with independently decoded closed direct-branch CFG and at least one RET.',
                                           'Every candidate instruction byte reachable from the entry; no overlapping instruction starts.',
                                           'At least one direct CALL from another independently closed candidate CFG.',
                                           'No foreign direct transfer into the interior across all decoded candidate ranges.',
                                           'Any overlap with manually reviewed extents is omitted; manual records take precedence.'],
                           'limitations': ['Entry hypotheses originate in analyzer candidates, not original debug symbols.',
                                           'Indirect entries and whole-program reachability are not proven.',
                                           'Indirect branches, tail calls, discontiguous bodies and shared tails are excluded.',
                                           'Anonymous labels encode addresses only; no original name, source identity or TU ownership is inferred.']},
            'summary': {'candidate_functions': len(rows), 'candidate_statuses': dict(sorted(Counter(r['status'] for r in rows.values()).items())),
                        'direct_transfers_checked': len(transfers), 'function_count': len(functions),
                        'code_bytes': sum(f['size'] for f in functions), 'text_raw_bytes': len(text)},
            'excluded_examples': conflicts[:2], 'manual_overlap_exclusions': deduplicated,
            'functions': functions}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--version', choices=VERSIONS)
    parser.add_argument('--orig-dir', type=Path, default=ROOT / 'orig')
    parser.add_argument('--config-dir', type=Path, default=ROOT / 'config/platforms')
    parser.add_argument('--output-dir', type=Path, help='Generate registries here instead of checking committed metadata')
    args = parser.parse_args()
    for version in ((args.version,) if args.version else VERSIONS):
        document = generate(version, args.orig_dir, args.config_dir)
        if args.output_dir:
            path = args.output_dir / version / REGISTRY
            path.parent.mkdir(parents=True, exist_ok=True)
            header = {k: v for k, v in document.items() if k != 'functions'}
            contents = json.dumps(header, indent=2)[:-2] + ',\n  "functions": [\n'
            contents += ',\n'.join('    ' + json.dumps(f, separators=(',', ':')) for f in document['functions'])
            path.write_text(contents + '\n  ]\n}\n', encoding='utf-8')
        else:
            expected = json.loads((args.config_dir / version / REGISTRY).read_text())
            require(document == expected, f'{version}: anonymous boundary registry differs from actual-original validation')
        print(f"{version}: {document['summary']['function_count']} anonymous extents / "
              f"{document['summary']['code_bytes']} bytes verified; coverage remains partial")


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError) as error:
        raise SystemExit(f'error: {error}')
