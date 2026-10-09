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
from platforms.france_npc_animation_tables import generate_unit,INDEPENDENT,OriginalArrays,START,SPAN
from platforms.france_goal_sequence import original_data,STANDARD
from platforms.dwarf1 import iter_dies
from platforms.ps2_type_layouts import aggregate_layouts


@unittest.skipUnless(os.environ.get("BFBB_FRANCE_TEST_ORIG"),"Set BFBB_FRANCE_TEST_ORIG for original fixtures")
class NPCAnimationOriginalTests(unittest.TestCase):
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
        self.assertEqual(self.proof["counts"]["code_bytes"],9808)
        self.assertEqual(len(self.proof["functions"]),17)
        for row in self.proof["sequence_proofs"]:
            self.assertEqual(row["span_bytes"],9884)
            self.assertEqual(sum(g["size"] for g in row["inter_body_alignment"]),76)
        for f in self.proof["functions"]:
            self.assertEqual({p["version"] for p in f["provenance"]},set(REFERENCES))
            self.assertTrue(f["corroboration"]["local_control_flow"]["passes"])
        for row in self.proof["data_proofs"]:
            self.assertEqual(len(row["complete_direct_transfers"]),149)
            self.assertEqual(len(row["operands"]),257)
            self.assertEqual(sum(p["kind"]=="initializer" for p in row["operands"]),12)

    def test_body_call_callback_and_alignment_mutations_reject(self):
        target=self.originals[TARGET]
        gap=next(g for g in self.proof["sequence_proofs"][0]["inter_body_alignment"] if g["size"])
        call=self.proof["data_proofs"][0]["complete_direct_transfers"][0]
        for address in (0x2e3b20+64,call["caller_address"]+call["offset"],0x214010+40,gap["address"],START+SPAN):
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

    def test_complete_tables_literals_and_initializer_payloads_reject(self):
        target=self.originals[TARGET];proof=self.proof["data_proofs"][0]
        tables=proof["typed_string_tables"]
        string=tables["g_strz_roboanim"]["strings"][-1]
        init=next(p for p in proof["operands"] if p["kind"]=="initializer")
        literal=next(p for p in proof["operands"] if p["kind"]=="string")
        addresses=(0x4deaa0,0x4deb48,string["target_address"]+string["size"]-2,
                   init["target_address"]+init["size"]-1,literal["target_address"])
        for address in addresses:
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

    def test_original_array_element_type_and_stack_location_are_required(self):
        original=self.originals[REFERENCES[0]];target=self.originals[TARGET]
        data=OriginalArrays(original,target)
        pair=next(p for p in self.proof["data_proofs"][0]["operands"] if p["kind"]=="initializer")
        record=next(f for f in self.proof["functions"] if f["address"]==pair["caller_address"])
        a=record["provenance"][0]["source_address"];function=next(f for f in original.functions if f["low"]==a)
        section=next(s for s in original.metadata["sections"] if s["name"]==".debug")
        type_die=section["offset"]+pair["type_die"]
        type_size=int.from_bytes(original.data[type_die:type_die+4],"little")
        type_offset=original.data.index(bytes.fromhex("0855000800"),type_die,type_die+type_size)+3
        declaration=section["offset"]+pair["declaration_die"]
        decl_size=int.from_bytes(original.data[declaration:declaration+4],"little")
        loc=data.by[pair["declaration_die"]][1][2]
        stack_offset=original.data.index(loc,declaration,declaration+decl_size)+6
        for offset in (type_offset,stack_offset):
            with self.subTest(offset=offset):
                saved=original.data;changed=bytearray(saved);changed[offset]^=4;original.data=bytes(changed)
                try:
                    with self.assertRaises(ValueError):
                        OriginalArrays(original,target).initializer(function,pair,pair["count"])
                finally:original.data=saved

    def test_initializer_copy_cannot_hide_a_control_transfer(self):
        original=self.originals[REFERENCES[0]];target=self.originals[TARGET]
        data=OriginalArrays(original,target)
        pair=next(p for p in self.proof["data_proofs"][0]["operands"] if p["kind"]=="initializer")
        record=next(f for f in self.proof["functions"] if f["address"]==pair["caller_address"])
        a=record["provenance"][0]["source_address"];function=next(f for f in original.functions if f["low"]==a)
        segment=next(s for s in original.loaded if s["address"]<=a<s["address"]+s["file_size"])
        offset=segment["offset"]+a-segment["address"]
        saved=original.data;changed=bytearray(saved);changed[offset:offset+4]=(0x10000001).to_bytes(4,"little")
        original.data=bytes(changed)
        try:
            with self.assertRaisesRegex(ValueError,"not straight-line"):
                data.initializer(function,pair,pair["count"])
        finally:original.data=saved


if __name__=="__main__":unittest.main()
