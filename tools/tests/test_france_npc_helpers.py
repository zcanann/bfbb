"""Authenticated complete NPC motion/goal cluster regression checks."""
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools"))
from platforms.verify_reviewed import Original,REFERENCES,TARGET
from platforms.france_npc_helpers import generate_unit,INDEPENDENT


@unittest.skipUnless(os.environ.get("BFBB_FRANCE_TEST_ORIG"),"Set BFBB_FRANCE_TEST_ORIG for original fixtures")
class NPCHelpersOriginalTests(unittest.TestCase):
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

    def mutate(self,address,body):
        target=self.originals[TARGET]
        segment=next(s for s in target.loaded if s["address"]<=address and address+len(body)<=s["address"]+s["file_size"])
        offset=segment["offset"]+address-segment["address"]
        saved=target.data;changed=bytearray(saved);changed[offset:offset+len(body)]=body;target.data=bytes(changed)
        return saved

    def test_complete_three_original_cluster_and_json(self):
        self.assertEqual(json.loads(json.dumps(self.proof)),self.proof)
        self.assertEqual(self.proof["counts"]["code_bytes"],6400)
        self.assertEqual(len(self.proof["functions"]),31)
        for f in self.proof["functions"]:
            self.assertEqual({p["version"] for p in f["provenance"]},set(REFERENCES))
            self.assertTrue(f["corroboration"]["local_control_flow"]["passes"])
            self.assertTrue(all(not p["data_address_operands"] for p in f["provenance"]))
        for row in self.proof["data_proofs"]:
            self.assertEqual(len(row["complete_direct_transfers"]),34)
            self.assertEqual(row["changed_data_operands"],0)

    def test_changed_helper_caller_or_transfer_rejects(self):
        target=self.originals[TARGET]
        transfer=self.proof["data_proofs"][0]["complete_direct_transfers"][0]
        for address in (0x2d1250,0x2afff0,transfer["caller_address"]+transfer["offset"]):
            with self.subTest(address=hex(address)):
                saved=self.mutate(address,bytes([target.read(address,1)[0]^1]))
                try:
                    with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
                finally:target.data=saved

    def test_duplicate_complete_helper_rejects(self):
        target=self.originals[TARGET]
        saved=self.mutate(0x400000,target.read(0x2d1250,112))
        try:
            with self.assertRaisesRegex(ValueError,"not uniquely located"):
                generate_unit(self.originals,self.registry)
        finally:target.data=saved

    def test_missing_or_changed_independent_identity_rejects(self):
        known={f["address"]:f for path in self.registry.glob("*functions.json")
               for f in json.loads(path.read_text())["functions"] if f["address"] in INDEPENDENT}
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/"test-functions.json"
            rows=list(known.values())
            path.write_text(json.dumps({"functions":rows[:-1]}))
            with self.assertRaisesRegex(ValueError,"dependencies missing"):
                generate_unit(self.originals,Path(directory))
            changed=json.loads(json.dumps(rows));changed[0]["sha256"]="0"*64
            path.write_text(json.dumps({"functions":changed}))
            with self.assertRaises(ValueError):generate_unit(self.originals,Path(directory))


if __name__=="__main__":unittest.main()
