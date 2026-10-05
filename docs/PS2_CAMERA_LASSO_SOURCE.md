# PS2 camera and lasso source comparisons

The complete `zCamera.cpp` and `zLasso.cpp` translation units compile with the
established PS2 compiler and production flags. Only PS2 include selections
change: camera uses the existing public `xpkrsvc_api.h` asset-TOC definition,
and lasso includes the existing original-backed `rwim3d.h` rendering API and
vertex setters. No function body, SDK declaration, type layout, or compiler
option changes.

Actual comparisons against SLUS-20680, SLES-51968 and SLES-51970 agree:

| Unit | Original functions / bytes | Matched functions / bytes | Fuzzy similarity |
| --- | ---: | ---: | ---: |
| zCamera | 32 / 17,664 | 20 / 2,296 | 74.98573% |
| zLasso | 13 / 14,432 | 4 / 1,176 | 50.583427% |

All 24 matched functions / 3,472 bytes also pass independent raw verification:
actual source relocations are applied using original named function/data
addresses, and every resulting byte agrees with each authenticated original.
This does not prove either complete translation unit or an executable relink.
All unmatched original functions remain in the normal comparisons. France is
not enabled without separately reviewed original ownership.

Each regional profile is generated from original DWARF ownership, canonical
linkage and actual instruction operands. Camera has 68 restored named transfers
and 352 GP-relative references; lasso has 33 of each. The remaining 8 camera and
16 lasso references stay unresolved. Regional profiles are combined only after
all fields other than their original SHA-1 allowlists agree.

Actual debug-object validation checks every shared aggregate variant and direct
bitfield metadata against each original: 129 names for camera and 76 for lasso.
The existing documented `xNPCBasic` bitfield representation is compared by real
member offsets, sizes and bit positions. Debug and normal compilations have
identical ordered allocated sections. Actual GameCube production compilations
preserve all 10 camera and 7 lasso allocated sections exactly.

Private evidence is in `build/render232`: `profiles.json`, regional reports and
relocation inventories, all-region raw proofs, aggregate-layout inventories,
normal/debug compiler commands, and `gc/proof.json`. Original executables,
compiler binaries and generated objects are not public artifacts.
