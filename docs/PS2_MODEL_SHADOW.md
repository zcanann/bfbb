# PS2 model and simple-shadow source comparison

The original PS2 `RwCamera` is a complete 400-byte structure, with
`RwObjectHasFrame` at 0, projection/update callbacks at 20/24/28, aligned
`viewMatrix` at 32, frame/z rasters at 96/100, view vectors at 104/112/120,
near/far/fog planes at 128/132/136, z scale/shift at 140/144, frustum planes
at 148, bounding box at 268, and corners at 292. This matches the complete
established RenderWare SDK declaration in `include/rwsdk/rwcore.h` and all
three debug originals. The previously established matrix alignment naturally
supplies its size; no padding members are added.

The complete SDK `RwPlane` (16), `RwFrustumPlane` (20), and
`RxRenderStateVector` (44) declarations and their ordinary enums are restored.
Every rendering-state member is independently present in original shadow DWARF:
Flags0, ShadeMode4, SrcBlend8, DestBlend12, TextureRaster16, AddressModeU20,
AddressModeV24, FilterMode28, BorderColor32, FogType36, and FogColor40.

The additional functions/macros are existing SDK camera/raster/frame APIs,
world camera membership, the three camera accessors, render-state loading,
and immediate-render transform flags. `iModelInit`, `iModelSetMaterialAlpha`,
`iModelVertCount`, and `iModelMaterialMul` reuse the established platform API;
all three originals confirm their canonical parameter signatures. The four game API
prototypes are declarations, not substitute implementations.

`xModel.cpp` explicitly includes the PS2 engine globals header used by the SDK
current-camera accessor. `xShadowSimple.cpp` selects the existing complete SDK
collision/immediate-render headers and directly includes `xJSP.h` for the
actual `colltree` access. Neither source file changes any function body. The
GameCube include context remains unchanged.

| Whole source | Original functions | Original bytes | Matched functions | Matched code | Fuzzy |
| --- | ---: | ---: | ---: | ---: | ---: |
| xModel.cpp | 21 | 6,056 | 11 | 1,860 | 86.701454% |
| xShadowSimple.cpp | 10 | 6,916 | 5 | 2,092 | 79.97224% |

These complete-TU comparisons retain every original function and all partial
matches. All measures agree in SLUS-20680, SLES-51968, and SLES-51970: 12,972
bytes of source comparison and 3,952 normal matched code bytes. There is no
new France profile or claim of complete-TU/executable linkage.

Independent application of actual source relocations to original named addresses
proves 11 functions / 2,784 bytes exactly per region. Normal matches with
remaining unnamed callees are `CameraDestroy` (140), `xModelEvalSingle` (412),
world camera enter/exit wrappers (8 each), and `xShadowSimple_SceneCollide`
(600). These are normal objdiff code matches, not raw linked-byte claims.

Seventeen consumed aggregate records match all original full sizes and direct
member offsets in each debug region. The actual full source debug objects also
have identical ordered allocated sections to their ordinary builds. All 224 GC
source objects were rebuilt with identical allocated sections; the final shadow
include addition was checked again with the same result.

Private evidence is under `build/ps2render198/`: original SDK/game type records,
canonical iModel signatures, actual whole-source compiler logs, per-region
reports and independent relocation proofs, compiler DWARF layout comparisons,
candidate profiles, and GC comparisons. The initial broader inventory also
recorded xFX's missing platform/plugin headers; that source is not enabled.

The full existing profile was rebuilt after merging `56aec6f0f`, including the
complete Robo source. All 70 existing TUs compile. The 5,162 non-Robo function
records are unchanged from verified `7d9c08d96` CI, and all 229 Robo records are
identical to its original per-region source report. Existing totals remain
69,392 matched bytes / 557 functions before enabling the two new profiles.
