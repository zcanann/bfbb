# PS2 Prawn source comparison

The complete `zNPCTypePrawn.cpp` is compiled with the established PS2 compiler
profile. All 53 original functions / 25,544 bytes remain in the comparison for
USA, Europe and German originals. France is not enabled by this change.

## Platform renderer restoration

The television renderer now names the portable `RwIm2DVertex`. On PS2 this is the
complete 64-byte Sky vertex restored in [the screen-effects foundation](PS2_SCREEN_EFFECTS.md).
The original background allocates 384 bytes for six vertices, stores screen
coordinates at 0/4/8, UV at 16/20, reciprocal depth at 24, and float RGBA at
32/36/40/44. Its reciprocal depth is `1.0f / 0.3f`, matching the existing camera's
near plane; color channels are 255. The ordinary SDK setters express those actual
stores. The GC branch retains its original field assignments.

The original uses the camera-texture raster type, value 5. The source now names
`rwRASTERTYPECAMERATEXTURE` instead of combining two unrelated GC flag names
whose values happened to produce 5. GC-only texture flushes remain on GC; the PS2
original has no corresponding calls. Portable `xabs` uses the established native
float absolute-value implementation on each platform.

Actual original instruction bodies embed the vertex setter and the television
move, update and texture-assignment helpers in their callers. PS2-only inline
annotations restore these boundaries. No original spelling of the inline keyword
is claimed. Setter inlining closes the 352-byte background function; the other
three annotations improve closeup from 41.217052% to 73.04651%. Every other
function record is unchanged by these bounded inline controls. Whole-TU fuzzy
matching improves from 62.921234% to 65.06436%.

The added camera, texture, frame, world and immediate-mode declarations and enums
are ordinary existing RenderWare SDK interfaces, with no structure layout changes.
`RpGeometryLock` and `RpGeometryUnlock` also use the canonical SDK prototypes in
`include/rwsdk/rpworld.h`, enabling the following geometry consumers. These two
functions have no named declaration records in the three original DWARF tables;
their signatures are supported by the SDK, not an invented debug record.

## Validation

USA, Europe and German each report 23 matching functions / 6,800 matching bytes,
with 65.06436% whole-TU fuzzy matching. All 53 functions remain selected, including
holdouts. Independent application of actual source relocations to named original
addresses proves 17 functions / 1,784 bytes exactly in each original. The other
normal code matches retain unresolved SDK references or anonymous constants;
normal objdiff matching is not a raw linked-byte identity claim.

All 208 shared aggregate names emitted by the actual debug compilation match
every original size/member-offset variant in each region. The normal and debug
whole-source objects have identical ordered allocated sections.

All 224 actual GC source objects compile with identical ordered allocated sections.
The final Prawn-only control also preserves its complete allocated-section sequence.
The source does not claim a complete PS2 source-linked executable or TU completion.
Private reproducible evidence is under `build/ps2prawn214`: compiler commands,
all three reports and profiles, original call/vertex evidence, full emitted type
comparisons, independent raw relocation proofs, and GC object comparisons.
