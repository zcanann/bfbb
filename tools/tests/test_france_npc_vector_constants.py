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
from platforms.france_npc_vector_constants import generate_unit,INDEPENDENT,original_constants
from platforms.france_goal_sequence import original_data,STANDARD
from platforms.dwarf1 import iter_dies
from platforms.ps2_type_layouts import aggregate_layouts


@unittest.skipUnless(os.environ.get("BFBB_FRANCE_TEST_ORIG"),"Set BFBB_FRANCE_TEST_ORIG for original fixtures")
class NPCVectorConstantsOriginalTests(unittest.TestCase):
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
        self.assertEqual(self.proof["counts"]["code_bytes"],6492)
        self.assertEqual(len(self.proof["functions"]),10)
        for f in self.proof["functions"]:
            self.assertEqual({p["version"] for p in f["provenance"]},set(REFERENCES))
            self.assertTrue(f["corroboration"]["local_control_flow"]["passes"])
        for row in self.proof["data_proofs"]:
            self.assertEqual(len(row["complete_direct_transfers"]),33)
            self.assertEqual(row["changed_data_operands"],33)

    def test_changed_helper_caller_or_transfer_rejects(self):
        target=self.originals[TARGET]
        transfer=self.proof["data_proofs"][0]["complete_direct_transfers"][0]
        for address in (0x2d6520,0x2af3f0,transfer["caller_address"]+transfer["offset"]):
            with self.subTest(address=hex(address)):
                saved=self.mutate(address,bytes([target.read(address,1)[0]^1]))
                try:
                    with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
                finally:target.data=saved

    def test_duplicate_complete_helper_rejects(self):
        target=self.originals[TARGET]
        saved=self.mutate(0x400000,target.read(0x2d6520,244))
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

    def test_vector_scalar_payload_anchor_and_opaque_context_reject(self):
        target=self.originals[TARGET]
        for address in (0x4f89a0,0x4f89b8,0x4f89c4,0x4f89d0,0x4fc508,0x2c70a0,0x114930+60):
            with self.subTest(address=hex(address)):
                saved=self.mutate(address,bytes([target.read(address,1)[0]^4]))
                try:
                    with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
                finally:target.data=saved

    def test_typed_vector_and_local_float_operands_reject(self):
        target=self.originals[TARGET]
        pairs=self.proof["data_proofs"][0]["typed_field_operands"]
        selected=[next(p for p in pairs if p["target_address"]==a) for a in
                  (0x4f89a0,0x4f89b4,0x4f89c8,0x4f89d0,0x4fc508,0x52cc14)]
        for pair in selected:
            address=pair["caller_address"]+pair["lo_offset"]
            with self.subTest(address=hex(address)):
                saved=self.mutate(address,bytes([target.read(address,1)[0]^4]))
                try:
                    with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
                finally:target.data=saved

    def test_original_vector_layout_is_required(self):
        original=self.originals[REFERENCES[0]]
        section=next(s for s in original.metadata["sections"] if s["name"]==".debug")
        debug=original.data[section["offset"]:section["offset"]+section["size"]]
        layout=aggregate_layouts(debug,"SB/Game/zNPCGoalRobo.cpp",{"xVec3"})["xVec3"]
        rows=list(iter_dies(debug));by={off:attrs for off,tag,owner,attrs in rows}
        member=next(m for m in layout["members"] if m["name"]=="y")
        off=member["die_offset"];attrs=by[off];die=section["offset"]+off
        length=int.from_bytes(original.data[die:die+4],"little")
        offset=original.data.index(attrs[2],die,die+length)+1
        saved=original.data;changed=bytearray(saved);changed[offset]^=4;original.data=bytes(changed)
        try:
            with self.assertRaisesRegex(ValueError,"vector original layout differs"):
                original_constants(original,self.originals[TARGET])
        finally:original.data=saved

    def test_original_local_float_type_is_required(self):
        original=self.originals[REFERENCES[0]]
        section=next(s for s in original.metadata["sections"] if s["name"]==".debug")
        debug=original.data[section["offset"]:section["offset"]+section["size"]]
        declarations=[off for off,tag,owner,attrs in iter_dies(debug)
                      if owner.replace("\\","/").endswith("SB/Game/zNPCGoalRobo.cpp") and
                      tag==12 and attrs.get(3)=="dst_tetherMax"]
        self.assertEqual(len(declarations),1)
        die=section["offset"]+declarations[0]
        length=int.from_bytes(original.data[die:die+4],"little")
        offset=original.data.index(bytes.fromhex("55000e00"),die,die+length)+2
        saved=original.data;changed=bytearray(saved);changed[offset]=7;original.data=bytes(changed)
        try:
            with self.assertRaisesRegex(ValueError,"not the reviewed float"):
                original_constants(original,self.originals[TARGET])
        finally:original.data=saved


if __name__=="__main__":unittest.main()
