"""Authenticated complete boss animation-builder regression checks."""
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools"))
from platforms.verify_reviewed import Original,REFERENCES,TARGET
from platforms.france_npc_remaining_animation import generate_unit,INDEPENDENT,OriginalArrays,MEMBERS


@unittest.skipUnless(os.environ.get("BFBB_FRANCE_TEST_ORIG"),"Set BFBB_FRANCE_TEST_ORIG for original fixtures")
class NPCRemainingAnimationOriginalTests(unittest.TestCase):
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

    def test_complete_original_bodies_and_json(self):
        self.assertEqual(json.loads(json.dumps(self.proof)),self.proof)
        self.assertEqual(self.proof["counts"]["code_bytes"],5116)
        self.assertEqual(len(self.proof["functions"]),5)
        self.assertEqual(len(self.proof["sequence_proofs"]),15)
        for f in self.proof["functions"]:
            self.assertEqual({p["version"] for p in f["provenance"]},set(REFERENCES))
            self.assertTrue(f["corroboration"]["local_control_flow"]["passes"])
        for row in self.proof["data_proofs"]:
            self.assertEqual(len(row["complete_direct_transfers"]),69)
            self.assertEqual(len(row["operands"]),128)
            self.assertEqual(sum(p["size"] for p in row["operands"] if p["kind"]=="initializer"),236)

    def test_body_call_callback_and_alignment_mutations_reject(self):
        target=self.originals[TARGET]
        call=self.proof["data_proofs"][0]["complete_direct_transfers"][0]
        for address in (0x336f70+100,call["caller_address"]+call["offset"],0x214010+40,0x336f70+1852):
            with self.subTest(address=hex(address)):
                saved=self.mutate(address,bytes([target.read(address,1)[0]^1]))
                try:
                    with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
                finally:target.data=saved

    def test_duplicate_complete_cluster_rejects(self):
        target=self.originals[TARGET];saved=self.mutate(0x400000,target.read(0x336f70,1852))
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
        tables=proof["typed_string_tables_by_source"]["SB/Game/zNPCTypeBossPatrick.cpp"]
        string=tables["g_strz_bossanim"]["strings"][-1]
        init=next(p for p in proof["operands"] if p["kind"]=="initializer")
        literal=next(p for p in proof["operands"] if p["kind"]=="string")
        addresses=(0x4e0180,0x4e0150,string["target_address"]+string["size"]-2,
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
        pair=next(p for p in self.proof["data_proofs"][0]["operands"] if p["kind"]=="initializer")
        record=next(f for f in self.proof["functions"] if f["address"]==pair["caller_address"])
        a=record["provenance"][0]["source_address"];function=next(f for f in original.functions if f["low"]==a)
        data=OriginalArrays(original,target,function["source"])
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
                        OriginalArrays(original,target,function["source"]).initializer(function,pair,pair["count"])
                finally:original.data=saved

    def test_initializer_copy_cannot_hide_a_control_transfer(self):
        original=self.originals[REFERENCES[0]];target=self.originals[TARGET]
        pair=next(p for p in self.proof["data_proofs"][0]["operands"] if p["kind"]=="initializer")
        record=next(f for f in self.proof["functions"] if f["address"]==pair["caller_address"])
        a=record["provenance"][0]["source_address"];function=next(f for f in original.functions if f["low"]==a)
        data=OriginalArrays(original,target,function["source"])
        segment=next(s for s in original.loaded if s["address"]<=a<s["address"]+s["file_size"])
        offset=segment["offset"]+a-segment["address"]
        saved=original.data;changed=bytearray(saved);changed[offset:offset+4]=(0x10000001).to_bytes(4,"little")
        original.data=bytes(changed)
        try:
            with self.assertRaisesRegex(ValueError,"bounded-copy prefix"):
                data.initializer(function,pair,pair["count"])
        finally:original.data=saved

    def test_initializer_owner_count_and_copy_coverage_are_required(self):
        original=self.originals[REFERENCES[0]];target=self.originals[TARGET]
        pair=next(p for p in self.proof["data_proofs"][0]["operands"] if p["kind"]=="initializer")
        record=next(f for f in self.proof["functions"] if f["address"]==pair["caller_address"])
        a=record["provenance"][0]["source_address"];function=next(f for f in original.functions if f["low"]==a)
        data=OriginalArrays(original,target,function["source"])
        with self.assertRaisesRegex(ValueError,"identity ambiguous"):
            data.initializer({**function,"source":"SB/Game/zNPCTypeRobot.cpp"},pair,pair["count"])
        with self.assertRaisesRegex(ValueError,"owner or extent"):
            data.initializer(function,pair,pair["count"]+1)
        address=a+pair["copy_stores"][-1]["instruction_offset"]
        segment=next(s for s in original.loaded if s["address"]<=address<s["address"]+s["file_size"])
        offset=segment["offset"]+address-segment["address"]
        saved=original.data;changed=bytearray(saved);changed[offset:offset+4]=bytes(4);original.data=bytes(changed)
        try:
            with self.assertRaises(ValueError):data.initializer(function,pair,pair["count"])
        finally:original.data=saved

    def test_sandy_loop_control_stride_and_final_store_are_literal(self):
        original=self.originals[REFERENCES[0]];target=self.originals[TARGET]
        pair=next(p for p in self.proof["data_proofs"][0]["operands"] if p["kind"]=="initializer")
        record=next(f for f in self.proof["functions"] if f["address"]==pair["caller_address"])
        a=record["provenance"][0]["source_address"];function=next(f for f in original.functions if f["low"]==a)
        data=OriginalArrays(original,target,function["source"])
        segment=next(s for s in original.loaded if s["address"]<=a<s["address"]+s["file_size"])
        offset=segment["offset"]+a-segment["address"]
        for position in (24,36,48,56,60,88):
            saved=original.data;changed=bytearray(saved);changed[offset+position]^=1;original.data=bytes(changed)
            try:
                with self.assertRaisesRegex(ValueError,"bounded-copy prefix"):
                    data.initializer(function,pair,pair["count"])
            finally:original.data=saved


if __name__=="__main__":unittest.main()
