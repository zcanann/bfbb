# PS2 Boss Patrick source comparison

The complete existing `zNPCTypeBossPatrick.cpp` compiles with the established PS2 b38 profile after adding a PS2-only include of the existing `rwim3d.h`. This supplies the render-state enum/API declarations used by the real freeze-breath renderer. There are no function-body, shared-header, layout or compiler-flag changes.

All three debug originals own 63 functions / 41,132 bytes. The complete per-TU comparisons produce 35 standard code matches / 8,300 bytes and an 81.60391% fuzzy code score in SLUS-20680, SLES-51968 and SLES-51970. The independently generated relocation records agree across these versions, permitting one profile with three authenticated executable hashes. They restore 475 known direct call/tail-jump operands and 40 GP operands; 25 unmodeled references remain unmodified.

Applying actual source relocations against independently named original function/data addresses reconstructs 28 functions / 4,028 bytes exactly in every version. Seven further standard code matches retain unresolved references: AnimPick, the Hit/Run/Taunt Enter methods, RenderExtra, the animation table and playSplat. No function with completely resolved references has a raw mismatch. Callback spelling reused in other TUs is disambiguated by original TU ownership and actual source definition/local binding; no new global symbol alias is introduced.

All 250 shared aggregate variants match every original, including direct bitfield type/storage, bit offset and width, with the already justified wrapper normalizations retained. Normal/debug PS2 compilation produces identical ordered allocated sections. An actual GameCube rebuild preserves all 19 allocated sections; only nonallocated debugging information differs.

Private evidence under `build/npc228/zNPCTypeBossPatrick` includes complete regional reports, original relocation records, `profiles.json`, `validation.json`, `raw-proof.json`, layout inventories, compile commands and the GC identity comparison. All 63 original members remain in the partial source profile. No executable-link completion or data-layout match is claimed.
