# PS2 platform source comparison

The unchanged complete `zPlatform.cpp` source now compares all 20 original
functions / 17,172 bytes in USA, Europe and Germany. Each version matches
10 functions / 1,100 bytes, with 83.634056% fuzzy matching. This raises each
debug version to 57,092 matched bytes / 467 functions.

## Original and compiler evidence

Each region independently supplies original DWARF ownership, canonical linkage
and exact function boundaries. All members remain in the comparison, including
partial functions. Actual compilation uses the pinned PS2 compiler and existing
flags; this change adds no source, header, compiler or runtime changes.

The actual compiler debug object agrees with all three original binaries on
33 consumed aggregate sizes and direct member offsets. These include every
platform asset variant, platform runtime, entity/frame, motion/drive, model,
collision, matrices and player/global aggregates. Normal and debug compilations
have identical ordered allocated sections, including duplicate section names.

Named original J/JAL callees and GP data references have independently checked
original-address inverses. The unidentified call in `zPlatform_Update` at
byte offset 816 remains unresolved rather than receiving an inferred name.

Nine exact functions / 988 bytes also reconstruct identically at each original
address after applying actual source relocations. The remaining 112-byte
`zPlatform_Setup` matches under normal objdiff policy but has two anonymous
string references without reviewed original identities. Those four HI16/LO16
fields remain explicit in the raw proof; no raw-equality claim is made for it.

The standard objdiff relocation policy is unchanged. No data match, complete-TU
or executable relink is claimed. Full-game denominators and completion remain
unchanged; France and Xbox profiles are untouched.

## Local evidence

`build/targets196/zPlatform/` contains the original and compiled layout records,
all three regional reports, full profile, unknown relocations and `raw-proof.json`.
The ordinary CI pipeline recompiles the complete source and reports all members.
