# PS2 laser and decal source comparison

The original PS2 `RxObjSpace3DVertex` differs from GameCube despite both being
36 bytes: position is at 0, `RxColorUnion` at 12, normal at 16, and texture
coordinates at 28/32. The complete PS2 vertex and color union are now declared,
along with the original complete `RwTexture` (88), `RwRaster` (52), and
`tagiRenderInput` (128) layouts. No GameCube field offsets are reused for rendering.
All sizes and every direct member offset are checked against all three original
PS2 debug executables and the actual compiler's emitted DWARF.

The existing complete `include/rwsdk/rpptank.h` supplies the particle-tank API,
callbacks, enums, and structures; its dependency includes now select the PS2 SDK
foundation on that platform. `RwV2d` and `RpAtomicCallBackRender` are ordinary
SDK declarations. The private particle-tank fields consumed by decal code are
independently visible in retail: `update` ORs `0x800000` into offset `0x40`
(`instFlags`), then writes the used count to offset `4` (`actPCount`), after
reading the plugin pointer through the original `_rpPTankAtomicDataOffset`.
These sequences occur at USA `0x3aef90`, Europe `0x3af480`, and Germany `0x3ae7d0`.
Unused SDK particle-tank fields are inherited from the complete existing SDK;
no independent original DWARF proof is claimed for them.

The PS2 immediate-rendering header preserves the existing SDK setters' captured
position/color values and public rendering prototypes, and writes the actual PS2
aggregate members. Retail uses packed color stores. A bounded whole-TU comparison
of component stores versus ordinary aggregate assignment produced identical
ordered allocated sections; the simpler aggregate assignments are retained.
This supplies the SDK API semantics without inline assembly or register controls.
Original unnamed rendering callees are not assigned guessed target identities.

Neither `xLaserBolt.cpp` nor `xDecal.cpp` needs a function-body edit. Both whole
source files compile with the published compiler settings, including every
original function and all remaining nonmatches:

| Whole TU | Original functions | Original code | Matched functions | Code matched | USA fuzzy |
| --- | ---: | ---: | ---: | ---: | ---: |
| xLaserBolt.cpp | 14 | 10,720 | 3 | 1,580 | 52.841045% |
| xDecal.cpp | 11 | 5,624 | 3 | 156 | 40.099575% |

Original sizes and matched counts are the same in SLUS-20680, SLES-51968, and
SLES-51970. Laser fuzzy is 52.836567% in Europe/Germany; decal fuzzy is identical
across all three. All six matched functions / 1,736 bytes also pass independent
raw verification after applying
actual source-object relocations to original named addresses. For decal static
data, the original DWARF's recorded `linkage_name` supplies the anonymous-namespace
mangled symbols; their identities are not inferred from source bytes. France has
no new profile here, and no complete-TU or whole-executable link is claimed.

Normal and debug whole-source objects have identical allocated sections. Thirteen
emitted aggregate records (including gameplay consumers and SDK types) reproduce
the original full sizes/member offsets in every debug version. All 224 GameCube
source objects were rebuilt with identical ordered allocated sections.

Private evidence is under `build/ps2laser188/`: the two candidate profiles,
all-region reports, independently applied relocation proofs, original/compiled
layout registries, original packed-store and particle-tank access windows, the
aggregate setter control, and the GC comparison. `xPtankPool.cpp` itself remains
outside this batch: its concrete `RwFrame` layout and additional SDK calls require
further evidence before enabling that whole source file.

The frozen `3ec59b58b` production profile was also rebuilt with the new headers:
all 60 existing source TUs compiled, and every one of the 5,391 complete function
records is identical to the verified `platform190` CI report. Its 52,656 matched
bytes / 426 functions remain unchanged before enabling these two new profiles.
