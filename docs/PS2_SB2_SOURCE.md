# PS2 complete SB2 source comparison

The complete zNPCTypeBossSB2.cpp compares all 88 original functions / 55,508
bytes in USA, Europe and Germany. Each has 20 standard code matches / 5,008
bytes and 66.273834% fuzzy matching. Every partial function remains represented.
No complete-TU or executable-link claim is made.

The only source change excludes an unused GameCube new.h include on PS2.
Every new expression in this TU uses the existing inherited
RyzMemData::operator new(size_t, S32, RyzMemGrow*) declaration. No replacement
allocation implementation, function-body change, compiler flag or scoring
change is introduced.

Actual ordinary and debug objects have identical ordered allocated sections.
All 263 shared concrete aggregate names, including every distinct nested layout
variant, agree on complete sizes and member offsets in all three debug originals.
Original-only and opaque definitions are not claimed verified. xNPCBasic's six
bitfields are checked explicitly after expanding its source wrappers.

The audit records source/original naming differences rather than hiding them.
node_hook midpoint/points/pos correspond to original center/loc_size/loc: the
boolean stays at byte 8, signed count at byte 12, and three xVec3 elements at byte
16, with full size 52. Both array bounds and the 12-byte element layout are
verified. Original model_enum occupies four bytes at byte 4; current source
uses signed S32 storage there. This is a physical-layout correspondence, not a
claim of identical C++ type declarations. The eight-byte curve_node names its
second float scale instead of original value; both floats and offsets agree.
The distinct 12-byte curve_node is retained separately in the comparison.

Each original independently validates canonical names, function extents and
named relocation inverses. Twenty-nine initially unknown references remain
unassigned. Applying real source relocations to original named addresses
reconstructs 14 functions / 816 bytes exactly in every region. Other standard
matches retain unresolved addresses and are not claimed raw linked equality.

The GameCube all-source build and retail SHA1 check pass, with its complete
progress report unchanged. Private evidence is under build/npc218: ordinary
and debug compiler objects/logs, audit_sb2.py, per-region layout inventories,
zNPCTypeBossSB2/all-region-summary.json and raw-proof.json, and
gc-verification.json.
