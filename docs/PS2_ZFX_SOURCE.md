# PS2 zFX source and goo rendering

The complete `SB/Game/zFX.cpp` translation unit now compiles with the established
PS2 compiler and flags. Existing PS2-compatible skin/math/asset headers are
selected, and the SDK field macros `RwFrameGetMatrix` and
`RpAtomicGetGeometryMacro` are restored from `include/rwsdk/rwcore.h` and
`include/rwsdk/rpworld.h`. No SDK implementation or synthetic layout is added.

Original PS2 DWARF exposes a real platform difference: `zFXGooInstance` is 112
bytes and has no `orig_uvs` member. The GameCube structure is 116 bytes. Each
PS2 member, size and offset is restored exactly; the GameCube definition stays
unchanged.

The original PS2 goo setup allocates only saved vertex and color arrays. It does
not perform GameCube's geometry cloning, UV allocation or initial array copies.
The original 208-byte render callback assigns `xFXgooPipeline`, locates the
instance, calls `iFXgooSetParams`, then invokes `gAtomicRenderCallBack`. It does
not draw GameCube's immediate-mode frozen overlay. These two platform branches
are restored from the complete original instructions and their DWARF locals.

The real `SB/Core/p2/iFXgoo.cpp` definition supplies canonical linkage
`iFXgooSetParams__FP5xVec3UiffffPf`, void return, and all seven formal parameters.
Original data declarations independently identify `xFXgooPipeline` as
`RxPipeline*`. The PS2 header declares these interfaces without inventing an
implementation or deriving a callee identity from a source relocation.

An original bug is deliberately preserved and commented: if the instance search
reaches 24, the render callback still passes the one-past instance's center and
warb-array addresses. The actual original helper unconditionally reads the three
center floats. No extra array element, padding or null substitution is added.
The restored 208-byte callback is independently byte-exact in all three regions,
including those address calculations and calls.

Actual complete-unit results for SLUS-20680, SLES-51968 and SLES-51970 are:

- 38 original functions / 18,912 bytes, all included in the comparison.
- 17 normal code matches / 3,408 bytes, with 60.682953% fuzzy similarity.
- 15 functions / 2,888 bytes independently raw-exact after applying real source
  relocations to original named addresses. `zFXGooEnable` and
  `zFXGooEventSetWarb` still contain unproven `memcpy` or default-render-callback
  relocations, so their normal matches are not claimed as raw/link proof.
- All 143 shared aggregate names and variants, including direct bitfields,
  agree with each original. Normal and debug objects have identical ordered
  allocated sections.
- The actual GameCube production compilation preserves all 17 allocated
  sections exactly.

There are 167 named direct transfers and 32 named GP-relative references per
regional profile; 19 remaining references stay unresolved. Germany retains a
separate SHA-gated profile because three header-helper target aliases have a
different original address. France remains disabled without reviewed ownership.
No complete-unit or executable-link claim is made.

Private evidence under `build/fx234` contains original function disassemblies,
API/formal-parameter and data-anchor records, all-region layout inventories,
normal reports, independent raw proofs, compiler commands, `gc/proof.json`,
`profiles.json` and `summary.json`. Original bytes and compiled objects remain
private. The added interfaces have no uses in previously enabled source units;
the goo structure is consumed only by `zFX.cpp`.
