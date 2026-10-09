# PS2 spline helper ownership

Retail PS2 inlines `CoefToUnity3` into `CoefSeg3`. Mark that helper inline for
PS2, and express its three-component traversal as an unsigned forward loop.
`BasisToCoef3` likewise uses an unsigned index, matching the retail unsigned
loop comparisons in its inlined callers. Both loops visit the same elements
in the same order; coefficient expressions and stores are unchanged.

The PS2 `ArcLength3` now names both endpoint speeds before their final sum.
This retains the existing floating-point arithmetic and accumulation grouping
while recovering the retail endpoint register lifetimes.

Fresh complete-unit reports for SLUS-20680, SLES-51968 and SLES-51970 agree:

| Function | Before | After |
| --- | ---: | ---: |
| CoefSeg3 | 81.860214% | 93.412186% |
| EvalBspline3 | 90.088710% | 90.661290% |
| xSpline3_EvalSeg | 98.525345% | 99.078340% |
| ArcLength3 | 99.188034% | 99.914530% |

The unit fuzzy score rises from 94.41555% to 96.37248%. Its five exact
functions and 1,756 exact code bytes remain unchanged, and no other function
score regresses. These profiles do not measure PS2 source data or establish a
complete source-linked executable. The French profile also compares all
thirteen functions; its combined validation is recorded below.

A full GameCube USA source rebuild produces a report exactly equal to the
baseline; the normal retail link still has SHA1
`306526d90b48e99894c3138f5fc8f2716d9fecf6`. The helper loop edits do not alter
GameCube instruction scores. No compiler flags, compiler patches, assembly or
volatile accesses are added. Original source spelling is not claimed.

Private evidence: `build/ps2spline-oct08/validation.json`, the six regional
unit reports, GC report and build logs. Earlier unsuccessful GC-only spline
trials are under `build/spline-oct08/`; the xParEmitter ternary trial was
rejected because its code gain displaced jump-table entries and lost matched
data.

## Tridiagonal solver address induction

On PS2, name the predecessor and successor vectors through pointers and use
an explicit descending `for` loop for back substitution. Retail carries the
neighbor address separately from the current array cursor. The prior index
locals and post-decrement condition recreated extra address calculations and
a different loop guard. The PS2 path retains the same three component
expressions, store order, allocations and frees. GameCube keeps the existing
index-based loops, which avoid unwanted loop unrolling in that compiler.

Fresh whole-unit reports in all three debug regions change only
`Tridiag_Solve`, from 83.79518% to 98.650604%. The xSpline unit score increases
from 96.37248% to 97.75168%; five exact functions and 1,756 exact code bytes
remain unchanged. Remaining differences include register allocation and load
ordering. No compiler-version cause is assumed.

The full rebuilt GameCube USA report is again identical to the original
baseline, and its normal retail SHA1 is unchanged. Private evidence is
`build/ps2tridiag-oct08/`, including all six regional unit reports and
`validation.json`. PS2 source data and a complete linked executable remain
outside these unit comparisons.

The French whole-unit before/after comparison independently confirms the same
five improvements from both changes, including `Tridiag_Solve`. Its fuzzy score
rises from 94.41555% to 97.75168%, with exact counts and every other function
unchanged. The configured target object comes from the previously verified
French profile; the complete source is rebuilt with the current pinned profile
and French version define. Evidence is `build/oct08-spline-france-{before,after}`
and `build/oct08-spline-france.log` in the staging checkout.

Integration also rebuilt all source and verified the full retail DOL checksum
for each of GQPE78, GQPP78 and GU4Y78. All three complete GameCube reports are
unchanged by the spline edits.
