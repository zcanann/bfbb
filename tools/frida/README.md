# frida instrumentation for mwcceppc.exe (clause H toolkit, 2026-08-25)

Live-compile instrumentation that found clause H (see docs/DUPLOTRON.md and
tools/patch_compiler.py). Requires `pip install frida`. All scripts spawn
mwcceppc.exe DIRECTLY (bypassing sjiswrap) so the hooks land in the compiler
process; static VAs are rebased from the module base at runtime.

- `probe_alias.py <cc> <src> <out.o>` -- log every 0x511fc0 may-alias query
  (with caller) and every 0x511a30 VN store-kill, with memref decodes.
- `probe_cm3.py <cc> <src> <out.o>` -- log CodeMotion's 0x511cb0 queries:
  pairs (store memref) x (candidate memref, captured from 0x512e20 at
  recursion depth 0) with the answer.
- `hookscript.js` + `battery.py <mode> <unit>...` -- compile real units with
  a candidate predicate FORCED at 0x511cb0 (modes: off/h1/h1e0/h2/h3/h4) and
  print per-function match deltas vs mode `off`. Because the shipped
  compiler already contains clause H, `off` = clause H and any mode measures
  only its own widening.
- `fsolo.py <unit> [mode] [symfrag]` -- one unit, one mode, full listing.
- `mkclauseH.py <out.exe> [--null]` -- standalone builder for the clause-H
  variant from the 5965be76 (pre-H) compiler; `--null` applies only the
  section growth (used for the null test). Kept for archaeology; the real
  patch lives in tools/patch_compiler.py.

Pitfalls (learned the slow way): a frida `InvocationReturnValue` is valid
only inside its own onLeave -- copy with `rv.add(0)` before storing.
Helpers like 0x512e20 recurse -- capture at depth 0. Compiles finish in
tens of ms, so collect via send() during the run, not rpc polling. Paths in
battery.py/fsolo.py hardcode the worktree ROOT near the top; fix them if
this directory moves.

- `sigrank.py` + `sigprobe.js` + `flip.js` -- tree-wide measurement without
  deriving a compiler. `snap` records every game function's match percent
  (optionally under a frida script); `probe` counts `may_alias` query
  signatures per function; `rank` orders them by non-matching over matching
  occurrences; `flip` inverts the answer for chosen signatures and compares.
  A tree-wide snap takes about 30 s. Repo-relative, output in build/sigrank/.
  Clause W was found this way (docs/DUPLOTRON.md).
