"""Shared helpers for the regalloc tools: repo root, build.ninja unit rules, compiler selection,
filename sanitising and C++ (CodeWarrior-mangled) function selection.

Function identity.  The compiler's Object carries only the bare name ('Render', '__ct'); rcap also
walks its namespace chain (Object+6 -> NameSpace{+0 parent, +4 HashNameNode *name}) and records
the qualified name ('NPCBlinker::Render').  Captures can therefore be selected by bare name,
qualified name or mangled symbol ('Render__10NPCBlinkerFPC5xVec3fPC8RwRaster'); same-class
overloads (identical qualified names) are told apart by --idx N / TMAP_IDX, or automatically from
the address order of the same-qualified functions in our compiled object (= codegen order).
"""
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))


def find_root():
    """Repo root: $BFBB_ROOT, else the first ancestor of this file, then of cwd, with build.ninja."""
    env = os.environ.get('BFBB_ROOT')
    if env:
        return os.path.abspath(env).replace('\\', '/')
    for start in (HERE, os.getcwd()):
        d = os.path.abspath(start)
        while True:
            if os.path.isfile(os.path.join(d, 'build.ninja')) and os.path.isdir(os.path.join(d, 'tools')):
                return d.replace('\\', '/')
            up = os.path.dirname(d)
            if up == d:
                break
            d = up
    # tools/regalloc/ -> repo (works even before configure.py has written build.ninja)
    return os.path.dirname(os.path.dirname(HERE)).replace('\\', '/')


ROOT = find_root()


def safe_name(s):
    """A filename-safe version of a (possibly qualified/mangled) function name."""
    return re.sub(r'[^A-Za-z0-9_.-]', '_', s)


# ---------------------------------------------------------------- build.ninja

_BUILD_RE = re.compile(r"^build (\S+\.o):(?: \$\n\s+| )(mwcc\w*) ((?:.*\n)*?)  basedir", re.M)
_SRC_RE = re.compile(r"(?:^|\s)(src[\\/]\S+\.(?:c|cp|cpp))(?:\s|$)")


def unit_rules():
    N = open(os.path.join(ROOT, 'build.ninja')).read()
    out = []
    for m in _BUILD_RE.finditer(N):
        body = m.group(3)
        mw = re.search(r"mw_version = (\S+)", body)
        cf = re.search(r"cflags = ((?:.*\$\n)*.*)\n", body)
        src = _SRC_RE.search(body)
        if not (mw and cf):
            continue
        out.append(dict(obj=m.group(1).replace('\\', '/'), rule=m.group(2),
                        mw=mw.group(1).replace('\\', '/'),
                        flags=re.sub(r"\s+", " ", cf.group(1).replace("$\n", " ")).strip(),
                        src=src.group(1).replace('\\', '/') if src else None))
    return out


def find_rule(unit):
    """build.ninja rule for a unit given as a substring ('bamatlst'), a path ('SB/Core/x/xMath3',
    'src/SB/Core/x/xMath3.cpp') or an object path.  Exact stem/path-suffix matches win over
    plain substring matches (so 'xMath3' does not pick xMath3d)."""
    u = unit.replace('\\', '/')
    u = re.sub(r'\.(c|cp|cpp|o)$', '', u)
    if u.startswith('src/'):
        u = u[4:]
    rules = unit_rules()
    hits = [r for r in rules if u in r['obj']]
    if not hits:
        raise SystemExit('unit %r not found in build.ninja' % unit)
    exact = [r for r in hits if r['obj'][:-2].endswith('/' + u)]
    pick = exact or hits
    if len(pick) > 1 and not exact:
        print('note: unit %r is ambiguous (%s); using %s' % (
            unit, ', '.join(r['obj'] for r in pick[:5]), pick[0]['obj']))
    return pick[0]


def compiler_version(rule):
    """$RCAP_MW (e.g. GC/2.0p1f) overrides; default is the unit's own mw_version from build.ninja."""
    return (os.environ.get('RCAP_MW') or rule['mw']).replace('\\', '/')


def extra_flags(rule, src):
    """$RCAP_EXTRA_FLAGS, plus '-i <unit source dir>' when compiling a COPY of the unit's source
    from elsewhere (its #include "local.h" would otherwise not resolve)."""
    ex = os.environ.get('RCAP_EXTRA_FLAGS', '').strip()
    if rule.get('src') and src:
        orig = os.path.normcase(os.path.abspath(os.path.join(ROOT, rule['src'])))
        if os.path.normcase(os.path.abspath(src)) != orig:
            ex = (ex + ' -i "%s"' % os.path.dirname(orig).replace('\\', '/')).strip()
    return ex


# ---------------------------------------------------------------- C++ names

def parse_mangled(sym):
    """CodeWarrior-mangled 'name__<qual>F<params>' -> (bare, [scope, ...]); plain C names give
    (sym, []).  Returns None if sym does not look mangled."""
    i = sym.find('__', 1)
    while i > 0:
        bare, rest = sym[:i], sym[i + 2:]
        q = _parse_qual(rest)
        if q is not None and bare.strip('_'):
            return bare, q
        i = sym.find('__', i + 1)
    return None


def _parse_qual(rest):
    def ident(s):
        m = re.match(r'(\d+)', s)
        if not m:
            return None, s
        n = int(m.group(1))
        s = s[m.end():]
        return (s[:n], s[n:]) if len(s) >= n else (None, s)
    scopes = []
    if rest.startswith('Q') and rest[1:2].isdigit():
        k = int(rest[1])
        rest = rest[2:]
        for _ in range(k):
            nm, rest = ident(rest)
            if nm is None:
                return None
            scopes.append(nm)
    elif rest[:1].isdigit():
        nm, rest = ident(rest)
        if nm is None:
            return None
        scopes.append(nm)
    if rest == '' or rest.startswith('F') or rest.startswith('CF'):
        return scopes
    return None


