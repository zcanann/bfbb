"""Authenticated complete model-leaf and typed-storage regression checks."""
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools"))
from platforms.verify_reviewed import Original,REFERENCES,TARGET
from platforms.france_imodel_kernels import generate_unit,typed_storage,ANCHOR,MEMBERS


@unittest.skipUnless(os.environ.get("BFBB_FRANCE_TEST_ORIG"),"Set BFBB_FRANCE_TEST_ORIG for original fixtures")
class IModelKernelsOriginalTests(unittest.TestCase):
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

    def test_complete_original_leaves_and_json(self):
        self.assertEqual(json.loads(json.dumps(self.proof)),self.proof)
        self.assertEqual(self.proof["counts"]["code_bytes"],1272)
        self.assertEqual(len(self.proof["functions"]),4)
        for f in self.proof["functions"]:
            self.assertEqual({p["version"] for p in f["provenance"]},set(REFERENCES))
            self.assertTrue(f["corroboration"]["local_control_flow"]["passes"])
            self.assertTrue(all(not p["direct_transfers"] for p in f["provenance"]))
        for seq in self.proof["sequence_proofs"]:
            vu=next(f for f in seq["complete_bodies"] if f["target_address"]==0x1aee30)
            self.assertEqual(len(vu["raw_vector_instruction_words"]),42)
        for p in self.proof["data_proofs"]:
            self.assertEqual(len(p["operands"]),3)
            self.assertEqual({k:v["size"] for k,v in p["typed_storage"]["objects"].items()},
                             {"sMaterialColor":64,"sMaterialAlpha":16,"globals.camera.frustplane":192})

    def test_body_address_padding_and_raw_vector_bits_reject(self):
        target=self.originals[TARGET]
        for address in (0x1ad240+100,0x1ad240+12,0x1ad240+712,0x1aee30+24,0x1aee30+180):
            saved=self.mutate(address,bytes([target.read(address,1)[0]^1]))
            try:
                with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
            finally:target.data=saved

    def test_duplicate_complete_body_rejects(self):
        target=self.originals[TARGET];saved=self.mutate(0x400000,target.read(0x1ad240,712))
        try:
            with self.assertRaisesRegex(ValueError,"not uniquely located"):generate_unit(self.originals,self.registry)
        finally:target.data=saved

    def known(self):
        return {f["address"]:f for path in self.registry.glob("*functions.json")
                for f in json.loads(path.read_text())["functions"] if f["address"]==ANCHOR[0]}

    def test_missing_or_changed_global_anchor_rejects(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/"test-functions.json"
            path.write_text(json.dumps({"functions":[]}))
            with self.assertRaises(ValueError):generate_unit(self.originals,Path(directory))
            record=json.loads(json.dumps(self.known()[ANCHOR[0]]));record["sha256"]="0"*64
            path.write_text(json.dumps({"functions":[record]}))
            with self.assertRaises(ValueError):generate_unit(self.originals,Path(directory))

    def test_original_extent_is_required(self):
        original=self.originals[REFERENCES[0]];a=self.proof["functions"][0]["provenance"][0]["source_address"]
        f=next(f for f in original.functions if f["low"]==a);saved=f["high"];f["high"]+=4
        try:
            with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
        finally:f["high"]=saved

    def test_original_array_count_element_and_frustum_offset_reject(self):
        original=self.originals[REFERENCES[0]];target=self.originals[TARGET];known=self.known()
        proof=self.proof["data_proofs"][0]["typed_storage"];objects=proof["objects"]
        section=next(s for s in original.metadata["sections"] if s["name"]==".debug")
        alpha=section["offset"]+objects["sMaterialAlpha"]["type_die"]
        size=int.from_bytes(original.data[alpha:alpha+4],"little")
        desc=bytes.fromhex("000a00000000000f0000000855000300")
        start=original.data.index(desc,alpha,alpha+size)
        plane=section["offset"]+objects["globals.camera.frustplane"]["member_die"]
        size=int.from_bytes(original.data[plane:plane+4],"little")
        loc=original.data.index(bytes.fromhex("047002000007"),plane,plane+size)+1
        for position in (start+7,start+len(desc)-2,loc):
            saved=original.data;changed=bytearray(saved);changed[position]^=1;original.data=bytes(changed)
            try:
                with self.assertRaises(ValueError):typed_storage(original,target,known)
            finally:original.data=saved

    def test_original_scalar_field_type_and_global_base_reject(self):
        original=self.originals[REFERENCES[0]];target=self.originals[TARGET];known=self.known()
        proof=self.proof["data_proofs"][0]["typed_storage"]
        section=next(s for s in original.metadata["sections"] if s["name"]==".debug")
        field=section["offset"]+proof["layouts"]["RwRGBA"]["members"][-1]["die_offset"]
        size=int.from_bytes(original.data[field:field+4],"little")
        position=original.data.index(bytes.fromhex("55000300"),field,field+size)+2
        decl=section["offset"]+proof["objects"]["globals.camera.frustplane"]["declaration_die"]
        size=int.from_bytes(original.data[decl:decl+4],"little")
        reference=proof["objects"]["globals.camera.frustplane"]["reference_address"]-624
        loc=original.data.index(b"\x03"+reference.to_bytes(4,"little"),decl,decl+size)+1
        for offset in (position,loc):
            saved=original.data;changed=bytearray(saved);changed[offset]^=1;original.data=bytes(changed)
            try:
                with self.assertRaises(ValueError):typed_storage(original,target,known)
            finally:original.data=saved

if __name__=="__main__":unittest.main()
