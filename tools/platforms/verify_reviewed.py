#!/usr/bin/env python3
"""Recheck France reviewed evidence against all four authenticated PS2 originals.

Run: python tools/platforms/verify_reviewed.py --orig-dir orig

This read-only proof command checks named DWARF1 extents, explicit relocation
exceptions, recorded direct calls, frame/return/padding evidence, and unique
callee correspondences. Explicit allocator address fields and reviewed GP data
uses also validate against each original; stripped BSS uses startup evidence. It does not rerun the entry CFG or replace the
individual semantic/boundary review. It never generates progress or symbols.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from platforms.ps2 import inspect_elf
from platforms.ps2_report import _function_ranges, _source_name

TARGET = "SLES-53623"
REFERENCES = ("SLUS-20680", "SLES-51968", "SLES-51970")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def normalized(data: bytes) -> tuple[int, ...]:
    """Preserve every bit except actual MIPS J/JAL destination fields."""
    return tuple(word & 0xfc000000 if word >> 26 in (2, 3) else word
                 for (word,) in struct.iter_unpack("<I", data))


class Original:
    def __init__(self, version: str, record: dict, orig_dir: Path):
        self.version = version
        require(record["platform"] == "ps2", f"{version}: not a PS2 original")
        path = orig_dir / version / Path(record["executable"]["path"]).name
        self.data = path.read_bytes()
        self.sha1 = hashlib.sha1(self.data).hexdigest()
        require(self.sha1 == record["executable"]["sha1"], f"{version}: executable SHA-1 mismatch")
        self.metadata = inspect_elf(path)
        require(self.metadata["sha1"] == self.sha1, f"{version}: original changed during inspection")
        self.loaded = [s for s in self.metadata["segments"] if s["type"] == 1 and s["file_size"]]
        self.functions = []
        if version in REFERENCES:
            debug = [s for s in self.metadata["sections"] if s["name"] == ".debug" and s["size"]]
            require(len(debug) == 1, f"{version}: expected one DWARF1 debug section")
            section = debug[0]
            self.functions = _function_ranges(self.data[section["offset"]:section["offset"] + section["size"]])
            for function in self.functions:
                function["source"] = _source_name(function["source"])
            require(bool(self.functions), f"{version}: no DWARF1 functions")
        self.by_address = {f["low"]: f for f in self.functions}

    def read(self, address: int, size: int) -> bytes:
        require(isinstance(address, int) and isinstance(size, int) and size >= 0,
                f"{self.version}: invalid address/size")
        spans = [s for s in self.loaded if s["address"] <= address and
                 address + size <= s["address"] + s["file_size"]]
        require(len(spans) == 1, f"{self.version}: range {address:#x}+{size:#x} lacks unique loaded owner")
        offset = spans[0]["offset"] + address - spans[0]["address"]
        return self.data[offset:offset + size]

    def word(self, address: int) -> int:
        require(address % 4 == 0, f"{self.version}: unaligned instruction {address:#x}")
        return struct.unpack("<I", self.read(address, 4))[0]

    def transfer(self, address: int, target: int, opcode: int = 3) -> None:
        word = self.word(address)
        decoded = ((address + 4) & 0xf0000000) | ((word & 0x3ffffff) << 2)
        require(word >> 26 == opcode and decoded == target,
                f"{self.version}: direct transfer {address:#x} does not target {target:#x}")


def verified_body(entry: dict, original: Original) -> bytes:
    address, size = entry["address"], entry["size"]
    require(size > 0 and size % 4 == 0 and address % 4 == 0, f"{entry['name']}: invalid extent")
    body = original.read(address, size)
    require(sha256(body) == entry["sha256"], f"{entry['name']}: reviewed byte hash mismatch")
    return body


def reference_body(entry: dict, proof: dict, originals: dict) -> tuple[Original, bytes]:
    original = originals[proof["version"]]
    require(original.version in REFERENCES and proof["executable_sha1"] == original.sha1,
            f"{entry['name']}: reference executable identity mismatch")
    function = original.by_address.get(proof["source_address"])
    require(function is not None and function["name"] == proof["name"] == entry["name"] and
            function["source"] == proof["source"] == entry["source"] and
            function["high"] - function["low"] == entry["size"],
            f"{entry['name']}: reference name/source/extent differs from actual DWARF1")
    body = original.read(function["low"], entry["size"])
    if "reference_sha256" in proof:
        require(sha256(body) == proof["reference_sha256"], f"{entry['name']}: reference byte hash mismatch")
    return original, body


def provenance(entry: dict) -> list[dict]:
    proofs = entry["provenance"]
    require(len(proofs) == len(REFERENCES) and {p["version"] for p in proofs} == set(REFERENCES),
            f"{entry['name']}: expected provenance from each of the three debug versions")
    return proofs


def compare_explicit(entry: dict, french: bytes, reference: bytes, offsets: list[int]) -> None:
    differences = [i for i in range(0, len(french), 4) if french[i:i+4] != reference[i:i+4]]
    require(sorted(offsets) == differences and len(offsets) == len(set(offsets)),
            f"{entry['name']}: changed instructions differ from explicit relocation exceptions")
    for offset in offsets:
        require(0 <= offset < len(french) and offset % 4 == 0, f"{entry['name']}: invalid relocation offset")
        left, right = struct.unpack("<I", french[offset:offset+4])[0], struct.unpack("<I", reference[offset:offset+4])[0]
        require(left >> 26 == right >> 26 and left >> 26 in (2, 3),
                f"{entry['name']}: exception changes more than a J/JAL destination")


def corroboration(entry: dict, target: Original) -> None:
    evidence = entry.get("corroboration", {})
    start, end = entry["address"], entry["address"] + entry["size"]
    calls = evidence.get("direct_entry_calls", [])
    if "direct_entry_call" in evidence:
        calls = [*calls, evidence["direct_entry_call"]]
    for call in calls:
        target.transfer(call, start)
    cfg = evidence.get("entry_cfg")
    if cfg:
        require(cfg["entry_point"] == target.metadata["entry_point"], "Recorded ELF entry differs")
        chain = cfg["direct_call_chain"]
        require(bool(chain) and chain[-1]["target"] == start, f"{entry['name']}: incomplete direct-call witness")
        for edge in chain:
            target.transfer(edge["call_site"], edge["target"])
    padding = evidence.get("following_padding")
    if padding:
        require(padding["address"] == end and padding["all_zero"] is True and
                target.read(end, padding["size"]) == bytes(padding["size"]), f"{entry['name']}: padding differs")
    terminal = evidence.get("terminal_return")
    control = evidence.get("local_control_flow")
    if isinstance(control, dict):
        terminal = {"address": control["return_instruction"], "delay_slot": control["return_instruction"] + 4,
                    "end_exclusive": control["end_exclusive"]}
        branch = target.word(control["conditional_branch"])
        displacement = (branch & 65535) - (65536 if branch & 32768 else 0)
        require(branch >> 26 in (4, 5, 6, 7) and
                control["conditional_branch"] + 4 + displacement * 4 == control["conditional_target"],
                f"{entry['name']}: recorded conditional branch differs")
        target.transfer(control["conditional_call"], control["conditional_call_target"])
    if terminal:
        require(target.word(terminal["address"]) == 0x03e00008 and
                terminal["delay_slot"] == terminal["address"] + 4 and
                terminal["end_exclusive"] == terminal["address"] + 8 == end,
                f"{entry['name']}: return/delay-slot boundary differs")
    frame = evidence.get("stack_frame")
    if frame:
        for address, adjustment in ((frame["allocation_address"], -frame["allocation_bytes"]),
                                    (frame["deallocation_address"], frame.get("deallocation_bytes", frame["allocation_bytes"]))):
            word = target.word(address)
            require(word >> 26 in (9, 25) and (word >> 21) & 31 == 29 and (word >> 16) & 31 == 29 and
                    word & 65535 == adjustment & 65535, f"{entry['name']}: stack adjustment differs")
        for field, opcodes in (("return_address_save", (43, 63)), ("return_address_restore", (35, 55))):
            word = target.word(frame[field])
            require(word >> 26 in opcodes and (word >> 21) & 31 == 29 and (word >> 16) & 31 == 31 and
                    word & 65535 == frame["return_address_stack_offset"] & 65535,
                    f"{entry['name']}: return-address save/restore differs")
    caller = evidence.get("independent_caller")
    if caller:
        require(sha256(target.read(caller["address"], caller["size"])) == caller["sha256"],
                f"{entry['name']}: independent caller bytes differ")
        target.transfer(caller["call_site"], start)


def pair_address(original: Original, base: int, pair: dict) -> int:
    high = original.word(base + pair['high_offset'])
    low = original.word(base + pair['low_offset'])
    register = (high >> 16) & 31
    require(high >> 26 == 15 and (high >> 21) & 31 == 0 and register != 0 and
            low >> 26 == pair['low_opcode'] and pair['low_opcode'] in (9, 35) and
            (low >> 21) & 31 == register,
            'Address pair is not the recorded LUI plus ADDIU/LW dependency')
    if pair['low_opcode'] == 9:
        require((low >> 16) & 31 == register, 'Address ADDIU changes its base register')
    immediate = low & 65535
    return (((high & 65535) << 16) + immediate - (65536 if immediate & 32768 else 0)) & 0xffffffff


def memory_contains(original: Original, address: int, size: int) -> bool:
    return sum(s['type'] == 1 and s['address'] <= address and
               address + size <= s['address'] + s['memory_size']
               for s in original.metadata['segments']) == 1


def original_gp(original: Original) -> int:
    sections = [s for s in original.metadata['sections'] if s['name'] == '.reginfo']
    require(len(sections) == 1 and sections[0]['size'] == 24, 'Missing unique MIPS .reginfo')
    return struct.unpack_from('<I', original.data, sections[0]['offset'] + 20)[0]


def verify_data_anchors(registry: dict, originals: dict, functions: dict) -> tuple[int, int]:
    """Verify the single reviewed GP address, including stripped startup BSS evidence."""
    from platforms.dwarf1 import iter_dies
    target = originals[TARGET]
    require(registry['version'] == TARGET and registry['executable_sha1'] == target.sha1 and
            registry['coverage_complete'] is False and registry['eligible_for_progress'] is False,
            'Reviewed data registry identity/scope differs')
    require(len(registry['anchors']) == 1, 'Expected the one explicitly reviewed GP anchor')
    entry = registry['anchors'][0]
    require(entry['identity_confirmation'] is True and entry['eligible_for_progress'] is False and
            entry['size'] == 4 and entry['storage'] == 'runtime_zero_fill', 'Data anchor scope/type differs')
    runtime = entry['runtime_zero_fill']
    require(sha256(target.read(runtime['address'], runtime['size'])) == runtime['sha256'],
            'Startup zero-fill bytes differ')
    bounds = []
    for key, value in (('lower_pair', 'lower_bound'), ('upper_pair', 'upper_bound')):
        hi, lo = runtime[key]
        decoded = pair_address(target, 0, {'high_offset': hi, 'low_offset': lo, 'low_opcode': 9})
        require(decoded == runtime[value], 'Startup zero-fill bound differs')
        bounds.append(decoded)
    # R5900 SQ zero,0(v0); SLTU at,v0,v1; BNE at,zero,store; ADDIU v0,v0,16.
    require(target.word(runtime['zero_store']) == 0x7c400000 and
            target.word(runtime['comparison']) == 0x0043082b and
            target.word(runtime['stride_instruction']) == 0x24420010,
            'Startup zero-fill store/comparison/stride differs')
    branch = target.word(runtime['loop_branch'])
    immediate = branch & 65535
    require(branch & 0xffff0000 == 0x14200000 and runtime['loop_branch'] + 4 +
            4 * (immediate - (65536 if immediate & 32768 else 0)) == runtime['zero_store'] and
            runtime['stride_instruction'] == runtime['loop_branch'] + 4,
            'Startup zero-fill loop branch differs')
    require((target.word(runtime['lower_pair'][0]) >> 16) & 31 == 2 and
            (target.word(runtime['upper_pair'][0]) >> 16) & 31 == 3 and
            bounds[0] <= entry['address'] and entry['address'] + entry['size'] <= bounds[1],
            'Reviewed data is outside the startup zero-fill interval')
    setup = runtime['gp_initialization']
    require((target.word(setup['high_address']) >> 16) & 31 == 4 and
            pair_address(target, 0, {'high_offset': setup['high_address'],
            'low_offset': setup['low_address'], 'low_opcode': 9}) == original_gp(target) and
            target.word(setup['move_address']) == 0x0080e025,
            'Startup GP initialization differs from original .reginfo')
    require(len(entry['reviewed_uses']) == 1, 'Unexpected number of GP witnesses')
    use = entry['reviewed_uses'][0]
    owner = functions.get(use['function_address'])
    require(owner is not None and owner['name'] == use['function'] and owner['source'] == use['source'] and
            0 <= use['offset'] <= owner['size'] - 4 and use['relocation_type'] == 'R_MIPS_GPREL16',
            'GP use lacks independently reviewed function ownership')
    def check_use(original, pc, gp, address):
        word = original.word(pc)
        immediate = word & 65535
        require(word >> 26 == use['opcode'] == 35 and (word >> 21) & 31 == 28 and
                original_gp(original) == gp and
                gp + immediate - (65536 if immediate & 32768 else 0) == address,
                'Actual LW/GP effective address differs')
        return word
    target_word = check_use(target, use['function_address'] + use['offset'], use['gp'], entry['address'])
    for proof in provenance(entry):
        reference = originals[proof['version']]
        require(reference.sha1 == proof['executable_sha1'], 'Data reference identity differs')
        section = next(s for s in reference.metadata['sections'] if s['name'] == '.debug')
        debug = reference.data[section['offset']:section['offset'] + section['size']]
        die = next((d for d in iter_dies(debug) if d[0] == proof['die_offset']), None)
        require(die is not None, 'Data declaration DIE is absent')
        _, tag, source, attrs = die
        require(tag in (7, 12) and _source_name(source) == proof['source'] == use['source'] and
                attrs.get(3) == proof['name'] == entry['name'] and
                attrs.get(5) == proof['fundamental_type'] == 9 and
                attrs.get(2) == b'\x03' + struct.pack('<I', proof['address']),
                'Data declaration name/type/location differs from actual DWARF')
        function = reference.by_address.get(proof['function_address'])
        require(function is not None and function['name'] == proof['function'] == owner['name'] and
                function['source'] == owner['source'] and proof['use_offset'] == use['offset'] and
                memory_contains(reference, proof['address'], entry['size']),
                'Data reference function or mapped-memory extent differs')
        require(check_use(reference, proof['function_address'] + proof['use_offset'], proof['gp'],
                          proof['address']) == target_word, 'GP use instruction differs across originals')
    return tuple(bounds)


def verify_allocator(anchor: dict, originals: dict, functions: dict, runtime_bounds: tuple) -> None:
    """Authenticate complete neighboring functions, relocating only explicit address fields."""
    target = originals[TARGET]
    verified_body(anchor, target)
    group = anchor['corroboration']['allocator_neighborhood']
    base, size = group['address'], group['size']
    body = target.read(base, size)
    require(sha256(body) == group['sha256'], 'Allocator neighborhood hash differs')
    members = group['members']
    require(members and members[0]['offset'] == 0 and
            all(a['offset'] + a['size'] == b['offset'] for a, b in zip(members, members[1:])) and
            members[-1]['offset'] + members[-1]['size'] == size,
            'Allocator neighborhood contains unowned bytes')
    member = next((m for m in members if m['name'] == anchor['name']), None)
    require(member is not None and base + member['offset'] == anchor['address'] and
            member['size'] == anchor['size'], 'Allocator anchor membership differs')
    fields = []
    for pair in group['address_pairs']:
        require(pair_address(target, base, pair) == pair['address'] and
                runtime_bounds[0] <= pair['address'] < runtime_bounds[1],
                'Allocator data pointer is outside the proven runtime interval')
        fields += [(pair['high_offset'], 0xffff), (pair['low_offset'], 0xffff)]
    call = group['direct_call']
    target.transfer(base + call['offset'], call['target'])
    callee_body = target.read(call['target'], call['size'])
    require(sha256(callee_body) == call['sha256'], 'Allocator subsidiary callee hash differs')
    fields.append((call['offset'], 0x3ffffff))
    require(len({o for o, _ in fields}) == len(fields) and
            all(0 <= o <= size - 4 and o % 4 == 0 for o, _ in fields), 'Invalid allocator address fields')
    def normalize(data):
        words = list(struct.unpack('<' + 'I' * (len(data) // 4), data))
        for offset, mask in fields:
            words[offset // 4] &= ~mask
        return tuple(words)
    normalized_body = normalize(body)
    for proof in provenance(anchor):
        reference, _ = reference_body(anchor, proof, originals)
        ref_base = proof['neighborhood_address']
        require(ref_base + member['offset'] == proof['source_address'], 'Reference allocator membership differs')
        ref_body = reference.read(ref_base, size)
        require(sha256(ref_body) == proof['neighborhood_sha256'], 'Reference allocator neighborhood hash differs')
        for m in members:
            named = reference.by_address.get(ref_base + m['offset'])
            require(named is not None and named['name'] == m['name'] and named['source'] == anchor['source'] and
                    named['high'] - named['low'] == m['size'], 'Reference allocator member lacks named DWARF extent')
        require(len(proof['address_pair_targets']) == len(group['address_pairs']), 'Reference pair count differs')
        for pair, address in zip(group['address_pairs'], proof['address_pair_targets']):
            require(pair_address(reference, ref_base, pair) == address and memory_contains(reference, address, 4),
                    'Reference allocator address pair or mapped memory differs')
        reference.transfer(ref_base + call['offset'], proof['direct_call_target'])
        callee = reference.by_address.get(proof['direct_call_target'])
        require(callee is not None and callee['name'] == call['name'] and callee['source'] == call['source'] and
                callee['high'] - callee['low'] == call['size'], 'Allocator subsidiary callee lacks named extent')
        ref_callee = reference.read(callee['low'], call['size'])
        require(ref_callee == callee_body and sha256(ref_callee) == proof['direct_call_reference_sha256'],
                'Allocator subsidiary callee complete body differs')
        # Recover the exact target bytes from the reference by applying only the
        # decoded relocation fields. No opcode, register or arithmetic bits change.
        reconstructed = bytearray(ref_body)
        values = {}
        for pair in group['address_pairs']:
            # Signed low halves require the standard high-half carry adjustment.
            values[pair['high_offset']] = ((pair['address'] + 0x8000) >> 16) & 65535
            values[pair['low_offset']] = pair['address'] & 65535
        values[call['offset']] = (call['target'] >> 2) & 0x3ffffff
        for offset, mask in fields:
            a = struct.unpack_from('<I', ref_body, offset)[0]
            b = struct.unpack_from('<I', body, offset)[0]
            require(a & ~mask == b & ~mask, 'Allocator relocation changes non-address bits')
            struct.pack_into('<I', reconstructed, offset, (a & ~mask) | values[offset])
        require(bytes(reconstructed) == body, 'Explicit allocator relocations do not reproduce the complete body')
        matches = [f['low'] for f in reference.functions
                   if normalize(reference.read(f['low'], size)) == normalized_body]
        require(matches == [ref_base] and proof['matching_named_dwarf_neighborhoods'] == 1,
                'Allocator neighborhood identity is not unique among named DWARF starts')
    for use in anchor['reviewed_uses']:
        owner = functions.get(use['function_address'])
        require(owner is not None and owner['name'] == use['function'] and
                use['relocation_type'] == 'R_MIPS_26' and 0 <= use['offset'] <= owner['size'] - 4 and
                use['call_address'] == owner['address'] + use['offset'], 'Allocator call witness lacks ownership')
        target.transfer(use['call_address'], anchor['address'])


def verify(manifest: Path, orig_dir: Path, registry_dir: Path) -> dict:
    versions = json.loads(manifest.read_text(encoding="utf-8"))["versions"]
    originals = {version: Original(version, versions[version], orig_dir) for version in (TARGET, *REFERENCES)}
    target = originals[TARGET]
    reviewed = json.loads((registry_dir / "reviewed-functions.json").read_text(encoding="utf-8"))
    call_registry = json.loads((registry_dir / "reviewed-call-targets.json").read_text(encoding="utf-8"))
    for registry in (reviewed, call_registry):
        require(registry["version"] == TARGET and registry["executable_sha1"] == target.sha1,
                "Registry targets another executable")
        require(registry["coverage_complete"] is False, "Partial reviewed coverage must remain explicit")
    functions = reviewed["functions"]
    anchors = call_registry["anchors"]
    by_address = {entry["address"]: entry for entry in functions}
    by_name = {(entry["source"], entry["name"]): entry for entry in functions}
    anchor_by_address = {entry["address"]: entry for entry in anchors}
    require(len(by_address) == len(functions) and len(anchor_by_address) == len(anchors), "Duplicate reviewed address")
    ordered = sorted(functions, key=lambda entry: entry["address"])
    require(all(a["address"] + a["size"] <= b["address"] for a, b in zip(ordered, ordered[1:])), "Reviewed functions overlap")
    for entry in functions:
        require(entry["boundary_confirmation"] is True, f"{entry['name']}: boundary not confirmed")
        body = verified_body(entry, target)
        for proof in provenance(entry):
            reference, reference_bytes = reference_body(entry, proof, originals)
            layout = entry.get("corroboration", {}).get("tu_layout")
            if layout:
                base = by_name.get((entry["source"], layout["anchor_function"]))
                require(base is not None and base["address"] == layout["anchor_address"] and
                        entry["address"] - base["address"] == layout["offset_from_anchor"],
                        f"{entry['name']}: reviewed TU layout anchor differs")
                base_proof = next(p for p in provenance(base) if p["version"] == proof["version"])
                require(proof["source_address"] - base_proof["source_address"] == layout["offset_from_anchor"],
                        f"{entry['name']}: reference TU relative position differs")
            neighborhood = entry.get("corroboration", {}).get("exact_neighborhood")
            if neighborhood:
                offset = neighborhood["offset_from_start"]
                require(0 <= offset and offset + entry["size"] <= neighborhood["size"] and
                        entry["address"] - offset == neighborhood["address"],
                        f"{entry['name']}: invalid exact neighborhood bounds")
                block = target.read(neighborhood["address"], neighborhood["size"])
                require(sha256(block) == neighborhood["sha256"] and
                        reference.read(proof["source_address"] - offset, len(block)) == block,
                        f"{entry['name']}: original neighborhood differs")
                # The compound retail witness disambiguates tiny getters/setters;
                # compiled source bytes do not establish this identity or extent.
                for original in (target, reference):
                    occurrences = 0
                    for segment in original.loaded:
                        span = original.read(segment["address"], segment["file_size"])
                        position = span.find(block)
                        while position >= 0:
                            occurrences += 1
                            position = span.find(block, position + 1)
                    require(occurrences == neighborhood["loaded_image_exact_occurrences"] == 1,
                            f"{entry['name']}: exact neighborhood is not unique")
            changes = proof.get("call_target_differences", [])
            compare_explicit(entry, body, reference_bytes, [change["offset"] for change in changes])
            for change in changes:
                offset = change["offset"]
                target.transfer(entry["address"] + offset, change["french_target"])
                reference.transfer(proof["source_address"] + offset, change["reference_target"])
                callee = reference.by_address.get(change["reference_target"])
                anchor = anchor_by_address.get(change["french_target"])
                require(callee is not None and anchor is not None and
                        callee["name"] == anchor["name"] == change["reference_target_name"] and
                        callee["source"] == anchor["source"] == change["reference_target_source"],
                        f"{entry['name']}: explicit relocation has no independently named callee")
        corroboration(entry, target)
    data_path = registry_dir / 'reviewed-data-anchors.json'
    data_registry = json.loads(data_path.read_text(encoding='utf-8')) if data_path.exists() else None
    runtime_bounds = verify_data_anchors(data_registry, originals, by_address) if data_registry else None
    for anchor in anchors:
        require(anchor["identity_confirmation"] is True and anchor["eligible_for_progress"] is False,
                f"{anchor['name']}: identity or progress scope differs")
        if anchor.get('proof_kind') == 'explicit_allocator_neighborhood':
            require(runtime_bounds is not None, 'Allocator proof needs reviewed runtime memory bounds')
            verify_allocator(anchor, originals, by_address, runtime_bounds)
            continue
        body = verified_body(anchor, target)
        occurrences = 0
        for segment in target.loaded:
            span = target.read(segment["address"], segment["file_size"])
            position = span.find(body)
            while position >= 0:
                occurrences += 1
                position = span.find(body, position + 1)
        require(occurrences == anchor["corroboration"]["loaded_image_exact_occurrences"] == 1,
                f"{anchor['name']}: target bytes are not unique in the loaded image")
        for proof in provenance(anchor):
            reference, reference_bytes = reference_body(anchor, proof, originals)
            compare_explicit(anchor, body, reference_bytes, proof["changed_instruction_offsets"])
            group = anchor["corroboration"]["wrapper_group"]
            require(group["first_address"] + group["position"] * group["stride"] == anchor["address"],
                    f"{anchor['name']}: wrapper group position differs")
            for index in range(group["wrapper_count"]):
                french_address = group["first_address"] + index * group["stride"]
                reference_address = proof["source_address"] + french_address - anchor["address"]
                neighbor = reference.by_address.get(reference_address)
                require(neighbor is not None and neighbor["source"] == anchor["source"] and
                        neighbor["high"] - neighbor["low"] == group["wrapper_size"] and
                        normalized(target.read(french_address, group["wrapper_size"])) ==
                        normalized(reference.read(reference_address, group["wrapper_size"])),
                        f"{anchor['name']}: reference wrapper neighborhood differs")
            matches = [f for f in reference.functions if f["high"] - f["low"] == anchor["size"] and
                       normalized(reference.read(f["low"], anchor["size"])) == normalized(body)]
            require(len(matches) == proof["matching_named_dwarf_ranges"] == 1 and
                    matches[0]["low"] == proof["source_address"], f"{anchor['name']}: reference identity not unique")
        transfer = anchor["corroboration"]["internal_transfer"]
        target.transfer(anchor["address"] + transfer["offset"], transfer["address"],
                        2 if transfer["kind"] == "tail_jump" else 3)
        for use in anchor["reviewed_uses"]:
            owner = by_address.get(use["function_address"])
            require(owner is not None and owner["name"] == use["function"] and use["relocation_type"] == "R_MIPS_26" and
                    0 <= use["offset"] < owner["size"] and use["call_address"] == owner["address"] + use["offset"],
                    f"{anchor['name']}: reviewed call site lacks function ownership")
            target.transfer(use["call_address"], anchor["address"])
    return {"originals": len(originals), "reviewed_functions": len(functions),
            "reviewed_code_bytes": sum(entry["size"] for entry in functions), "call_anchors": len(anchors), "data_anchors": len(data_registry["anchors"]) if data_registry else 0}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=ROOT / "config/platforms/versions.json")
    parser.add_argument("--orig-dir", type=Path, default=ROOT / "orig")
    parser.add_argument("--registry-dir", type=Path, default=ROOT / "config/platforms" / TARGET)
    args = parser.parse_args()
    try:
        result = verify(args.manifest, args.orig_dir, args.registry_dir)
        print(f"Verified {result['originals']} original hashes, {result['reviewed_functions']} reviewed functions "
              f"({result['reviewed_code_bytes']} bytes), and {result['call_anchors']} independent call anchors, {result['data_anchors']} data anchors; "
              "no progress or source-link claim.")
    except (OSError, ValueError, KeyError, TypeError, struct.error) as error:
        parser.exit(1, f"error: {error}\n")


if __name__ == "__main__":
    main()
