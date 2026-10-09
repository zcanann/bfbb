"""Original-backed sound control identity and rejection tests."""
import json,os,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from platforms.verify_reviewed import Original,REFERENCES,TARGET
from platforms.france_sound_controls import generate_unit,sound_object,INDEPENDENT

@unittest.skipUnless(os.environ.get('BFBB_FRANCE_TEST_ORIG'),'Set BFBB_FRANCE_TEST_ORIG for original fixtures')
class SoundControlsOriginalTests(unittest.TestCase):
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
        self.assertEqual(self.proof['counts']['code_bytes'],1392)
        self.assertEqual(len(self.proof['functions']),8)
        for f in self.proof['functions']:
            self.assertEqual({p['version'] for p in f['provenance']},set(REFERENCES))
            self.assertTrue(f['corroboration']['local_control_flow']['passes'])
        for p in self.proof['data_proofs']:self.assertEqual(len(p['operands']),12)
        for p in self.proof['call_proofs']:self.assertEqual(len(p['complete_callees']),4)

    def test_body_call_address_and_padding_mutations_reject(self):
        target=self.originals[TARGET]
        for address in (0x1b88f0+20,0x1b8980+432,0x1b8b70+72,0x1b8c30+140):
            saved=self.mutate(address,bytes([target.read(address,1)[0]^1]))
            try:
                with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
            finally:target.data=saved

    def test_duplicate_complete_template_rejects(self):
        target=self.originals[TARGET];saved=self.mutate(0x400000,target.read(0x1b88f0,144))
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

    def test_complete_callee_bytes_required(self):
        target=self.originals[TARGET];address=0x34bd30+40
        saved=self.mutate(address,bytes([target.read(address,1)[0]^1]))
        try:
            with self.assertRaises(ValueError):generate_unit(self.originals,self.registry)
        finally:target.data=saved

    def test_original_global_declaration_and_voice_flag_type_required(self):
        from platforms.france_sound_playback import playback_data
        original=self.originals[REFERENCES[0]];target=self.originals[TARGET]
        known={f['address']:f for path in self.registry.glob('*functions.json') for f in json.loads(path.read_text())['functions'] if f['address'] in INDEPENDENT}
        snd=playback_data(original)[0]
        section=next(s for s in original.metadata['sections'] if s['name']=='.debug')
        offset=section['offset']+snd['declaration_die']
        size=int.from_bytes(original.data[offset:offset+4],'little')
        loc=original.data.index(b'\x03'+snd['reference_address'].to_bytes(4,'little'),offset,offset+size)+1
        member=next(m for m in snd['layouts']['xSndVoiceInfo']['members'] if m['name']=='flags')
        offset=section['offset']+member['die_offset'];size=int.from_bytes(original.data[offset:offset+4],'little')
        component=original.data.index(bytes.fromhex('041400000007'),offset,offset+size)+1
        for position in (loc,component):
            saved=original.data;changed=bytearray(saved);changed[position]^=1;original.data=bytes(changed)
            try:
                with self.assertRaises(ValueError):sound_object(original,target,known)
            finally:original.data=saved

if __name__=='__main__':unittest.main()
