# PS2 credits texture dimensions

The textured-quad path in `xCMrender` now uses the original regional screen
dimensions: USA 640 by 448, and Europe/Germany 512 by 512. Its previous shared
640 by 480 dimensions came from GameCube. These PS2 dimensions also agree with
the existing regional font-renderer constants.

The original USA body loads float 640 (`0x44200000`) and 448 (`0x43e00000`)
before scaling the texture corners. Both PAL originals load 512 (`0x44000000`)
and reuse that value for both axes. The source now reproduces those constants
and the PAL shared-scale calculation. GameCube retains its existing regional
dimensions and Xbox retains 640 by 480.

| Version | Render fuzzy before | Render fuzzy after | Unit fuzzy after |
| --- | ---: | ---: | ---: |
| SLUS-20680 | 86.594986% | 86.595795% | 87.8942% |
| SLES-51968 | 85.72955% | 86.57409% | 87.87663% |
| SLES-51970 | 85.72955% | 86.57409% | 87.87663% |

Every other function score is unchanged. Exact totals remain four functions
and 204 bytes per region; the larger render routine still has substantial
register and stack-layout differences. The existing France configuration has
no enabled credits source profile, so this change makes no France matching
claim. No target, registry, profile or compiler setting changes.

The full GameCube USA build retains its complete progress report and passes
the retail DOL SHA-1 check. Private regional evidence is
`build/credits-oct09/*-{before,dimensions}.json`. Separate unsuccessful source
probes of the credits state toggle and camera helpers were discarded.
