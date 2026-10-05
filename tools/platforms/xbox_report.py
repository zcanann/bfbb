"""Prepare partial Xbox section inventory and independently reviewed functions.

Original function bytes remain private. Only confirmed individual extents enter
code measures; Ghidra candidates and whole-section inventory remain separate.
"""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct

from .xbox import inspect_xbe
from .coff import function_object
from collections import defaultdict


# XBE flags cannot distinguish code from data in this title: even .rdata and
# .data are executable. These PE-origin names establish the limited scope.
_SECTION_KINDS = {
    ".text": ("code", 0x60000020),
    ".rdata": ("initialized_data", 0x40000040),
    ".data": ("initialized_data", 0xC0000040),
}


def _target_coff(data: bytes, sections: list[dict]) -> bytes:
    """Wrap authenticated raw sections in i386 COFF, without inventing symbols.

    COFF offsets are zero-based within each section. Original virtual addresses
    and virtual zero-fill tails remain in coverage metadata. This object is for
    section inventory only; it has no recovered relocations and is not linkable
    as a replacement for the original XBE.
    """
    position = 20 + len(sections) * 40
    headers = []
    payloads = []
    for section in sections:
        name = section["name"]
        raw = data[section["raw_offset"]:section["raw_offset"] + section["raw_size"]]
        if len(raw) != section["raw_size"]:
            raise ValueError(f"Truncated XBE section {name}")
        headers.append(struct.pack(
            "<8sIIIIIIHHI", name.encode("ascii").ljust(8, b"\0"),
            0, 0, len(raw), position, 0, 0, 0, 0, _SECTION_KINDS[name][1],
        ))
        payloads.append(raw)
        position += len(raw)
    # IMAGE_FILE_MACHINE_I386, zero timestamp, no symbol/relocation tables.
    header = struct.pack("<HHIIIHH", 0x014C, len(sections), 0, 0, 0, 0, 0)
    return header + b"".join(headers) + b"".join(payloads)


def _write_registries(metadata: dict, output_dir: Path, functions: list[dict] | None = None) -> None:
    functions = functions or []
    identity = {"schema_version": 1, "executable_sha1": metadata["sha1"]}
    symbols = {**identity, "status": "verified_bounds", "coverage_complete": False,
               "provenance": "Reviewed identities plus conservatively verified anonymous CFG extents; unresolved analyzer candidates are separate",
               "symbols": [{k: f[k] for k in ('name', 'canonical_identifier', 'source', 'address', 'size', 'identity_kind')}
                           for f in functions]}
    splits = {**identity, "status": "partial_function_bounds" if functions else "section_only", "symbol_file": "symbols.json",
              "translation_unit_ownership_recovered": False, "relocations_recovered": False,
              "retail_relink_verified": False, "source_build_available": False,
              "publish_matching_report": False,
              "notes": ["Authenticated sections with reviewed and machine-verified function extents; incomplete TU splits.",
                        "Only the closed-CFG subset in verified-anonymous-functions.json enters anonymous coverage; other analyzer candidates are excluded.",
                        "Mixed SDK sections and embedded assets require further classification."],
              "sections": [{**section, "classification": _SECTION_KINDS.get(section["name"], ("unclassified", 0))[0]}
                           for section in metadata["sections"]]}
    text = next(s for s in metadata['sections'] if s['name'] == '.text')
    cursor = text['virtual_address']
    end = cursor + min(text['raw_size'], text['virtual_size'])
    ranges = []
    for function in functions:
        address, size = function['address'], function['size']
        if size <= 0 or address < cursor or address + size > end:
            raise ValueError('Reviewed Xbox functions overlap or exceed .text')
        if cursor < address:
            ranges.append({'kind': 'unclassified', 'address': cursor, 'size': address - cursor})
        ranges.append({'kind': 'function', 'address': address, 'size': size,
                       'canonical_identifier': function['canonical_identifier']})
        cursor = address + size
    if cursor < end:
        ranges.append({'kind': 'unclassified', 'address': cursor, 'size': end - cursor})
    splits.update({'known_code_bytes': sum(f['size'] for f in functions),
                   'unclassified_text_bytes': sum(r['size'] for r in ranges if r['kind'] == 'unclassified'),
                   'text_ranges': ranges})
    output_dir.mkdir(parents=True, exist_ok=True)
    for name, value in (("symbols.json", symbols), ("splits.json", splits)):
        (output_dir / name).write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8", newline="\n")


def generate_registries(executable: Path, output_dir: Path) -> None:
    """Generate only binary-backed section metadata and the empty confirmed symbol registry."""
    _write_registries(inspect_xbe(Path(executable)), Path(output_dir))


