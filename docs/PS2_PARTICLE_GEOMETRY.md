# PS2 particle geometry and RenderWare types

The complete shared `xParEmitterType.cpp` now compiles against authenticated PS2 geometry declarations. All 19 original functions / 13,116 bytes are included in the comparison. Each debug-region original reports 50.89509% fuzzy code match, with no code-exact functions yet. This is a partial source placement and header-coverage improvement; it is not a complete unit or source-linked executable.

## Original types and APIs

Complete declarations for `RpAtomic` (112 bytes), `RpGeometry` (96), `RpMorphTarget` (28), `RpInterpolator` (20), `RwSphere` (16) and `RwTexCoords` (8) come from the original `xParEmitterType.cpp` DWARF. Pointer-only dependencies remain forward declarations; no prefix types or invented padding substitute for the real layouts. The existing material-list and object/frame types are reused. The retail `pad` member in `RpAtomic` is retained.

The actual whole-unit compiler debug output matches every direct member offset and aggregate size in all three original executables. The same verification covers `xPar`, `xParEmitterAsset` and `xParEmitter`, including their existing unions and embedded model tag. Debug information leaves all allocated section bytes unchanged.

The platform APIs are original canonical declarations:

- `iModelVertEval__FP8RpAtomicUiUiP11RwMatrixTagP5xVec3P5xVec3`, returning unsigned int.
- `iModelTagEval__FP8RpAtomicPC9xModelTagP11RwMatrixTagP5xVec3`, returning void.

PS2 emitter headers use the existing lightweight entity declarations and a forward declaration of the pointer-only `xParSys` dependency. GameCube keeps its original include context.

## Helper ownership and verification

Initial whole-unit compilation compared at 32.546814%. Original wrappers contain the work currently separated into the private `ocircle_emit`, `transform_ent_bone` and two `get_random_offset` helpers. Marking these anonymous helpers inline on PS2 restores their embedded ownership and raises the complete-unit comparison to 50.89509%. Bodies, arithmetic, calls and side effects are unchanged. All public function definitions remain present. Ambiguous original weak-helper names remain unresolved rather than being assigned guessed relocation identities.

All 224 shared/game GameCube translation units compile with identical allocated bytes, verified using the ordered list of every allocated section, including duplicate section names. Source comparison evidence is under private `build/ps2emitter172/`: original `layouts.json` and `api-proof.json`, actual full-source compiler commands, `compiled-layouts.json`, three-region reports and the GC comparison. Layout extraction is reproducible through `tools/platforms/ps2_type_layouts.py` with original source `SB/Core/x/xParEmitterType.cpp` and the types above. Original binaries and compiled objects are not committed.
