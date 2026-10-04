"""wcap.py: one compile, two captures: colouring graph (rcap) + '@' temp creation causes (tcap).

usage: python wcap.py <src.c|cpp> <unit> --fn NAME [--json out.json] [--cls 4] [--all]

NAME is bare or qualified (NPCBlinker::Render); every function it matches is captured, each
capture carries its own 'temps', 'qual' and 'fnidx' (see rcap.py for RCAP_MW/RCAP_EXTRA_FLAGS).

Prints every object-backed GPR web of NAME (plus, with --all, objectless ones) with:
vreg, object name, creation cause (PARAM/LOCAL for source names; INLINE/IRO-CSE/IRO-LOOP/...
for '@' temps), final colour and simplify pop position.  The json gets 'temps': {name: cause}.
"""
import sys, os, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rcap, tcap
from pe import PE
import re, struct


def iro_markers():
    """IrOptimizer.c trace-string pushes: {address: label}. Hitting one means the passes before it ran."""
    pe = PE(os.path.join(rcap.ROOT, 'build/compilers/GC/2.0p1a/mwcceppc.exe'))
    tb, t = pe.text()
    o = pe.va2off(0x5a5830)
    out = {}
    for m in re.finditer(rb'[\x20-\x7e]{4,}\x00', pe.b[o:o + 0x420]):
        va = pe.off2va(o + m.start())
        lab = m.group()[:-1].decode()
        if lab.endswith('.c'):
            continue
        pat = struct.pack('<I', va)
        i = t.find(pat)
        while i >= 0:
            if 0x42dd00 <= tb + i - 1 < 0x42e400:
                out[tb + i - 1] = lab
            i = t.find(pat, i + 1)
    return out


MARK = iro_markers()


def run(src, unit, fn, cls=4):
    import tempfile
    td = tempfile.mkdtemp(prefix='wcap_')
    cmd = rcap.compile_cmd(os.path.abspath(src), unit, os.path.join(td, 'o.o'))
    d = rcap.Dbg(cmd, rcap.ROOT)
    d.extra = [tcap.BP_HASH2] + sorted(MARK)
    cap = rcap.Capture(fn)
    pending = []
    temps = {}
    state = {'fn': None, 'phase': 'FE'}

    def on_bp(d, addr, regs):
        if addr in MARK:
            lab = MARK[addr]
            # 'Before X' markers precede pass X; bare pass names are post-pass dumps, so a temp
            # created after bare marker M belongs to the NEXT pass (named by its own marker).
            state['phase'] = 'BACKEND' if lab == 'After IRO_Optimizer' else ('in ' + lab[7:] if lab.startswith('Before ') else 'after ' + lab)
            return
        if addr == tcap.BP_HASH2:
            sp = regs['esp']
            try:
                s = d.rd(d.u32(sp + 4), 32).split(b'\0')[0]
            except OSError:
                return
            if s.startswith(b'@') and s[1:2].isdigit():
                ch = tcap.stack_rets(d, sp, 9)
                cz = tcap.cause(ch)
                ph = state['phase']
                if ph == 'FE':
                    mods = [tcap.modmap.guess(a) for a in ch[1:7]]
                    if any(m.startswith('IroLinearForm') for m in mods):
                        tag = 'IRO(linearize)'   # IRO converts the ENode tree before its first marker
                    elif any(m.startswith('CInline') for m in mods):
                        tag = 'INLINE'
                    else:
                        tag = 'FE-' + cz
                elif ph == 'BACKEND':
                    tag = 'BACKEND'
                else:
                    tag = 'IRO(%s)%s' % (ph.replace('IRO_', '').replace(' ', '_')[:24], '' if cz == '?' else ':' + cz)
                pending.append((s.decode(), tag, ch))
            return
        if addr == rcap.BP_COLORINSTR:
            obj = d.u32(regs['esp'] + 4)
            if obj != state['fn']:      # new function (colorinstructions runs once per class)
                state['fn'] = obj
                state['phase'] = 'FE'
                cap(d, addr, regs)
                if cap.matches(cap.fn, cap.qual):
                    temps[cap.nfn] = {n: (c, ' '.join('%06x' % x for x in ch)) for n, c, ch in pending}
                pending.clear()
                return
        cap(d, addr, regs)
    rc = d.run(on_bp)
    if rc != 0:
        rcap.report_failure(cmd, rc)
    for c in cap.out:
        c['temps'] = temps.get(c['fnidx'], {})
    # backward compatible second value: the temps of the first captured function
    return cap.out, (cap.out[0]['temps'] if cap.out else {})


def main():
    src, unit = sys.argv[1], sys.argv[2]
    fn = sys.argv[sys.argv.index('--fn') + 1]
    cls = int(sys.argv[sys.argv.index('--cls') + 1]) if '--cls' in sys.argv else 4
    out, temps = run(src, unit, fn, cls)
    if '--json' in sys.argv:
        json.dump(out, open(sys.argv[sys.argv.index('--json') + 1], 'w'))
    for c in out:
        if c['cls'] != cls:
            continue
        temps = c['temps']
        N = {int(k): v for k, v in c['nodes'].items()}
        pos = {v: i for i, v in enumerate(c['order'])}
        print('== %s [fnidx %d] cls %d (%d webs)' % (c.get('qual') or c['fn'], c['fnidx'], c['cls'], c['used'] - c['nreal']))
        if '--temps' in sys.argv:
            have = {n['name'] for n in N.values() if n['name']}
            for t in sorted(temps, key=lambda x: int(x[1:])):
                print('   %-6s %-34s %s %s' % (t, temps[t][0], 'web' if t in have else 'noweb', temps[t][1]))
        for v in sorted(N):
            if v < c['nreal']:
                continue
            n = N[v]
            nm = n['name']
            if not nm and '--all' not in sys.argv:
                continue
            kind = temps.get(nm, ('SOURCE', ''))[0] if nm and nm.startswith('@') else ('SOURCE' if nm else 'objless')
            print('v%-4d %-10s %-26s col=%-4s pop=%-4s deg=%-3d fl=%x' % (
                v, nm or '-', kind, n['final'], pos.get(v, '--'), len(n['nb']), n['flags']))


if __name__ == '__main__':
    main()
