"""Protect parser isolation and lazy rejection across repeated original proofs."""
import struct,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from platforms.dwarf1 import iter_dies

def die(tag,name):
    payload=struct.pack('<HH',tag,0x38)+name.encode()+b'\0'
    return struct.pack('<I',len(payload)+4)+payload

class DwarfReuseTests(unittest.TestCase):
    def test_attributes_are_isolated_even_during_initial_iteration(self):
        debug=die(0x11,'isolation.cpp')+die(2,'value')
        iterator=iter_dies(debug)
        first=next(iterator)
        first[3][3]='altered'; first[3][99]=[1]
        list(iterator)
        again=list(iter_dies(debug))
        self.assertEqual(again[0][3],{3:'isolation.cpp'})
        self.assertEqual(again[1][2],'isolation.cpp')
        again[1][3].clear()
        self.assertEqual(list(iter_dies(debug))[1][3],{3:'value'})

    def test_distinct_buffers_keep_distinct_owners(self):
        a=die(0x11,'a.cpp')+die(2,'member')
        b=die(0x11,'b.cpp')+die(2,'member')
        for debug,owner in ((a,'a.cpp'),(b,'b.cpp'),(a,'a.cpp')):
            self.assertEqual(list(iter_dies(debug))[1][2],owner)

    def test_trailing_failure_stays_lazy_and_repeatable(self):
        debug=die(0x11,'valid.cpp')+b'\x03\0\0\0'
        for _ in range(2):
            iterator=iter_dies(debug)
            self.assertEqual(next(iterator)[3],{3:'valid.cpp'})
            with self.assertRaises(ValueError): next(iterator)

    def test_incomplete_iteration_cannot_hide_trailing_records(self):
        debug=die(0x11,'partial.cpp')+die(2,'tail')
        iterator=iter_dies(debug)
        next(iterator); iterator.close()
        self.assertEqual(len(list(iter_dies(debug))),2)

    def test_mutable_input_changes_are_observed(self):
        debug=bytearray(die(0x11,'before.cpp'))
        self.assertEqual(list(iter_dies(debug))[0][3][3],'before.cpp')
        at=debug.index(b'before.cpp'); debug[at:at+10]=b'after_.cpp'
        self.assertEqual(list(iter_dies(debug))[0][3][3],'after_.cpp')

if __name__=='__main__': unittest.main()
