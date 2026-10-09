"""Original-backed complete smoothing identity, typed-data and ambiguity tests."""
import json,os,sys,tempfile,unittest
from pathlib import Path
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from platforms.verify_reviewed import Original,REFERENCES,TARGET
from platforms import france_npc_smooth as smooth


@unittest.skipUnless(os.environ.get('BFBB_FRANCE_TEST_ORIG'),'Set BFBB_FRANCE_TEST_ORIG for original fixtures')
class NPCSmoothOriginalTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        records=json.loads((ROOT/'config/platforms/versions.json').read_text())['versions']
        cls.originals={v:Original(v,records[v],Path(os.environ['BFBB_FRANCE_TEST_ORIG'])) for v in (*REFERENCES,TARGET)}
        for v,o in cls.originals.items():
            layout=json.loads((ROOT/'config/platforms'/v/'region-layout.json').read_text());assert layout['executable_sha1']==o.sha1
            o._stream_regions={r['name']:r for r in layout['regions']}
        cls.registry=Path(os.environ.get('BFBB_FRANCE_TEST_REGISTRY',ROOT/'config/platforms/SLES-53623'))
        cls.proof=smooth.generate_unit(cls.originals,cls.registry)

    def mutate(self,address,body):
        target=self.originals[TARGET]
        segment=next(s for s in target.loaded if s['address']<=address and address+len(body)<=s['address']+s['file_size'])
        offset=segment['offset']+address-segment['address'];saved=target.data
        changed=bytearray(saved);changed[offset:offset+len(body)]=body;target.data=bytes(changed)
        return saved

    def test_complete_originals_json_and_scope(self):
        self.assertEqual(json.loads(json.dumps(self.proof)),self.proof)
        self.assertEqual(self.proof['counts']['code_bytes'],508)
        f=self.proof['functions'][0]
        self.assertEqual({p['version'] for p in f['provenance']},set(REFERENCES))
        self.assertTrue(f['corroboration']['local_control_flow']['passes'])
        self.assertFalse(f['corroboration']['whole_translation_unit_claimed'])
        for p in self.proof['sequence_proofs']:
            self.assertEqual(p['uniqueness']['matching_addresses'],[smooth.START])
            self.assertEqual(p['uniqueness']['masked_instruction_count'],3)
            self.assertEqual(p['uniqueness']['unchanged_anchor_bytes'],296)
        with tempfile.TemporaryDirectory() as directory:
            (Path(directory)/'future-functions.json').write_text('{"functions":[{"address":4194304}]}')
            self.assertEqual(smooth.generate_unit(self.originals,Path(directory)),self.proof)

    def test_body_address_gp_and_padding_mutations_reject(self):
        target=self.originals[TARGET]
        for address in (smooth.START,smooth.START+28,smooth.START+252,smooth.START+smooth.SIZE):
            saved=self.mutate(address,bytes([target.read(address,1)[0]^1]))
            try:
                with self.assertRaises(ValueError):smooth.generate_unit(self.originals,self.registry)
            finally:target.data=saved

    def test_complete_duplicate_rejects(self):
        target=self.originals[TARGET];saved=self.mutate(0x400000,target.read(smooth.START,smooth.SIZE))
        try:
            with self.assertRaisesRegex(ValueError,'not uniquely located'):smooth.generate_unit(self.originals,self.registry)
        finally:target.data=saved

    def test_data_payload_bss_extent_and_gp_lifetime_reject(self):
        target=self.originals[TARGET]
        for address,body in ((smooth.TARGETS['yews']+12,b'\x01'),(0x10014c,(0x3c040051).to_bytes(4,'little'))):
            saved=self.mutate(address,body)
            try:
                with self.assertRaises(ValueError):smooth.generate_unit(self.originals,self.registry)
            finally:target.data=saved
        region=target._stream_regions['runtime_bss'];saved=region['size']
        region['size']=smooth.TARGETS['prepute']+60-region['address']
        try:
            with self.assertRaisesRegex(ValueError,'escapes original storage'):smooth.generate_unit(self.originals,self.registry)
        finally:region['size']=saved
        section=next(s for s in target.metadata['sections'] if s['name']=='.reginfo')
        saved=target.data;changed=bytearray(saved);changed[section['offset']+20]^=1;target.data=bytes(changed)
        try:
            with self.assertRaisesRegex(ValueError,'GP differs'):smooth.generate_unit(self.originals,self.registry)
        finally:target.data=saved

    def test_original_owner_name_extent_required(self):
        original=self.originals[REFERENCES[0]];a=self.proof['functions'][0]['provenance'][0]['source_address']
        f=next(f for f in original.functions if f['low']==a)
        for key,value in (('high',f['high']+4),('source','SB/Game/zNPCTypeRobot.cpp'),('name','OtherSmooth')):
            saved=f[key];f[key]=value
            try:
                with self.assertRaises(ValueError):smooth.generate_unit(self.originals,self.registry)
            finally:f[key]=saved

    def test_typed_local_owner_signedness_and_array_count_required(self):
        decoder=smooth.iter_dies
        for mode in ('owner','signedness','array_count'):
            def changed(data):
                for off,tag,owner,attrs in decoder(data):
                    attrs=dict(attrs)
                    if mode=='owner' and attrs.get(3)=='prepute':owner='C:/Other.cpp'
                    if mode=='signedness' and attrs.get(3)=='init':attrs[5]=9
                    if mode=='array_count' and attrs.get(10)==smooth.FLOAT_ARRAY:
                        desc=bytearray(attrs[10]);desc[7]=4;attrs[10]=bytes(desc)
                    yield off,tag,owner,attrs
            with patch.object(smooth,'iter_dies',changed):
                with self.assertRaises(ValueError):smooth.generate_unit(self.originals,self.registry)

if __name__=='__main__':unittest.main()
