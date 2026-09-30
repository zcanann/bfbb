#!/usr/bin/env python3
"""Compare a unit's DATA sections against the target, byte by byte and
relocation by relocation.

report.json scores a data symbol as matched only at exactly 100%, so a single
wrong pointer in a 36KB table costs the whole table. semdiff only reads code;
this reads everything else.

Two kinds of difference matter and they need different fixes:

  bytes        a literal in a table is wrong -- an index, a flag, a float
  relocations  a pointer is wrong, missing, or points at the wrong symbol;
               this is the NULL-dispatch-table class

Relocations are compared three ways, because each failure looks different:
  count      we have fewer entries than retail -> NULLs where pointers belong
  positional same offset, different target -> the wrong function or string
  set        same targets, different offsets -> declaration order differs

Usage:
  datadiff.py                       every game unit with a data gap, worst first
  datadiff.py <unit-frag> ...       detail for matching units
  datadiff.py <unit-frag> -n 40     show up to N differences per section
  datadiff.py --consts              .sdata2 constants that differ in VALUE,
                                    compared as multisets so pool order is
                                    ignored -- semdiff's blind spot
"""

import json
import os
import re
import subprocess
import sys
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OD = os.path.join(ROOT, "build/binutils/powerpc-eabi-objdump")
RE = os.path.join(ROOT, "build/binutils/powerpc-eabi-readelf")
CFG = json.load(open(os.path.join(ROOT, "objdiff.json")))

DATA_SECS = (".data", ".rodata", ".sdata", ".sdata2", ".ctors", ".dtors")
GAME = "main/SB/"


