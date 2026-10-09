# PS2 collision SDK foundation and whole-source comparison

The complete `xCollide.cpp` now builds with the published PS2 compiler profile.
It retains all 36 original functions / 36,648 bytes, including the larger partial
functions. SLUS-20680, SLES-51968, and SLES-51970 agree at 53.607944% fuzzy match,
with eight functions / 2,236 code-matched bytes. No full-TU completion or retail
executable source link is claimed. France requires independent function mapping.

## Complete declarations and their evidence

The existing repository RenderWare collision headers are reused directly.
`rpcollis.h` selects the PS2 geometry headers on PS2, while its full API and
collision declarations remain shared. `rpcollbsptree.h` and the geometry accessor
macros are unchanged. This avoids importing GameCube's platform header or creating
partial structures to reach specific offsets.

The SDK declaration provenance is the repository's RenderWare 3.5 interface:
`rwplcore.h` records library version 0x35000; the private BSP declarations were
introduced in commit 68113e6f3 and subsequently made C-compatible in a817f0a4e.
The original PS2 BSP types corroborate this interface. This is bounded evidence
for the declarations used here, not a claim that every GameCube SDK layout is
interchangeable with PS2.

All three authenticated originals agree with actual whole-source compiler DWARF
on every direct member offset and complete size of:

| Type | Size |
| --- | ---: |
| RwLine | 24 |
| RwSplitBits | 4 |
| RpTriangle | 8 |
| RpPolygon | 8 |
| RpWorldSector | 144 |
| RpV3dGradient | 24 |
| RpCollBSPBranchNode | 16 |
| RpCollBSPLeafNode | 4 |
| RpCollBSPTree | 16 |
| RpIntersectData | 24 |
| RpIntersection | 28 |
| RpCollisionTriangle | 40 |
| xSweptSphere | 336 |

The existing SDK's complete `RpCollisionData` declaration is reused, not
reconstructed from selected fields. Its flags/tree/count/map arrangement is used
by the existing `src/rwsdk/plugin/collis/rpcollis.c` allocation and stream routines.
The PS2 original independently corroborates the fields consumed by this TU:
`xSweptSphereToModel` loads the geometry-plugin pointer through the named offset,
then its BSP tree at +4; `SweptSphereModelCB` loads the triangle map at +12. The
latter complete 260-byte callback reproduces original bytes after relocation.
There is no concrete `RpCollisionData` DWARF name in the game units; unused members
are supported by the complete SDK declaration, not claimed as DWARF-verified.

Eleven `iCollide` declarations have matching original linkage signatures in all
three debug executables. The portable vector macros and atomic/geometry accessors
retain the established SDK expressions. `xClumpColl.h` avoids its unused
intersection-utility include on PS2, and `xCollide.cpp` explicitly includes the
existing complete JSP declarations previously supplied transitively on GameCube.

## Original PS2 normalization

In the US original `xSweptSphereToTriangle`, 0x1c8e40..0x1c8eb0 takes the square
root of the squared normal length, divides one by that result only when positive,
and scales the normal. It proceeds directly to the plane-dot calculations.
The PS2 source now expresses those operations with ordinary `sqrtf` and a positive
check. The original GameCube inverse-square-root and NaN guard are preserved.
This is an observed platform behavior difference, not a new math helper or an
assembly workaround.

## Validation

All 224 actual GameCube game/engine source compilations preserve every allocated
section in order, including duplicate section names. Normal and debug PS2
compilations of the complete collision source also have identical allocated bytes.
The existing 45-source-unit PS2 report preserves every one of its 5,391 complete
function records. Newer motion/grid consumers are integrated separately by the
root multi-version CI run; this private regression snapshot predates their profile
addition.

Independent application of actual source relocations proves seven functions /
1,744 bytes exactly in each original. The normally code-matched
`xSweptSphereToEnv` (492 bytes) retains an unresolved SDK-call identity, so it is
not claimed as a raw linked-byte match. Standard report metrics remain unchanged.

Private reproduction evidence is under `build/ps2collision178`: complete compiler
inventories/commands; `layouts.json` and `api-proof.json`; per-unit `profile.json`,
`all-region-summary.json`, `compiled-layouts.json`, and `raw-proof.json`; the
normalization instruction window in `triangle-retail.txt`; and the full regression
and GameCube section comparisons. The type registry is reproducible with
`tools/platforms/ps2_type_layouts.py` against authenticated originals.

## Platform ray passes and animated collision storage (2026-10-09)

The original PS2 iRayHitsEnv does not perform the backward world-intersection
pass present in the GameCube source. Removing that pass only from the PS2
translation unit restores the complete 720-byte function in all three debug
releases. The forward JSP/world paths, optional collision world and min_t
adjustment remain. The original model-ray function does retain its backward
atomic pass; its vector copies now use the existing typed xVec3Copy helper.

