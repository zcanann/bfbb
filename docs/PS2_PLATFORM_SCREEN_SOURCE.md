# PS2 platform screen effects

The PS2-specific `iScrFX.cpp` restores seven original routines using the existing
Sky2 SDK vertex macros and typed storage. The complete new source file compiles
with the pinned compiler and production flags for USA, Europe, and Germany.
All seven routines report 100% in each version, adding 1,356 code-matched bytes:

| Function | Bytes | Independently replayed raw bytes |
| --- | ---: | ---: |
| `iScrFxDrawBox` | 924 | 920 |
| `iScrFxBegin` | 116 | 84 |
| `iScrFxEnd` | 104 | 76 |
| `iCameraSetBlurriness` | 128 | 128 |
| `iCameraMotionBlurActivate` | 8 | 8 |
| `iScrFxInit` | 8 | 8 |
| `iScrFxCameraDestroyed` | 68 | 64 |

The profile retains all thirteen original functions / 5,788 bytes. The six bodies
not yet implemented remain at zero; they are not replaced by stubs or excluded
from comparison. This is 23.427782% of the complete original unit. The profile
records independently named original calls and twelve typed GP references per
version. All prior profiles remain unchanged, with no new target identities or
known-code denominator increase. France currently has no recovered identities
for this unit and receives no speculative profile.

`DrawBox` uses a static array of four 64-byte Sky2 vertices and a static index
array containing 0, 1, 2, 3. The original only sets screen X/Y and float colors;
it leaves the statically initialized depth fields alone. Four ordinary SDK color
setter invocations reproduce the original unsigned-to-float conversion control
flow exactly. This differs from the separate GameCube implementation, which
uses stack vertices and explicitly sets depth. The 304-byte motion-blur state
has its vertex array at 16, indices at 272, and dimensions at 284/288, with native
SDK vertex alignment. No invented padding or register-pressure scaffolding is
used. All four original aggregate layouts and the compiled storage sizes agree
in every debug region; the compiled eight-byte index data matches each original.

Raw verification applies the actual object's call, GP, and HI16/LO16 relocations
using original DWARF function/data identities. Function-local vertex/index names
are resolved through their unique original owner and declaration, not numeric
compiler suffixes. Every arithmetic/control-flow word then agrees. The remaining
17 words per version are unnamed SDK transfer destinations: fifteen render-state
calls, one indexed-draw tail transfer, and one raster-destroy call. Source SDK
spellings use existing declarations; no original runtime alias is inferred from
them. Complete runtime-link closure is not claimed, and the normal exporter
checks remain unchanged.

Only the new PS2 source file and PS2 profile are added. GameCube and Xbox keep
their existing platform implementation and headers. The combined production
original/target/source gate remains the integration check.

Private evidence in the isolated worktree: `build/iscrfx-originals.py` / `.txt`,
`iscrfx-original-profile-proof.json`, `iscrfx-first-comparison.json`,
`iscrfx-first-raw-proof.json`, `iscrfx-first-layout-proof.json`, and the actual
per-version objects, compiler logs, and reports in `iscrfx-pilot/`. The source
baseline is `7cbc85884` plus the two independent memory-card follow-ups
(replayed as `703f1290e` / `0dc8f0fe4`).
