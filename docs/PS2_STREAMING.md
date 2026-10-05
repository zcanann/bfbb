# PS2 streaming service and public packer API

The complete shared `xstransvc.cpp` compiles against a small public packer API.
`xpkrsvc_api.h` contains the existing asset handler table, asset TOC record,
layer enumeration, reader callback table, and five public entry declarations.
`xpkrsvc.h` includes that API and retains the private packer layouts and their
platform dependencies. PS2 streaming uses only the API; it does not import
GameCube SDK headers or invent a PS2 packer implementation.

This separation agrees with original PS2 DWARF: `st_PACKER_READ_DATA` is opaque
in the streaming translation unit. In all three debug-bearing executables,
`st_PACKER_ASSETTYPE` is 40 bytes, `st_PKR_ASSET_TOCINFO` 24,
`st_PACKER_READ_FUNCS` 68, `st_STRAN_SCENE` 276, and `st_STRAN_DATA` 4,420.
Actual whole-TU compiler debug output reproduces every direct member name,
offset and size. The reader's LoadAsset callback uses the original PS2 char*
parameter, while the other platforms retain their existing const char* type.
Adding debug information leaves allocated source sections unchanged.

Original PS2 `xSTLoadStep` calls PKRLoadStep and the real iFileAsyncService.
It does not call the GameCube disc-state helper. The PS2 source therefore omits
that platform call and the GameCube weak empty file-service fallback, preserving
the real asynchronous service invocation. The C string header gains the ordinary
strcpy declaration needed by existing source calls.

Nine file-local helpers are inline on PS2: XST_unlock_all, XST_cnt_locked,
XST_PreLoadScene, XST_translate_sid, XST_translate_sid_path, XST_reset_raw,
XST_unlock, XST_get_rawinst and XST_nth_locked. Their operations are present inside
original callers, while the original separately emitted XST_lock_next and
XST_find_bySID stay out of line. Existing helper bodies and public-function
behavior are unchanged.

All 19 original functions / 5,584 bytes remain in the comparison profile.
Fourteen functions / 2,820 bytes match in SLUS-20680, SLES-51968 and SLES-51970;
whole-unit fuzzy matching is 97.534386%. Independently applying source J/JAL,
GP-relative and HI16/LO16 relocations to named original addresses reproduces
all 2,820 bytes in each original, including real data addends into g_xstdata.
This private inverse-link check requires no production backend or report-policy
change. Five functions remain unmatched. This is not a complete source-linked
PS2 executable claim; France remains excluded pending its own identity evidence.

All 224 actual GameCube game/engine objects retain identical allocated sections,
including every consumer of the factored public packer declarations.

Private evidence is under `build/ps2stream164`: original function/call inventory,
actual compiler command/object, `profile.json`, `all-region-summary.json`,
`raw-proof.json`, original/compiled type inventories and `gc/comparison.json`.
Original layouts can be regenerated with:

```sh
PYTHONPATH=tools python -m platforms.ps2_type_layouts \
  --version SLUS-20680 --version SLES-51968 --version SLES-51970 \
  --orig-root orig --source SB/Core/x/xstransvc.cpp \
  --type st_PACKER_ASSETTYPE --type st_PKR_ASSET_TOCINFO \
  --type st_PACKER_READ_FUNCS --type st_STRAN_SCENE --type st_STRAN_DATA \
  --output build/ps2-stream-layouts.json
```
