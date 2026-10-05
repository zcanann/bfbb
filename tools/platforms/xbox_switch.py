"""Validate the reviewed twelve-way MSVC unsigned-index switch form.

The table is data in .text, outside the recovered instruction extent. Unknown
indirect transfers remain rejected; this does not accept arbitrary table shapes.
"""
from __future__ import annotations

import hashlib
import struct


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def decode_switch(text: bytes, base: int, entry: int, bound: int,
                  instruction, spec: dict) -> dict:
    require(set(spec) == {'kind', 'symbol'} and spec.get('kind') == 'msvc_esi_u32_12' and isinstance(spec.get('symbol'), str)
            and bool(spec['symbol']), 'Unsupported finite switch specification')
    address = instruction.address
    offset = address - base
    require(instruction.size == 7 and bytes(instruction.bytes[:3]) == bytes.fromhex('ff24b5'),
            'Switch is not JMP dword ptr [ESI*4+disp32]')
    # CMP ESI,11; PUSH EBX; PUSH EDI; JA default; JMP table[ESI].
    # The two pushes preserve both ESI and the comparison flags.
    guard = address - 11
    require(entry <= guard and text[offset - 11:offset - 4] == bytes.fromhex('83fe0b53570f87'),
            'Switch lacks its reviewed unsigned twelve-way guard')
    default = address + struct.unpack_from('<i', text, offset - 4)[0]
    table = struct.unpack_from('<I', instruction.bytes, 3)[0]
    require(entry <= default < bound and default != address and base <= table and table + 48 <= base + len(text),
            'Switch default or table lies outside its mapped code ownership')
    raw = text[table - base:table - base + 48]
    entries = list(struct.unpack('<12I', raw))
    require(all(entry <= target < min(bound, table) for target in entries),
            'Switch table target leaves the function or enters table data')
    return {'kind': spec['kind'], 'symbol': spec['symbol'], 'guard_address': guard,
            'dispatch_address': address, 'default_address': default, 'address': table,
            'size': 48, 'entries': entries, 'sha256': hashlib.sha256(raw).hexdigest()}


def validate_switch_cfg(tables: list[dict], edges: list[list[int]], decoded: dict,
                        extent_end: int, spec: dict | None) -> None:
    require(len(tables) == (1 if spec is not None else 0), 'Missing or repeated reviewed switch')
    for table in tables:
        guard, dispatch = table['guard_address'], table['dispatch_address']
        # Only the checked fallthrough path may enter guard suffix or dispatch.
        # This prevents branches from bypassing the range check.
        for node, predecessor in ((guard + 3, guard), (guard + 4, guard + 3),
                                  (guard + 5, guard + 4), (dispatch, guard + 5)):
            require({a for a, b in edges if b == node} == {predecessor} and node in decoded,
                    'Switch guard does not dominate its dispatch')
        require(all(target in decoded for target in table['entries']) and
                table['default_address'] in decoded and table['address'] >= extent_end,
                'Switch destinations lack complete CFG coverage or table overlaps code')


def source_switch_anchors(tables: list[dict], highlow: set[int], map_bound: int) -> dict:
    result = {}
    for table in tables:
        require(table['address'] + table['size'] <= map_bound,
                'Source switch table crosses the next independently named MAP function')
        entries = set(range(table['address'], table['address'] + 48, 4))
        require(table['dispatch_address'] + 3 in highlow and
                {a for a in highlow if table['address'] <= a < table['address'] + 48} == entries,
                'Source switch pointer fields lack exact PE HIGHLOW coverage')
        result[table['symbol']] = {'address': table['address'], 'size': 48}
    return result


def switch_anchors_from_text(text: bytes, base: int, function: dict) -> dict:
    """Replay recorded original proof against authenticated .text bytes."""
    cfg = function.get('corroboration', {}).get('closed_cfg', {})
    spec = cfg.get('switch_table')
    if spec is None:
        return {}
    from .xbox_source import _leaf_extent
    start, size = function['address'], function['size']
    actual = _leaf_extent(text, base, start, start + size,
                          {a: 'original-call' for a in cfg['direct_call_targets']}, spec)
    require(actual['size'] == size and actual['instruction_bytes'] == cfg['instruction_bytes']
            and actual['internal_gap_ranges'] == cfg['internal_gap_ranges']
            and actual['switch_tables'] == cfg['switch_tables'],
            'Original finite switch proof or table bytes changed')
    return {t['symbol']: {'address': t['address'], 'size': t['size']}
            for t in actual['switch_tables']}


def original_switch_anchors(original, function: dict) -> dict:
    if function.get('corroboration', {}).get('closed_cfg', {}).get('switch_table') is None:
        return {}
    section = original.section(function['address'], function['size'])
    require(section['name'] == '.text', 'Original switch function lacks text ownership')
    base = section['virtual_address']
    text = original.read(base, min(section['raw_size'], section['virtual_size']))
    return switch_anchors_from_text(text, base, function)


def switch_signature(entry: int, tables: list[dict]) -> list[dict]:
    """Address-independent case mapping, after each image's own CFG proof."""
    return [{'kind': table['kind'], 'symbol': table['symbol'],
             'default_offset': table['default_address'] - entry,
             'target_offsets': [target - entry for target in table['entries']]}
            for table in tables]


def verify_switch_correspondence(extent: dict, expected: list[dict] | None) -> None:
    # Restricted to identical case layouts for now. Different code layouts need
    # a richer correspondence proof, not a silently normalized dispatch pointer.
    require(expected is not None and len(expected) == 1 and
            switch_signature(extent['address'], extent.get('switch_tables', [])) == expected,
            'Source switch targets differ from independently verified original case mapping')
