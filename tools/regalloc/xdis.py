import os as _os
import sys as _sys
_sys.path.insert(0, _os.path.dirname(_os.path.abspath(__file__)))
import ra_common
_REPO = ra_common.ROOT
import sys
from pe import PE
from capstone import *
v=sys.argv[1]; a=int(sys.argv[2],16); n=int(sys.argv[3],16)
p=PE(_REPO+'/build/compilers/GC/%s/mwcceppc.exe'%v)
md=Cs(CS_ARCH_X86, CS_MODE_32)
for ins in md.disasm(p.read(a,n), a):
    print('%08x  %-8s %s'%(ins.address, ins.mnemonic, ins.op_str))
