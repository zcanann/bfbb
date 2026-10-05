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
