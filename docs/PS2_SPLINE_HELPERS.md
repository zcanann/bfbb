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
complete source-linked executable. France has no xSpline comparison profile.

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
