# PS2 frame alignment, particle tanks, and pickups

The PS2 RenderWare matrix needs 16-byte alignment. This is supported by actual
original embedding, not inferred from a desired match: all three debug originals
contain `iFXshadow.cpp::rpAtomicPS2AllLightData`, size 96, with a surface pointer
at 0, `RwMatrixTag invMat` at 16, and the two scale floats at 80/84. The complete
original matrix is 64 bytes. The compiler's ordinary alignment attribute
reproduces that complete layout without new padding members.

`RwFrame` reuses the complete established declaration in
`include/rwsdk/rwcore.h`: object at 0, dirty-list link at 8, modelling matrix at
16, local-to-world matrix at 80, object list at 144, and child/next/root pointers
at 152/156/160. Matrix alignment naturally gives the original 176-byte frame.
All field offsets and full size match the original debug records in each region.

An independent retail consumer verifies the matrix's effect inside the complete
SDK particle-tank declarations. Original `create_ptank` reads the plugin pointer
through `_rpPTankAtomicDataOffset` and stores 1 at offset `0xb4`. With aligned
matrices, the SDK's `RpPTankAtomicExtPrv.publicData.vertexAlphaBlend` is exactly
at `0xb4` (formerly `0xac`). The USA store is at `0x3b0660`, instruction
`0xac6400b4`; the same access is present in both other debug originals. A private
whole-TU debug compilation checks this offset, frame size, and every member of
the original light-data embedding, while preserving all allocated source bytes.

The additional declarations are existing SDK matrix classifications,
`RwMatrixScale`, `RwV3dTransformPoints`, `RpAtomicSetFrame`, and
`RpMaterialSetTexture`, the SDK `RpAtomicRender` callback macro, and the standard
`qsort` declaration using the already-established PS2 `size_t`. Particle-tank
source selects the existing complete SDK header on PS2. No function body in
`xPtankPool.cpp` or `zEntPickup.cpp` changes.

Both actual complete source files compile with the published production profile.
All original functions, including nonmatches, remain in the normal comparison:

| Whole source | Original functions | Original bytes | Matched functions | Matched code | Fuzzy |
| --- | ---: | ---: | ---: | ---: | ---: |
| xPtankPool.cpp | 9 | 3,208 | 2 | 408 | 75.91147% |
| zEntPickup.cpp | 32 | 22,380 | 11 | 2,780 | 76.65201% |

These measures are identical for SLUS-20680, SLES-51968, and SLES-51970. This
adds 25,588 bytes of source comparison and 3,188 matched code bytes. France is
not enabled without independent function metadata. Neither whole-TU completion
nor an executable link is claimed.

Independent application of actual source relocations to original named addresses
proves 11 functions / 2,200 bytes exactly in each region. The normal code matches
for `create_ptank` (216 bytes) still have four unresolved SDK callees;
`zEntPickup_GivePickup` (772 bytes) has an unresolved anonymous data relocation.
Those normal objdiff matches are not presented as raw linked-byte proof.

Seventeen emitted aggregate records across the two actual debug source objects
match every original size/direct member offset in all three debug originals.
Debug and ordinary source objects have identical ordered allocated sections.
All 224 GameCube source objects were rebuilt with identical ordered allocated
sections; the PS2 header selection does not alter the GC source path.

Private evidence is under `build/ps2ptank192/`: original matrix/frame and
light-data records, original particle-tank access windows, complete SDK offset
assertions, original/compiler layout registries, all-region reports, independent
relocation proofs, candidate profiles, and the GC comparison. The source build
for the larger pickup TU uses the inventory tool's 600-second timeout; its
compiler flags are unchanged.

The frozen `13b479420` production profile was rebuilt with these headers. All 66
existing source TUs compile, and all 5,391 complete function records are identical
to the verified `platform194` CI report. The existing 55,992 matched bytes / 457
functions remain unchanged before enabling the two candidate profiles.
