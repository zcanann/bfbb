"""Original-backed pow identity, callable bounds and cubic-call rejection checks."""
from contextlib import contextmanager
from copy import deepcopy
import hashlib,json,os
from pathlib import Path
import struct,sys,unittest

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from platforms.verify_xbox_reviewed import Original
from platforms.xbox_pow import verify_pow_original,verify_pow_vendor
from platforms.xbox_calls import normalize_calls,reconstruct_calls

@unittest.skipUnless(os.environ.get('BFBB_XBOX_TEST_ORIG'),'Set BFBB_XBOX_TEST_ORIG for authenticated fixtures')
class PowOriginalTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        records=json.loads((ROOT/'config/platforms/versions.json').read_text())['versions']
        cls.originals={v:Original(v,records[v],Path(os.environ['BFBB_XBOX_TEST_ORIG'])) for v in ('XBOX-US','XBOX-EU')}
        cls.documents={v:json.loads((ROOT/'config/platforms'/v/'reviewed-functions.json').read_text()) for v in cls.originals}

    def function(self,version):
        return deepcopy(next(f for f in self.documents[version]['functions'] if f['canonical_identifier']=='__CIpow'))

    @contextmanager
    def changed(self,original,address,payload):
        before=original.data
        section=original.section(address,len(payload))
        offset=section['raw_offset']+address-section['virtual_address']
        original.data=before[:offset]+payload+before[offset+len(payload):]
        try: yield
        finally: original.data=before

    def refresh_hash(self,original,function):
        context=function['corroboration']['runtime_pow']['vendor_container']
        context['sha256']=hashlib.sha256(original.read(context['address'],context['size'])).hexdigest()

    def test_complete_context_and_cubic_inverse(self):
        for version,original in self.originals.items():
            verify_pow_original(original,self.function(version))
            functions=self.documents[version]['functions']
            cubic=next(f for f in functions if f['canonical_identifier'].startswith('xMathSolveCubic('))
            targets={f['canonical_identifier']:f['address'] for f in functions}
            raw=original.read(cubic['address'],cubic['size'])
            normalized,relocations=normalize_calls(raw,cubic['address'],cubic['direct_calls'],targets)
            self.assertEqual(len(relocations),8)
            self.assertEqual(reconstruct_calls(normalized,cubic['address'],relocations,targets),raw)

    def test_changed_nonrelocated_body(self):
        for version,original in self.originals.items():
            function=self.function(version); address=function['address']+40
            with self.changed(original,address,bytes([original.read(address,1)[0]^1])):
                self.refresh_hash(original,function)
                with self.assertRaises(ValueError): verify_pow_original(original,function)

    def test_changed_entry_jump(self):
        for version,original in self.originals.items():
            function=self.function(version)
            with self.changed(original,function['address']+1,b'\x01'):
                self.refresh_hash(original,function)
                with self.assertRaises(ValueError): verify_pow_original(original,function)

    def test_repeated_runtime_operand_changed(self):
        for version,original in self.originals.items():
            function=self.function(version); address=function['address']+2+30
            value=struct.unpack('<I',original.read(address,4))[0]
            with self.changed(original,address,struct.pack('<I',(value+16)&0xffffffff)):
                self.refresh_hash(original,function)
                with self.assertRaises(ValueError): verify_pow_original(original,function)

    def test_pow_name_changed(self):
        for version,original in self.originals.items():
            function=self.function(version)
            name=struct.unpack('<I',original.read(function['address']+2+140,4))[0]
            with self.changed(original,name,b'cow\0'):
                with self.assertRaises(ValueError): verify_pow_original(original,function)

    def test_bounds_and_vendor_identity_changed(self):
        for version,original in self.originals.items():
            function=self.function(version); function['size']=28
            with self.assertRaises(ValueError): verify_pow_original(original,function)
            function=self.function(version)
            function['corroboration']['runtime_pow']['vendor']['default_size']=526
            with self.assertRaises(ValueError): verify_pow_original(original,function)

    def test_cubic_call_to_unproved_destination(self):
        for version,original in self.originals.items():
            functions=self.documents[version]['functions']
            cubic=next(f for f in functions if f['canonical_identifier'].startswith('xMathSolveCubic('))
            targets={f['canonical_identifier']:f['address'] for f in functions}
            raw=bytearray(original.read(cubic['address'],cubic['size']))
            value=struct.unpack_from('<I',raw,316)[0]
            struct.pack_into('<I',raw,316,(value+1)&0xffffffff)
            with self.assertRaises(ValueError): normalize_calls(bytes(raw),cubic['address'],cubic['direct_calls'],targets)

    @unittest.skipUnless(os.environ.get('BFBB_XBOX_TEST_CRT'),'Set BFBB_XBOX_TEST_CRT for vendor archive replay')
    def test_pinned_vendor(self):
        self.assertEqual(verify_pow_vendor(Path(os.environ['BFBB_XBOX_TEST_CRT']))['symbol'],'__CIpow')

if __name__=='__main__': unittest.main()
