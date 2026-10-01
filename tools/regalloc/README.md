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
