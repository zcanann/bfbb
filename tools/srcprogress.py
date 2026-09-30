#!/usr/bin/env python3
"""How much of the source is DONE, as opposed to how much of it matches.

report.json's "exact" counts a function only when our object is byte-identical
to retail's. That conflates two very different states:

  - our C says something different from retail's C            (work remains)
  - our C is right and only the scheduler or register allocator
    puts the instructions somewhere else                      (nothing to write)

The second is real progress the exact metric refuses to show, and it is a large
share of what is left. This tool separates them and reports a second number:

  source-complete = exact + codegen-only + store-then-reload

A function counts as codegen-only when semdiff's multiset test finds no
difference: registers, branch destinations and anonymous pool ordinals erased,
every literal and memory offset kept. Reordering cannot change a multiset and
renaming cannot change a normalised one, so a clean multiset means the two
objects execute the same operations on the same values.

THE RELOAD-ONLY CLASS
---------------------
One family of "semantic" differences is not a source problem at all. Retail
reloads a static it has just stored; our mwcc forwards the stored value and
skips the load. docs/DUPLOTRON.md records this as a known compiler defect, gated
at the -O2 threshold inside the optimizer, deliberately deprioritised, and
already worked around at a handful of sites with the `volatile` device.

The project's rule for that device is that it may only be installed where it
takes a function ALL the way to 100.0 -- otherwise it banks no bytes and masks
the compiler-side fix. Most of these functions are capped below 100 by the
register allocator anyway, so the device does not apply and there is nothing
to write.

They are therefore counted as compiler-track, but reported on their own line
so they stay visible: if the value-numbering path is ever widened, this is the
number that would move.

Signature, applied per TERM rather than per function: a target-only load
whose operand is stored on both sides (retail reads back what it just
wrote), or one whose term is merely rarer on our side rather than absent
(retail re-reads a value we held in a register). Register copies on our
side are the other half of the same pattern.

Judging this per function -- the first version of this tool -- meant one
unexplained term threw the whole function into "needs source", and every
one of its bytes with it. zEntPlayer_Update contributed 18188 bytes on the
strength of two source-shaped clusters while 25 of its 76 terms were this
defect. Ranking by unexplained TERMS instead of bytes moves it from first
place to sixth, which is where it belongs.

RANK BY TERMS, NOT BYTES
------------------------
A function's size says nothing about how much of it is wrong. An 18KB function
with two bad statements and a 200-byte function that is wrong throughout both
land in "needs source", and by bytes the first looks ninety times the problem.
The per-function tables below therefore rank by *unexplained terms* -- differing
instruction terms that are not the reload-only class -- and print bytes beside
them rather than instead of them.

WHAT THIS NUMBER IS NOT
-----------------------
It is an upper bound on completeness, not a proof. The multiset test has one
known hole: two different float constants both normalise to `lfs R, @P@sda21`,
so a wrong .sdata2 value cancels out. Units where `datadiff.py --consts`
reports a value difference are therefore marked (!) and their codegen-only
verdicts are weaker than the rest. The iCollide 0.7010677/0.70710677 bug lived
in exactly that hole. `tools/pooldiff.py` covers it directly by resolving each
pool reference to the bytes it names; run it before trusting a verdict here.

The same hole widens the reload-only class slightly: a target-only
`lfs R, @P@sda21` is credited as a re-read whenever we load any pool constant
anywhere, even if retail's is a different number. pooldiff is the check.

Treat source-complete as "nothing more to write here that we can see", and the
DOL sha1 as the only proof of anything.

Usage:
  srcprogress.py                 whole-project summary plus per-unit table
  srcprogress.py <unit-frag> ... restrict to matching units
  srcprogress.py --list          list every function still needing source work
  srcprogress.py --json          machine-readable, for tracking over time
"""

import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import semdiff

ROOT = semdiff.ROOT
GAME = "main/SB/"


