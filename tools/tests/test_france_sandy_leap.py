"""Original-backed complete Sandy Leap tail and conservative effect tests."""
import json,os,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from platforms.verify_reviewed import Original,REFERENCES,TARGET
from platforms.france_sandy_leap import generate_unit,leap_gpr_writes,tail_bounds,INDEPENDENT,START,SIZE,CALLEE


class SandyLeapEffectsTests(unittest.TestCase):
    def test_scalar_forms_and_reserved_fields(self):
        for word in (0x46051981,0x46010044,0x46000834,0x46000836,0x4602101a,0x4601085c):
            self.assertEqual(leap_gpr_writes(word),set())
        for word in (0x46010044|0x800,0x46000834|0x40,0x46000836|0x40,0x4602101a|0x40,
                     0x46251981,0x4600003f):
            with self.assertRaises(ValueError):leap_gpr_writes(word)
        for op in (0x44000000,0x44400000):
            for reg in (2,29,31):self.assertEqual(leap_gpr_writes(op|(reg<<16)),{reg})


@unittest.skipUnless(os.environ.get('BFBB_FRANCE_TEST_ORIG'),'Set BFBB_FRANCE_TEST_ORIG for original fixtures')
class SandyLeapOriginalTests(unittest.TestCase):
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

    def test_complete_body_and_json(self):
        self.assertEqual(json.loads(json.dumps(self.proof)),self.proof)
        self.assertEqual(self.proof['counts']['code_bytes'],484)
        f=self.proof['functions'][0]
        self.assertEqual({p['version'] for p in f['provenance']},set(REFERENCES))
        self.assertTrue(f['corroboration']['local_control_flow']['passes'])
        for p in self.proof['data_proofs']:self.assertEqual(len(p['operands']),2)

    def test_body_tail_operand_padding_and_complete_callee_mutations_reject(self):
        target=self.originals[TARGET]
        for address in (START+20,START+476,START+60,START+484,CALLEE+40):
            saved=self.mutate(address,bytes([target.read(address,1)[0]^1]))
            try:
                with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
            finally:target.data=saved

    def test_duplicate_complete_template_rejects(self):
        target=self.originals[TARGET];saved=self.mutate(0x400000,target.read(START,SIZE))
        try:
            with self.assertRaisesRegex(ValueError,'not uniquely located'):generate_unit(self.originals,self.registry)
        finally:target.data=saved

    def test_original_owner_and_extent_required(self):
        original=self.originals[REFERENCES[0]];a=self.proof['functions'][0]['provenance'][0]['source_address']
        f=next(f for f in original.functions if f['low']==a)
        for key,value in (('high',f['high']+4),('source','SB/Game/zNPCTypeRobot.cpp')):
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

    def test_boundary_rejects_stack_ra_external_branch_and_wrong_tail(self):
        target=self.originals[TARGET]
        # Dead destination rewrites still fail: no SP/RA change is permitted,
        # including COP1 transfers the local floating arithmetic cases exclude.
        for off,word in ((4,0x241dfffd),(4,0x241ffffd),(4,0x441f0000),(4,0x445d0000),
                         (100,0x10007fff),(476,0x08000000)):
            saved=self.mutate(START+off,word.to_bytes(4,'little'))
            try:
                with self.assertRaises(ValueError):tail_bounds(target,START,CALLEE)
            finally:target.data=saved

if __name__=='__main__':unittest.main()
