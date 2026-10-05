# PS2 fog source and minimal RenderWare types

The complete existing `xFog.cpp` now compiles using the shared camera API header.
The PS2 RenderWare header adds only the original `RwRGBA` byte fields, `RwFogType`
enumerators, and an opaque `RwCamera` declaration. No camera layout or platform
implementation is invented. `xFog.h` needs only an `xScene` pointer on PS2, so it
avoids the unrelated entity/model dependency while preserving GameCube includes.

The original DWARF in all three debug-bearing versions records `RwRGBA` as
4 bytes, `iFogParams` as 28, `xFogAsset` as 36, and `_xFog` as 20. The actual
whole-TU `-g` output agrees on every direct member offset and size, and adding
debug information leaves allocated source sections unchanged.

The matching source reconstruction restores three ordinary boundaries:

* The typed initialization and reset helpers are inline on PS2, as required by
  their code appearing inside the original public functions rather than as
  separate calls. Both helpers are used only within this translation unit.
* The event callback uses the original DWARF `_xFog* t` local and reads
  `t->tasset` for each assignment. Caching the asset pointer instead removed
  twelve original pointer reloads across the temporary fog-parameter stores.
* Initialization keeps a derived `_xFog* fog` local when assigning/reading its
  asset. This removes an unnecessary field-address temporary and reproduces
  the original direct load. The precise original spelling of this local is
  not known.

All six original functions / 412 bytes match in SLUS-20680, SLES-51968, and
SLES-51970. Independently applying the actual source object's J/JAL relocations
and adjacent HI16/LO16 callback pair to original named destinations reproduces
all 412 original bytes in each version. The target callback restoration uses
the existing strict address-pair validator; no backend extension is needed.
This is function-code proof, not a complete source-linked PS2 executable claim.

All 224 actual GameCube game/engine objects retain identical allocated sections.
The local-variable refinements are shared; only the helper inline declarations
and include dependency change are PS2-specific.

Private evidence is under `build/ps2fog160`: actual command and complete object,
`profile.json`, `all-region-summary.json`, `raw-proof.json`, original and compiled
type inventories, and `gc/comparison.json`. Type evidence can be reproduced with:

```sh
PYTHONPATH=tools python -m platforms.ps2_type_layouts \
  --version SLUS-20680 --version SLES-51968 --version SLES-51970 \
  --orig-root orig --source SB/Core/x/xFog.cpp \
  --type RwRGBA --type iFogParams --type xFogAsset --type _xFog \
  --output build/ps2-fog-layouts.json
```
