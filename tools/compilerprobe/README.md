# compilerprobe: run-time experiments on the patched compiler

Diagnostic tools used to attribute non-matching code to specific behaviour of
the patched CodeWarrior `GC/2.0p1a` (see `tools/patch_compiler.py` for its
alias-patch clauses and `docs/RW_RESIDUE.md` for the findings). None of them
modifies a compiler binary on disk; they patch the running process under a
small Win32 debugger (built on `tools/regalloc/rcap.py`).

- `alt.py` — compile a unit (or a private source copy) with any compiler under
  `build/compilers/GC/` and score it against the target.
- `ablate.py` — switch individual alias-patch hooks off (stub a clause
  predicate, or restore a dispatch entry to the stock 2.0p1 handler) for one
  compile.
- `cablate.py` — conditional ablation of clause V's walk only for a given kind
  of frame store (`--mode vtmp` compiler temporaries, `vdecl` declared
  locals, `vfr` all frame stores, `log` to just record hits).
- `trace.py` — log which scheduler/alias clauses answer may-alias queries
  while compiling a function.
