# regalloc: GC/2.0p1a register-allocator capture

Tools for understanding why a function's registers differ from retail when
the instructions already match. They drive the patched CodeWarrior
(`build/compilers/GC/2.0p1a/mwcceppc.exe`) under a tiny Win32 debugger and dump
its graph-colouring state.

- `rcap.py <src.c> <unit-substring> [--fn NAME] [--json out.json]` — compile
  with the unit's flags and capture, per function, the interference graph,
  the simplify (pop) order and the final colours. Breakpoints are hard-wired
  to GC/2.0p1a addresses (`colorinstructions` 0x508680, `colorgraph`
  0x508900, its return 0x508766).
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
