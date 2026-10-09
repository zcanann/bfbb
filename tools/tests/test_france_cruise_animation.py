"""Optional authenticated-original tests for the Cruise animation cluster."""
import json
import os
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools"))
from platforms.verify_reviewed import Original,REFERENCES,TARGET
from platforms.france_cruise_animation import generate_unit


@unittest.skipUnless(os.environ.get("BFBB_FRANCE_TEST_ORIG"),"Set BFBB_FRANCE_TEST_ORIG for original fixtures")
class CruiseAnimationOriginalTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        records=json.loads((ROOT/"config/platforms/versions.json").read_text())["versions"]
        cls.originals={v:Original(v,records[v],Path(os.environ["BFBB_FRANCE_TEST_ORIG"]))
                       for v in (*REFERENCES,TARGET)}
        for v,o in cls.originals.items():
            layout=json.loads((ROOT/"config/platforms"/v/"region-layout.json").read_text())
            assert layout["executable_sha1"]==o.sha1
            o._stream_regions={r["name"]:r for r in layout["regions"]}
        cls.registry=Path(os.environ.get("BFBB_FRANCE_TEST_REGISTRY",ROOT/"config/platforms/SLES-53623"))
        cls.proof=generate_unit(cls.originals,cls.registry)

    def test_complete_three_original_cluster(self):
        self.assertEqual(self.proof["counts"]["code_bytes"],1148)
        self.assertEqual(len(self.proof["functions"]),3)
        for f in self.proof["functions"]:
            self.assertEqual({p["version"] for p in f["provenance"]},set(REFERENCES))
            self.assertTrue(f["corroboration"]["local_control_flow"]["passes"])
        for data in self.proof["data_proofs"]:
            self.assertEqual(len(data["string_pointer_arrays"][0]["strings"]),37)
            self.assertEqual(len(data["opaque_runtime_contexts"]),3)
            self.assertTrue(all(c["no_identity_or_extent_claim"] and c["transfer_word_unmasked"]
                                for c in data["opaque_runtime_contexts"]))

    def test_mutated_body_padding_array_field_or_opaque_context_rejects(self):
        target=self.originals[TARGET]
        data=self.proof["data_proofs"][0]
        last_string=data["string_pointer_arrays"][0]["strings"][-1]
        addresses=(0x2a45a0,0x2a440c,last_string["target_address"],0x2a4410+16,0x118c08)
        saved=target.data
        for address in addresses:
            with self.subTest(address=hex(address)):
                segment=next(s for s in target.loaded if s["address"]<=address<s["address"]+s["file_size"])
                offset=segment["offset"]+address-segment["address"]
                changed=bytearray(saved);changed[offset]^=1;target.data=bytes(changed)
                try:
                    with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
                finally:target.data=saved


if __name__=="__main__":unittest.main()
