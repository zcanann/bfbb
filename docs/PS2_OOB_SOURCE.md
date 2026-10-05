# PS2 out-of-bounds player source

The complete `SB/Game/zEntPlayerOOBState.cpp` translation unit now compiles using
existing PS2 immediate-mode and asset headers. Its rectangle helpers use the real
SDK `RwIm2DVertex` typedef, which resolves to `RwSky2DVertex` on PS2 and the existing
GameCube type on GameCube. No synthetic type alias or layout is introduced.

The original 244-byte PS2 `set_rect_vert` stores XYZ at offsets 0/4/8, reciprocal
Z at 24, and four float color components at 32/36/40/44. Existing authenticated
Sky SDK macros express these stores. The complete helper reproduces every byte
of all three debug originals. GameCube retains its original packed-color body.

Original `render_fade` and `move_right` instructions establish the regional screen
constants: US 640 by 448, EU/German 512 by 512. The corresponding aspect constants
are `0x3fb6db6e` and `0x3f800000`. The sole PS2 `FABS` call uses the existing `xabs`
interface; its original instruction is `ABS.S`. The only new declaration,
`RwReal RwIm2DGetFarScreenZ(void)`, is the complete existing vendor declaration in
`include/rwsdk/rwplcore.h`. It supplies no replacement implementation.

Actual independently compiled regional results are:

| Original | Functions / original bytes | Normal and raw exact | Fuzzy |
| --- | --- | --- | --- |
| SLUS-20680 | 47 / 13,104 | 12 / 1,012 bytes | 53.984737% |
| SLES-51968 | 47 / 13,096 | 12 / 1,012 bytes | 53.941357% |
| SLES-51970 | 47 / 13,100 | 11 / 908 bytes | 53.906567% |

All normal matches independently reproduce the original bytes after real source
relocations are applied to named original addresses. Germany's different
`read_persistent` remains a partial match. All 47 members remain in each profile,
with 14 unresolved original references retained honestly. Three SHA-gated profiles
preserve actual regional extents and references. France is not enabled without
reviewed ownership. No complete-unit or executable-link claim is made.

All 185 shared aggregate names and variants, including direct bitfields, agree
with the originals. A same-name collision is resolved by actual type references:
the OOB original emits the 4-byte virtual `ztalkbox::callback`, while current
headers additionally emit the unrelated 12-byte `xtextbox::callback`. Original
`xFont.cpp`'s `xtextbox.cb` pointer refers to that exact callback DIE, whose three
members are `render` at 0, `layout_update` at 4 and `render_update` at 8. The
compiled `xtextbox.cb` points to the corresponding compiled DIE. Both callback
variants remain in the comparison; none is discarded by size.

Normal and debug source objects have identical allocated sections. Actual
GameCube production compilation preserves all 11 ordered allocated sections.
The added SDK function declaration has no uses in previously enabled PS2 units;
its other source consumers are the still-unprofiled `zGame` and `xShadow`.

Private evidence under `build/oob238` includes authenticated original instruction
and local/type records, compiler commands and regional objects, all-region normal
reports, independently applied relocation proofs, qualified callback references,
layout inventories, `gc/proof.json`, `profiles.json` and `summary.json`. Original
and compiled bytes remain private.
