# PS2 UI vertex setup

The original `zUI_Render` writes camera depth and reciprocal depth to each
PS2 vertex, and performs a second color pass using the existing `U8` color
locals. Those operations were missing from the shared source. Original DWARF
identifies the color locals, camera pointer and reciprocal-depth local; the
instructions load `globals.camera.lo_cam` and its near clip plane, then store
the depth fields and convert the color channels to floats.

The restored setup uses the original USA 640x448 and Europe/Germany 512x512
screen dimensions, with asset coordinates still based on 640x480. The second
corner calculations use the screen constants directly. Initial white colors
are set with each vertex, followed by the existing second color pass. PS2
model blend defaults use separate assignments matching the original branches.

| Version | Renderer before | Renderer after | Unit before | Unit after |
| --- | ---: | ---: | ---: | ---: |
| SLUS-20680 | 57.989197% | 97.61574% | 86.768265% | 93.79371% |
| SLES-51968 | 57.916924% | 97.62308% | 86.73968% | 93.797104% |
| SLES-51970 | 57.916924% | 97.62308% | 86.73968% | 93.797104% |

These are fuzzy gains in complete 2,592-byte USA and 2,600-byte PAL renderer
bodies. Exact totals remain 21/28 functions and 6,304 bytes per debug release;
every other function score is unchanged. The enabled France profile contains
three other functions, whose complete report is unchanged. The renderer has
no enabled France boundary, so no France renderer matching claim is made.

The full GameCube USA report remains identical and the retail DOL SHA-1 check
passes. Camera-depth setters are no-ops on GameCube; its blend-default source
form remains intact. No compiler flags, profiles, target boundaries, registries
or scoring rules change. Remaining differences include call argument lifetime,
branch layout and alignment; they are not attributed to a compiler version.

Private evidence is `build/ui-oct09/*-{before,after}.json`, source snapshots
and instruction diffs for the separate camera, color, constant and blend probes.

## Initializer call and inline boundaries

The original entity initializer calls `load_anim_list`; automatic inlining had
expanded it in the caller. A PS2-only `dont_inline` scope restores that call,
raising the 664-byte initializer from 79.48795% to 98.79518% in all three debug
releases. Its remaining difference is two extra loop-alignment NOPs.

Conversely, original portal scans inline `xBaseIsValid`. Supplying its existing
`zScene.cpp` definition locally under PS2 removes the repeated external calls.
The pause-task name carry increments the preceding digit before resetting the
current digit, matching the original load/store order. The complete 3,408-byte
portal initializer becomes exact in all three debug releases.

Each debug unit gains **3,408 exact bytes and one function**, reaching 22/28
functions and 9,712 bytes. USA unit fuzzy matching reaches 99.2933%; Europe
and Germany reach 99.293686%. Every other function score remains unchanged,
as does France's complete three-function report. The full GameCube USA report
remains identical and its retail DOL SHA-1 passes. Private reports use the
`*-inline.json` suffix.

## Manager setup and menu INI names

Declaring the count before the allocation size under PS2 restores the setup
function's saved-register assignment. Separating the menu-name digit increment
from its comparison and carrying the preceding digit before resetting the
current one restores the original parser loads and stores. The 416-byte setup
and 424-byte parser become exact in all three debug releases: **840 bytes and
two functions** each. Units reach 24/28 functions and 10,552 exact bytes.

France's existing setup body also becomes exact, adding **416 bytes and one
function** to its measured subset (2/3 functions, 584/756 bytes). The parser is
not present in that subset. No other function score decreases. GameCube keeps
its original local declaration order; its full report is identical and the
retail DOL SHA-1 passes. Private reports use `*-small.json`.

## Face-button event dispatch

The original PS2 update sends event 0x42 (Triangle) for mask 0x40000 and event
0x40 (Square) for mask 0x80000. Restoring those two masks locally under PS2
matches the complete 640-byte update in all three debug releases. Shared pad
definitions remain unchanged: this establishes UI dispatch, not a global
physical-button mapping. Each debug unit reaches 25/28 functions and 11,192
exact bytes. France's subset does not contain this update body and is unchanged.

All other function scores remain unchanged; the full GameCube USA report is
identical and its retail DOL SHA-1 passes. Private reports use `*-buttons.json`.
