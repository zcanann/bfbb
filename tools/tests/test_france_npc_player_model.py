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
from platforms.france_npc_player_model import generate_unit,INDEPENDENT
from platforms.france_goal_sequence import original_data,STANDARD
from platforms.dwarf1 import iter_dies
from platforms.ps2_type_layouts import aggregate_layouts


@unittest.skipUnless(os.environ.get("BFBB_FRANCE_TEST_ORIG"),"Set BFBB_FRANCE_TEST_ORIG for original fixtures")
class NPCPlayerModelOriginalTests(unittest.TestCase):
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
        self.assertEqual(self.proof["counts"]["code_bytes"],2948)
        self.assertEqual(len(self.proof["functions"]),8)
        for f in self.proof["functions"]:
            self.assertEqual({p["version"] for p in f["provenance"]},set(REFERENCES))
            self.assertTrue(f["corroboration"]["local_control_flow"]["passes"])
            self.assertTrue(all(p["data_address_operands"] for p in f["provenance"]))
        for row in self.proof["data_proofs"]:
            self.assertEqual(len(row["complete_direct_transfers"]),19)
            self.assertEqual(row["changed_data_operands"],9)

    def test_changed_helper_caller_or_transfer_rejects(self):
        target=self.originals[TARGET]
        transfer=self.proof["data_proofs"][0]["complete_direct_transfers"][0]
        for address in (0x31b290,0x2c1100,transfer["caller_address"]+transfer["offset"]):
            with self.subTest(address=hex(address)):
                saved=self.mutate(address,bytes([target.read(address,1)[0]^1]))
                try:
                    with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
                finally:target.data=saved

    def test_duplicate_complete_helper_rejects(self):
        target=self.originals[TARGET]
        saved=self.mutate(0x400000,target.read(0x31b290,104))
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

    def test_typed_component_anchor_and_opaque_context_reject(self):
        target=self.originals[TARGET]
        addresses=[p["caller_address"]+p["lo_offset"] for p in self.proof["data_proofs"][0]["typed_field_operands"]]
        addresses.extend((0x2c70a0,0x114690+60,0x114930+60))
        for address in addresses:
            with self.subTest(address=hex(address)):
                saved=self.mutate(address,bytes([target.read(address,1)[0]^4]))
                try:
                    with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
                finally:target.data=saved

    def test_original_model_member_offset_is_required(self):
        original=self.originals[REFERENCES[0]]
        section=next(s for s in original.metadata["sections"] if s["name"]==".debug")
        debug=original.data[section["offset"]:section["offset"]+section["size"]]
        layout=aggregate_layouts(debug,STANDARD,{"xEnt"})["xEnt"]
        rows=list(iter_dies(debug));by={off:attrs for off,tag,owner,attrs in rows}
        begin=layout["die_offset"];end=by[begin][1]
        members=[(off,attrs) for off,tag,owner,attrs in rows if begin<off<end and tag==13 and attrs.get(3)=="model"]
        self.assertEqual(len(members),1)
        off,attrs=members[0];die=section["offset"]+off;length=int.from_bytes(original.data[die:die+4],"little")
        offset=original.data.index(attrs[2],die,die+length)+1
        saved=original.data;changed=bytearray(saved);changed[offset]^=4;original.data=bytes(changed)
        try:
            with self.assertRaisesRegex(ValueError,"aggregate member missing"):original_data(original)
        finally:original.data=saved


if __name__=="__main__":unittest.main()
