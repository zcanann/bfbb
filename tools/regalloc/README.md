# regalloc: GC/2.0p1a register-allocator capture

Tools for understanding why a function's registers differ from retail when
the instructions already match. They drive the patched CodeWarrior
(`build/compilers/GC/2.0p1a/mwcceppc.exe`) under a tiny Win32 debugger and dump
its graph-colouring state.

- `rcap.py <src.c|cpp> <unit> [--fn NAME] [--json out.json]` — compile
  with the unit's flags and compiler (build.ninja `mw_version`) and capture, per function, the interference graph,
  the simplify (pop) order and the final colours. Breakpoints are hard-wired
  to GC/2.0p1a addresses (`colorinstructions` 0x508680, `colorgraph`
  0x508900, its return 0x508766); the derived 2.0p1b..f share them, and rcap refuses a compiler
  whose bytes there differ.
- `replay.py` — re-runs the colouring algorithm on a capture; reproduces the
  compiler's colours exactly (validates the model).
- `tmap.py` — maps captured virtual registers to the target's registers using
  `tools/fdiff.py` output; `solve.py` / `solve2.py` / `perm.py` / `dfs*.py`
  search renumberings or pop orders that would yield the target colouring.
- `uses.py`, `show.py`, `capvar.py`, `try.py`, `cc.py`, `xdis.py`, `pe.py` —
  helpers (vreg uses, pop order, variant capture/scoring, standalone compile,
  disassembly, PE parsing).

Measured on GC/2.0p1a: simplify scans virtual registers in ascending order and
pushes nodes of degree < K (K=29 GPR, ~32 FPR); colouring takes the lowest
free volatile register, else claims a new callee-saved one from r31 down.
Named locals are numbered in reverse declaration order (first declared pops
first and gets r31), parameters before locals, inline expansions in reverse
call order; frontend CSE temps (@NNN) after named locals, optimizer temps
last. Most residual register-only mismatches need a CSE/optimizer temp to
swap rank with a named web, which declaration order cannot do.

## Temp-creation capture and residue diagnosis (added)

- `tcap.py` / `wcap.py <src.c> <unit> --fn NAME [--temps] [--all]` — also break at the
  unique-name hash (0x441570, called from the `@NNN` generator at 0x4e976a) and at the
  IrOptimizer.c trace-string pushes, so every `@` object is attributed to the phase/module that
  created it (INLINE, IRO(linearize), IRO(in LoopUnroller), IRO(in FindLoops), IRO CSE
  ("after Second pass"), IROUseDef web split ("after EvaluateConditionals"), IroVars
  scalarisation). `modmap.py addr...` names the module of a code address via assert strings.
- `diag.py <src.c> <unit-path> <fn> [--pairs]` — one-shot diagnosis: wcap capture, target
  colours (tmap), exact replay check, then the rank moves (single, else pair) that make the
  replayed colouring equal retail. Writes `<fn>.json` and `<fn>.json.tgt<cls>`.
- `hyp.py caps...` — re-ranks families of webs (all `@` forward, each family reversed, ...) to
  test whether a residue is a systematic numbering difference; matched controls must stay at 0.
- `vsolo.py --src copy.c <solo args>` / `vfdiff.py <unit> <sym> [--src copy.c]` — measure a
  private COPY of a unit (nothing in build/ or src/ is touched). `tmap.py` now uses vfdiff and
  honours `TMAP_SRC`. `rcap.py` honours `RCAP_EXTRA_FLAGS` / `RCAP_MW`.
- `webs.py cap.json fn [cls]` — per-vreg name/colour/pop position/first defs.

Numbering rules measured with these (vreg ascending): params (decl order) < the function's own
named locals (reverse textual declaration order over all scopes) < `@` objects in reverse
creation order, i.e. IROUseDef split webs < CSE temps (reverse first-occurrence order) <
FindLoops temps < unroller temps < IRO linearise temps (`?:`, `&&`) < inline-expansion objects
(later call lower; inside one expansion: modified params last-to-first, then top-level locals in
DECLARATION order, block locals, return temp) < objectless lowering temps (instruction order).
`x = c ? a : b` leaves x without a web (the value is an objectless temp). The same declaration
pair therefore ranks oppositely out of line and in an inlined copy.

