"""Original-owned internal J labels keep genuine ELF REL addends."""
import copy
import json
import os
from pathlib import Path
import struct
import sys
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from platforms import ps2_source
from platforms.ps2_report import _target_object
from tools.tests.test_ps2_target_symbols import elf_symbols_and_relocations

SOURCE = 'SB/Core/p2/iMorph.cpp'
KERNELS = {'FastS16unpack': (148, 168), 'FastS16weight2': (180, 200),
           'FastS16weight4': (244, 264)}


class InternalJumpTests(unittest.TestCase):
    def fixture(self):
        function = dict(source=SOURCE, name='kernel', low=0x1000, high=0x1020,
                        bytes=struct.pack('<8I', 0x08000404, 0, 0, 0, 0, 0, 0x03e00008, 0))
        call = dict(function='kernel', offset=0, target_source=SOURCE,
                    target_function='kernel', symbol='kernel__Fv', opcode=2, interior_offset=16)
        unit = dict(target_unit=SOURCE, symbols={'kernel': 'kernel__Fv'}, calls=[call])
        return function, call, unit

    def prepare(self, function, unit):
        with patch.object(ps2_source, 'load_profile', return_value={'units': [unit]}):
            return ps2_source.prepare_functions([function], b'', [], 'fixture')

    def test_addend_and_defined_symbol_survive_elf_writer(self):
        f, call, unit = self.fixture()
        evidence = self.prepare(f, unit)
        self.assertEqual(struct.unpack_from('<I', f['bytes'])[0], 0x08000004)
        self.assertEqual((evidence[0]['symbol_address'], evidence[0]['addend'],
                          evidence[0]['target_address']), (0x1000, 16, 0x1010))
        symbols, relocs = elf_symbols_and_relocations(_target_object([f], 0))
        self.assertEqual(len(relocs), 1)
        offset, info = relocs[0]
        self.assertEqual((offset, info & 255), (0, 4))
        self.assertEqual(symbols[info >> 8], ('kernel__Fv', 1, 0x12, 32))
        # Resolve the REL field as a linker does; it must reconstruct retail.
        normalized = struct.unpack_from('<I', f['bytes'])[0]
        self.assertEqual((normalized & 0xfc000000) |
                         (((normalized & 0x3ffffff) * 4 + f['low']) >> 2), 0x08000404)

    def test_entry_jal_default_is_unchanged(self):
        for opcode in (2, 3):
            f, call, unit = self.fixture()
            f['bytes'] = struct.pack('<I', (opcode << 26) | 0x400) + f['bytes'][4:]
            if opcode == 3:
                del call['opcode']
            del call['interior_offset']
            evidence = self.prepare(f, unit)
            self.assertEqual(struct.unpack_from('<I', f['bytes'])[0], opcode << 26)
            self.assertNotIn('addend', evidence[0])
            self.assertNotIn('symbol_address', evidence[0])

    def test_invalid_labels_opcodes_symbols_and_operands_reject(self):
        changes = [{'interior_offset': x} for x in (0, -4, 2, 32, 36, True, 16.0, '16')]
        changes += [{'opcode': 3}, {'symbol': 'another__Fv'}, {'interior_offset': 20}]
        for change in changes:
            with self.subTest(change=change):
                f, call, unit = self.fixture()
                call.update(change)
                with self.assertRaises(ValueError): self.prepare(f, unit)
        for word in (0x0c000404, 0x08000405):
            f, call, unit = self.fixture()
            f['bytes'] = struct.pack('<I', word) + f['bytes'][4:]
            with self.assertRaises(ValueError): self.prepare(f, unit)

    def test_other_function_and_extent_mismatch_reject(self):
        f, call, unit = self.fixture()
        other = dict(f, name='other', low=0x2000, high=0x2020)
        unit['symbols']['other'] = 'other__Fv'
        call['target_function'] = 'other'
        f['bytes'] = struct.pack('<I', 0x08000804) + f['bytes'][4:]
        with patch.object(ps2_source, 'load_profile', return_value={'units': [unit]}):
            with self.assertRaisesRegex(ValueError, 'within its own original function'):
                ps2_source.prepare_functions([f, other], b'', [], 'fixture')
        for high in (0x101c, 0x1024, None):
            f, call, unit = self.fixture()
            f['high'] = high
            with self.assertRaises(ValueError): self.prepare(f, unit)