def section(obj, name):
    out = subprocess.run([OD, "-s", "-j", name, obj], capture_output=True,
                         text=True).stdout
    buf = {}
    for line in out.splitlines():
        m = re.match(r"\s*([0-9a-f]{4,8}) ((?:[0-9a-f]{2,8} )+)", line)
        if not m:
            continue
        hexs = m.group(2).replace(" ", "")
        blob = bytes.fromhex(hexs[:len(hexs) // 2 * 2])
        base = int(m.group(1), 16)
        for i, v in enumerate(blob):
            buf[base + i] = v
    if not buf:
        return b""
    return bytes(buf.get(i, 0) for i in range(max(buf) + 1))


def relocs(obj):
    out = subprocess.run([RE, "--wide", "-r", obj], capture_output=True,
                         text=True).stdout
    cur, d = None, {}
    for line in out.splitlines():
        m = re.match(r"Relocation section '(\S+)'", line)
        if m:
            cur = m.group(1)
            continue
        if cur and re.match(r"^[0-9a-f]{8}\s", line):
            p = line.split()
            if len(p) >= 5:
                d.setdefault(cur, []).append(
                    (int(p[0], 16), p[4], p[6] if len(p) > 6 else "0"))
    return {k: sorted(v) for k, v in d.items()}


def report_gaps():
    r = json.load(open(os.path.join(ROOT, "build/GQPE78/report.json")))
    out = {}
    for u in r["units"]:
        if not u["name"].startswith(GAME):
            continue
        m = u["measures"]
        td, md = int(m.get("total_data") or 0), int(m.get("matched_data") or 0)
        if td and td != md:
            out[u["name"]] = (td - md, m.get("matched_data_percent", 0.0), md, td)
    return out


def detail(unit, limit):
    tgt = os.path.join(ROOT, unit["target_path"])
    ours = os.path.join(ROOT, unit["base_path"])
    if not (os.path.exists(tgt) and os.path.exists(ours)):
        return
    print("### %s" % unit["name"].replace(GAME, ""))
    TR, OR = relocs(tgt), relocs(ours)
    for sec in DATA_SECS:
        T, O = section(tgt, sec), section(ours, sec)
        rel = ".rela" + sec
        tr, orl = TR.get(rel, []), OR.get(rel, [])
        if T == O and tr == orl:
            continue
        print("  %-9s bytes target=%-7d ours=%-7d   relocs target=%-4d ours=%d"
              % (sec, len(T), len(O), len(tr), len(orl)))

        n = min(len(T), len(O))
        db = [i for i in range(n) if T[i] != O[i]]
        if db or len(T) != len(O):
            print("     byte differences: %d%s" % (
                len(db), "" if len(T) == len(O) else "  (+ size differs)"))
            # Group runs so a shifted table reads as one finding, not hundreds.
            runs, start, prev = [], None, None
            for i in db:
                if start is None:
                    start = prev = i
                elif i == prev + 1:
                    prev = i
                else:
                    runs.append((start, prev))
                    start = prev = i
            if start is not None:
                runs.append((start, prev))
            for a, b in runs[:limit]:
                print("       0x%05x-0x%05x  target %s   ours %s"
                      % (a, b, T[a:min(b + 1, a + 8)].hex(),
                         O[a:min(b + 1, a + 8)].hex()))
            if len(runs) > limit:
                print("       ... +%d more runs" % (len(runs) - limit))

        if tr != orl:
            toff = {x[0] for x in tr}
            ooff = {x[0] for x in orl}
            only_t = sorted(toff - ooff)
            only_o = sorted(ooff - toff)
            if only_t:
                print("     offsets relocated in target only: %s%s"
                      % (["0x%x" % x for x in only_t[:limit]],
                         "" if len(only_t) <= limit else " ...+%d" % (len(only_t) - limit)))
            if only_o:
                print("     offsets relocated in ours only  : %s%s"
                      % (["0x%x" % x for x in only_o[:limit]],
                         "" if len(only_o) <= limit else " ...+%d" % (len(only_o) - limit)))
            td = {x[0]: (x[1], x[2]) for x in tr}
            od = {x[0]: (x[1], x[2]) for x in orl}
            same = sorted(toff & ooff)
            mism = [o for o in same if td[o] != od[o]]
            if mism:
                print("     same offset, different target: %d" % len(mism))
                for o in mism[:limit]:
                    print("       0x%05x  target %s + %s\n                ours   %s + %s"
                          % (o, td[o][0][:58], td[o][1], od[o][0][:58], od[o][1]))
                if len(mism) > limit:
                    print("       ... +%d more" % (len(mism) - limit))
            tset, oset = Counter((x[1], x[2]) for x in tr), Counter((x[1], x[2]) for x in orl)
            miss, extra = tset - oset, oset - tset
            if miss or extra:
                print("     targets retail has that we lack: %d;  we have that retail lacks: %d"
                      % (sum(miss.values()), sum(extra.values())))
                for (s, a), c in list(miss.items())[:limit]:
                    print("       target-only: %s + %s  x%d" % (s[:58], a, c))
                for (s, a), c in list(extra.items())[:limit]:
                    print("       ours-only  : %s + %s  x%d" % (s[:58], a, c))


def consts():
    """Compare .sdata2 / .sbss2 as multisets of 4-byte words.

    Those sections hold nothing but numeric constants, so order is irrelevant
    and a multiset difference means a constant is genuinely wrong or missing --
    the class semdiff cannot see, because two different floats both normalise
    to `lfs R, @P@sda21`.
    """
    import struct
    for u in CFG["units"]:
        if not u["name"].startswith(GAME):
            continue
        t = os.path.join(ROOT, u["target_path"])
        o = os.path.join(ROOT, u["base_path"])
        if not (os.path.exists(t) and os.path.exists(o)):
            continue
        for sec in (".sdata2", ".sbss2"):
            T, O = section(t, sec), section(o, sec)
            if T == O:
                continue
            def words(b):
                return Counter(struct.unpack(">I", b[i:i + 4])[0]
                               for i in range(0, len(b) // 4 * 4, 4))
            A, B = words(T), words(O)
            miss, extra = A - B, B - A
            # A lone zero on the target side with a 4-byte size difference is
            # section padding, not a constant.
            miss.pop(0, None)
            extra.pop(0, None)
            if not miss and not extra:
                continue
            print("### %-40s %s  (target %d bytes / ours %d)"
                  % (u["name"].replace(GAME, ""), sec, len(T), len(O)))
            def fmt(w):
                return "0x%08x  %-16g" % (
                    w, struct.unpack(">f", struct.pack(">I", w))[0])
            for w, c in sorted(miss.items()):
                print("    target-only: %s x%d" % (fmt(w), c))
            for w, c in sorted(extra.items()):
                print("    ours-only  : %s x%d" % (fmt(w), c))


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    if "--consts" in sys.argv:
        return consts() or 0
    limit = 12
    if "-n" in sys.argv:
        limit = int(sys.argv[sys.argv.index("-n") + 1])
        args = [a for a in args if a != str(limit)]

    gaps = report_gaps()
    if not args:
        print("%d game unit(s) with a data gap, %d bytes total\n"
              % (len(gaps), sum(v[0] for v in gaps.values())))
        for n, (miss, pct, md, td) in sorted(gaps.items(), key=lambda x: -x[1][0]):
            print("  %7d missing  %6.2f%%  %-44s (%d/%d)"
                  % (miss, pct, n.replace(GAME, ""), md, td))
        return 0

    for u in CFG["units"]:
        if not u["name"].startswith(GAME):
            continue
        if not any(a.lower() in u["name"].lower() for a in args):
            continue
        detail(u, limit)
    return 0


if __name__ == "__main__":
    sys.exit(main())
