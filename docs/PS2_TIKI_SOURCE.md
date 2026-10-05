# Complete PS2 Tiki source comparison

`zNPCTypeTiki.cpp` needs its real streaming declarations from `xstransvc.h` on
PS2. Adding that include permits the complete shared translation unit to compile
with pinned MW PS2 3.0b38 and the existing production flags. No implementation,
platform structure, compiler option, or source-order change is required.

The original-derived profile retains all 32 functions / 17,404 bytes in each of
SLUS-20680, SLES-51968 and SLES-51970. Each actual regional objdiff report gives
6 standard code matches / 768 bytes and 63.105034% fuzzy similarity. All partial
functions remain represented. The three unproven original references (two calls
and one GP address) remain unresolved; no callee or global identity is inferred.

Independent application of actual source ELF relocations using authenticated
original symbol identities reconstructs five functions / 644 bytes exactly in
all three originals. The 124-byte animation-table function is a standard code
match with unresolved literal/local addresses; it is not claimed reconstructed
or linked. Standard report policy is unchanged.

Normal and `-g` compiler output have identical allocated sections. Compiled DWARF
matches all 202 shared, concrete, uniquely named aggregate layouts in each original,
including the corrected HAZCollide / HAZTarTar / NPCHazard layouts. Original and
compiled xNPCBasic and HAZCollide bit offsets, widths and storage types also agree.
Ambiguous unqualified tri_data and opaque/original-only declarations are outside
that name intersection and are not claimed verified. The already-integrated hazard
fix corrects one Tiki field-store displacement and slightly improves its fuzzy
score from 63.104805%; no function loses score.

The exact Tiki source bytes match the parent candidate that passed a normal
GameCube all_source build, full unchanged report comparison, and retail DOL hash
in `build/npc212/gc-verification.json`. The include and subsequent hazard correction
are conditional on PS2, preserving the GameCube preprocessing branch.

Private proof artifacts are in `build/tiki212-review`: `validation.json`,
`gc-verification.json`, `fixed/profile.json`, `fixed/layout-inventory.json`,
`fixed/all-region-summary.json`, `fixed/*-hazard-bitfields.json`, and
`fixed/raw-proof.json`. This is partial source comparison, not a full PS2 link or
whole-unit completion claim.
