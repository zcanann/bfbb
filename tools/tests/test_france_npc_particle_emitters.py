"""Original-backed ordered particle emitter clusters and array ownership tests."""
import json,os,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from platforms.verify_reviewed import Original,REFERENCES,TARGET
from platforms.france_npc_particle_emitters import generate_unit,original_arrays,INDEPENDENT,GROUPS


@unittest.skipUnless(os.environ.get('BFBB_FRANCE_TEST_ORIG'),'Set BFBB_FRANCE_TEST_ORIG for original fixtures')
class ParticleEmittersOriginalTests(unittest.TestCase):
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

    def test_complete_groups_and_json(self):
        self.assertEqual(json.loads(json.dumps(self.proof)),self.proof)
        self.assertEqual(self.proof['counts']['code_bytes'],2904)
        self.assertEqual(len(self.proof['functions']),12)
        for f in self.proof['functions']:
            self.assertEqual({p['version'] for p in f['provenance']},set(REFERENCES))
            self.assertTrue(f['corroboration']['local_control_flow']['passes'])
        for p in self.proof['sequence_proofs']:
            self.assertEqual([len(g['member_addresses']) for g in p['complete_groups']],[4,8])
        for p in self.proof['call_proofs']:
            self.assertTrue(all(c['transfer_word_unmasked'] and c['no_identity_or_extent_claim'] for c in p['opaque_runtime_contexts']))

    def test_body_operand_padding_runtime_parameter_and_callee_mutations_reject(self):
        target=self.originals[TARGET]
        for address in (0x3b48b0+20,0x3b48b0+12,0x3b48b0+204,0x118c08+40,0x506ca0+152,0x3b89d0+40):
            saved=self.mutate(address,bytes([target.read(address,1)[0]^1]))
            try:
                with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
            finally:target.data=saved

    def test_duplicate_complete_groups_reject(self):
        target=self.originals[TARGET]
        for start,end in GROUPS:
            saved=self.mutate(0x400000,target.read(start,end-start))
            try:
                with self.assertRaisesRegex(ValueError,'not uniquely located'):generate_unit(self.originals,self.registry)
            finally:target.data=saved

    def test_original_owner_name_extent_and_complete_membership_required(self):
        original=self.originals[REFERENCES[0]];a=self.proof['functions'][0]['provenance'][0]['source_address']
        f=next(f for f in original.functions if f['low']==a)
        for key,value in (('high',f['high']+4),('source','SB/Game/zNPCHazard.cpp'),('name','NPAR_EmitWrong')):
            saved=f[key];f[key]=value
            try:
                with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
            finally:f[key]=saved
        fake={**f,'low':a+204,'high':a+208};original.functions.append(fake)
        try:
            with self.assertRaisesRegex(ValueError,'membership/order'):generate_unit(self.originals,self.registry)
        finally:original.functions.remove(fake)

    def test_array_bounds_and_element_type_required(self):
        original=self.originals[REFERENCES[0]];section=next(s for s in original.metadata['sections'] if s['name']=='.debug')
        for array in self.proof['data_proofs'][0]['complete_typed_arrays']:
            die=section['offset']+array['type_die'];size=int.from_bytes(original.data[die:die+4],'little')
            descriptor=bytes.fromhex(array['subscript_descriptor']);position=original.data.index(descriptor,die,die+size)
            for delta in (7,14):
                saved=original.data;changed=bytearray(saved);changed[position+delta]^=1;original.data=bytes(changed)
                try:
                    with self.assertRaises(ValueError):original_arrays(original,self.originals[TARGET])
                finally:original.data=saved

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
