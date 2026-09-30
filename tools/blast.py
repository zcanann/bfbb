#!/usr/bin/env python3
"""Who actually compiles this header?

Before touching a shared header you need to know which translation units see
it, because that is what a change can break -- and it is much wider than a
filename grep suggests, since headers reach TUs transitively. What matters
most is whether any of them is `Matching`: a Matching unit must stay
byte-identical or the DOL sha1 breaks.

  blast.py <header-fragment>          TUs that transitively include it
  blast.py <header-fragment> -q       counts only
  blast.py <header-fragment> --why <unit-frag>
                                      shortest include chain to that TU

Output separates units by their configure.py status. Read the Matching list
first; those are the ones that must be re-verified with a DOL build rather
than with objdiff (objdiff pairs symbols by name and is blind to definition
order -- see references/measuring.md).

Include resolution is by basename, which is what CodeWarrior's -I soup makes
practical here. A header whose basename is ambiguous is reported as such
rather than guessed at.
"""
import os
import re
import sys
from collections import deque

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")
INC = os.path.join(ROOT, "include")
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.M)
OBJ_RE = re.compile(r'Object\(\s*(\w+)\s*,\s*"([^"]+)"')


def walk(*roots):
    for root in roots:
        for dirpath, _, names in os.walk(root):
            for n in names:
                if n.endswith(('.c', '.cp', '.cpp', '.h', '.hpp')):
                    yield os.path.join(dirpath, n)


def build_graph():
    """basename -> set of files that #include it, plus path bookkeeping."""
    by_base = {}
    includes = {}
    for p in walk(SRC, INC):
        base = os.path.basename(p).lower()
        by_base.setdefault(base, []).append(p)
        try:
            text = open(p, 'r', encoding='utf-8', errors='replace').read()
        except OSError:
            continue
        # Normalise both slash flavours; the tree mixes them.
        includes[p] = {os.path.basename(m.replace('\\', '/')).lower()
                       for m in INCLUDE_RE.findall(text)}
    included_by = {}
    for p, incs in includes.items():
        for base in incs:
            included_by.setdefault(base, set()).add(p)
    return by_base, includes, included_by


def unit_status():
    """src-relative path -> Matching / NonMatching / Equivalent."""
    cfg = open(os.path.join(ROOT, "configure.py"), encoding='utf-8',
               errors='replace').read()
    return {m.group(2).replace('\\', '/'): m.group(1)
            for m in OBJ_RE.finditer(cfg)}


def resolve(by_base, frag):
    frag = frag.lower()
    exact = [b for b in by_base if b == frag]
    if exact:
        return exact[0]
    hits = sorted(b for b in by_base if frag in b)
    if not hits:
        raise SystemExit("no header matching %r" % frag)
    if len(hits) > 1:
        # Prefer an unambiguous basename match before giving up.
        stem = [h for h in hits if h.rsplit('.', 1)[0] == frag.rsplit('.', 1)[0]]
        if len(stem) == 1:
            return stem[0]
        raise SystemExit("ambiguous: " + ", ".join(hits[:12]))
    return hits[0]


def reachers(included_by, target):
    """Every file that transitively includes `target` (by basename)."""
    seen, out = set(), set()
    q = deque([target])
    while q:
        base = q.popleft()
        for p in included_by.get(base, ()):
            if p in out:
                continue
            out.add(p)
            b = os.path.basename(p).lower()
            if b not in seen:
                seen.add(b)
                q.append(b)
    return out


def chain(includes, by_base, start_file, target_base):
    """Shortest include chain from a TU down to the target header."""
    prev = {start_file: None}
    q = deque([start_file])
    while q:
        cur = q.popleft()
        for base in includes.get(cur, ()):
            if base == target_base:
                path, node = [target_base], cur
                while node is not None:
                    path.append(os.path.relpath(node, ROOT).replace('\\', '/'))
                    node = prev[node]
                return list(reversed(path))
            for nxt in by_base.get(base, ()):
                if nxt not in prev:
                    prev[nxt] = cur
                    q.append(nxt)
    return None


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('-')]
    if not args:
        raise SystemExit(__doc__)
    quiet = '-q' in sys.argv or '--quiet' in sys.argv
    why = None
    if '--why' in sys.argv:
        i = sys.argv.index('--why')
        why = sys.argv[i + 1] if i + 1 < len(sys.argv) else None
        args = [a for a in args if a != why]

    by_base, includes, included_by = build_graph()
    target = resolve(by_base, args[0])
    status = unit_status()

    hit = reachers(included_by, target)
    tus = sorted(p for p in hit if p.endswith(('.c', '.cp', '.cpp')))

    buckets = {}
    for p in tus:
        rel = os.path.relpath(p, SRC).replace('\\', '/')
        buckets.setdefault(status.get(rel, 'not-built'), []).append(rel)

    print("%s: %d TUs compile it (%d headers in between)"
          % (target, len(tus), len(hit) - len(tus)))
    for k in ('Matching', 'Equivalent', 'NonMatching', 'not-built'):
        if k in buckets:
            print("  %-12s %4d" % (k, len(buckets[k])))
    if 'Matching' in buckets:
        print("\n  !! %d Matching unit(s) -- these must stay byte-identical;"
              " verify with the DOL sha1, not objdiff." % len(buckets['Matching']))

    if why:
        starts = [p for p in tus if why.lower() in p.lower()]
        if not starts:
            raise SystemExit("no TU matching %r among the includers" % why)
        for s in starts[:3]:
            c = chain(includes, by_base, s, target)
            print("\n  " + " -> ".join(c) if c else "\n  (no chain found)")
        return

    if quiet:
        return
    for k in ('Matching', 'Equivalent', 'NonMatching', 'not-built'):
        if k not in buckets:
            continue
        print("\n%s:" % k)
        for rel in buckets[k]:
            print("  " + rel)


main()
