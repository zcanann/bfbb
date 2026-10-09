# Xbox particle damping

The Xbox originals calculate the damping factor before loading the particle
list head. Restoring this order only for Xbox makes
`xParCmd_DampenSpeed_Update` match all 81 bytes in both authenticated releases,
raising its report score from 98.57143% to 100%. The function has no calls or
relocations; its compiled body is checked directly against each original XBE.
The other 21 compared particle-command function objects remain unchanged.

All code/data symbol scores remain unchanged in all three GameCube versions.
Every allocated source ELF section, including duplicate section names, remains
byte-identical in all four PS2 versions. No compiler, profile, registry or
function-boundary changes are needed. Full Xbox source linking remains pending.

Evidence: the Xbox worktree's `build/xbox-dampen-raw-proof.json`,
`build/xbox-dampen-{order,eu}`, `build/nonxbox-parrand/proof.json` and
`build/nonxbox-ps2-dampen/proof.json`.
