"""Original-only anchored sound wrapper identity rejection tests."""
import json,os,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from platforms.verify_reviewed import Original,REFERENCES,TARGET
from platforms.france_sound_wrappers import generate_unit,ANCHOR,SPAN,SOURCE

@unittest.skipUnless(os.environ.get('BFBB_FRANCE_TEST_ORIG'),'Set BFBB_FRANCE_TEST_ORIG for original fixtures')
class SoundWrappersOriginalTests(unittest.TestCase):
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
        offset=segment['offset']+address-segment['address'];saved=target.data;changed=bytearray(saved)
        changed[offset:offset+len(body)]=body;target.data=bytes(changed);return saved

    def test_complete_cluster_with_two_explicit_tails(self):
        self.assertEqual(json.loads(json.dumps(self.proof)),self.proof)
        self.assertEqual(self.proof['counts']['code_bytes'],124)
        self.assertEqual(len(self.proof['functions']),3)
        self.assertEqual(sum(f['size'] for f in self.proof['functions']),124)
        for seq in self.proof['sequence_proofs']:
            self.assertEqual(len(seq['complete_members']),4)
            self.assertEqual(seq['span'],1520)
            self.assertEqual(seq['unique_full_cluster_template']['matching_addresses'],[ANCHOR])
        for f in self.proof['functions']:
            self.assertEqual({p['version'] for p in f['provenance']},set(REFERENCES))
            if f['address'] in (0x20a310,0x20a330):
                self.assertTrue(f['corroboration']['local_control_flow']['return_address_preserved'])
                self.assertTrue(all(len(p['direct_transfers'])==1 and p['direct_transfers'][0]['opcode']==2 for p in f['provenance']))

    def test_fixed_arguments_delay_slots_tail_targets_and_padding(self):
        target=self.originals[TARGET]
        for a in (0x20a2c0+32,0x20a310,0x20a310+16,0x20a310+20,0x20a330+24,0x20a2c0+72,0x20a310+24,0x20a330+28):
            saved=self.mutate(a,bytes([target.read(a,1)[0]^1]))
            try:
                with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
            finally:target.data=saved

    def test_complete_anchor_body_is_required(self):
        target=self.originals[TARGET];a=ANCHOR+100;saved=self.mutate(a,bytes([target.read(a,1)[0]^1]))
        try:
            with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
        finally:target.data=saved

    def test_duplicate_full_cluster_rejects(self):
        target=self.originals[TARGET];saved=self.mutate(0x400000,target.read(ANCHOR,SPAN))
        try:
            with self.assertRaisesRegex(ValueError,'not uniquely located'):generate_unit(self.originals,self.registry)
        finally:target.data=saved

    def test_original_order_extent_owner_and_membership_required(self):
        original=self.originals[REFERENCES[0]];a=self.proof['functions'][1]['provenance'][0]['source_address']
        f=next(f for f in original.functions if f['low']==a)
        for key,value in (('high',f['high']+4),('low',f['low']+4),('source','SB/Core/p2/iSnd.cpp')):
            saved=f[key];f[key]=value
            try:
                with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
            finally:f[key]=saved
        start=self.proof['sequence_proofs'][0]['source_address'];fake={'source':SOURCE,'name':'unexpected','low':start+4,'high':start+8};original.functions.append(fake)
        try:
            with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
        finally:original.functions.remove(fake)

    def test_fixed_anchor_missing_altered_and_dependency_fence(self):
        known=[f for path in self.registry.glob('*functions.json') for f in json.loads(path.read_text())['functions'] if f['address']==ANCHOR]
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'test-functions.json';path.write_text(json.dumps({'functions':[]}))
            with self.assertRaises(ValueError):generate_unit(self.originals,Path(directory))
            changed=json.loads(json.dumps(known));changed[0]['sha256']='0'*64;path.write_text(json.dumps({'functions':changed}))
            with self.assertRaises(ValueError):generate_unit(self.originals,Path(directory))
            path.write_text(json.dumps({'functions':known+[{'address':0x400000}]}))
            self.assertEqual(generate_unit(self.originals,Path(directory)),self.proof)

if __name__=='__main__':unittest.main()