def verify_registries(generated_dir: Path, committed_dir: Path) -> None:
    """Verify deterministic registries and candidate body hashes, without rerunning Ghidra.

    Byte validation authenticates what was analyzed; it does not promote
    heuristic function boundaries to original symbols or source matches.
    """
    generated_dir, committed_dir = Path(generated_dir), Path(committed_dir)
    for filename in ("symbols.json", "splits.json"):
        actual = json.loads((generated_dir / filename).read_text(encoding="utf-8"))
        expected = json.loads((committed_dir / filename).read_text(encoding="utf-8"))
        if actual != expected:
            raise ValueError(f"Generated Xbox registry differs from {committed_dir / filename}")
    candidates_path = committed_dir / "symbol-candidates.json"
    if not candidates_path.exists():
        return
    candidates = json.loads(candidates_path.read_text(encoding="utf-8"))
    splits = json.loads((generated_dir / "splits.json").read_text(encoding="utf-8"))
    if candidates["executable_sha1"] != splits["executable_sha1"] or candidates["status"] != "analysis_candidates":
        raise ValueError("Xbox candidate identity or status mismatch")
    section = next(s for s in splits["sections"] if s["name"] == ".text")
    obj = (generated_dir / "target.obj").read_bytes()
    if len(obj) < 20 or struct.unpack_from("<H", obj)[0] != 0x14C:
        raise ValueError("Invalid candidate-verification target COFF")
    text = None
    for index in range(struct.unpack_from("<H", obj, 2)[0]):
        fields = struct.unpack_from("<8sIIIIIIHHI", obj, 20 + index * 40)
        if fields[0].rstrip(b"\0") == b".text":
            text = obj[fields[4]:fields[4] + fields[3]]
    if text is None or len(text) != section["raw_size"] or hashlib.sha1(text).hexdigest() != section["sha1"]:
        raise ValueError("Candidate-verification .text bytes differ from original")
    start = section["virtual_address"]
    end = start + len(text)
    intervals = []
    entries = set()
    for function in candidates["functions"]:
        entry = function["entry"]
        if entry in entries or not start <= entry < end:
            raise ValueError("Duplicate or out-of-bounds candidate entry")
        entries.add(entry)
        digest = hashlib.sha1()
        size = 0
        previous_end = start
        contains_entry = False
        for low, high in function["ranges"]:
            if not start <= low < high <= end or low < previous_end:
                raise ValueError("Invalid candidate function body range")
            contains_entry |= low <= entry < high
            digest.update(text[low - start:high - start])
            size += high - low
            previous_end = high
            intervals.append((low, high))
        if not contains_entry or size != function["body_bytes"] or size != function["instruction_bytes"]:
            raise ValueError("Candidate function body accounting mismatch")
        if digest.hexdigest() != function["body_sha1"]:
            raise ValueError(f"Candidate body hash differs at {entry:#x}")
    intervals.sort()
    if any(right[0] < left[1] for left, right in zip(intervals, intervals[1:])):
        raise ValueError("Overlapping candidate function bodies")
    # Uploadable metadata only. Candidate bodies never enter matching measures.
    (generated_dir / "symbol-candidates.json").write_text(
        candidates_path.read_text(encoding="utf-8"), encoding="utf-8", newline="\n")


