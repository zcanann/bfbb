# PS2 environment and RenderWare geometry headers

The minimal PS2 `rwcore.h`, `rpworld.h`, and `iEnv.h` declarations are recovered
from the authenticated retail DWARF in all three debug-bearing versions. They
provide geometry/container types needed by the real whole `xEnv.cpp`; they do
not import GameCube vertex formats, endian macros, or platform implementations.
Pointer-only dependencies remain incomplete types.

The concrete layouts are `RwV3d` 12, `RwBBox` 24, `RwObject` 8, `RwLLLink` 8,
`RwLinkList` 8, `RpMaterialList` 12, and `RpWorld` 112 bytes. The world bounding
box is at offset 80 and its sector-render callback and pipeline are at 104/108.
Every declared member offset is recorded in the originals. Canonical DWARF
linkage names confirm the six iEnv API signatures; omitted unused parameters
in the older source dumps must not be mistaken for parameterless functions.

`iEnv` is 48 bytes, and its embedding in `xEnv` begins at offset 16 after a
four-byte pointer; `lightKit` is at offset 64 and `xEnv` is 80 bytes. These
embedding offsets establish 16-byte alignment rather than merely unexplained
tail size. The genuine compiler supports trailing `__attribute__((aligned(16)))`.
That declaration reproduces the original offsets without fabricated padding.
The precise original spelling of the alignment declaration is not known.

The existing complete `xEnv.cpp` emits four functions / 208 bytes. Applying its
actual four call relocations and two GP-relative `gCurXEnv` stores to the
independently named original addresses reproduces all 208 bytes in each of
SLUS-20680, SLES-51968, and SLES-51970. The standard objdiff result is also 4/4
exact. This establishes code matching, not a complete source-linked executable.
France requires its separate reviewed function/callee/data identity evidence.

`xEnv.h` forward-declares its pointer-only light-kit dependency on PS2. The
GameCube include path is preserved; all 224 actual GameCube game/engine objects
were recompiled with byte-identical allocated sections.

Private reproducible evidence is in `build/ps2env156`: original `type-layouts.json`
and `verified-layouts.json`, actual compiler command/object, `profile.json`,
`raw-proof.json` with original hashes and applied relocations, and
`gc/comparison.json`. The layout reader follows original DIE child/sibling links
and does not infer absent members, macros, or alignment attributes.
