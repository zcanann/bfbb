"""Original-backed robotic-goal streak cluster checks."""
import json
import os
from pathlib import Path
import sys
import unittest

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools"))
from platforms.verify_reviewed import Original,REFERENCES,TARGET
from platforms.france_goalrobo_streaks import generate_unit,original_streaks


@unittest.skipUnless(os.environ.get("BFBB_FRANCE_TEST_ORIG"),"Set BFBB_FRANCE_TEST_ORIG for original fixtures")
class GoalRoboStreakOriginalTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        records=json.loads((ROOT/"config/platforms/versions.json").read_text())["versions"]
        cls.originals={v:Original(v,records[v],Path(os.environ["BFBB_FRANCE_TEST_ORIG"]))
                       for v in (*REFERENCES,TARGET)}
        for v,o in cls.originals.items():
            layout=json.loads((ROOT/"config/platforms"/v/"region-layout.json").read_text())
            assert layout["executable_sha1"]==o.sha1
            o._stream_regions={r["name"]:r for r in layout["regions"]}
        cls.proof=generate_unit(cls.originals,None)

    def test_all_original_complete_bodies_and_json_roundtrip(self):
        self.assertEqual(json.loads(json.dumps(self.proof)),self.proof)
        self.assertEqual(self.proof["counts"]["code_bytes"],2808)
        self.assertEqual(len(self.proof["functions"]),4)
        for f in self.proof["functions"]:
            self.assertEqual({p["version"] for p in f["provenance"]},set(REFERENCES))
            self.assertTrue(f["corroboration"]["local_control_flow"]["passes"])
        for row in self.proof["data_proofs"]:
            self.assertEqual([a["count"] for a in row["typed_streak_array"]["arrays"]],[10,50,2])
            self.assertEqual(row["independent_complete_consumer_addresses"],[0x1e8170,0x1e8240])

    def test_changed_body_call_or_typed_operand_rejects(self):
        target=self.originals[TARGET];saved=target.data
        for address,mask in ((0x2b7590,1),(0x2b8a50+580,1),(0x1e8170+20,4),(0x1e8240+20,4)):
            with self.subTest(address=hex(address)):
                segment=next(s for s in target.loaded if s["address"]<=address<s["address"]+s["file_size"])
                offset=segment["offset"]+address-segment["address"]
                changed=bytearray(saved);changed[offset]^=mask;target.data=bytes(changed)
                try:
                    with self.assertRaises(ValueError):generate_unit(self.originals,None)
                finally:target.data=saved

    def test_duplicate_complete_caller_rejects(self):
        target=self.originals[TARGET];address=0x400000
        segment=next(s for s in target.loaded if s["address"]<=address<s["address"]+s["file_size"])
        offset=segment["offset"]+address-segment["address"]
        saved=target.data;changed=bytearray(saved);changed[offset:offset+1636]=target.read(0x2b7590,1636);target.data=bytes(changed)
        try:
            with self.assertRaisesRegex(ValueError,"not uniquely located"):
                generate_unit(self.originals,None)
        finally:target.data=saved

    def test_original_nested_ring_array_bound_is_required(self):
        original=self.originals[REFERENCES[0]]
        array=self.proof["data_proofs"][0]["typed_streak_array"]["arrays"][1]
        section=next(s for s in original.metadata["sections"] if s["name"]==".debug")
        die=section["offset"]+array["type_die"];length=int.from_bytes(original.data[die:die+4],"little")
        offset=original.data.index(bytes.fromhex(array["descriptor"]),die,die+length)+7
        saved=original.data;changed=bytearray(saved);changed[offset]^=1;original.data=bytes(changed)
        try:
            with self.assertRaisesRegex(ValueError,"array bound or element"):
                original_streaks(original)
        finally:original.data=saved


if __name__=="__main__":unittest.main()
