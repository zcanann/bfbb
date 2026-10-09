"""Optional authenticated-original proof and adversarial operand checks."""
import json
import os
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from platforms.verify_reviewed import Original, REFERENCES, TARGET
from platforms.france_cruise_tweaks import ANCHOR, CLUSTER_KIND, generate_unit


@unittest.skipUnless(os.environ.get("BFBB_FRANCE_TEST_ORIG"), "Set BFBB_FRANCE_TEST_ORIG for original fixtures")
class CruiseOriginalTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        records = json.loads((ROOT / "config/platforms/versions.json").read_text())["versions"]
        cls.originals = {v: Original(v, records[v], Path(os.environ["BFBB_FRANCE_TEST_ORIG"]))
                         for v in (*REFERENCES, TARGET)}
        for version, original in cls.originals.items():
            layout = json.loads((ROOT / "config/platforms" / version / "region-layout.json").read_text())
            assert layout["executable_sha1"] == original.sha1
            original._stream_regions = {r["name"]: r for r in layout["regions"]}
        cls.registry = ROOT / "config/platforms/SLES-53623"
        cls.proof = generate_unit(cls.originals, cls.registry)

    def test_complete_original_cluster_is_unique_and_three_version_backed(self):
        self.assertEqual(self.proof["counts"]["code_bytes"], 11980)
        self.assertEqual(len(self.proof["functions"]), 1)
        for record in self.proof["functions"]:
            self.assertEqual(record["confirmation_kind"], CLUSTER_KIND)
            self.assertTrue(record["corroboration"]["local_control_flow"]["passes"])
            self.assertEqual({p["version"] for p in record["provenance"]}, set(REFERENCES))
        self.assertTrue(all(row["uniqueness"]["matching_addresses"] == [ANCHOR]
                            for row in self.proof["sequence_proofs"]))

    def test_changed_instruction_string_or_known_callee_is_rejected(self):
        target = self.originals[TARGET]
        literal = self.proof["data_proofs"][0]["complete_strings"][0]["target_address"]
        saved = target.data
        field = 0x20F260
        for address in (ANCHOR, literal, field):
            with self.subTest(address=hex(address)):
                segment = next(s for s in target.loaded if s["address"] <= address < s["address"] + s["file_size"])
                offset = segment["offset"] + address - segment["address"]
                altered = bytearray(saved)
                altered[offset] ^= 1
                target.data = bytes(altered)
                try:
                    with self.assertRaises(ValueError):
                        generate_unit(self.originals, self.registry)
                finally:
                    target.data = saved


if __name__ == "__main__":
    unittest.main()
