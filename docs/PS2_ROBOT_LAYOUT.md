# PS2 Robot hazard layout verification

The complete `zNPCTypeRobot.cpp` build exposed a real PS2 header-layout error. MW
PS2 3.0b38 omits storage for the anonymous struct containing HAZCollide's three
bitfields. All three original debug executables
record signed-32-bit fields at byte 120, with bit offsets 0/8/16 and widths 8/8/16.
The PS2 declaration now expresses those same fields directly. The original
GameCube/Xbox declaration remains unchanged.

The authenticated originals give HAZCollide 132, HAZTarTar 164 and NPCHazard 228 bytes;
the former source emitted 128, 160 and 224. Correcting the declaration restores the
actual `npc_owner` offset 216 (previously 212) and the tartar target-position offset 196
(previously 192). Robot LaunchProjectile uses both. Its only changed instructions
are the field-store displacement and the address of that target vector.

A normal and a `-g` build with the pinned production b38 flags have identical
allocated sections. The compiled DWARF agrees with each of SLUS-20680, SLES-51968
and SLES-51970 on all 231 shared, concretely defined, uniquely named aggregate
layouts, including RxObjSpace3DVertex and all Robot classes. The xNPCBasic wrapper
bitfields and HAZCollide field widths/bit offsets were also compared explicitly.
The ambiguous unqualified nested name tri_data is excluded from this broad name
intersection; opaque or original-only definitions are not claimed verified.

Robot retains all 221 original functions / 65844 bytes in its partial source profile.
Each region has 103 standard code matches / 23392 bytes, fuzzy 72.294815%
(previously 72.294754%). Only LaunchProjectile's score changes, 36.577038% to 36.58006%.
Independent application of real source ELF relocations reconstructs 80 functions /
12292 bytes exactly in each original. The other 23 standard matches have unresolved
literal/local-static addresses; they are not claimed byte-exact or linked. Original
data-DIE canonical linkages disambiguate the two class-owned rast_blink globals.

All four already-published PS2 header consumers were rebuilt. zEntPickup,
zNPCGoalStd and zNPCTypeCommon have identical allocated sections. zNPCGoalRobo has
no score losses and gains the 204-byte zNPCGoalAttackFodder::Enter, 99.960785% to 100%,
in each debug region (9112 to 9316 matched bytes). The new function independently
reconstructs byte-for-byte after original-backed relocation application. Four
other Robo functions improve slightly. These four profiles currently cover the
three debug regions only; no France extent or denominator was changed.

The optional GP target_linkage filter was independently replayed against all 5391
original functions and 4909 existing relocation records: target bytes and relocation
records remain identical. No new scoring policy or inferred global identity is used.

Private evidence is in `build/robot206-review`: `verify_regions.py`,
`fixed/all-region-summary.json`, `fixed/*-hazard-bitfields.json`,
`fixed/raw-proof.json`, `consumers/regional-comparison.json`,
`consumers/raw-proof.json`, `gp-regression.json`, and `validation.json`.
