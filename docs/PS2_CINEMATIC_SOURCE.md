# PS2 cinematic effects source comparison

`SB/Game/zNPCFXCinematic.cpp` now compiles as a complete translation unit with
Metrowerks PS2 3.0b38 and the established production flags. The source changes
only select existing PS2-compatible includes: the RenderWare skin API, standard
math declarations, and the actual `xParSys` definition used by the particle
emitter code. No function bodies, SDK implementations, layouts, or compiler
options change.

The comparison includes all 77 functions / 24,488 original bytes for each of
SLUS-20680, SLES-51968, and SLES-51970. Each has 21 code matches / 2,544 bytes
and 65.17527% overall fuzzy similarity. These are normal objdiff source
comparisons, not a complete translation-unit or executable-link claim. France
is not enabled without independently reviewed original function ownership.

Original DWARF supplies each function's source ownership, extent and canonical
linkage. Actual original instructions establish 150 named direct transfers and
15 named GP-relative operands per version. The independently generated regional
profiles agree after their executable SHA-1 allowlists are removed, so one
profile may select all three authenticated originals. Sixteen remaining
original references are explicitly unresolved; no runtime identity is guessed.

Validation used actual complete source objects:

- Normal and debug compilations have identical ordered allocated sections.
- All 246 shared aggregate names, including all emitted variants and direct
  bitfield metadata, agree with each original's DWARF. Existing documented
  anonymous-union and `xNPCBasic` bitfield representations are normalized by
  their actual member offsets, sizes and bit positions.
- Independently applying source relocations to original named addresses proves
  17 functions / 1,984 bytes byte-for-byte in every region. The other four normal
  code matches still contain unproven anonymous literals or local-static/guard
  addresses. They are not reported as raw matches or link-complete.
- The actual GameCube production compilation preserves all 17 ordered allocated
  sections exactly.

Private artifacts are under `build/cinematic228`: `profiles.json`, `summary.json`,
regional reports and restored-relocation inventories, the all-region raw proof,
all-region aggregate-layout inventories, compiler commands, and GameCube control.
Original executable bytes and compiler/runtime binaries remain private.
