# PS2 robotic goals

`zNPCGoalAlertChuck::ZoomMove` now uses direct compound subtraction on PS2.
The previous inverted subtraction emitted an extra negation; the original
body subtracts the traveled distance directly. The 464-byte body improves
from 98.18965% to exactly 100% in USA, PAL, German and French originals.
No compiler patch is required.

Full original-backed unit comparisons show no other function changes:

| Region | Exact functions before/after | Exact bytes before/after | Fuzzy before/after |
| --- | --- | --- | --- |
| USA | 192 / 193 of 229 | 62004 / 62468 | 98.25487 / 98.26354 |
| PAL | 191 / 192 of 229 | 61268 / 61732 | 98.25478 / 98.26346 |
| German | 191 / 192 of 229 | 61268 / 61732 | 98.25478 / 98.26346 |
| French selected bodies | 21 / 22 of 22 | 5256 / 5720 | 99.85315 / 100 |

Fresh builds of all three GameCube units preserve every function record and
unit measure. The platform conditional retains their existing expression.
Evidence: `build/goalrobo-zoom-source-summary.json`, the regional before/after
reports, and `build/goalrobo-zoom-gc-check.log`.

`zNPCGoalRespawn::DoAppearFX` also matches exactly after restoring the original
PS2 addition operand order for the box-bound height. The previous expression
added the half-dimension before the offset; the original adds the offset first.
This changes the floating-point register assignment in six instructions.
The 940-byte function improves from 99.85107% to 100% in all three debug regions,
with no other full-unit function changes. USA reaches 194/229 exact functions
and 63,408 exact bytes (98.26499% fuzzy); PAL and German reach 193/229 and
62,672 exact bytes (98.26491%). The existing French selected profile is unaffected.
All three GameCube unit reports remain unchanged. Evidence is
`build/goalrobo-appear-source-summary.json` and
`build/goalrobo-appear-gc-check.log`.

DeathRayUpdate now matches all 2,260 original bytes in USA, PAL and German
(96.12212% to 100%). Its PS2 warmup check uses the original integer predicate
stage, its local segment table initializes before the warmup division, and
its two decrements use MAX(0, count - 1). Reversing the MAX operands emitted a
different branch shape. The GameCube integer bit expression remains behind
the platform guard; the declaration/initialization order works in both builds.

Full regional GoalRobo reports change only DeathRayUpdate: +2,260 exact bytes
and one exact function in each debug PS2 region. The currently recovered
French subset is unchanged because it does not yet identify this body.
All three freshly built GameCube units preserve every function record and
measure. Evidence: `build/goalrobo-warmup-source-summary.json`, its regional
reports and `build/goalrobo-warmup-gc.log`. No compiler patch was needed.
