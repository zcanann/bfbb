"""Verify original particle callback registration and closed callback boundaries.

The evidence comes only from authenticated originals: straight-line registration
stores establish entry witnesses; decoded local CFGs establish extents. No source
object or source-match score participates in this proof.
"""
from __future__ import annotations

import struct


def verify_callback(original, function: dict) -> None:
    import capstone as cs
    from capstone.x86 import X86_OP_IMM, X86_OP_MEM, X86_OP_REG
    from .xbox_source import _leaf_extent

    def require(condition, message):
        if not condition:
            raise ValueError(message)

    proof = function['corroboration']['callback_registration']
    initializer = proof['initializer']
    original.check_hash(initializer)
    decoder = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
    decoder.detail = True
    raw = original.read(initializer['address'], initializer['size'])
    instructions = list(decoder.disasm(raw, initializer['address']))
    require(sum(i.size for i in instructions) == len(raw) and instructions[-1].mnemonic == 'ret',
            'Incomplete callback initializer decode')
    registers, stores = {}, {}
    table, count, stride = proof['table_address'], proof['slot_count'], proof['slot_stride']
    require(count == 35 and stride == 12, 'Unreviewed particle registration table shape')
    owners = [s for s in original.metadata['sections'] if s['virtual_address'] <= table and
              table + count * stride <= s['virtual_address'] + s['virtual_size'] and
              s['name'] == '.data' and s['flags'] & 1]
    require(len(owners) == 1, 'Callback table is not uniquely mapped writable data')
    for instruction in instructions:
        operands = instruction.operands
        if instruction.mnemonic in ('push', 'pop'):
            require(len(operands) == 1 and operands[0].type == X86_OP_REG, 'Unexpected registration stack operation')
            if instruction.mnemonic == 'pop':
                registers.pop(operands[0].reg, None)
            continue
        if instruction.mnemonic == 'ret':
            require(instruction.address + instruction.size == initializer['address'] + len(raw),
                    'Early initializer return')
            continue
        if instruction.mnemonic == 'xor':
            require(len(operands) == 2 and operands[0].type == operands[1].type == X86_OP_REG and
                    operands[0].reg == operands[1].reg and operands[0].size == 4,
                    'Unreviewed initializer arithmetic')
            registers[operands[0].reg] = 0
            continue
        require(instruction.mnemonic == 'mov' and len(operands) == 2 and
                operands[0].size == operands[1].size == 4, 'Unreviewed initializer instruction')
        left, right = operands
        require(right.type == X86_OP_IMM or right.type == X86_OP_REG and right.reg in registers,
                'Initializer value is not independently constant')
        value = (right.imm if right.type == X86_OP_IMM else registers[right.reg]) & 0xffffffff
        if left.type == X86_OP_REG:
            registers[left.reg] = value
        else:
            require(left.type == X86_OP_MEM and not left.mem.base and not left.mem.index and
                    table <= left.mem.disp <= table + count * stride - 4 and (left.mem.disp - table) % 4 == 0,
                    'Initializer store leaves callback table')
            require(left.mem.disp not in stores, 'Repeated callback registration field')
            stores[left.mem.disp] = (value, instruction.address)
    slot = proof['command_type']
    require(isinstance(slot, int) and 0 <= slot < count, 'Invalid callback type')
    base = table + slot * stride
    require(stores.get(base, (None,))[0] == slot and stores.get(base + 4, (None,))[0] == proof['asset_size'] and
            stores.get(base + 8) == (function['address'], proof['callback_store_address']),
            'Original command type, asset size or callback entry differs')
    start, size = function['address'], function['size']
    section = original.section(start, size)
    text_address = section['virtual_address']
    text = original.read(text_address, min(section['raw_size'], section['virtual_size']))
    targets = proof['direct_call_targets']
    require(len(targets) == len(set(targets)), 'Repeated CFG call target')
    extent = _leaf_extent(text, text_address, start, start + size,
                          {address: hex(address) for address in targets})
    require(extent['size'] == size and extent['instruction_bytes'] == function['instruction_bytes'] and
            extent['internal_gap_ranges'] == proof['internal_gap_ranges'], 'Callback CFG boundary differs')
    actual_calls = sorted({start + c['offset'] + 4 + struct.unpack_from('<i', original.read(start, size), c['offset'])[0]
                           for c in extent['direct_calls']})
    require(actual_calls == sorted(targets), 'Callback direct-call witness set differs')