Both original model-ray and model-sphere routines inline animation-collision
apply/restore logic. Bounded private helpers in this platform file restore
the flag test, xModelAnimCollRefresh boundary and vertex-pointer exchange.
They use xModel's existing external anim_coll_old_mt rather than the separate
static reconstruction in xCollide.cpp. All three original iCollide, xCollide
and xModel DWARF records refer to one externally named 28-byte RpMorphTarget:
USA 0x568bd0, PAL 0x5686d0 and Germany 0x5680d0. The verts member is at offset
20. Original instruction replay proves the saved-pointer store and restore
load in both affected functions, as well as the independently named refresh
call. RpAtomic.geometry=24, RpGeometry.morphTarget=92 and the relevant
xModelInstance fields also agree with the current complete declarations.
No shared declarations or shared implementation are changed.

All three full 21-function / 10,532-byte iCollide units improve from
94.01481% to 97.69807% fuzzy. Exact coverage increases from 3,168 bytes /
14 functions to 3,888 bytes / 15 functions. iRayHitsEnv improves from
73.54444% to 100%; iRayHitsModel1152 from 84.416664% to 94.36806%; and
iSphereHitsModel3 1588 from 94.73048% to 99.94459%. Other function records
and data measures are unchanged. The remaining model-sphere source mismatch
is an equivalent subtraction operand/load allocation; the model-ray residual
includes vector temporary lifetime differences.

The environment-ray raw proof reproduces 712 of 720 bytes after replaying
originally named function, callback and global relocations. Two world-SDK
call words remain unresolved and are excluded from raw equality. No runtime
aliases or original metadata are added. France is outside the current
platform iCollide source profile; no new French identity or coverage is
inferred. GameCube selects its separate gc/iCollide.cpp, and this PS2-only
source file is absent from the Xbox production profile.

Private evidence is build/icollide-ray-final-comparison.json,
build/icollide-ray-final-changes.json, build/icollide-ray-raw-proof.json and
build/icollide-anim-global-proof.json in the PS2 worktree. The baseline
snapshot is cdab1fcf7. Compiler settings, profiles and scoring are unchanged.

## Sphere-triangle distance lifetimes (2026-10-09)

properSphereIsectTri now computes the complete plane-distance difference in
its original dist2plane local, keeps radius2 as the squared radius before
the edge loop, and uses the original scalar sqrt.s boundary for the final
distance. The previous reconstruction kept both plane dot products alive
across the triangle test and recomputed the squared radius inside the loop.
Original DWARF confirms the float locals and their shared register lifetimes:
dist2plane/dist2 use f21, while dist/radius2 use f20 in all three releases.

All three complete iCollide source units gain this entire 552-byte function,
which improves from 88.108696% to 100%. Whole-unit fuzzy matching increases
from 97.69807% to 98.321304%, with exact coverage rising from 3,888 bytes /
15 functions to 4,440 bytes / 16 functions. All other function records and
data measures are unchanged. Independent original relocation replay proves
all 552 bytes exactly, including the square-root word and both named calls
(PointWithinTriangle and FindNearestPointOnLine), without unresolved runtime
operands in this function. The platform scope remains debug PS2 only.

Private evidence is build/icollide-sphere-final-comparison.json,
build/icollide-sphere-final-changes.json, build/icollide-sphere-raw-proof.json
and build/icollide-sphere-locals-proof.json. The baseline is c73f98aa6.

## Floor-contact index lifetime (2026-10-09)

The original sphereHitsEnv3CB reads the active FLOOR index directly when
comparing the existing floor contact, then assigns idx after accepting the
replacement. Removing the premature idx copy and using FLOOR for those
comparisons restores the original byte loads and integer conversion
boundaries. Every successful path still assigns idx before using it.
The contact tests, replacement choice and output calculations are unchanged.

All three full source units gain the complete 1,804-byte callback, which
improves from 96.57206% to 100%. Full-unit fuzzy matching rises from
98.321304% to 98.90847%, and exact coverage from 4,440 bytes / 16 functions
to 6,244 bytes / 17 functions. Other function records and data measures are
unchanged. Original-DWARF function/global relocation replay reproduces all
1,804 bytes in each release, with no unresolved operands. The existing
PS2-only platform source scope is unchanged.

Evidence is build/icollide-floor-final-comparison.json,
build/icollide-floor-final-changes.json and build/icollide-floor-raw-proof.json.
The baseline is 678797b54.

## Environment contact reciprocal and triangle projection choice (2026-10-09)

Initializing the contact scale directly as 1.0f / c->dist restores the
original reciprocal lifetime in iSphereHitsEnv4. Conditional assignments for
the dominant triangle-projection dimension restore the original nested branch
joins in PointWithinTriangle. Both changes preserve the previous comparisons,
constants and arithmetic.

All three full source units gain iSphereHitsEnv4's 636 bytes, improving it
from 96.06918% to 100%. PointWithinTriangle912 improves from 97.34649% to
99.12281%. Whole-unit fuzzy matching rises from 98.90847% to 99.29966%, and
exact coverage from 6,244 bytes / 17 functions to 6,880 bytes / 18 functions.
Other function records and data measures are unchanged.

