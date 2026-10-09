"""Optional original-backed animation-builder proof and mutation checks."""
import json
import os
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from platforms.verify_reviewed import Original, REFERENCES, TARGET
from platforms.france_player_animation_tables import MEMBERS, animation_literal, generate_unit


class AnimationLiteralTests(unittest.TestCase):
    def test_complete_long_ascii_and_invalid_or_unterminated_rejection(self):
        class Bytes:
            def __init__(self, data):
                self.data = data
            def read(self, address, size):
                return self.data[address:address + size]
        value = b"idle " * 100 + b"\0"
        self.assertEqual(animation_literal(Bytes(value), 0), value)
        for value in (b"x" * 4096, b"idle\x01\0"):
            with self.assertRaises(ValueError):
                animation_literal(Bytes(value), 0)


@unittest.skipUnless(os.environ.get("BFBB_FRANCE_TEST_ORIG"), "Set BFBB_FRANCE_TEST_ORIG for original fixtures")
class PlayerAnimationTableOriginalTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        records = json.loads((ROOT / "config/platforms/versions.json").read_text())["versions"]
        cls.originals = {v: Original(v, records[v], Path(os.environ["BFBB_FRANCE_TEST_ORIG"]))
                         for v in (*REFERENCES, TARGET)}
        for version, original in cls.originals.items():
            layout = json.loads((ROOT / "config/platforms" / version / "region-layout.json").read_text())
            assert layout["executable_sha1"] == original.sha1
            original._stream_regions = {r["name"]: r for r in layout["regions"]}
        cls.registry = Path(os.environ.get("BFBB_FRANCE_TEST_REGISTRY", ROOT / "config/platforms/SLES-53623"))
        cls.proof = generate_unit(cls.originals, cls.registry)

    def test_five_complete_original_builders_have_unique_closed_bodies(self):
        self.assertEqual(self.proof["counts"]["code_bytes"], 32904)
        self.assertEqual({f["address"] for f in self.proof["functions"]}, {m[1] for m in MEMBERS})
        for record in self.proof["functions"]:
            self.assertEqual({p["version"] for p in record["provenance"]}, set(REFERENCES))
            self.assertTrue(record["corroboration"]["local_control_flow"]["passes"])
            self.assertFalse(record["corroboration"]["whole_translation_unit_claimed"])
        self.assertTrue(all(p["uniqueness"]["matching_addresses"] == [p["target_start"]]
                            for p in self.proof["sequence_proofs"]))

    def test_changed_body_literal_or_callback_pointer_rejects(self):
        target = self.originals[TARGET]
        literal = next(p for row in self.proof["data_proofs"] for p in row.get("complete_strings", [])
                       if p["size"] > 4)
        callback = next(p for row in self.proof["data_proofs"] for p in row.get("known_code_pointers", []))
        saved = target.data
        addresses = (MEMBERS[0][1], literal["target_address"] + 2,
                     MEMBERS[0][1] + callback["lo_offset"], callback["target_address"])
        for address in addresses:
            with self.subTest(address=hex(address)):
                segment = next(s for s in target.loaded if s["address"] <= address < s["address"] + s["file_size"])
                offset = segment["offset"] + address - segment["address"]
                changed = bytearray(saved)
                changed[offset] ^= 1
                target.data = bytes(changed)
                try:
                    with self.assertRaises(ValueError):
                        generate_unit(self.originals, self.registry)
                finally:
                    target.data = saved

    def test_changed_complete_player_context_or_long_string_rejects(self):
        target = self.originals[TARGET]
        rows = next(row["complete_nonpromoted_player_callback_contexts"] for row in self.proof["data_proofs"]
                    if "complete_nonpromoted_player_callback_contexts" in row)
        idle = next(row for row in rows if row["name"] == "IdleCB")
        timer = next(n for row in rows for n in row["complete_callee_contexts"] if n["name"] == "oob_timer")
        long_string = next(p for row in self.proof["data_proofs"] for p in row.get("complete_strings", [])
                           if p["size"] > 300)
        addresses = (idle["target_address"] + 0x9c, timer["target_address"] + 4,
                     long_string["target_address"] + 300)
        saved = target.data
        for address in addresses:
            with self.subTest(address=hex(address)):
                segment = next(s for s in target.loaded if s["address"] <= address < s["address"] + s["file_size"])
                offset = segment["offset"] + address - segment["address"]
                changed = bytearray(saved)
                changed[offset] ^= 1
                target.data = bytes(changed)
                try:
                    with self.assertRaises(ValueError):
                        generate_unit(self.originals, self.registry)
                finally:
                    target.data = saved

    def test_duplicate_complete_timer_context_is_rejected(self):
        from platforms.france_player_animation_context import OriginalData, oob_context
        original, target = self.originals[REFERENCES[0]], self.originals[TARGET]
        rows = next(row["complete_nonpromoted_player_callback_contexts"] for row in self.proof["data_proofs"]
                    if "complete_nonpromoted_player_callback_contexts" in row)
        timer = next(n for row in rows for n in row["complete_callee_contexts"] if n["name"] == "oob_timer")
        address = 0x400000
        segment = next(s for s in target.loaded if s["address"] <= address < s["address"] + s["file_size"])
        offset = segment["offset"] + address - segment["address"]
        saved = target.data
        changed = bytearray(saved)
        changed[offset:offset+52] = target.read(timer["target_address"], 52)
        target.data = bytes(changed)
        try:
            with self.assertRaisesRegex(ValueError, "Complete OOB timer context is not unique"):
                oob_context(original,target,timer["source_address"],timer["target_address"],OriginalData(original))
        finally:
            target.data = saved


if __name__ == "__main__":
    unittest.main()
