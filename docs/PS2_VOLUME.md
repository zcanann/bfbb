# PS2 volume source comparison

The genuine `zVolume.cpp` translation unit now participates in all three debug-bearing PS2 comparisons. The profile retains all five original functions (2,068 bytes), with original DWARF linkage names, named calls, and GP data anchors. France has no new recovered identities in this change.

The original debug records describe `nvols` as an unqualified unsigned short, `gOccludeCount` as an unqualified signed integer, and `gOccludeList` as an array of ordinary pointers. The PS2 declarations follow those types while GameCube keeps its existing declarations. The local typed initialization helper is inline on PS2, consistent with the original unit having only the public initialization body and its direct calls to `xVolume::Init`.

The full original and compiled layouts of `zVolume`, `xVolume`, `xVolumeAsset`, and `PreCalcOcclude` agree in all three debug versions. A private debug compile also preserves every allocated section of the normal object.

All three standard objdiff comparisons report 128 matched code bytes in two functions: `zVolumeSetup` (100) and `zVolumeGetVolume` (28). The unit's fuzzy score rises from 48.061897 to 49.97292 with the source corrections; the event callback improves from 95.379745 to 98.67088, and initialization from 82.44643 to 92.85714. Other original functions remain visible and unmatched. The two stripped math-call identities in occlusion precalculation remain unresolved. This is source comparison, not a completed source-linked unit.

A bounded PS2 vector-inline restoration was evaluated and rejected because it lowered the occlusion function score. No shared vector-header change is retained. All 224 GameCube source objects compile with allocated sections identical to the verified baseline.

Private reproducible evidence is under `build/ps2volume164`: the unchanged and corrected whole-TU compile commands, regional original-derived profiles and reports, `type_proof.py`/`type-proof.json`, and the full GameCube allocated-section inventory.
