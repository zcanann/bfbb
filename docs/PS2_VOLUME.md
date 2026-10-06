# PS2 volume source comparison

The genuine `zVolume.cpp` translation unit now participates in all three debug-bearing PS2 comparisons. The profile retains all five original functions (2,068 bytes), with original DWARF linkage names, named calls, and GP data anchors. France has no new recovered identities in this change.

The original debug records describe `nvols` as an unqualified unsigned short, `gOccludeCount` as an unqualified signed integer, and `gOccludeList` as an array of ordinary pointers. The PS2 declarations follow those types while GameCube keeps its existing declarations. The local typed initialization helper is inline on PS2, consistent with the original unit having only the public initialization body and its direct calls to `xVolume::Init`.

The full original and compiled layouts of `zVolume`, `xVolume`, `xVolumeAsset`, and `PreCalcOcclude` agree in all three debug versions. A private debug compile also preserves every allocated section of the normal object.

All three standard objdiff comparisons report 128 matched code bytes in two functions: `zVolumeSetup` (100) and `zVolumeGetVolume` (28). The unit's fuzzy score rises from 48.061897 to 49.97292 with the source corrections; the event callback improves from 95.379745 to 98.67088, and initialization from 82.44643 to 92.85714. Other original functions remain visible and unmatched. The two stripped math-call identities in occlusion precalculation remain unresolved. This is source comparison, not a completed source-linked unit.

A bounded PS2 vector-inline restoration was evaluated and rejected because it lowered the occlusion function score. No shared vector-header change is retained. All 224 GameCube source objects compile with allocated sections identical to the verified baseline.

Private reproducible evidence is under `build/ps2volume164`: the unchanged and corrected whole-TU compile commands, regional original-derived profiles and reports, `type_proof.py`/`type-proof.json`, and the full GameCube allocated-section inventory.


## Initializer count expression

The original initializer keeps the volume count as the unsigned-short `nvols`
global through the allocation expression. All three original DWARF records list
only `i` (unsigned short), `size` (unsigned integer), and `asset` as locals; there
is no widened count capture. The PS2 source now uses `nvols` directly in the guard
and allocation expression. This recovers the original `ANDI 0xffff` and subsequent
loop alignment. The existing GameCube/Xbox volatile capture path is preserved.

The complete unit improves initialization from 92.85714 to 100 percent in all
three debug regions: 3 / 5 functions and 352 / 2,068 code bytes, adding 224 bytes
and one function per region. The other four function records are unchanged.
Source initialization grows from 216 to the original 224 bytes. Every other
allocated section and its relocation records stay identical. Applying actual
source relocations to independently named original calls and data reconstructs
all 224 original bytes exactly in each debug region, with no unresolved fields.

The full French source object is identical to the other three compiled objects;
its existing single reviewed `zVolumeSetup` member remains unchanged at 100 bytes
and 100 percent. No new French boundaries are inferred. Actual GameCube compilation
preserves all five ordered allocated sections; no Xbox profile or source dependency
currently compiles this TU. This is an initializer match, not whole-unit completion.

Private evidence: build/near267/{proof.json,original-proof.json,raw/raw-proof.json,
gc/proof.json,baseline/<version>/report.json,direct/<version>/report.json}.
No compiler, shared header, target metadata or scoring changes accompany the fix.
