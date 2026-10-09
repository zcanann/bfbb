# PS2 package-loader source recovery

## Asset names and helper call boundaries (2026-10-09)

All three debug DWARFs describe `st_PACKER_ATOC_NODE` as 96 bytes, with
`basename` at offset 0x40 and type char[32]. The shared header previously
contained only the 64-byte GameCube layout and returned a constant name.
The PS2 conditional field and Name accessor restore the original allocation,
indexing, and lookup behavior. `LOD_r_ADBG` copies at most 31 characters into
the field and terminates byte 31, matching the original operands. Its runtime
copy interface is reconstructed as `strncpy`; no SDK identity is promoted.

Direct calls in the original executables, resolved against named DWARF
functions, establish the helper boundaries retained by scoped PS2
`dont_inline` directives. In particular, the dictionary reader calls the
asset/layer TOC readers; PACK calls PVER/PLAT; the top-level parser calls
DICT/STRM; LoadAsset tail-calls FindAsset; and the asset header reader calls
the type lookup and debug reader. Allocation/release calls are retained too.
The current compiler otherwise expands these bodies into the callers.

Normal complete-unit comparisons, each containing 50 functions, show no
regressions in USA, Europe, or Germany. All three gain **748 exact bytes and
four exact functions**, from 2,816/24 to 3,564/28. The newly exact functions:

| Function | Bytes | Before | After |
| --- | ---: | ---: | ---: |
| PKR_AssetName | 108 | 76.07407% | 100% |
| PKR_LoadAsset | 8 | 0% | 100% |
| PKR_parse_TOC | 424 | 0% | 100% |
| LOD_r_DICT | 208 | 0% | 100% |

Other gains shared by all three versions:

| Function | Before | After |
| --- | ---: | ---: |
| LOD_r_PACK | 26.5% | 98.666664% |
| LOD_r_AHDR | 53.39706% | 99.007355% |
| LOD_r_ADBG | 89.18033% | 96.72131% |
| PKR_LoadStep_Async | 52.49259% | 79.94074% |
| PKR_LayerMemRelease | 0% | 74.09722% |
| PKR_ReadInit | 85.367645% | 98.52941% |
| PKR_ReadDone | 79.23602% | 99.62733% |

Fresh `tools/solo.py Core/x/xpkrsvc --top 3` remains 76/76 exact for GameCube.
The PS2 header layout change requires the normal integration source gates;
this local check is not a fresh full-project report. France has no enabled
xpkrsvc source profile, so no French score is claimed. The resource-arena
behavior left incomplete at this checkpoint is recovered below.

Private evidence is under `build/packer-oct09`: original `layout-proof.json`
and `original-calls.json`, before/after normal unit reports, and
`after-proof.json`. Original executables are USA
`32e3b7dda09fd8d7fcba4eb769fa17c08cea05df`, Europe
`0c3e685edc13a362d0d27a10dccb0b9698eb5597`, and Germany
`83bf81a139ea24fdb015f1419377de85b643828a`. No registry, profile, compiler,
runtime alias, or non-PS2 layout changes are made.

## Resource-arena loading

The PS2 handoff path allocates a RenderWare resource entry rather than using
the GameCube heap mark. `PKR_specialGet_loadbuf` empties the arena, queries its
size, allocates `amount + align` bytes, aligns the data after the 24-byte entry,
and clears the requested amount. The original named destruction callback is
empty. Pending asynchronous reads refresh the entry's position in the used
list, preventing it from becoming a reuse candidate. Release clears the
global ownership references, frees the entry, and clears the layer pointer.

The new header declarations reproduce the RwResEntry size and all five member
offsets authenticated independently in all three debug DWARFs. The private
RwModuleInfo layout is likewise authenticated (two integers, eight bytes).
The local rwResources layout comes from the existing vendor `baresour.c`;
the usedEntries offset 0x24 is independently present in the original pending
read path. This is not a claim that a complete rwResources DWARF type exists.
The original named `g_RWarena_resEntry`, `g_RWarena_resOwner`, resourcesModule,
and ourGlobals references provide the data identities.

SDK interfaces are reconstructed as RwResourcesEmptyArena, GetArenaSize,
AllocateResEntry, and FreeResEntry. The original bodies' arena traversal,
size access, and allocation/free behavior support those source spellings;
their runtime symbol identities are not added to a registry. The local
inline `PKR_specialReturn_loadbuf` name is inferred, not authenticated. Its
release-and-clear operation preserves the original's two layer-pointer
clears when inlined into the common release path. The original skip guard
is also restored explicitly.

All three normal 50-function unit comparisons gain another **1,828 exact
bytes and five functions**, with no regressions:

| Function | Bytes | Previous | Now |
| --- | ---: | ---: | ---: |
| PKR_specialGet_loadbuf | 160 | 0% | 100% |
| PKR_special_loadbuf_killed | 8 | 0% | 100% |
| PKR_LayerMemReserve | 292 | 75.54794% | 100% |
| PKR_LayerMemRelease | 288 | 74.09722% | 100% |
| PKR_LoadStep_Async | 1080 | 79.94074% | 100% |

The five complete original/source bodies also have equal lengths and equal
raw words after masking their source relocation fields (9, 0, 5, 8, and 57
respectively). No branch displacement, register operand, or non-relocation
constant is masked. This audit does not prove the masked linked SDK/data
identities. Fresh GC solo remains 76/76 exact; the shared PS2 header requires
the full integration gates. Private evidence is `resource-layout-proof.json`,
`resource-proof.json`, and `resource-raw-proof.json` under `build/packer-oct09`.
