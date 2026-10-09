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
