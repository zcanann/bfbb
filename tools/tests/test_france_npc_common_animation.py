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
from platforms.france_npc_common_animation import generate_unit,INDEPENDENT,OriginalArrays,START,SPAN
from platforms.france_goal_sequence import original_data,STANDARD
from platforms.dwarf1 import iter_dies
from platforms.ps2_type_layouts import aggregate_layouts


@unittest.skipUnless(os.environ.get("BFBB_FRANCE_TEST_ORIG"),"Set BFBB_FRANCE_TEST_ORIG for original fixtures")
class NPCCommonAnimationOriginalTests(unittest.TestCase):
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

    def test_complete_original_cluster_and_json(self):
        self.assertEqual(json.loads(json.dumps(self.proof)),self.proof)
        self.assertEqual(self.proof["counts"]["code_bytes"],392)
        self.assertEqual(len(self.proof["functions"]),2)
        for row in self.proof["sequence_proofs"]:
            self.assertEqual(row["span_bytes"],400)
            self.assertEqual(sum(g["size"] for g in row["inter_body_alignment"]),8)
        for f in self.proof["functions"]:
            self.assertEqual({p["version"] for p in f["provenance"]},set(REFERENCES))
            self.assertTrue(f["corroboration"]["local_control_flow"]["passes"])
        for row in self.proof["data_proofs"]:
            self.assertEqual(len(row["complete_direct_transfers"]),6)
            self.assertEqual(len(row["operands"]),10)

    def test_body_call_callback_and_alignment_mutations_reject(self):
        target=self.originals[TARGET]
        gap=next(g for g in self.proof["sequence_proofs"][0]["inter_body_alignment"] if g["size"])
        call=self.proof["data_proofs"][0]["complete_direct_transfers"][0]
        for address in (0x2cfe70+64,call["caller_address"]+call["offset"],0x214010+40,gap["address"]):
            with self.subTest(address=hex(address)):
                saved=self.mutate(address,bytes([target.read(address,1)[0]^1]))
                try:
                    with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
                finally:target.data=saved

    def test_duplicate_complete_cluster_rejects(self):
        target=self.originals[TARGET];saved=self.mutate(0x400000,target.read(START,SPAN))
        try:
            with self.assertRaisesRegex(ValueError,"not uniquely located"):generate_unit(self.originals,self.registry)
        finally:target.data=saved

    def test_missing_or_changed_independent_identity_rejects(self):
        known={f["address"]:f for path in self.registry.glob("*functions.json")
               for f in json.loads(path.read_text())["functions"] if f["address"] in INDEPENDENT}
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/"test-functions.json";rows=list(known.values())
            path.write_text(json.dumps({"functions":rows[:-1]}))
            with self.assertRaisesRegex(ValueError,"dependencies missing"):generate_unit(self.originals,Path(directory))
            changed=json.loads(json.dumps(rows));changed[0]["sha256"]="0"*64
            path.write_text(json.dumps({"functions":changed}))
            with self.assertRaises(ValueError):generate_unit(self.originals,Path(directory))

    def test_table_and_literal_mutations_reject(self):
        target=self.originals[TARGET];proof=self.proof["data_proofs"][0]
        string=proof["typed_string_tables"]["g_strz_lassanim"]["strings"][-1]
        for address in (0x4de8e0,string["target_address"]+string["size"]-2,0x4fe467):
            with self.subTest(address=hex(address)):
                saved=self.mutate(address,bytes([target.read(address,1)[0]^1]))
                try:
                    with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
                finally:target.data=saved

    def test_original_extent_is_required(self):
        original=self.originals[REFERENCES[0]]
        ref=self.proof["functions"][0]["provenance"][0]["source_address"]
        f=next(f for f in original.functions if f["low"]==ref);saved=f["high"];f["high"]+=4
        try:
            with self.assertRaisesRegex(ValueError,"identity differs"):generate_unit(self.originals,self.registry)
        finally:f["high"]=saved

    def test_original_string_pointer_array_type_is_required(self):
        original=self.originals[REFERENCES[0]];target=self.originals[TARGET]
        data=OriginalArrays(original,target);table=data.tables["g_strz_lassanim"]
        section=next(s for s in original.metadata["sections"] if s["name"]==".debug")
        die=section["offset"]+table["type_die"];length=int.from_bytes(original.data[die:die+4],"little")
        desc=data.by[table["type_die"]][1][10]
        offset=original.data.index(desc,die,die+length)+len(desc)-2
        saved=original.data;changed=bytearray(saved);changed[offset]^=4;original.data=bytes(changed)
        try:
            with self.assertRaisesRegex(ValueError,"element type differs"):OriginalArrays(original,target)
        finally:original.data=saved


if __name__=="__main__":unittest.main()