## C++ units, compiler selection, source copies

All of this works from any cwd (repo root = `$BFBB_ROOT`, else the ancestor of `tools/regalloc`
holding `build.ninja`).

- Compiler: the unit's own `mw_version` from build.ninja (currently GC/2.0p1f); `RCAP_MW=GC/2.0p1a`
  overrides it for rcap/wcap/diag AND the vfdiff/tmap target alignment (both sides must agree).
- `RCAP_EXTRA_FLAGS="-DFOO -i dir"` is appended to the unit's cflags (capture and vfdiff).
- A source COPY outside the unit's directory (e.g. a scratchpad `zcopy.cpp`) automatically gets
  `-i <unit source dir>` so its `#include "local.h"` still resolves.
- `<unit>` may be a fragment (`bamatlst`) or a path (`SB/Game/zNPCSupport`); exact path/stem
  matches win over substrings.
- Function names: the compiler Object only holds the bare name (`Render`, `__ct`), so rcap also
  walks its namespace chain (Object+6 -> NameSpace {+0 parent, +4 name}) and stores `qual`
  (`NPCBlinker::Render`) and `fnidx` (codegen ordinal among matching functions) in each capture.
  Pass the function to diag/wcap/tmap as bare, qualified or mangled:

```
python tools/regalloc/diag.py src/rwsdk/world/bamatlst.c rwsdk/world/bamatlst _rpMaterialListStreamRead
python tools/regalloc/diag.py src/SB/Core/x/xMath3.cpp SB/Core/x/xMath3 xMat3x3Mul
python tools/regalloc/diag.py src/SB/Game/zNPCSupport.cpp SB/Game/zNPCSupport NPCBlinker::Render
python tools/regalloc/diag.py src/SB/Game/zNPCSupport.cpp SB/Game/zNPCSupport Render__8NPCLaserFP5xVec3P5xVec3
python tools/regalloc/diag.py src/SB/Game/zNPCSupport.cpp SB/Game/zNPCSupport Render --idx 1
```

  A bare name matching several functions is an error listing the choices. `--sym <mangled>` /
  `TMAP_SYM` selects by mangled symbol; same-class overloads (`LERP__Ffff` vs `LERP__FfUcUc`) are
  then matched to captures by the address order of those symbols in our object (= codegen
  order, a note is printed); `--idx N` / `TMAP_IDX` forces the index among the candidates.
- diag output files use a sanitised name (`NPCBlinker__Render.json`, `.json.tgt<cls>`,
  `.diff.json` = the objdiff dump reused by tmap via `--diff`).
- `vfdiff.py <unit> <symbol>` accepts a mangled name, mangled prefix (`Render__10NPCBlinker`),
  qualified or bare name (ambiguity lists candidates); importable (`diff_json`, `rows`).
- The capture runs the compiler directly (not through sjiswrap), so a source with non-ASCII
  (Shift-JIS) text may not compile identically; on a compile failure rcap re-runs the command and
  prints the compiler's diagnostics.

## Corpus and allocator-model tests (added)

- `corpus.py residues res.json... --out DIR` captures every REG-shaped residue of a
  `tools/residue.py --json` dump (`diag.py --capture-only`: graph, target colours, replay check).
  `corpus.py controls UNIT... --out DIR` captures every function of whole units (`rcap.py`).
- `ordmodel.py --res DIR --ctl DIR [-v]` replays alternative simplify/spill/select/numbering
  rules on both corpora. It counts residues fixed and matched controls broken. Spilling functions
  are replayed round by round, and each earlier round must reproduce its failed set.
- Spill support: `rcap.py` now records the real spill cost (`IGNode+0xc`; `+8` was read before)
  and the no-spill mark (`0x5e0898`). `replay.simplify` uses both, so the spill pick is exact.
  `diag.py` and `tmap.py` use the last colouring round of a function that spilled.
- Result (2026-10-04, 12 747 matched function/class colourings, 344 units): base replay is
  exact everywhere, and every alternative rule breaks matched functions. The colouring code is
  identical in GC/2.0p1 to 2.7. See docs/RW_RESIDUE.md section 2.
- Set `BFBB_ROOT=C:/Projects/bfbb` when running from a worktree without `build/`.
