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
per-version objects, compiler logs, and reports frozen in `iscrfx-first/`. The source
baseline is `7cbc85884` plus the two independent memory-card follow-ups
(replayed as `703f1290e` / `0dc8f0fe4`).

## Motion-blur overlay and lifecycle

The next source restoration adds the typed overlay renderer and the two camera
lifecycle wrappers. Both 84-byte wrappers become exact in all three debug
versions, increasing the unit to 1,524 code-matched bytes / nine functions.
The 1,144-byte overlay reports 99.65035%, raising whole-unit fuzzy matching from
23.427782% to 46.02626%. Every previously compared function and data result is
unchanged. The remaining three original bodies still have no source implementation.

The overlay sets all four Sky2 float colors, preserves the original render-state
sequence, and submits four vertices with six indices. Original DWARF records a
16-byte `RwRect` local with x/y/w/h signed fields at 0/4/8/12. Its otherwise unused
zero initializer is present in every original and is therefore retained. The
compiler emits one additional NOP in that initializer loop: the source function
is 1,148 bytes, with corresponding forward/backward branch displacement changes.
A private original-aware replay proves this precise residual while keeping all
sixteen unnamed SDK calls unresolved. Production comparison and export rules
are not altered, and the function remains non-exact.

`iScrFxCameraCreated` replays all 84 original bytes. `iScrFxCameraEndScene`
replays 80 / 84; its anonymous GP-relative initial white color remains unresolved.
The inline rendering wrapper preserves the original front-buffer guard and its
separate call to the static overlay function. No new target/profile/header
changes are involved in this follow-up. The shared SDK declarations are used
as before, with the original `RwRect` declaration local to this PS2 source file.

Private evidence: `iscrfx-overlay-comparison.json`, `iscrfx-overlay-changes.json`,
`iscrfx-overlay-raw-proof.json`, `iscrfx-overlay-raw-residual.json`, and
`iscrfx-overlay-layout-proof.json`, with frozen objects/reports in
`build/iscrfx-overlay/`. The source parent is `a98c1efb8`.

## Motion-blur allocation and vertex setup

The next restoration implements the original 272-byte allocation routine and
its static 452-byte vertex setup helper. Both functions retain their original
sizes. In USA, PAL, and Germany, their normal report scores are 97.05882% and
94.95575%, respectively. The complete thirteen-function / 5,788-byte unit rises
from 46.02626% to 58.002766% fuzzy matching; its prior 1,524 exact bytes / nine
functions and all earlier function/data results remain unchanged. Only the
2,396-byte distortion renderer remains unimplemented. Neither new body is
claimed exact, and no comparison/export rules or target metadata change.

Allocation preserves the original empty-rectangle initialization, rejects
nonpositive framebuffer dimensions, creates an unallocated camera-texture
raster, attaches the framebuffer subrectangle, destroys a failed attachment,
and reports the original `Error creating raster\n` diagnostic on allocation
failure. The 23-byte null-terminated string and its original HI16/LO16 operands
were checked separately in all three originals. `RwRasterSubRaster` is declared
locally with its SDK interface; that spelling does not establish an original
runtime identity. The four external destinations remain unresolved. The named
call to the static setup helper uses its original DWARF identity. A raw replay
checks every non-NOP instruction, branch destination, and delay slot, with only
the original zero-fill/guard NOP placement differing. This is diagnostic
evidence, not a relaxation of the normal score.

Setup uses the original 512/1024 width selection, half-texel UV offsets, camera
near-plane depth, reciprocal depth, and four white Sky2 vertices. Original
DWARF names the horizontal/vertical position and UV steps. A one-cell grid
construction retains the original explicit zero/one products and four-corner
ordering; the precise source loop syntax is a reconstruction. Direct literal
corner expressions were also tested, but this compiler folds away operations
present in every original. An independent instruction-level symbolic replay
executes both width branches of the actual original and compiled functions,
with ABI caller clobbers at the single unresolved SDK call. All 44 ordered
vertex writes have identical floating-point expression trees, without
reassociation, and both sides read camera near-plane exactly once. This checks
the reconstructed arithmetic and side effects separately from fuzzy scoring.
Remaining differences concern UV temporary registers/scheduling and two
constant GPR assignments. No compiler-version explanation or patch is claimed.

