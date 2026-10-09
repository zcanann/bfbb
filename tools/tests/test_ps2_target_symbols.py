"""Check emitted ELF relocation ownership independently of the target writer."""
import struct
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools'))
from platforms.ps2_report import _target_object


def elf_symbols_and_relocations(data):
    header = struct.unpack_from('<HHIIIIIHHHHHH', data, 16)
    sections = [struct.unpack_from('<10I', data, header[5] + i * header[10])
                for i in range(header[11])]
    table = next(section for section in sections if section[1] == 2)
    strings = sections[table[6]]
    names = data[strings[4]:strings[4] + strings[5]]
    symbols = []
    for offset in range(table[4], table[4] + table[5], 16):
        name, value, size, info, other, owner = struct.unpack_from('<IIIBBH', data, offset)
        symbols.append((names[name:names.index(0, name)].decode(), owner, info, size))
    relocations = [struct.unpack_from('<II', data, offset)
                   for section in sections if section[1] == 9
                   for offset in range(section[4], section[4] + section[5], 8)]
    return symbols, relocations


class TargetSymbolTests(unittest.TestCase):
    def functions(self):
        caller = {'name': 'caller', 'linkage_name': 'caller__Fv', 'low': 0x1000,
                  'bytes': struct.pack('<8I', 0x0c000000, 0, 0x0c000000, 0,
                                       0x0c000000, 0, 0x03e00008, 0),
                  'relocations': [{'offset': 0, 'symbol': 'caller__Fv'},
                                  {'offset': 8, 'symbol': 'callee__Fv'},
                                  {'offset': 16, 'symbol': 'external__Fv'}]}
        callee = {'name': 'callee', 'linkage_name': 'callee__Fv', 'low': 0x2000,
                  'bytes': struct.pack('<2I', 0x03e00008, 0)}
        return [caller, callee]

    def test_self_and_internal_calls_reference_real_function_definitions(self):
        symbols, relocations = elf_symbols_and_relocations(_target_object(self.functions(), 0))
        for offset, expected in ((0, 'caller__Fv'), (8, 'callee__Fv')):
            entry = next(info for at, info in relocations if at == offset)
            symbol = symbols[entry >> 8]
            self.assertEqual(entry & 0xff, 4)
            self.assertEqual(symbol[0], expected)
            self.assertGreater(symbol[1], 0)
            self.assertEqual(symbol[2], 0x12)
            self.assertGreater(symbol[3], 0)
            self.assertEqual(sum(row[0] == expected for row in symbols), 1)

    def test_external_call_stays_undefined(self):
        symbols, relocations = elf_symbols_and_relocations(_target_object(self.functions(), 0))
        entry = next(info for at, info in relocations if at == 16)
        self.assertEqual(symbols[entry >> 8], ('external__Fv', 0, 0x10, 0))

    def test_duplicate_definition_cannot_redirect_a_call(self):
        functions = self.functions()
        functions[1]['linkage_name'] = functions[0]['linkage_name']
        with self.assertRaisesRegex(ValueError, 'Duplicate target function linkage'):
            _target_object(functions, 0)


if __name__ == '__main__':
    unittest.main()