def consts_suspect():
    """Units whose .sdata2 holds a value we do not have, or vice versa.

    A codegen-only verdict inside one of these is less trustworthy, because the
    multiset test cannot see a wrong float.
    """
    import datadiff
    import struct
    bad = set()
    for u in datadiff.CFG["units"]:
        if not u["name"].startswith(GAME):
            continue
        t = os.path.join(ROOT, u["target_path"])
        o = os.path.join(ROOT, u["base_path"])
        if not (os.path.exists(t) and os.path.exists(o)):
            continue
        for sec in (".sdata2", ".sbss2"):
            T, O = datadiff.section(t, sec), datadiff.section(o, sec)
            if T == O:
                continue
            def words(b):
                from collections import Counter
                return Counter(struct.unpack(">I", b[i:i + 4])[0]
                               for i in range(0, len(b) // 4 * 4, 4))
            A, B = words(T), words(O)
            miss, extra = A - B, B - A
            miss.pop(0, None)
            extra.pop(0, None)
            if miss or extra:
                bad.add(u["name"])
    return bad


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    want_list = "--list" in sys.argv
    want_json = "--json" in sys.argv

    report = json.load(open(os.path.join(ROOT, "build/GQPE78/report.json")))
    sizes, pcts = {}, {}
    for u in report["units"]:
        if not u["name"].startswith(GAME):
            continue
        for f in u.get("functions", []):
            sizes[(u["name"], f["name"])] = int(f.get("size") or 0)
            pcts[(u["name"], f["name"])] = f.get("fuzzy_match_percent")

    suspect = consts_suspect()

    units = [u for u in semdiff.CFG["units"] if u["name"].startswith(GAME)
             and (not args or any(a.lower() in u["name"].lower() for a in args))]

    LOADS = ("lfs ", "lwz ", "lfd ", "lbz ", "lhz ", "lha ")
    COPIES = ("mr ", "fmr ")

    def split_terms(hit):
        """(reload-class terms, residual terms) for one function.

        The store-then-reload defect is recognised per TERM, not per function.
        A target-only load whose operand is also stored on both sides is retail
        reloading a value our mwcc kept in a register: the store cancels between
        the streams, so only the load shows up. Register copies on our side are
        the other half of the same pattern.

        Doing this per function -- the tool's first version -- meant a single
        residual term threw the whole function into "needs source", and with it
        every one of its bytes. zEntPlayer_Update alone put 18188 bytes there on
        the strength of two source-shaped clusters, while 22 of its 76 terms
        were this defect.
        """
        stored = hit.get("stored_both") or set()
        ours = hit.get("ours_terms") or set()
        rl, res = 0, 0
        for t in hit["target_only"]:
            parts = t.split(None, 1)
            reload_shaped = t.startswith(LOADS) and (
                # retail stores a value and reads it straight back
                (len(parts) == 2 and parts[1] in stored)
                # or it simply reads the same thing again where we held it in a
                # register: the term is not absent from our stream, only rarer
                or t in ours)
            if reload_shaped:
                rl += 1
            else:
                res += 1
        for o in hit["ours_only"]:
            if o.startswith(COPIES):
                rl += 1
            else:
                res += 1
        return rl, res

    semantic = set()
    reload_cls = set()
    residual = {}
    reloaded = {}
    for u in units:
        for hit in semdiff.scan(u):
            key = (hit["unit"], hit["name"])
            rl, res = split_terms(hit)
            residual[key] = res
            reloaded[key] = rl
            if res == 0:
                reload_cls.add(key)
            else:
                semantic.add(key)

    rows = {}
    todo = []
    for (un, fn), sz in sizes.items():
        if args and not any(a.lower() in un.lower() for a in args):
            continue
        r = rows.setdefault(un, dict(exact_n=0, exact_b=0, cg_n=0, cg_b=0,
                                     rl_n=0, rl_b=0, sem_n=0, sem_b=0,
                                     res_t=0, rl_t=0))
        p = pcts[(un, fn)]
        if p == 100.0:
            r["exact_n"] += 1
            r["exact_b"] += sz
        elif (un, fn) in reload_cls:
            r["rl_n"] += 1
            r["rl_b"] += sz
        elif (un, fn) in semantic:
            r["sem_n"] += 1
            r["sem_b"] += sz
            r["res_t"] += residual[(un, fn)]
            r["rl_t"] += reloaded[(un, fn)]
            todo.append((residual[(un, fn)], sz, p, un, fn, reloaded[(un, fn)]))
        else:
            r["cg_n"] += 1
            r["cg_b"] += sz

    tot = dict(exact_n=0, exact_b=0, cg_n=0, cg_b=0, rl_n=0, rl_b=0,
               sem_n=0, sem_b=0, res_t=0, rl_t=0)
    for r in rows.values():
        for k in tot:
            tot[k] += r[k]
    allb = tot["exact_b"] + tot["cg_b"] + tot["rl_b"] + tot["sem_b"]
    alln = tot["exact_n"] + tot["cg_n"] + tot["rl_n"] + tot["sem_n"]

    if want_json:
        print(json.dumps(dict(total_bytes=allb, total_functions=alln,
                              **tot,
                              suspect_units=sorted(suspect)), indent=1))
        return 0

    if want_list:
        print("%d function(s) still needing source work, worst first\n" % len(todo))
        print("  %5s %6s  %7s  %-24s %s"
              % ("terms", "reload", "bytes", "unit", "function"))
        for res, sz, p, un, fn, rl in sorted(todo, reverse=True):
            mark = " (!)" if un in suspect else ""
            print("  %5d %6d  %7d  %-24s %s%s"
                  % (res, rl, sz, un.replace(GAME, "")[:24], fn[:46], mark))
        print("")
        print("  terms  = differing terms that are NOT the store-then-reload defect")
        print("  reload = terms in the same function that ARE, needing no source work")
        return 0

    def pc(x):
        return 100.0 * x / allb if allb else 0.0

    print("Game code: %d functions, %d bytes\n" % (alln, allb))
    print("  exact            %5d fns  %8d bytes  %6.2f%%"
          % (tot["exact_n"], tot["exact_b"], pc(tot["exact_b"])))
    print("  codegen-only     %5d fns  %8d bytes  %6.2f%%   source right, "
          "compiler places it differently" % (tot["cg_n"], tot["cg_b"], pc(tot["cg_b"])))
    print("  reload-only      %5d fns  %8d bytes  %6.2f%%   retail re-reads what "
          "we hold in a register" % (tot["rl_n"], tot["rl_b"], pc(tot["rl_b"])))
    print("  " + "-" * 68)
    sc_n = tot["exact_n"] + tot["cg_n"] + tot["rl_n"]
    sc_b = tot["exact_b"] + tot["cg_b"] + tot["rl_b"]
    print("  SOURCE-COMPLETE  %5d fns  %8d bytes  %6.2f%%" % (sc_n, sc_b, pc(sc_b)))
    print()
    print("  needs source     %5d fns  %8d bytes  %6.2f%%"
          % (tot["sem_n"], tot["sem_b"], pc(tot["sem_b"])))
    print("      %d unexplained term(s), and %d more inside those same functions"
          % (tot["res_t"], tot["rl_t"]))
    print("      that are only retail re-reading a value, and need no source work.")
    print("      Bytes overstate this bucket: a function contributes every byte it")
    print("      has as soon as one term is unexplained, however small the fix.")
    if suspect:
        print("\n  %d unit(s) carry a .sdata2 value difference, so codegen-only "
              "verdicts\n  inside them are weaker -- marked (!) below and in --list."
              % len(suspect))

    print("\nUnits with source work left, by unexplained terms:\n")
    print("  %5s %6s %8s %5s  %8s  %-30s"
          % ("terms", "reload", "bytes", "fns", "src-done", "unit"))
    for un, r in sorted(rows.items(),
                        key=lambda x: (-x[1]["res_t"], -x[1]["sem_b"])):
        if not r["sem_b"]:
            continue
        ub = r["exact_b"] + r["cg_b"] + r["rl_b"] + r["sem_b"]
        done = 100.0 * (r["exact_b"] + r["cg_b"] + r["rl_b"]) / ub if ub else 0.0
        mark = " (!)" if un in suspect else ""
        print("  %5d %6d %8d %5d  %7.2f%%  %-30s%s"
              % (r["res_t"], r["rl_t"], r["sem_b"], r["sem_n"], done,
                 un.replace(GAME, "")[:30], mark))
    done_units = [u for u, r in rows.items() if not r["sem_b"]]
    print("\n  %d of %d units are source-complete." % (len(done_units), len(rows)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
