# PS2 LaserBolt and KingJelly regional timing

The existing GameCube PAL timing branches in `xLaserBolt.cpp` and `zNPCTypeKingJelly.cpp` also apply to the independently verified PS2 PAL and German originals. They now accept `VERSION_SLES_51968` and `VERSION_SLES_51970`, supplied by the authenticated version resolver. No GameCube macro aliases, new compiler flags or French timing assumption are introduced.

The original instruction evidence covers every affected consumer, not just one representative call:

- LaserBolt `emit` constructs `1/60` at function offsets 712/716, 760/764 and 808/812 in USA, versus `1/50` at the same offsets in PAL/German. `reset` does the same at 256/260, 296/300 and 336/340.
- KingJelly `generate_thump_particles` constructs the frame time at USA 292/296, PAL/German 284/288. `generate_ring_particles` uses 428/436 in all three.
- KingJelly's reciprocal rates preserve the existing arithmetic expression. USA constructs `0x426fffff` (59.999996) in `generate_thump_particles` at 28/36 and `Reset` at 644/652 and 760/768. PAL/German instead load exactly 50.0 with a single LUI at 28, 640 and 752 respectively. Replacing the expression with a nominal 60.0 literal would be incorrect.

The frame-time bit patterns are `0x3c888889` for USA and `0x3ca3d70a` for PAL/German. Actual whole-TU compilation and complete original-member comparisons improve only the expected regional functions:

| PAL/German function | Previous fuzzy match | Correct timing |
| --- | ---: | ---: |
| LaserBolt emit | 77.73478% | 77.76087% |
| LaserBolt reset | 67.89349% | 67.92899% |
| KingJelly generate_thump_particles | 54.19% | 54.81% |
| KingJelly generate_ring_particles | 57.561226% | 57.57143% |
| KingJelly Reset | 58.390804% | 61.21839% |

LaserBolt's regional unit score rises from 52.836567% to 52.841045%; KingJelly rises from 70.1273% to 70.18376%. There are no newly exact functions or score regressions. All other allocated sections remain identical; the two reciprocal-rate consumers each shrink by eight source bytes. USA allocated bytes and complete unit reports are unchanged. Existing independently reconstructed exact matches remain 3 / 1,580 bytes for LaserBolt and 15 / 2,380 for KingJelly in every region.

Actual GameCube compiler controls preserve all ten LaserBolt and 21 KingJelly allocated sections. No type/header changes are involved. Existing full-unit partial profiles remain unchanged; this is not an executable-link completion claim.

Private evidence in `build/region233` includes the authenticated original timing operands, baseline and candidate commands/objects, full regional report comparisons, allocated-section changes, raw reconstruction and actual GC verification. The source changes depend only on the previously validated PS2 version-definition support.
