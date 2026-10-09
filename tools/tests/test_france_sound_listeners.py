"""Authenticated original checks for the independent sound listener cluster."""
import json
import os
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools"))
from platforms.verify_reviewed import Original,REFERENCES,TARGET
from platforms.france_sound_listeners import MEMBERS,generate_unit,original_data


@unittest.skipUnless(os.environ.get("BFBB_FRANCE_TEST_ORIG"),"Set BFBB_FRANCE_TEST_ORIG for original fixtures")
class SoundListenerOriginalTests(unittest.TestCase):
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

    def test_all_originals_unique_closed_and_independent(self):
        self.assertEqual(self.proof["counts"]["code_bytes"],1988)
        self.assertEqual(len(self.proof["functions"]),9)
        for f in self.proof["functions"]:
            self.assertEqual({p["version"] for p in f["provenance"]},set(REFERENCES))
            self.assertTrue(f["corroboration"]["local_control_flow"]["passes"])
            self.assertFalse(f["corroboration"]["whole_translation_unit_claimed"])
        for row in self.proof["sequence_proofs"]:
            self.assertEqual(len(row["complete_bodies"]),9)
            for body in row["complete_bodies"]:
                self.assertEqual(body["uniqueness"]["matching_addresses"],[body["address"]])

    def test_changed_instruction_field_or_internal_call_rejects(self):
        target=self.originals[TARGET]
        saved=target.data
        operand=next(p for p in self.proof["data_proofs"][0]["typed_fields"]
                     if p["caller"]=="xSndProcessSoundPos")
        for address in (MEMBERS[0][1],0x20a650+operand["lo_offset"],0x20a530+260):
            with self.subTest(address=hex(address)):
                segment=next(s for s in target.loaded if s["address"]<=address<s["address"]+s["file_size"])
                offset=segment["offset"]+address-segment["address"]
                changed=bytearray(saved);changed[offset]^=1;target.data=bytes(changed)
                try:
                    with self.assertRaises(ValueError):generate_unit(self.originals,None)
                finally:target.data=saved

    def test_duplicate_complete_body_rejects(self):
        target=self.originals[TARGET]
        address=0x400000
        segment=next(s for s in target.loaded if s["address"]<=address<s["address"]+s["file_size"])
        offset=segment["offset"]+address-segment["address"]
        saved=target.data
        changed=bytearray(saved);changed[offset:offset+72]=target.read(0x209550,72);target.data=bytes(changed)
        try:
            with self.assertRaisesRegex(ValueError,"not uniquely located"):
                generate_unit(self.originals,None)
        finally:target.data=saved

    def test_original_array_count_is_required(self):
        original=self.originals[REFERENCES[0]]
        snd,_=original_data(original)
        descriptor=bytes.fromhex(snd["voice_array"]["descriptor"])
        section=next(s for s in original.metadata["sections"] if s["name"]==".debug")
        die=section["offset"]+snd["voice_array"]["type_die"]
        length=int.from_bytes(original.data[die:die+4],"little")
        offset=original.data.index(descriptor,die,die+length)+7
        saved=original.data
        changed=bytearray(saved);changed[offset]^=1;original.data=bytes(changed)
        try:
            with self.assertRaisesRegex(ValueError,"array bound"):
                original_data(original)
        finally:original.data=saved


if __name__=="__main__":unittest.main()
