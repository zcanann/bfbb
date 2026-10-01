"""tcap.py: log creation of '@NNN' compiler objects (frontend inline copies, IRO temps) in GC/2.0p1a.

usage: python tcap.py <src.c> <unit-substring> [--fn NAME] [--depth N]

Breaks at GetHashNameNode (0x4155f0, located via the "__ct"/"this" string xrefs) and, for
every '@'-prefixed name, prints the name and the return-address chain (ebp walk), so each
temp can be attributed to its creating routine. Function boundaries come from the
colorinstructions breakpoint (0x508680), which fires once per function AFTER all of its
temps exist, so temps printed before 'END fn' belong to fn.
"""
import sys, os, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rcap

BP_HASH = 0x4155f0
BP_HASH2 = 0x441570  # name hash used by the unique-'@'-name generator (call site 0x4e976a)


from pe import PE
_PE = PE(os.path.join(rcap.ROOT, 'build/compilers/GC/2.0p1a/mwcceppc.exe'))
_TB, _T = _PE.text()


def is_ret(a):
    o = a - _TB
    if not (5 <= o < len(_T)):
        return False
    return _T[o - 5] == 0xE8 or (_T[o - 2] == 0xFF and (_T[o - 1] & 0x38) == 0x10) or         (_T[o - 3] == 0xFF and (_T[o - 2] & 0x38) == 0x10) or (_T[o - 6] == 0xFF and (_T[o - 5] & 0x38) == 0x10)


def stack_rets(d, sp, depth):
    raw = d.rd(sp, 2048)
    out = []
    for k in range(0, len(raw), 4):
        v = struct.unpack_from('<I', raw, k)[0]
        if is_ret(v):
            out.append(v)
            if len(out) >= depth:
                break
    return out


import modmap


GENERIC = ('CParser', 'CFunc', 'IroUtil', 'BitVector', 'StackFrame', 'CPrep')
TAGS = (('IroCSE', 'IRO-CSE'), ('IroLoop', 'IRO-LOOP'), ('IroLinearForm', 'IRO-LF'), ('IroPropagate', 'IRO-PROP'),
        ('IroVars', 'IRO-VARS'), ('IroTransform', 'IRO-XFORM'), ('IroUnrollLoop', 'IRO-UNROLL'),
        ('IroEmptyLoop', 'IRO-EMPTY'), ('IROUseDef', 'IRO-SPLIT'), ('IrOptimizer', 'IRO'), ('IroFlowgraph', 'IRO-FG'), ('CInline', 'INLINE'),
        ('CodeGen', 'CODEGEN'), ('CExpr', 'FE-EXPR'), ('CDecl', 'FE-DECL'), ('CMachine', 'FE-MACH'))


def cause(chain):
    """Attribute an '@' object to the first non-generic module on its (heuristic) return chain."""
    for a in chain[1:]:
        m = modmap.guess(a).split('-')[0]
        if m.startswith(GENERIC):
            continue
        for key, tag in TAGS:
            if m.startswith(key):
                return tag
        return m.split('.')[0]
    return '?'


def main():
    src = os.path.abspath(sys.argv[1]); unit = sys.argv[2]
    want = sys.argv[sys.argv.index('--fn') + 1] if '--fn' in sys.argv else None
    depth = int(sys.argv[sys.argv.index('--depth') + 1]) if '--depth' in sys.argv else 6
    import tempfile
    td = tempfile.mkdtemp(prefix='tcap_')
    cmd = rcap.compile_cmd(src, unit, os.path.join(td, 'o.o'))
    d = rcap.Dbg(cmd, rcap.ROOT)
    hs = [int(x, 16) for x in sys.argv[sys.argv.index('--hash') + 1].split(',')] if '--hash' in sys.argv else [BP_HASH]
    rcap.BP_COLORGRAPH = hs[0]  # reuse rcap's arm list: colorinstr + up to two name hooks
    rcap.BP_COLORRET = hs[1] if len(hs) > 1 else hs[0]
    buf = []
    seen = set()

    def on_bp(d, addr, regs):
        if addr == rcap.BP_COLORINSTR:
            fn = rcap.objname(d, d.u32(regs['esp'] + 4))
            if fn not in seen:
                seen.add(fn)
                if want is None or fn == want:
                    for l in buf:
                        print(l)
                    print('END', fn)
            buf.clear()
            return
        if addr not in hs:
            return
        sp = regs['esp']
        try:
            s = d.rd(d.u32(sp + 4), 32).split(b'\0')[0]
        except OSError:
            return
        if not s.startswith(b'@') or not s[1:2].isdigit():
            return
        chain = stack_rets(d, sp, depth)
        buf.append('%-8s %-9s %s' % (s.decode(), cause(chain), ' '.join('%06x' % x for x in chain)))
    d.run(on_bp)


if __name__ == '__main__':
    main()