@unittest.skipUnless(os.environ.get('BFBB_FRANCE_TEST_ORIG'), 'Set BFBB_FRANCE_TEST_ORIG for original fixtures')
class OriginalMorphJumpTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from platforms.verify_reviewed import Original, REFERENCES, TARGET
        records = json.loads((ROOT / 'config/platforms/versions.json').read_text())['versions']
        cls.originals = {v: Original(v, records[v], Path(os.environ['BFBB_FRANCE_TEST_ORIG']))
                         for v in (*REFERENCES, TARGET)}
        cls.units = [u for u in ps2_source.load_profile()['units'] if u['target_unit'] == SOURCE]

    def test_all_twelve_original_labels_replay_and_reject_changed_claims(self):
        checked = 0
        for version, original in self.originals.items():
            records = json.loads((ROOT / 'config/platforms' / version / 'symbols.json').read_text())['symbols']
            members = [dict(source=SOURCE, name=r['name'], low=r['address'], high=r['address'] + r['size'],
                            bytes=original.read(r['address'], r['size']))
                       for r in records if r['source'] == SOURCE and r['name'] in KERNELS]
            self.assertEqual(len(members), 3)
            unit = copy.deepcopy(next(u for u in self.units if ps2_source.profile_enabled(u, original.sha1)))
            unit['symbols'] = {k: v for k, v in unit['symbols'].items() if k in KERNELS}
            unit['calls'] = [c for c in unit['calls'] if 'interior_offset' in c]
            unit.pop('gp_relocations', None)
            self.assertEqual(len(unit['calls']), 3)
            for call in unit['calls']:
                with self.subTest(version=version, function=call['function']):
                    f = next(f for f in members if f['name'] == call['function'])
                    off, interior = KERNELS[f['name']]
                    self.assertEqual((call['offset'], call['interior_offset'], call['opcode']), (off, interior, 2))
                    word = struct.unpack_from('<I', f['bytes'], off)[0]
                    self.assertEqual(word, 0x08000000 | ((f['low'] + interior) >> 2))
                    self.assertLess(interior, f['high'] - f['low'])
                    checked += 1
            fresh = copy.deepcopy(members)
            with patch.object(ps2_source, 'load_profile', return_value={'units': [unit]}):
                evidence = ps2_source.prepare_functions(fresh, original.data, original.metadata['segments'],
                                                       original.sha1, metadata=original.metadata)
            self.assertEqual(len(evidence), 3)
            for f, old in zip(fresh, members):
                off, interior = KERNELS[f['name']]
                word = struct.unpack_from('<I', f['bytes'], off)[0]
                self.assertEqual(word, 0x08000000 | (interior >> 2))
                restored = bytearray(f['bytes'])
                struct.pack_into('<I', restored, off, (word & 0xfc000000) | ((f['low'] + interior) >> 2))
                self.assertEqual(bytes(restored), old['bytes'])
            for index in range(3):
                bad = copy.deepcopy(unit)
                bad['calls'][index]['interior_offset'] += 4
                with patch.object(ps2_source, 'load_profile', return_value={'units': [bad]}):
                    with self.assertRaises(ValueError):
                        ps2_source.prepare_functions(copy.deepcopy(members), original.data,
                                                     original.metadata['segments'], original.sha1,
                                                     metadata=original.metadata)
                altered = copy.deepcopy(members)
                call = unit['calls'][index]
                changed = next(f for f in altered if f['name'] == call['function'])
                payload = bytearray(changed['bytes'])
                word = struct.unpack_from('<I', payload, call['offset'])[0]
                struct.pack_into('<I', payload, call['offset'], word ^ 1)
                changed['bytes'] = bytes(payload)
                with patch.object(ps2_source, 'load_profile', return_value={'units': [unit]}):
                    with self.assertRaises(ValueError):
                        ps2_source.prepare_functions(altered, original.data,
                                                     original.metadata['segments'], original.sha1,
                                                     metadata=original.metadata)
        self.assertEqual(checked, 12)


if __name__ == '__main__':
    unittest.main()
