"""Restore only reviewed x86 E8 calls and E9 tail transfers using independently named function entries.

Unlike absolute addresses, relative calls have no PE HIGHLOW record. Their
provenance is the decoded instruction and its actual destination, resolved against
original reviewed entries or actual source/dependency/runtime MAP symbols. Unknown/indirect
calls are rejected. Only the four displacement bytes enter COFF REL32 records.
"""
import struct


def normalize_calls(body: bytes, address: int, calls: list[dict], targets: dict[str, int]):
    import capstone as cs
    decoder = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
    decoder.detail = True
    instructions = list(decoder.disasm(body, address))
    if sum(i.size for i in instructions) != len(body):
        raise ValueError('Incomplete direct-call body decode')
    actual = {i.address - address + 1: i for i in instructions if i.group(cs.CS_GRP_CALL) or
              (i.mnemonic == 'jmp' and i.bytes[0] == 0xe9 and
               not address <= i.operands[0].imm < address + len(body))}
    expected = {call['offset']: call for call in calls}
    if len(expected) != len(calls) or set(expected) != set(actual):
        raise ValueError('Missing, repeated or unexpected direct call')
    result, relocations = bytearray(body), []
    for offset, call in sorted(expected.items()):
        instruction = actual[offset]
        name = call['symbol']
        if (name not in targets or instruction.size != 5 or instruction.bytes[0] != call.get('opcode', 0xe8) or
                call.get('opcode', 0xe8) not in (0xe8, 0xe9) or
                len(instruction.operands) != 1 or instruction.operands[0].type != cs.x86.X86_OP_IMM):
            raise ValueError('Transfer is not an allowed E8/E9 rel32 to an independently named function')
        destination = (address + offset + 4 + struct.unpack_from('<i', body, offset)[0]) & 0xffffffff
        if destination != targets[name] or instruction.operands[0].imm != destination:
            raise ValueError('Original/source call destination differs from named function')
        if address <= destination < address + len(body):
            raise ValueError('Internal calls are outside the reviewed external-call profile')
        struct.pack_into('<I', result, offset, 0)
        relocations.append({'offset': offset, 'symbol': name, 'addend': 0, 'type': 20})
    normalized = bytes(result)
    if reconstruct_calls(normalized, address, relocations, targets) != body:
        raise ValueError('Direct-call inverse reconstruction failed')
    return normalized, relocations


def reconstruct_calls(body: bytes, address: int, relocations: list[dict], targets: dict[str, int]) -> bytes:
    result, previous = bytearray(body), -4
    for relocation in sorted(relocations, key=lambda r: r['offset']):
        offset, name = relocation['offset'], relocation['symbol']
        if (relocation.get('type') != 20 or relocation.get('addend') != 0 or name not in targets or
                not previous + 4 <= offset <= len(body) - 4 or offset < 1 or body[offset - 1] not in (0xe8, 0xe9) or
                struct.unpack_from('<I', body, offset)[0] != 0):
            raise ValueError('Invalid reviewed COFF REL32 call')
        previous = offset
        struct.pack_into('<I', result, offset, (targets[name] - (address + offset + 4)) & 0xffffffff)
    return bytes(result)
