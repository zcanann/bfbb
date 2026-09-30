#!/usr/bin/env python3
"""Print the target disassembly of ONE function, instead of a whole object.

`dtk elf disasm` writes the entire object -- 1.2 MB for a unit like
zNPCGoalRobo -- and the usual next step is to grep around in it for the one
function you actually wanted. Every one of those greps and every chunk you
read stays in context for the rest of the session. This does the disasm once,
caches it, and hands back just the function.

Usage:
  tasm.py <unit-fragment> <symbol-fragment>   disassembly of matching functions
  tasm.py <unit-fragment> --list              every function, name and size
  tasm.py <unit-fragment> --pool              the .sdata2 literal pool, in slot
                                              order, with each slot's first user
  tasm.py <unit-fragment> --order             .text definition order, which is
                                              also literal interning order

Options:
  --raw        keep the `/* addr off bytes */` prefixes (default strips them)
  --no-cache   re-run dtk even if a cached disassembly is present

The cache lives beside the object under build/.tasm-cache/ and is invalidated
by the object's mtime, so it is always consistent with the extracted target.

This reads the TARGET object (build/GQPE78/obj/...), which is ground truth.
It never looks at our build output. To compare against ours, use solo.py.
"""
import json
import os
import re
import subprocess
import sys

import cwexec

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DTK = cwexec.dtk(ROOT)
CFG = json.load(open(os.path.join(ROOT, "objdiff.json")))
CACHE = os.path.join(ROOT, "build", ".tasm-cache")

INSN_RE = re.compile(r"^/\* [0-9A-Fa-f]{8} ")
PREFIX_RE = re.compile(r"^/\*.*?\*/\t")


def find_unit(frag):
    hits = [u for u in CFG["units"] if frag in u["name"]]
    if len(hits) != 1:
        exact = [u for u in hits if u["name"].endswith(frag)]
        if len(exact) == 1:
            return exact[0]
        raise SystemExit("ambiguous or no match: " +
                         ", ".join(u["name"] for u in hits[:12]))
    return hits[0]


def disasm(unit, use_cache=True):
    """Absolute path to a text disassembly of the unit's target object."""
    obj = os.path.join(ROOT, unit["target_path"])
    if not os.path.exists(obj):
        raise SystemExit("no target object for %s at %s" % (unit["name"], obj))
    if not os.path.exists(CACHE):
        os.makedirs(CACHE)
    out = os.path.join(CACHE, unit["name"].replace("/", "_") + ".s")
    if use_cache and os.path.exists(out) and \
            os.path.getmtime(out) >= os.path.getmtime(obj):
        return out
    r = subprocess.run([DTK, "elf", "disasm", obj, out],
                       capture_output=True, text=True)
    if not os.path.exists(out):
        raise SystemExit("dtk failed:\n" + (r.stdout + r.stderr)[-2000:])
    return out


def functions(path):
    """[(name, [lines]), ...] in .text definition order."""
    out = []
    cur = None
    for line in open(path, encoding="utf-8", errors="replace"):
        m = re.match(r"^\.fn (\S+),", line)
        if m:
            cur = (m.group(1), [])
            continue
        if line.startswith(".endfn"):
            if cur:
                out.append(cur)
            cur = None
            continue
        if cur is not None:
            cur[1].append(line.rstrip("\n"))
    return out


def size_of(lines):
    return 4 * sum(1 for l in lines if INSN_RE.match(l))


def pool(path):
    """[(slot_offset, tag, raw_u32), ...] from .sdata2, in layout order."""
    txt = open(path, encoding="utf-8", errors="replace").read()
    try:
        i = txt.index('.section .sdata2, "a"')
    except ValueError:
        return []
    j = txt.find("\n.section", i + 10)
    seg = txt[i:j if j > 0 else len(txt)]
    out = []
    for m in re.finditer(
            r"# \.sdata2:(0x[0-9A-Fa-f]+).*?\n\.obj \"(@\d+)\", local\n\t"
            r"\.4byte (0x[0-9A-Fa-f]+)", seg):
        out.append((int(m.group(1), 16), m.group(2), int(m.group(3), 16)))
    return out


def first_users(path):
    """{tag: name of the first function, in emission order, that loads it}."""
    seen = {}
    cur = None
    for line in open(path, encoding="utf-8", errors="replace"):
        m = re.match(r"^\.fn (\S+),", line)
        if m:
            cur = m.group(1)
            continue
        for tag in re.findall(r'"(@\d+)"@sda21', line):
            seen.setdefault(tag, cur)
    return seen


def as_float(u32):
    import struct
    return struct.unpack(">f", struct.pack(">I", u32))[0]


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    frag = sys.argv[1]
    raw = "--raw" in sys.argv
    unit = find_unit(frag)
    path = disasm(unit, use_cache="--no-cache" not in sys.argv)
    fns = functions(path)

    if "--list" in sys.argv:
        print("%s: %d functions" % (unit["name"], len(fns)))
        for name, lines in sorted(fns, key=lambda f: -size_of(f[1])):
            print("  %6db  %s" % (size_of(lines), name))
        return

    if "--order" in sys.argv:
        print("%s: .text definition order (= literal interning order)"
              % unit["name"])
        for k, (name, _) in enumerate(fns):
            print("  %3d  %s" % (k, name))
        return

    if "--pool" in sys.argv:
        p = pool(path)
        users = first_users(path)
        print("%s: %d .sdata2 slots" % (unit["name"], len(p)))
        for k, (off, tag, u32) in enumerate(p):
            print("  %3d  0x%03x  %-7s %14.6g   first use: %s"
                  % (k, off, tag, as_float(u32), users.get(tag, "(none)")))
        return

    sym = sys.argv[2]
    hits = [(n, l) for n, l in fns if sym in n]
    if not hits:
        raise SystemExit("no function matching %r in %s (try --list)"
                         % (sym, unit["name"]))
    for name, lines in hits:
        print("=== %s  %db ===" % (name, size_of(lines)))
        for l in lines:
            print(l if raw else PREFIX_RE.sub("\t", l))
        print()


main()
