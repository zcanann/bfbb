# PS2 curve evaluator source recovery

`xCurveAssetEvaluate` now retains the original generalized interval arithmetic
and calls the integer absolute-value runtime routine out of line on PS2. The
range helper takes the evolving time and clamp mode by reference. Its original
lower endpoint is zero, but the retail instructions still perform the endpoint
subtractions and final addition. Inlining this helper preserves those operations
and the original register lifetimes. The GameCube and Xbox source paths retain
their previous expressions.

The bounded `inline_intrinsics` pragma restores the retail out-of-line call;
it is reset after this translation unit's evaluator. No compiler patch, build
profile, target relocation, or symbol registry changed.

## Measured source comparison

The complete one-function unit improves from 74.04348% to 100% in the existing
report for SLUS-20680, SLES-51968, SLES-51970, and SLES-53623 (460 bytes each).
The GU4Y78 GameCube function remains 364/364 exact bytes. France uses the
existing independently reviewed evaluator extent at `0x3ae3b0`; this change
does not infer or register a new identity or boundary.

The report result has one explicit raw-proof limitation. All 114 non-call
instruction words (456 bytes), including floating-point operations and register
assignments, equal each original without modification. The remaining instruction
at evaluator offset `0xe0` calls `0x114b50` in all four originals. The compiled
object instead carries the ordinary `R_MIPS_26 abs` relocation there. That
original callee's 20-byte body is:

```
bgez  a0, return
move  v0, a0          # delay slot
subu  v0, zero, v0
return:
jr    ra
nop
```

This independently establishes signed integer absolute-value behavior, but its
name is absent from the examined original DWARF and ELF symbol tables. The
callee's symbol identity therefore remains unresolved. No runtime alias or
call-target evidence is inferred from the source comparison's 100% result.

## Reproduction artifacts

Private artifacts in the source worktree's `build` directory:

- `curve-original.py` / `.txt`: complete original instructions and DWARF locals.
- `curve-final-comparison.py` / `.json`, `curve-final-changes.json`: complete
  before/after reports for the three profiled PS2 regions.
- `curve-france.py`, `curve-france-comparison.json`: additional complete
  before/after comparison for the existing French evaluator target.
- `curve-raw-proof.py` / `.json`: checks all 115 instruction positions, demands
  exactly the single unresolved call difference, and records the original
  callee bytes separately without applying a guessed relocation.
- `checkcurve-gc.py`, `curve-gc-before.json`, `curve-gc-after.json`: unchanged
  complete GameCube translation-unit comparison.

The source baseline is `a90952bbc` (root `49857b786` plus the already-delivered
particle-renderer follow-up). These artifacts make no changes to shared targets
or generated evidence registries.
