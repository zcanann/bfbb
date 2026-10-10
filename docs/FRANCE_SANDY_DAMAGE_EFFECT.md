# French Sandy damage-effect leaf

`france_sandy_damage_effect.py` proves the complete 432-byte
`zNPCBSandy_BossDamageEffect` body at `0x335000`. Each of the three original
references must supply the same canonical linkage, source owner and complete
DWARF extent. Full instruction comparison, exact masks, closed control flow
and exhaustive unique whole-body matching identify the French leaf. It has
no direct calls or registry dependencies, and makes no whole-unit claim.

Its eight address operands resolve to the original `BDErecord[4]` array.
DWARF establishes a 264-byte record containing `float save_F32[64]` at zero,
a float timer at 256 and an `xModelInstance*` at 260. Both array descriptors,
all record fields, the pointed-to model type, its self-linked Next pointer
and float RGB members are checked. The entire 1,056-byte array must fit
original and French runtime BSS. Each operand must address the base or the
exact first-record timer/model field. No data extent is promoted.

Tests reject body/address mutations, duplicated full templates, changed
original owner/extent, declaration locations, record-field offsets, array
bounds and element types. JSON round trips preserve every evidence field;
empty and unrelated registries cannot change this independent proof.

The body matches French source exactly, adding 432 exact bytes and one
function. All fifteen previous Sandy records remain unchanged. The selected
16-function unit has 8,732 exact bytes, fifteen exact functions and 99.924416%
fuzzy matching across 10,584 bytes. Private evidence:
`build/sandy-damage-effect-proof.json`, `build/sandy-damage-effect-tests.txt`
and `build/sandy-damage-effect-france-pilot/report.json`. Canonical aggregate
regeneration and the full source report remain integration gates.
