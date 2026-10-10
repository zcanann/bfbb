"""Original-backed splash callers, complete neighboring helper and tail tests."""
import json,os,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from platforms.verify_reviewed import Original,REFERENCES,TARGET
from platforms.france_hazard_splashes import generate_unit,perp_tail,INDEPENDENT


@unittest.skipUnless(os.environ.get('BFBB_FRANCE_TEST_ORIG'),'Set BFBB_FRANCE_TEST_ORIG for original fixtures')
class HazardSplashesOriginalTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        records=json.loads((ROOT/'config/platforms/versions.json').read_text())['versions']
        cls.originals={v:Original(v,records[v],Path(os.environ['BFBB_FRANCE_TEST_ORIG'])) for v in (*REFERENCES,TARGET)}
        for v,o in cls.originals.items():
            layout=json.loads((ROOT/'config/platforms'/v/'region-layout.json').read_text());assert layout['executable_sha1']==o.sha1
            o._stream_regions={r['name']:r for r in layout['regions']}
        cls.registry=Path(os.environ.get('BFBB_FRANCE_TEST_REGISTRY',ROOT/'config/platforms/SLES-53623'))
        cls.proof=generate_unit(cls.originals,cls.registry)

    def mutate(self,address,body):
        target=self.originals[TARGET]
        segment=next(s for s in target.loaded if s['address']<=address and address+len(body)<=s['address']+s['file_size'])
        offset=segment['offset']+address-segment['address'];saved=target.data
        changed=bytearray(saved);changed[offset:offset+len(body)]=body;target.data=bytes(changed)
        return saved

    def test_complete_bodies_and_json(self):
        self.assertEqual(json.loads(json.dumps(self.proof)),self.proof)
        self.assertEqual(self.proof['counts']['code_bytes'],3136)
        self.assertEqual(len(self.proof['functions']),3)
        for f in self.proof['functions']:
            self.assertEqual({p['version'] for p in f['provenance']},set(REFERENCES))
            self.assertTrue(f['corroboration']['local_control_flow']['passes'])
        for p in self.proof['call_proofs']:self.assertEqual(len(p['complete_callees']),17)

    def test_helper_anchor_tail_delay_caller_and_padding_mutations_reject(self):
        target=self.originals[TARGET]
        for address in (0x31afa0+8,0x31afa0+48,0x31afa0+52,0x31afa0+56,0x31aed0+196,
                        0x31aed0+40,0x3c4560+40,0x3c4560+832,0x3c48f0+104,0x210d10+80):
            saved=self.mutate(address,bytes([target.read(address,1)[0]^1]))
            try:
                with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
            finally:target.data=saved

    def test_duplicate_complete_helper_cluster_and_callers_reject(self):
        target=self.originals[TARGET]
        for start,size in ((0x31aed0,264),(0x3c4560,908),(0x3c48f0,2172)):
            saved=self.mutate(0x400000,target.read(start,size))
            try:
                with self.assertRaisesRegex(ValueError,'not uniquely located'):generate_unit(self.originals,self.registry)
            finally:target.data=saved

    def test_original_owner_name_extent_and_cluster_membership_required(self):
        original=self.originals[REFERENCES[0]];a=self.proof['functions'][0]['provenance'][0]['source_address']
        f=next(f for f in original.functions if f['low']==a)
        for key,value in (('high',f['high']+4),('source','SB/Game/zNPCHazard.cpp'),('name','Wrong')):
            saved=f[key];f[key]=value
            try:
                with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
            finally:f[key]=saved
        fake={**f,'low':a-8,'high':a-4};original.functions.append(fake)
        try:
            with self.assertRaisesRegex(ValueError,'membership/order'):generate_unit(self.originals,self.registry)
        finally:original.functions.remove(fake)

    def test_tail_requires_exact_nonclobbering_word_inventory(self):
        target=self.originals[TARGET]
        for off,word in ((8,0x441f0000),(44,0x0080e82d),(44,0x0080f82d),(48,0x0c084344),(52,0)):
            saved=self.mutate(0x31afa0+off,word.to_bytes(4,'little'))
            try:
                with self.assertRaises(ValueError):perp_tail(target,0x31afa0,0x210d10)
            finally:target.data=saved

    def test_missing_changed_and_expanded_dependencies(self):
        known={f['address']:f for path in self.registry.glob('*functions.json') for f in json.loads(path.read_text())['functions'] if f['address'] in INDEPENDENT}
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'test-functions.json'
            path.write_text(json.dumps({'functions':list(known.values())[1:]}))
            with self.assertRaises(ValueError):generate_unit(self.originals,Path(directory))
            changed=json.loads(json.dumps(list(known.values())));changed[0]['sha256']='0'*64
            path.write_text(json.dumps({'functions':changed}))
            with self.assertRaises(ValueError):generate_unit(self.originals,Path(directory))
            path.write_text(json.dumps({'functions':list(known.values())+[{'address':0x400000}]}))
            self.assertEqual(generate_unit(self.originals,Path(directory)),self.proof)

if __name__=='__main__':unittest.main()
