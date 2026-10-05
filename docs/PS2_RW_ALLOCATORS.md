# PS2 RenderWare allocator foundation

Four complete shared source files now compile with the normal PS2 compiler profile.
This restores the SDK allocator interface rather than indexing guessed callback slots
or constructing a prefix structure around the two used fields.

The new PS2 `rwplcore.h` carries the complete existing RenderWare 3.5 declarations
from `include/rwsdk/rwplcore.h`: `RwGlobals`, `RwDevice`, `RwMemoryFunctions`,
`RwFileFunctions`, `RwStringFunctions`, `RwFreeList`, and `RwMetrics`, with their
original enums and callback prototypes. Unused fields come from that established SDK;
they are not claimed to have independent BFBB PS2 DWARF definitions. The pointer-only
immediate-mode vertex uses the original PS2 `RwSky2DVertex` name.

The missing varargs type is supported by the archived Metrowerks PS2 `mw_stdarg.h`
(2001), which declares `va_list` as `char*` and describes its GCC 2.95-compatible ABI:
[pinned header](https://github.com/crowded-street/3s-decomp/blob/be9b9bc69dc19822a8eca9ce3e72ba560d5a3835/include/mw_stdarg.h).
Only that type is provided; this change does not implement varargs operations.

All three debug originals declare `ourGlobals` as `unsigned int[4096]`. Their spline
allocation/free instructions access this named storage directly at offsets `0x134`
and `0x138`. The complete SDK structure places `memoryFuncs.rwmalloc` and
`memoryFuncs.rwfree` at exactly those offsets. The PS2 binding therefore uses the
original storage address instead of GameCube's loaded `RwEngineInstance` pointer.
The whole-source raw checks below corroborate these callback accesses in multiple
consumers, including both allocation and release. No storage, padding, helper body,
or allocator implementation is fabricated.

The actual production compiler also compiled the real `xSpline.cpp` with private
compile-time assertions for every direct member offset and full size of the seven
SDK structures. All assertions passed, and the resulting ordered allocated sections
were identical to the normal whole-source object. The verified sizes are:

| SDK structure | Bytes |
| --- | ---: |
| RwGlobals | 344 |
| RwDevice | 56 |
| RwMemoryFunctions | 16 |
| RwFileFunctions | 44 |
| RwStringFunctions | 68 |
| RwFreeList | 36 |
| RwMetrics | 28 |

Source changes are limited to genuine platform dependencies: standard PS2 math/string
APIs, excluding the GameCube PowerPC reciprocal-square-root implementation, direct
culling dependencies, and the existing intersection API header using PS2 geometry
declarations. `xglobals` is declared in its own `xGlobals.h` so culling need not import
all of `zGlobals.h`. Clump collision's two normalizations use the original PS2
SQRT.S, positive-result reciprocal, and scaling sequence. Each sequence was checked
in all three originals; the GameCube implementation remains unchanged.

All original functions remain in their comparisons:

| Whole source | Functions | Original code | Code matched | Fuzzy score |
| --- | ---: | ---: | ---: | ---: |
| xSpline.cpp | 13 | 7,152 | 1,432 | 68.72483% |
| xIni.cpp | 5 | 2,116 | 72 | 49.88091% |
| xUpdateCull.cpp | 7 | 3,040 | 248 | 83.69342% |
| xClumpColl.cpp | 11 | 10,260 | 1,280 | 68.62768% |

The values are identical for SLUS-20680, SLES-51968, and SLES-51970: 36 original
functions / 22,568 compared bytes, with 11 code-matched functions / 3,032 bytes.
An independent application of actual source relocations to original named addresses
verifies 10 functions / 2,700 bytes byte-for-byte in each region. The remaining
normal code match is `AllocSpline3` (332 bytes), whose two `memcpy` call identities
remain unresolved. This is the existing normal objdiff code metric, not a raw or
whole-TU link claim. France is not enabled by these debug-only profiles.

The normal/debug builds have identical allocated sections, and their emitted
DWARF matches the full sizes and every direct member offset of 22 relevant game and
geometry aggregates in each original. All 224 GameCube source objects were rebuilt;
every ordered allocated section remains identical to the integration baseline.

Private verification artifacts are under `build/ps2allocator184/`: the four complete
source profiles, all-region reports, independently applied relocation proofs, SDK
layout assertions/compiler output, original normalization words, aggregate layouts,
and GameCube comparison. Original allocator anchors are also recorded in the root
`build/spline176/original-allocator-evidence.json`.

The 51 already-enabled PS2 source TUs were rebuilt with these headers. Against the
frozen `a6b33530a` integration report, all 5,317 comparable complete function records
are unchanged. Four newer integration profiles absent from this worktree's profile
snapshot (camera, scene, combo, and hangable) are explicitly excluded from that local
comparison; the parent integration runs the final current-profile build.