All changes are confined to the PS2 source and this document. There is no
French identity/profile expansion, shared-header change, or GameCube/Xbox
source selection change. Private evidence: `iscrfx-motion-comparison.json`,
`iscrfx-motion-changes.json`, `iscrfx-motion-open-raw-proof.py` / `.json`, and
`iscrfx-motion-vertex-flow-proof.py` / `.json`; actual whole-source objects,
compiler logs and reports are frozen in `build/iscrfx-motion/<version>/`.
The source parent is `1d88117c1`.

## Distortion renderer

The remaining 2,396-byte distortion body is now restored. All three debug
versions report 99.926544% for the function and 99.36835% for the complete
thirteen-function / 5,788-byte source unit. Every earlier function result stays
unchanged, including the 1,524 exact bytes / nine functions. The source now
implements all thirteen original bodies; this does not make the remaining
fuzzy bodies exact or establish their unresolved SDK linkage.

The renderer reconstructs the original camera-facing particle corners, six
position-only immediate vertices, matrix multiplication and transformed-point
paths, four depth rejection branches, perspective divisions, and first-particle
diagnostic prints. The SDK vector macros operate through their `RwV3d` casts;
these preserve the original aliasing and otherwise-dead perspective stores.
Explicit component copies from each temporary matrix position preserve the
original loads and local lifetimes. No shared vector or SDK header is changed.
The matrix multiplication declaration is local, and its original destination
remains unresolved rather than being named from the source compiler output.

The original routine is unfinished in observable ways: it does not increment
the immediate-buffer position or write vertex UV/color fields, and its final
flush refers to the static buffer while the loop uses a local buffer of the
same name. Those behaviors are retained. The original diagnostic format is
`% 6.3f % 6.3f % 6.3f\n`; both original references and the compiled string were
checked. The original identity-matrix macro also preserves existing flag bits,
including its uninitialized local input. These are fidelity restorations, not
attempts to repair the retail renderer.

A raw replay of each actual compiled object applies only original DWARF-backed
call, GP and HI16/LO16 references plus the independently checked string.
2,264 / 2,396 bytes agree directly. Of the remaining 33 words, 23 are unresolved
SDK/runtime transfers; the other ten are precisely four accesses to the local
matrix-pointer stack slot (original 0xf0, source 0xfc) and two three-instruction
subtractions with swapped float temporary registers. All other arithmetic,
branches, delay slots, memory accesses and instruction positions agree. The
normal score and exporter continue to retain these differences. Equivalent
matrix-local/declaration forms did not remove the residual, and direct/SDK
macro subtraction variants regressed matching; no compiler patch is proposed.

All three originals independently confirm the six used aggregate layouts:
`xVec3` / `RwV3d` (12), `DistortionParticle` (48), `RwMatrixTag` (64),
`tagiRenderInput` (128), and `RxObjSpace3DVertex` (36), including their member
offsets. The static pointer and count occupy four bytes each in the original
load segment's zero-filled memory region. The existing platform declarations
agree; no guessed padding is introduced. France remains outside this unit's
proven profile, and GameCube/Xbox source selection stays unchanged.

Private evidence: `iscrfx-distortion-comparison.json`,
`iscrfx-distortion-changes.json`, `iscrfx-distortion-raw-proof.py` / `.json`,
`iscrfx-distortion-layout-proof.py` / `.json`, and frozen whole-source objects,
compiler logs and reports in `build/iscrfx-distortion/<version>/`. The source
parent is `fffa3020d`.
