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
