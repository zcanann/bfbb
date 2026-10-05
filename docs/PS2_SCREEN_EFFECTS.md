# PS2 screen-effects renderer

The complete existing `xScrFx.cpp` now compiles using the normal PS2 profile.
No function body changes are needed. All original functions stay in the comparison:

| Original | Functions | Original bytes | Normal matched code | Fuzzy score |
| --- | ---: | ---: | ---: | ---: |
| USA | 22 | 7,648 | 16 / 3,416 | 78.41841% |
| Europe | 22 | 7,640 | 16 / 3,416 | 78.17068% |
| German | 22 | 7,640 | 16 / 3,416 | 78.17068% |

The profiles are independently gated by executable SHA1 and record each original's
actual call offsets. The regional eight-byte code-size difference is retained.
France is not enabled here, and no complete source link is claimed.

## Original SDK vertex declaration

The new PS2 `rwim2d.h` uses the complete Sky2 vertex declarations and ordinary
setter macros from the [RenderWare 3.5 PS2 header](https://github.com/sigmaco/rwsdk-v3.5-ps2/blob/82a98fe97a6346388e27e0c73068fc326109de68/rwsdk/include/sky2/rwplcore.h).
This is the platform's actual float-color vertex, not the GameCube byte-color type.
`RwSky2DVertexFields` has screen position at 0, camera depth at 12, UV at 16/20,
reciprocal depth at 24, color at 32, and object normal at 48. Its named `pad1` and
`pad2` fields at 28/60 are present in the SDK and every original debug definition;
they are not invented padding. The complete vertex occupies 64 bytes.

The alignment overlay contains those fields and four native unsigned 128-bit
quadwords. The SDK calls this scalar `u_long128`; the header spells the actual
compiler type `unsigned __int128`. The compiled whole-source DWARF array descriptor,
including bounds 0..3 and unsigned-128 fundamental type, is byte-identical to all
three original descriptors. No synthetic alignment member or array substitute is
used. Screen position, UV, camera depth and RGBA setters retain SDK semantics.

`RwVideoMode` and the video-mode APIs also come from the same SDK. Its complete
24-byte layout matches all originals. Eleven emitted aggregate definitions,
including both vertex layers, camera, glare and distortion state, match the full
original sizes and member offsets. The ordinary and debug whole-source objects
have identical ordered allocated sections.

## Validation and limits

All 224 GameCube source objects rebuild with identical allocated sections.
The full frozen `5adf9e717` profile regression preserves all 5,319 non-xFX
function records against the subsequently published `084c62546` report. A separate
full xFX rebuild preserves its 191 ordered allocated sections. After integrating
the authentic hazard-layout correction and Robot profile, the actual Robot and
RoboGoal source objects also preserve all 594 and 614 allocated sections,
respectively. The newly published Plankton TU was compiled as a header-regression
control only and preserves all 431 allocated sections too.
Independent application of actual source relocations to named original addresses
proves ten functions / 764 bytes exactly in every debug region. The other normal
matches still contain unresolved SDK calls or anonymous constants; normal objdiff
code matching is not a claim of raw linked-byte identity.

Private evidence is in `build/ps2render208`: the pinned SDK header, authenticated
original layouts, compiled layouts, `qwords-type-proof.json`, three complete
reports/profiles, independent raw relocation proofs, and GC comparisons.