Independent relocation replay reproduces 624 of iSphereHitsEnv4's 636 bytes;
three unresolved runtime/SDK calls remain excluded from raw equality.
PointWithinTriangle reproduces 904 of 912 bytes directly. Only the order of
two independent instructions at offsets 4 and 8 differs: clearing v0 and
loading the normal's y component. No artificial scheduling operation or
compiler change is added for this residual.

Evidence is build/icollide-env4-final-comparison.json,
build/icollide-env4-final-changes.json, build/icollide-env4-raw-proof.json and
build/icollide-dimension-raw-proof.json. The baseline is c54b12b9e.


## Swept-sphere triangle vector transform (2026-10-09)

`xSweptSphereToTriangle` previously transformed its three vertices through
scalar matrix helpers. The USA, Europe and Germany originals instead share
an identical 216-byte inline VU kernel at function offsets 0x20 through 0xf8.
It loads the four inverse-basis rows, packs each unaligned 12-byte vertex,
transforms all three with full `xyzw` vector masks, and scatters nine result
floats into `xform[0..2]`. A PS2-local helper restores that kernel without
fixed GPR bindings or changes to shared matrix headers. Its descriptive name
is a reconstruction, not a recovered original symbol.

Original DWARF places `xform[4]` at stack offset zero in a 0xa0-byte frame,
followed by the edge/contact vectors. It also distinguishes the vertex
`distzsqr` input from the reused `testdist` output. Restoring those lifetimes,
reusing the named `invZ`, and preserving the edge/vertex `sqrt.s` inline
boundaries recovers most of the remaining scalar body. Normal length keeps
the built-in square root and uses the original conditional reciprocal join.
Using explicit square-root assembly for that normal instead was measured
and rejected because it changed the register and load schedule substantially.
The closing vertex uses the existing `xVec3Copy` helper. GameCube keeps its
previous transform, square-root, and temporary expressions.

Normal complete-unit builds for SLUS-20680, SLES-51968 and SLES-51970 improve
the 2,536-byte triangle body from 66.03155% to 98.33438%. All other 35 function
scores are unchanged. Exact coverage remains 10,528 bytes / 20 functions;
this is a fuzzy matching gain, not an exact-function gain. Fresh GameCube
solo validation retains 72 of 75 exact functions, including the triangle;
the existing box and two assignment residuals are unchanged. Full regional
production gates remain the integration check.

The three original kernel SHA-256 values are identical:
`dbd4692fb9eb18e43413257b3201db901860e7d3d1c58447bc845b01ff12d544`.
Independent raw-word checks verify all twelve vector operations, including
lane masks and operands, in each rebuilt region. Symbolic replay of integer
packing, vector multiply/add order and stores reproduces the same nine
output addresses and expressions. The complete bodies have 2,352 of 2,536
bytes directly equal, with no relocations. Remaining differences are GPR
allocation/instruction scheduling in the kernel and a few independent scalar
loads/stores or branch-adjacent operations. These are retained as residuals;
no compiler patch or compiler-version attribution is proposed.

Baseline: `3ac9a9a80`. Private evidence: `build/collide-oct09/triangle-proof.json`,
`triangle-locals.json`, `triangle-kernel-proof.json`, and
`triangle-raw-differences.json`. No registry or runtime aliases change.


## Parabola callback component lifetimes (2026-10-09)

`xParabolaEnvCB` originally uses a 0xb0-byte frame with seven saved general
registers and the named normal at stack offset 0xa0. The reconstructed helper
forms retained pointers to individual vertex and initial-position components
across `xVec3Normalize`, expanding the frame to 0xf0. PS2-local explicit
component copies, subtraction and plane-distance arithmetic remove those
unnecessary pointer lifetimes. The existing member dot operation expresses
the two parabola coefficients. No shared vector helper changes, register
bindings or padding are introduced; non-PS2 expressions are preserved.

The normal complete-unit checks for USA, Europe and Germany improve the
1,672-byte callback from 76.75598% to 88.61005%. The other 35 function scores,
including the preceding triangle gain, remain unchanged. Exact coverage
remains 10,528 bytes / 20 functions. The rebuilt callback has 1,652 bytes and
retains arithmetic load/register ordering and instruction scheduling residuals;
this is not an exact-byte claim. All three versions directly reproduce the
first 96 prologue bytes and the 0xb0 frame. The existing normalization call
and filter-global relocation remain at their original offsets 0x1e4 and 0x64.
Fresh GameCube solo validation retains the previous 72/75 exact result.

Direct versus free/member dot helpers, coefficient/plane-distance evaluation
order and component operand order were separately tested. Several equivalent
forms regressed; none establishes a compiler-version defect. The private
fast-probe snapshot was followed by normal builds of each complete regional
unit. Baseline: `3b6725643`. Evidence is
`build/collide-oct09/parabola-proof.json`, `parabola-locals.json`, and
`parabola-entry-proof.json`. Full regional production checks are deferred to
the integration gate, and no runtime identity or registry evidence changes.