def qual_of(sym):
    """Qualified name 'A::B::name' of a mangled symbol (or the plain name)."""
    p = parse_mangled(sym)
    if not p:
        return sym
    return '::'.join(p[1] + [p[0]])


def looks_mangled(s):
    p = parse_mangled(s)
    return bool(p) and '::' not in s


def strip_tmpl(s):
    return re.sub(r'<.*>', '', s)


def sym_matches(sname, demangled, want):
    """Does objdiff symbol (name, demangled_name) match `want` (mangled, qualified or bare)?"""
    if sname == want:
        return True
    if sname.startswith(want + '__') and (parse_mangled(sname) or ('',))[0] == want:
        return True   # bare name, or a mangled prefix like 'Render__10NPCBlinker'
    if parse_mangled(sname) and want.startswith(sname.split('__', 1)[0]) and sname.startswith(want):
        return True
    return qual_of(sname) == want or bool(demangled and demangled.startswith(want + '('))


def find_symbol(symbols, want, quiet=False):
    """Pick one function symbol from an objdiff symbol list.  Exact (mangled) name first, then
    qualified/bare-name matches; ambiguity raises SystemExit listing the candidates."""
    funcs = [s for s in symbols if s.get('name')]
    exact = [s for s in funcs if s['name'] == want]
    if exact:
        return exact[0]
    hits = [s for s in funcs if sym_matches(s['name'], s.get('demangled_name'), want)]
    hits = [s for s in hits if s.get('kind', 'SYMBOL_FUNCTION') == 'SYMBOL_FUNCTION'] or hits
    if len(hits) == 1:
        return hits[0]
    if not hits:
        raise SystemExit('symbol %r not found' % want)
    raise SystemExit('symbol %r is ambiguous; pass the mangled name (--sym / TMAP_SYM):\n  %s' % (
        want, '\n  '.join('%s   %s' % (s['name'], s.get('demangled_name', '')) for s in hits)))


# ---------------------------------------------------------------- capture selection

def functions(caps):
    """Captures grouped per compiled function, in codegen order: [(fnidx, [caps...]), ...]."""
    groups = {}
    for c in caps:
        groups.setdefault(c.get('fnidx', 0), []).append(c)
    return sorted(groups.items())


def cap_matches(c, name):
    return name in (c.get('fn'), c.get('qual'))


def select(caps, fn=None, sym=None, idx=None, our_syms=None, log=print):
    """Choose ONE function's captures.  fn: bare/qualified name (may be None if sym given);
    sym: mangled symbol; idx: index among the remaining same-named functions; our_syms: objdiff
    symbols of OUR object (for automatic overload resolution by address order)."""
    fns = functions(caps)
    if sym:
        q = qual_of(sym)
        bare = (parse_mangled(sym) or (sym, []))[0]
        cand = [g for g in fns if g[1][0].get('qual') == q]
        if not cand:   # e.g. template scopes spelled differently; fall back to the bare name
            cand = [g for g in fns if g[1][0].get('fn') == bare]
    else:
        cand = [g for g in fns if cap_matches(g[1][0], fn)] if fn else fns
    if not cand:
        raise SystemExit('no capture for %s (captured: %s)' % (
            sym or fn, ', '.join(sorted({g[1][0].get('qual') or g[1][0]['fn'] for g in fns})) or 'nothing'))
    if idx is not None:
        if not 0 <= idx < len(cand):
            raise SystemExit('--idx %d out of range (%d candidates)' % (idx, len(cand)))
        return cand[idx][1]
    if len(cand) == 1:
        return cand[0][1]
    q = cand[0][1][0].get('qual') or cand[0][1][0]['fn']
    if sym and our_syms:
        same = [s for s in our_syms if s.get('address') is not None and s.get('name')
                and qual_of(s['name']) == q]
        same.sort(key=lambda s: int(s['address']))
        names = [s['name'] for s in same]
        if sym in names and len(names) == len(cand):
            k = names.index(sym)
            log('note: %d captured functions named %s; %s is #%d by address in our object '
                '(override with --idx)' % (len(cand), q, sym, k))
            return cand[k][1]
    raise SystemExit('%d captured functions match %s; pass --sym <mangled> or --idx 0..%d' % (
        len(cand), sym or fn, len(cand) - 1))


def resolve(caps, fn=None, sym=None, idx=None, data=None, log=print):
    """(captures of the chosen function, its objdiff symbol name).  data: vfdiff.diff_json()."""
    ours = (data or {}).get('right', {}).get('symbols', [])
    chosen = select(caps, fn, sym, idx, ours, log)
    if sym or data is None:
        return chosen, sym or fn
    c0 = chosen[0]
    q = c0.get('qual') or c0['fn']
    same = sorted((s for s in ours if s.get('address') is not None and s.get('name')
                   and s.get('kind') == 'SYMBOL_FUNCTION' and qual_of(s['name']) == q),
                  key=lambda s: int(s['address']))
    if len(same) == 1:
        return chosen, same[0]['name']
    peers = [g for g in functions(caps) if (g[1][0].get('qual') or g[1][0]['fn']) == q]
    k = [g[0] for g in peers].index(c0.get('fnidx', 0))
    if len(same) == len(peers):
        log('note: %s is overloaded; using %s (#%d by address)' % (q, same[k]['name'], k))
        return chosen, same[k]['name']
    return chosen, q   # let find_symbol report what it can
