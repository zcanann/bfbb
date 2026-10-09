"""Exact COP1 MOV.S recognition must not hide address-register clobbers."""
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
from platforms.france_tu_sequences import address_pair, gpr_writes


class Cop1AddressEffectsTests(unittest.TestCase):
    def test_all_mov_s_source_and_destination_registers_preserve_gprs(self):
        for fs in range(32):
            for fd in range(32):
                word = 0x46000006 | fs << 11 | fd << 6
                self.assertEqual(gpr_writes(word), set())

    def test_other_formats_reserved_ft_and_unreviewed_functions_reject(self):
        for word in (0x46206346, 0x46806346, 0x46016346, 0x46006340, 0x46006347):
            with self.subTest(word=hex(word)), self.assertRaises(ValueError):
                gpr_writes(word)

    def test_mov_s_preserves_lui_but_mfc1_and_cfc1_clobber_it(self):
        # lui t0,0x50 / middle / addiu t0,t0,-0x20.
        def pair(middle):
            return address_pair(struct.pack("<III", 0x3C080050, middle, 0x2508FFE0), 0x100000, 8)
        self.assertEqual(pair(0x46006346), (0, 0x4FFFE0))
        for middle in (0x44086000, 0x4448F800):
            self.assertEqual(gpr_writes(middle), {8})
            with self.assertRaisesRegex(ValueError, "not produced by LUI"):
                pair(middle)
        self.assertEqual(pair(0x44096000), (0, 0x4FFFE0))


if __name__ == "__main__":
    unittest.main()
