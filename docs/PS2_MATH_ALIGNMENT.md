# PS2 matrix, quaternion, and vector alignment

The original PS2 layouts require 16-byte alignment for `xMat4x3`, `xQuat`, and
`xVec4`. These types retain their existing members and sizes; the PS2 declarations
now express the alignment rather than adding padding members. Other platforms
retain their original declarations. The PS2 `xiMat4x3Union` now contains its two
original complete, 64-byte views, `xMat4x3 xm` and `RwMatrix im`, both at offset zero.

## Original layout evidence

The three authenticated debug executables agree on the following layouts:

- `xEntMotion` is 128 bytes, with its variant union at 32 and owner/target pointers
  at 112/116. Its `xEntPenData` member is 80 bytes and contains `omat` at 16.
- `xEntMPData` is 64 bytes, with quaternion members `aquat` and `bquat` at 32/48.
  The independently recorded `xBinaryCamera` places its quaternion at 64 and has
  size 112 despite its final ordinary member ending at 100. This corroborates
  quaternion alignment outside motion's variant union.
- `xCamera` is 816 bytes and places its `xVec4` frustum-plane array at 624.
  The complete containing `xGlobals` is 1,792 bytes.
- `xEntFrame` is 240 bytes. Its previously restored explicit alignment remains.
- Collision DWARF records both complete matrix-union views and a 336-byte
  `xSweptSphere`, whose last member ends at 324. The latter supports the matrix
  alignment conclusion but is not claimed as a compiled-type validation here.

These are deductions from independently recorded aggregate sizes and member
placements, not claims that DWARF contains an explicit alignment attribute.
Whole-source debug compilations reproduce every direct member offset and size
for the motion variant types, frame, camera/globals, pad, and climate structures.
Their allocated sections are identical to the corresponding normal compilations.

## Whole-source comparisons

The unchanged complete source files now compile with the published PS2 compiler
profile. Every original function remains in each comparison, including holdouts.
Results agree across SLUS-20680, SLES-51968, and SLES-51970:

| Source unit | Original functions | Compared bytes | Code matched bytes | Fuzzy score |
| --- | ---: | ---: | ---: | ---: |
| xEntMotion | 20 | 14,616 | 956 | 81.82923% |
| xPad | 8 | 3,536 | 108 | 69.81901% |
| xClimate | 6 | 1,888 | 160 | 79.26271% |

This adds 20,040 bytes of actual source comparison and 1,224 code-matched bytes
per debug version. France requires independent function identities before adding
these profiles. No whole-unit completion or source-linked executable is claimed.

Independent application of actual source relocations to original addresses
proves 780 of those bytes exactly in each version. The remaining normally
code-matched bodies are `xEntOrbitMove` (328 bytes, unresolved `isin`/`icos` call
identities) and `xClimateInit` (116 bytes, unresolved anonymous string-address
relocations). They retain the standard report metric; they are not claimed as
raw linked-byte matches.

## Validation and reproduction

Private round-174 evidence lives under `build/ps2large174/`: per-unit `profile.json`,
`verify_regions.py`, `raw-proof.json`, and `all-region-summary.json`; original
`motion-layouts.json`, `collision-layouts.json`, and `quaternion-layouts.json`;
and per-unit original/compiled layout records. The original layouts can be
regenerated using `tools/platforms/ps2_type_layouts.py` with the relevant original
source unit and type names.

The final normal production PS2 run preserves all 5,391 existing function records
exactly across the 45 previously enabled source units, including 36,292 matched
bytes. `regression-final-diff.json` is empty. All 224 actual GameCube source
compilations preserve every allocated section's name, size, and bytes in order,
including repeated section names. Root integration performs the normal executable
build and multi-version CI checks before publication.
