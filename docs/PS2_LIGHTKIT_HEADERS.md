# PS2 light-kit source and RenderWare declarations

The complete existing `xLightKit.cpp` builds using a further small group of
RenderWare declarations. Original debug information in SLUS-20680, SLES-51968,
and SLES-51970 supplies every member of `RwRGBAReal` (16 bytes),
`RwObjectHasFrame` (20), `RwMatrixTag` (64), and `RpLight` (64). The existing
`xLightKit` (16) and `xLightKitLight` (96) declarations agree with the originals.
The real whole-TU debug compilation reproduces all six sizes and direct member
offsets. `RwMatrixTag` uses its actual named flags/pad members; no new padding or
unproven alignment attribute is added. Frame implementation remains opaque.

The public RenderWare C function signatures are copied from the existing shared
SDK declarations in `include/rwsdk/rwcore.h`, `rwplcore.h`, and `rpworld.h`.
Their stripped PS2 destinations are not independently named by the original
DWARF. Compiling these interfaces does not establish those missing identities;
no call-target aliases or target relocations are invented for them.

There are two supported shared-source changes:

* After a light-kit change, PS2 calls `iModel_SetLightKit(lkit)`. Its name,
  signature, and destination are present in the original PS2 DWARF. The call
  is inside the changed-kit branch after either the enable or disable path.
  This platform behavior was absent from the GameCube-derived source.
* The existing early return in `xLightKit_Prepare` now spells its predicate as
  `if (currlight->platLight)`. Under the established PS2 compiler and flags,
  this ordinary pointer truth conversion emits the original `sltu`, `xori`,
  and branch sequence. The explicit `!= NULL` spelling emitted a direct
  branch and made the function eight bytes short. No flag or compiler patch
  is involved; the rest of the function's instructions align.

The standard objdiff report, using the same `functionRelocDiffs=none` policy as
other versions, is 4/4 functions and 1,312/1,312 code bytes in all three debug
versions. This is not raw full-TU or executable-link proof: twenty direct calls
to stripped SDK/runtime targets remain without restored named relocations.
Only `xLightKit_GetCurrent` (8 bytes) currently has a complete independent raw
proof after applying its real GP-relative relocation to the named original
`gLastLightKit` address. Other functions remain honest code-score results.

All 224 actual GameCube game/engine objects have identical allocated sections
after these changes, including the unguarded pointer predicate simplification.
The PS2 platform call is guarded; no GameCube behavior is added.

Private evidence in `build/ps2light157` includes `command.json`, the full actual
source object, `profile.json`, `all-region-summary.json`, `raw-proof.json`,
`verified-layouts.json`, `compiled-layouts.json`, `enable-retail.txt`, and
`gc/comparison.json`. The original layout inventory is reproducible with:

```sh
PYTHONPATH=tools python -m platforms.ps2_type_layouts \
  --version SLUS-20680 --version SLES-51968 --version SLES-51970 \
  --orig-root orig --source SB/Core/x/xLightKit.cpp \
  --type RwMatrixTag --type RwObjectHasFrame --type RpLight --type RwRGBAReal \
  --type xLightKit --type xLightKitLight --output build/ps2-lightkit-layouts.json
```
