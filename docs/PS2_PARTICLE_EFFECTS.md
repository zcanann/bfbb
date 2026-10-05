# PS2 particle-effect source comparison

The complete `xFFX.cpp` compiles through the shared entity declarations in
`xEntTypes.h`. GameCube retains its original include path. Actual compiled
DWARF sizes and every direct member offset for `xEnt`, `xEntFrame`, `xFFX`,
`xFFXShakeState`, and `xFFXRotMatchState` agree with all three debug-bearing
originals. The debug compile leaves all allocated section bytes unchanged.

The original unit has no standalone `xFFXFree` or `xFFXRemoveEffectByFData`
functions. PS2 inline declarations recover that helper ownership without
changing their implementations. The normal whole-unit comparison matches
11 of 14 functions / 712 of 1,588 bytes. Every original function remains in
the profile. The callback overload, recursive application helper, and shake
update remain unmatched. A recursive-inline control reduced exact coverage
and was rejected; no compiler flags, forced bodies, assembly, or padding were
added.

The comparison profile uses original DWARF identities for named calls and
GP references. Independently applying the eleven exact functions' real source
R_MIPS_26 and GPREL16 relocations to named original addresses reproduces all
712 original bytes in each debug-bearing release (`raw-proof.json`).
Two stripped runtime calls in the unmatched functions remain unresolved. Standard code
matching does not establish complete relocation or data matching, and no
retail source link or complete-TU claim is made. France gains no inferred
function identity from this unit.

Private evidence: `build/ffx168`, including the actual whole-TU object,
`type-proof.json`, original layouts, and the standard objdiff report.
