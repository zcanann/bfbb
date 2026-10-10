"""Original-backed semantic call binding disambiguation and negative controls."""
import json,os,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from platforms.verify_reviewed import Original,REFERENCES,TARGET
from platforms.france_hazard_oil_splash import generate_unit,INDEPENDENT,START,SIZE,CALLS
from platforms.france_tu_sequences import unique_template


@unittest.skipUnless(os.environ.get('BFBB_FRANCE_TEST_ORIG'),'Set BFBB_FRANCE_TEST_ORIG for original fixtures')
class HazardOilSplashOriginalTests(unittest.TestCase):
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

    def test_complete_body_json_and_bound_template(self):
        self.assertEqual(json.loads(json.dumps(self.proof)),self.proof)
        self.assertEqual(self.proof['counts']['code_bytes'],1084)
        self.assertEqual({p['version'] for p in self.proof['functions'][0]['provenance']},set(REFERENCES))
        for p in self.proof['sequence_proofs']:
            evidence=p['placement_evidence'];self.assertEqual(evidence['search_masks'],{})
            self.assertEqual(evidence['uniqueness']['matching_addresses'],[START])
            self.assertEqual(evidence['uniqueness']['masked_instruction_count'],0)
        self.assertTrue(self.proof['functions'][0]['corroboration']['local_control_flow']['passes'])

    def test_unbound_template_really_is_ambiguous(self):
        original=self.originals[REFERENCES[0]];a=self.proof['functions'][0]['provenance'][0]['source_address']
        with self.assertRaisesRegex(ValueError,'not uniquely located'):
            unique_template(self.originals[TARGET],original.read(a,SIZE),{o:0xfc000000 for o,_ in CALLS},START)

    def test_body_callee_call_and_padding_mutations_reject(self):
        target=self.originals[TARGET]
        for address in (START+20,START+996,START+SIZE,0x3b52c0+40,0x31aed0+40):
            saved=self.mutate(address,bytes([target.read(address,1)[0]^1]))
            try:
                with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
            finally:target.data=saved

    def test_exact_duplicate_rejects_but_different_callee_is_distinct(self):
        target=self.originals[TARGET];duplicate=bytearray(target.read(START,SIZE))
        saved=self.mutate(0x400000,duplicate)
        try:
            with self.assertRaisesRegex(ValueError,'not uniquely located'):generate_unit(self.originals,self.registry)
        finally:target.data=saved
        # TarTar's different complete emitter destination must not become an
        # OilSplash identity merely because all other arithmetic agrees.
        duplicate[996:1000]=(0x0c000000|(0x3b4610>>2)).to_bytes(4,'little')
        saved=self.mutate(0x400000,duplicate)
        try:self.assertEqual(generate_unit(self.originals,self.registry),self.proof)
        finally:target.data=saved

    def test_original_owner_name_and_extent_required(self):
        original=self.originals[REFERENCES[0]];a=self.proof['functions'][0]['provenance'][0]['source_address']
        f=next(f for f in original.functions if f['low']==a)
        for key,value in (('high',f['high']+4),('source','SB/Game/zNPCSupport.cpp'),('name','TarTarSplash')):
            saved=f[key];f[key]=value
            try:
                with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
            finally:f[key]=saved

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
