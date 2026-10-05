# PS2 animation source and model types

The complete shared `xAnim.cpp` now compiles with the established PS2 compiler profile. The comparison includes all 34 original functions (17,656 bytes), including the remaining holdouts. Each of the three debug originals produces 77.96625% fuzzy code match and 17 code-exact functions totaling 3,032 bytes. This is partial source comparison, not a complete-unit or executable-link claim. France is not selected without separately reviewed function identities.

## Authentic dependencies

`xModelTypes.h` separates the existing `xModelPool` and `xModelInstance` declarations from animation, surface, lighting and bucket APIs. Their members are unchanged. Actual compiler debug output has the original 12-byte pool and 108-byte instance, with every direct member offset matching all three PS2 originals. Existing GameCube include context is retained by `xModel.h`.

The new PS2 `iAnim.h` exposes the original `iAnimInit`, `iAnimDuration`, `iAnimBoneCount`, `iAnimEval` and `iAnimBlend` signatures. `iModelAnimMatrices` is likewise recorded by original DWARF as `iModelAnimMatrices__FP8RpAtomicP5xQuatP5xVec3P11RwMatrixTag`. Original `xAnimFileEval` and `xAnimPlayEval` instructions independently use quaternion-to-translation offset 0x410, pose stride 0x720, and scratch-pose offsets 0xe40, 0x1560 and 0x1c80. These corroborate the existing 65-bone/five-pose scratch layout.

The retained `fprintf(stderr, ...)` diagnostic uses the PS2 newlib runtime. `reent.h` restores the complete original declarations rather than a partial prefix or guessed opaque storage: `tm` 36 bytes, `__sbuf` 8, `__sFILE` 88, `_atexit` 136, `_glue` 12 and `_reent` 752. Original DWARF records `_impure_ptr` and `_reent::_stderr` at offset 12. The header supplies no runtime replacement. Standard `rand`, `fprintf` and `atan2f` declarations complete the compile dependencies. PS2 uses the standard float math API; the existing GameCube double-wrapper spellings remain on GameCube.

## Verification

- Authenticated originals: SLUS-20680 `32e3b7dda09fd8d7fcba4eb769fa17c08cea05df`, SLES-51968 `0c3e685edc13a362d0d27a10dccb0b9698eb5597`, SLES-51970 `83bf81a139ea24fdb015f1419377de85b643828a`.
- All three reports agree on all function scores. Actual compiler debug output verifies model, animation and newlib aggregate sizes and direct member offsets against each original. Adding debug information changes no allocated section bytes.
- Independently applying actual source relocations to original named functions/data reproduces 16 functions / 2,940 bytes exactly in each original. The additional 92-byte code-exact `xAnimDefaultBeforeEnter` calls an unnamed `rand` runtime target; its call identity is not claimed as independently linked exact.
- All 224 shared/game GameCube translation units compile with identical ordered allocated-section names, sizes and bytes. Duplicate section names are retained in the comparison.

Private reproducible evidence is in `build/ps2model170/`: the initial seven-unit compiler inventory, original API/scratch witnesses, original layouts, and the `xAnim/` compile command, source object, three-region reports, compiled layout comparison and inverse-relocation proof. Original layout extraction uses `python -m platforms.ps2_type_layouts` with source `SB/Core/x/xAnim.cpp` and the named model, animation and newlib types. No original executable or compiled object is included in the repository.

## File-evaluation bone-count expression

Original xAnimFileEval keeps numBones as an unsigned32 local at stack offset
208. Expressing its one-bone/default-bone selection as one conditional assignment
recovers the original branch join. This retains the same tests, values and
subsequent decrement; no types, helper calls or mathematical expressions change.
Actual code differences are confined to the branch/NOP scheduling at offsets
240 through292. Every other allocated byte and all relocation destinations stay
identical; compiler-generated local label numbers are renumbered without changing
their section, offset, size, type or binding. Actual GameCube compilation preserves all seven allocated sections.

All four PS2 versions gain the 1,616-byte function as a normal code match. Their
full34-function /17,656-byte inventory now matches18 /4,648, fuzzy78.01155%,
with every other function record unchanged. All four independently compiled
whole-source objects are byte-identical. French comparison reuses its existing
verified full-unit extents and names; no boundary or profile is added.

Raw original reconstruction differs only at the unresolved floorf call at offset
468 in each debug original. This is documented as a standard code match, without
a raw-function or executable-link claim. Private evidence: build/near248/proof.json,
raw/raw-proof.json, gc/proof.json and candidate/<version>/report.json. No scoring,
compiler-flag, header or backend changes accompany the source simplification.
