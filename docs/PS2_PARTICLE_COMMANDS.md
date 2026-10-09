# PS2 particle-command source comparison

The unchanged complete `xParCmd.cpp` compiles with the existing pinned PS2
compiler and headers. All 30 original functions / 7,544 bytes are represented
in the comparison, including all unmatched functions. The initial standard
comparison matches 15 functions / 2,780 bytes, with 59.130436% fuzzy matching.
This is partial source coverage, not a completed TU or retail executable link.

Actual debug compilation confirms the sizes and every direct member offset
of all 22 concrete command/particle types present in this unit's original
DWARF, across all three debug-bearing releases. Debug information does not
change allocated source sections. The remaining source-only type names have
no corresponding concrete original declaration here and are not claimed to
have independently restored layouts.

Call and GP identities in the comparison profile derive from original DWARF.
No source functions, compiler settings, guessed API layouts, or original
boundaries were changed. All original-owned functions remain visible even
where compilation differs substantially. The normal code-match metric does
not establish complete relocation/data matching. France needs its own
independently verified identities before this unit can be reported there.

Private reproduction: `build/particles170/compile.py`, `compare.py`,
`type_proof.py`, and `type-proof.json`.

## Scalar particle command matching

The scalar command source now initializes particle traversal pointers after
computing command parameters, scales the random-motion timestep by `0.5f`
before applying the dimensions, and uses post-decrement tests in the three
piecewise interpolation loops. These changes follow the original instruction
ordering. The interpolation slopes are computed before loading the group head.
The existing Xbox pointer-order guards also apply to PS2 for KillSlow and Age.

All four PS2 releases improve from 21 to 25 exact functions and from 4,500 to
5,316 matched bytes out of the unchanged 7,544-byte unit. Newly exact functions
are KillSlow (176 bytes), OrbitPoint (200), OrbitLine (272), and MoveRandomPar
(168). Applying the two actual source `R_MIPS_26` relocations in MoveRandomPar
to the independently named `xurand` addresses reproduces every original byte
in each authenticated executable. The other three bodies have no relocations
and compare directly. No original boundaries or comparison profiles change.

Age and the three interpolation functions improve but retain two extra loop
NOPs with the pinned compiler; they remain unmatched. RandomVelocityPar retains
its unreconstructed PS2 VU implementation. These observations do not establish
that the residual differences are compiler-version defects.

The configured GameCube compiler preserves all 32 exact functions / 6,300 bytes
and every function score. Rebuilding the whole Xbox TU and genuine dependencies
with the pinned MSVC/LTCG production profile also improves both regional reports:
OrbitPoint and OrbitLine become exact, raising 12 to 14 exact functions and
3,380 to 3,817 matched bytes out of 5,492. No compared function regresses.

Private evidence in `C:/Projects/bfbb-agent-ps2-oct08/build` includes
`particle-raw-proof.json`, retained `ps2solo-*` whole-TU objects/reports,
`gc-before.json` / `gc-after.json`, and the `xbox-particle-{before,after}` and
`xbox-particle-eu-{before,after}` production build directories. The French
comparison reuses the verified target object from
`C:/Projects/bfbb-agent-rgb/build/france198/production/SLES-53623`; its four
new exact bodies were independently checked against the French boot ELF.
