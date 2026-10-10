"""Original-backed Sandy damage effect identity and rejection tests."""
import json,os,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from platforms.verify_reviewed import Original,REFERENCES,TARGET
from platforms.france_sandy_damage_effect import generate_unit,original_array

@unittest.skipUnless(os.environ.get('BFBB_FRANCE_TEST_ORIG'),'Set BFBB_FRANCE_TEST_ORIG for original fixtures')
class SandyDamageEffectOriginalTests(unittest.TestCase):
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
        offset=segment['offset']+address-segment['address']
        saved=target.data;changed=bytearray(saved);changed[offset:offset+len(body)]=body;target.data=bytes(changed)
        return saved

    def test_all_complete_bodies_and_json(self):
        self.assertEqual(json.loads(json.dumps(self.proof)),self.proof)
        self.assertEqual(self.proof['counts']['code_bytes'],432)
        self.assertEqual(len(self.proof['functions']),1)
        for f in self.proof['functions']:
            self.assertEqual({p['version'] for p in f['provenance']},set(REFERENCES))
            self.assertTrue(f['corroboration']['local_control_flow']['passes'])
        for p in self.proof['data_proofs']:self.assertEqual(len(p['operands']),8)

    def test_body_call_address_and_padding_mutations_reject(self):
        target=self.originals[TARGET]
        for address in (0x335000+100,0x335000+16,0x335000+76,0x335000+92):
            saved=self.mutate(address,bytes([target.read(address,1)[0]^1]))
            try:
                with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
            finally:target.data=saved

    def test_duplicate_complete_template_rejects(self):
        target=self.originals[TARGET];saved=self.mutate(0x400000,target.read(0x335000,432))
        try:
            with self.assertRaisesRegex(ValueError,'not uniquely located'):generate_unit(self.originals,self.registry)
        finally:target.data=saved

    def test_original_owner_linkage_and_extent_required(self):
        original=self.originals[REFERENCES[0]];a=self.proof['functions'][0]['provenance'][0]['source_address']
        f=next(f for f in original.functions if f['low']==a)
        for key,value in (('high',f['high']+4),('source','SB/Game/zNPCTypeRobot.cpp')):
            saved=f[key];f[key]=value
            try:
                with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
            finally:f[key]=saved

    def test_registry_independence(self):
        with tempfile.TemporaryDirectory() as directory:
            self.assertEqual(generate_unit(self.originals,Path(directory)),self.proof)

    def test_original_object_declaration_and_field_offset_required(self):
        original=self.originals[REFERENCES[0]];target=self.originals[TARGET]
        obj=original_array(original,target)
        section=next(s for s in original.metadata['sections'] if s['name']=='.debug')
        offset=section['offset']+obj['declaration_die']
        size=int.from_bytes(original.data[offset:offset+4],'little')
        loc=original.data.index(b'\x03'+obj['reference_address'].to_bytes(4,'little'),offset,offset+size)+1
        member=next(m for m in obj['record_layout']['members'] if m['name']=='BDEminst')
        offset=section['offset']+member['die_offset'];size=int.from_bytes(original.data[offset:offset+4],'little')
        component=original.data.index(bytes.fromhex('040401000007'),offset,offset+size)+1
        for position in (loc,component):
            saved=original.data;changed=bytearray(saved);changed[position]^=1;original.data=bytes(changed)
            try:
                with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
            finally:original.data=saved

    def test_original_array_counts_and_element_types_required(self):
        original=self.originals[REFERENCES[0]];target=self.originals[TARGET]
        obj=original_array(original,target)
        section=next(s for s in original.metadata['sections'] if s['name']=='.debug')
        for die,descriptor in ((obj['array_type_die'],obj['array_descriptor']),
                               (obj['saved_color_array_type_die'],obj['saved_color_descriptor'])):
            offset=section['offset']+die
            size=int.from_bytes(original.data[offset:offset+4],'little')
            start=original.data.index(bytes.fromhex(descriptor),offset,offset+size)
            for position in (start+7,start+len(bytes.fromhex(descriptor))-1):
                saved=original.data;changed=bytearray(saved);changed[position]^=1;original.data=bytes(changed)
                try:
                    with self.assertRaises(ValueError):original_array(original,target)
                finally:original.data=saved

if __name__=='__main__':unittest.main()