def prepare_report(executable: Path, output_dir: Path, reviewed_functions: Path | None = None,
                   anonymous_functions: Path | None = None) -> dict:
    """Write target.obj, objdiff.json and coverage.json; return coverage metadata.

    Outputs contain original executable bytes and must remain in an ignored
    private build directory. There are no base/source objects or completion
    claims. The caller runs objdiff report generation against this directory.
    """
    executable, output_dir = Path(executable), Path(output_dir)
    metadata = inspect_xbe(executable)
    data = executable.read_bytes()
    if hashlib.sha1(data).hexdigest() != metadata["sha1"]:
        raise ValueError("XBE changed during report preparation")
    sections = metadata["sections"]
    included = [section for section in sections if section["name"] in _SECTION_KINDS]
    names = [section["name"] for section in included]
    if len(names) != len(_SECTION_KINDS) or set(names) != set(_SECTION_KINDS):
        raise ValueError("Expected exactly one .text, .rdata and .data XBE section")

    output_dir.mkdir(parents=True, exist_ok=True)
    functions = []
    if reviewed_functions is not None:
        reviewed = json.loads(Path(reviewed_functions).read_text(encoding='utf-8'))
        if reviewed['executable_sha1'] != metadata['sha1']:
            raise ValueError('Reviewed Xbox functions identify another executable')
        from .xbox_relocations import verify_original_anchors, normalize
        anchors_path = Path(reviewed_functions).with_name('reviewed-data-anchors.json')
        anchors = {}
        has_switches = any(f.get('corroboration', {}).get('closed_cfg', {}).get('switch_table')
                           for f in reviewed['functions'])
        if anchors_path.is_file() or has_switches:
            from .verify_xbox_reviewed import Original
            versions = json.loads((Path(__file__).resolve().parents[2] / 'config/platforms/versions.json').read_text())['versions']
            original = Original(reviewed['version'], versions[reviewed['version']], executable.parent.parent)
            if anchors_path.is_file():
                anchors = verify_original_anchors(anchors_path, original)
        call_targets = {f['canonical_identifier']: f['address'] for f in reviewed['functions']
                        if f.get('boundary_confirmation') and f.get('identity_confirmation')}
        identifiers = set()
        for function in reviewed['functions']:
            function = dict(function)
            if not function.get('boundary_confirmation') or not function.get('identity_confirmation'):
                raise ValueError('Unconfirmed Xbox function')
            identifier = function['canonical_identifier']
            if not identifier or identifier in identifiers:
                raise ValueError('Empty or duplicate Xbox function identifier')
            identifiers.add(identifier)
            address, size = function['address'], function['size']
            owners = [s for s in sections if s['name'] == '.text' and
                      s['virtual_address'] <= address and address + size <= s['virtual_address'] +
                      min(s['raw_size'], s['virtual_size'])]
            if size <= 0 or len(owners) != 1:
                raise ValueError('Reviewed Xbox function lacks a file-backed .text owner')
            offset = owners[0]['raw_offset'] + address - owners[0]['virtual_address']
            raw = data[offset:offset + size]
            if hashlib.sha256(raw).hexdigest() != function['sha256']:
                raise ValueError('Reviewed Xbox function bytes differ')
            from .xbox_switch import original_switch_anchors
            switch_anchors = (original_switch_anchors(original, function) if has_switches else {})
            if set(anchors) & set(switch_anchors):
                raise ValueError('Switch table aliases an ordinary data anchor')
            function_anchors = {**anchors, **switch_anchors}
            normalized, relocations = normalize(raw, function.get('address_expressions', []), function_anchors)
            if function.get('direct_calls'):
                from .xbox_calls import normalize_calls
                normalized, call_relocations = normalize_calls(normalized, address, function['direct_calls'], call_targets)
                relocations = sorted(relocations + call_relocations, key=lambda r: r['offset'])
            function.update({'bytes': normalized, 'relocations': relocations,
                             'symbol': identifier, 'identity_kind': 'reviewed'})
            functions.append(function)
    if anonymous_functions is not None:
        from .xbox_boundaries import generate
        expected = json.loads(Path(anonymous_functions).read_text(encoding='utf-8'))
        # Re-decode the actual original and all incoming candidate transfers;
        # reading a previously generated registry alone is not validation.
        checked = generate(expected['version'], executable.parent.parent,
                           Path(anonymous_functions).parent.parent)
        if checked != expected or checked['executable_sha1'] != metadata['sha1']:
            raise ValueError('Anonymous Xbox extents differ from current original validation')
        text = next(s for s in sections if s['name'] == '.text')
        for entry in checked['functions']:
            offset = text['raw_offset'] + entry['address'] - text['virtual_address']
            raw = data[offset:offset + entry['size']]
            if hashlib.sha256(raw).hexdigest() != entry['sha256']:
                raise ValueError('Anonymous Xbox bytes changed after boundary validation')
            functions.append({**entry, 'name': None, 'source': None, 'bytes': raw,
                              'symbol': entry['canonical_identifier'], 'identity_kind': 'anonymous'})
        (output_dir / 'verified-anonymous-functions.json').write_text(
            json.dumps(checked, indent=2) + '\n', encoding='utf-8')
    functions.sort(key=lambda f: f['address'])
    if any(a['address'] + a['size'] > b['address'] for a, b in zip(functions, functions[1:])):
        raise ValueError('Reviewed and anonymous Xbox extents overlap')
    target = _target_coff(data, included)
    coverage = {
        "schema_version": 1,
        "status": "partial-target-only" if functions else "target_only_unsplit",
        "report_ready": bool(functions),
        "format": "XBE",
        "architecture": "i386",
        "executable_sha1": metadata["sha1"],
        "executable_size": metadata["size"],
        "target_object_sha1": hashlib.sha1(target).hexdigest(),
        "source_build_available": False,
        "source_comparison_available": False,
        "function_boundaries_recovered": bool(functions),
        "known_code_bytes": sum(f["size"] for f in functions),
        "function_count": len(functions),
        "anonymous_function_count": sum(f["identity_kind"] == "anonymous" for f in functions),
        "relocations_recovered": False,
        "retail_relink_verified": False,
        "coverage_complete": False,
        "publish_matching_report": False,
        "scope": "Reviewed identities and verified anonymous bounds plus separate section inventory; not whole-game progress",
        "included_code_section_bytes": sum(
            section["raw_size"] for section in included if section["name"] == ".text"
        ),
        "included_initialized_data_bytes": sum(
            section["raw_size"] for section in included if section["name"] != ".text"
        ),
        "included_sections": [
            {**section, "classification": _SECTION_KINDS[section["name"]][0],
             "unmeasured_virtual_tail_bytes": max(0, section["virtual_size"] - section["raw_size"])}
            for section in included
        ],
        "unclassified_sections": [
            {**section, "reason": "Requires explicit code/data or asset classification"}
            for section in sections if section["name"] not in _SECTION_KINDS
        ],
        "report_limitations": [
            "Only explicitly profiled source functions can be compared; full executable reconstruction remains pending.",
            "No function symbols are synthesized for the unsplit .text section.",
            "Reviewed identities and machine-verified anonymous extents enter code measures; coverage remains incomplete.",
            "objdiff defaults zero-denominator code/function percentages to100%; this is not measured progress.",
            "Do not publish this diagnostic report as a matching-progress baseline.",
            "Initialized data bytes are inventoried; virtual zero-fill tails are not measured.",
            "Mixed SDK sections and embedded assets are excluded from matching measures.",
            "This COFF is an inventory container, not a relocation-restored link input.",
        ],
    }
    config = {
        "$schema": "https://raw.githubusercontent.com/encounter/objdiff/main/config.schema.json",
        "build_target": False,
        "build_base": False,
        "units": [{
            "name": "main/xbox_unsplit",
            "target_path": "target.obj",
            "metadata": {"complete": False},
        }],
    }
    output_dir.mkdir(parents=True, exist_ok=True)
    groups = defaultdict(list)
    switch_proof = {'schema_version': 1, 'executable_sha1': metadata['sha1'], 'units': {}}
    data_proof = {'schema_version': 1, 'executable_sha1': metadata['sha1'],
                  'anchors': {}, 'target_objects': {}}
    if reviewed_functions is not None and anchors_path.is_file():
        anchor_document = json.loads(anchors_path.read_text(encoding='utf-8'))
        for anchor in anchor_document['anchors']:
            if anchor.get('external_binding_allowed') is True:
                data_proof['anchors'][anchor['name']] = {**anchors[anchor['name']],
                    'kind': 'data', 'source': anchor['source']}

    for function in functions:
        groups[function['source'] or 'unassigned/xbox'].append(function)
    for index, (source, group) in enumerate(sorted(groups.items())):
        name = f'functions-{index:04d}.obj'
        object_bytes = function_object(group)
        (output_dir / name).write_bytes(object_bytes)
        data_proof['target_objects'][source] = hashlib.sha256(object_bytes).hexdigest()
        from .xbox_switch import switch_signature
        checked_tables = {f['canonical_identifier']: switch_signature(f['address'],
            f['corroboration']['closed_cfg']['switch_tables']) for f in group
            if f.get('corroboration', {}).get('closed_cfg', {}).get('switch_table')}
        if checked_tables:
            switch_proof['units'][source] = {
                'target_object_sha256': hashlib.sha256(object_bytes).hexdigest(),
                'functions': checked_tables}

        config['units'].append({'name': source, 'target_path': name,
                                'metadata': {'complete': False,
                                             'progress_categories': ['anonymous_functions' if source == 'unassigned/xbox' else 'known_functions']}})
    config['progress_categories'] = [
        {'id': 'known_functions', 'name': 'Reviewed identities (partial coverage)'},
        {'id': 'anonymous_functions', 'name': 'Anonymous code extents (partial coverage)'},
    ]
    (output_dir / 'switch-tables.json').write_text(json.dumps(switch_proof, indent=2) + '\n', encoding='utf-8')
    (output_dir / 'original-data-bindings.json').write_text(json.dumps(data_proof, indent=2) + '\n', encoding='utf-8')
    _write_registries(metadata, output_dir, functions)
    (output_dir / "target.obj").write_bytes(target)
    (output_dir / "objdiff.json").write_text(json.dumps(config, indent=2) + "\n", encoding="utf-8")
    (output_dir / "coverage.json").write_text(json.dumps(coverage, indent=2) + "\n", encoding="utf-8")
    return coverage