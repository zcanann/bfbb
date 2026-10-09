"""Original-backed sound playback and scoped runtime-context regression checks."""
import json
import os
from pathlib import Path
import sys
import unittest

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools"))
from platforms.verify_reviewed import Original,REFERENCES,TARGET
from platforms.france_sound_playback import generate_unit,small_runtime_context,flush_uniqueness,playback_data
from platforms.france_tu_sequences import compare,unique_template


@unittest.skipUnless(os.environ.get("BFBB_FRANCE_TEST_ORIG"),"Set BFBB_FRANCE_TEST_ORIG for original fixtures")
class SoundPlaybackOriginalTests(unittest.TestCase):
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

    def changed(self,address,body):
        target=self.originals[TARGET]
        segment=next(s for s in target.loaded if s["address"]<=address and address+len(body)<=s["address"]+s["file_size"])
        offset=segment["offset"]+address-segment["address"]
        saved=target.data;changed=bytearray(saved);changed[offset:offset+len(body)]=body;target.data=bytes(changed)
        self.addCleanup(setattr,target,"data",saved)
        return saved

    def test_all_originals_and_exact_vector_sequence(self):
        self.assertEqual(json.loads(json.dumps(self.proof)),self.proof)
        self.assertEqual(self.proof["counts"]["code_bytes"],4924)
        self.assertEqual(len(self.proof["functions"]),9)
        for f in self.proof["functions"]:
            self.assertEqual({p["version"] for p in f["provenance"]},set(REFERENCES))
            self.assertTrue(f["corroboration"]["local_control_flow"]["passes"])
        for row in self.proof["data_proofs"]:
            self.assertEqual(len(row["typed_fields"]),36)
            self.assertEqual(len(row["opaque_runtime_contexts"]),9)
        for row in self.proof["sequence_proofs"]:
            vectors=[b for b in row["complete_bodies"] if b["source"].endswith("/xVec3.cpp")]
            self.assertEqual([b["uniqueness"]["member_offset"] for b in vectors],[0,224])
            self.assertTrue(all(b["uniqueness"]["matching_addresses"]==[0x210c30] for b in vectors))

    def test_runtime_body_branch_return_and_padding_are_exact(self):
        original,target=self.originals[REFERENCES[0]],self.originals[TARGET]
        for offset in (0,8,12,16,20):
            with self.subTest(offset=offset):
                saved=self.changed(0x114b50+offset,bytes([target.read(0x114b50+offset,1)[0]^1]))
                try:
                    with self.assertRaises(ValueError):small_runtime_context(original,target)
                finally:target.data=saved

    def test_runtime_duplicate_rejects(self):
        original,target=self.originals[REFERENCES[0]],self.originals[TARGET]
        self.changed(0x400000,target.read(0x114b50,20))
        with self.assertRaisesRegex(ValueError,"not unique"):small_runtime_context(original,target)

    def test_flush_and_vector_duplicate_sequences_reject(self):
        original,target=self.originals[REFERENCES[0]],self.originals[TARGET]
        flush=next(f for f in self.proof["functions"] if f["name"]=="HISFlushAsyncRequestsNoWait")
        a=flush["provenance"][0]["source_address"]
        _,_,masks=compare(original,target,a,0x34bb00,84)
        saved=self.changed(0x400000,target.read(0x34bb00,84))
        try:
            with self.assertRaisesRegex(ValueError,"not uniquely located"):
                flush_uniqueness(original,target,a,0x34bb00,masks)
        finally:target.data=saved
        self.changed(0x400000,target.read(0x210c30,448))
        with self.assertRaisesRegex(ValueError,"not uniquely located"):
            unique_template(target,target.read(0x210c30,448),{},0x210c30)

    def test_changed_literal_caller_transfer_rejects(self):
        target=self.originals[TARGET]
        address=0x1b8cc0+504
        self.changed(address,bytes([target.read(address,1)[0]^1]))
        with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)

    def test_changed_complete_diagnostic_string_rejects(self):
        target=self.originals[TARGET]
        literal=self.proof["data_proofs"][0]["complete_strings"][0]
        address=literal["target_address"]+literal["size"]-2
        self.changed(address,bytes([target.read(address,1)[0]^1]))
        with self.assertRaisesRegex(ValueError,"diagnostic literal"):
            generate_unit(self.originals,self.registry)

    def test_original_file_array_bound_is_required(self):
        original=self.originals[REFERENCES[0]]
        array=next(o for o in self.proof["data_proofs"][0]["typed_objects"] if o["name"]=="eeFiles")
        section=next(s for s in original.metadata["sections"] if s["name"]==".debug")
        die=section["offset"]+array["type_die"]
        length=int.from_bytes(original.data[die:die+4],"little")
        offset=original.data.index(bytes.fromhex(array["descriptor"]),die,die+length)+7
        saved=original.data;changed=bytearray(saved);changed[offset]^=1;original.data=bytes(changed)
        try:
            with self.assertRaisesRegex(ValueError,"array bound or type"):
                playback_data(original)
        finally:original.data=saved


if __name__=="__main__":unittest.main()
