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


## Platform and regional package validation

`ValidatePlatform` now uses the original PS2 platform and regional checks.
Its original DWARF `rc` is a signed 32-bit integer. Boolean conversion of
each comparison, short-circuit validation of recognized strings, and a
final normalized return reproduce the original control flow. The required
platform is `PlayStation 2`, rather than the previous GameCube literal.
USA requires NTSC and US Common. Europe and Germany require PAL and accept
United Kingdom, French, or German. Germany also performs an additional
comparison against German before testing the saved PAL result; its return
value is discarded in the original, and that behavior is preserved.

The original comparison call at 0x11bbf8 implements bytewise comparison;
its source interface remains `strcmp`, without promoting a runtime alias.
All comparison literals and their call offsets were extracted independently
from each original executable, including the German discarded-result call.
The original `fullname` local is a 128-byte array and `rc` has DWARF
fundamental type 8 (signed integer).

Normal whole-unit validation improves the USA 692-byte body from
72.6474% to **98.84393%**, Europe's 756-byte body from 65.994705% to
**98.941795%**, and Germany's 772-byte body from 61.751297% to
**98.96373%**. All other 49 functions retain their scores; this change
adds no exact functions. Fresh GC solo remains 76/76 exact. France has no
enabled packer source profile, so no French match is claimed.

The remaining difference is two extra NOPs in each compiled body. All
158/174/178 non-NOP words agree after masking 51/57/60 source relocation
fields and verifying relative branch destinations by non-NOP ordinal.
Register operands, non-relocation constants, and branch conditions are
unchanged. This is not proof of the masked runtime identities or a compiler
defect. Private evidence is in `build/packer-oct09/platform-original.json`,
`platform-proof.json`, and `platform-raw-proof.json`.


## Asset activation loop

The PS2 inner asset loop in `PKR_SetActive` now uses an explicit `while`
index, advancing it both after normal processing and before the existing
skip. This preserves empty-layer behavior and all callback/flag semantics.
It removes the compiler's separate byte-offset induction register and
restores the original 0x90-byte frame and per-iteration shifted index. The
372-byte target improves from **78.29032% to 90.10753%** in all three debug
regions. All other 49 functions retain their scores, with fresh GC solo
76/76 exact. The original still differs in entry-test placement, flag-test
scheduling, and saved-register assignment; no exact or compiler-defect claim
is made. Positive-body nesting and a break-at-top `for` form scored worse,
and moving initialization past the declarations made no difference.
Private whole-unit comparisons are in `build/packer-oct09/active-proof.json`.


## Allocation counter declaration

`g_memalloc_pair` is an ordinary `S32` on PS2. The original DWARF identifies
that name and signed integer type at USA 0x50fa94, Europe 0x50f594, and
Germany 0x50ef94. The original allocation/release helpers interleave this
counter's load/store with the neighboring running-total accesses. Removing
only this counter's unnecessary volatile qualifier recovers that schedule;
reordering the two source updates instead regressed both helpers. The
running-total declarations and the GC declarations are unchanged.

Normal whole-unit checks make `PKR_getmem` **240 bytes, 100%** (from
89.916664%) and `PKR_relmem` **180 bytes, 100%** (from 92.77778%) in all
three debug regions: **+420 exact bytes and two functions** per region.
All other 48 functions retain their scores, and fresh GC solo is 76/76
exact. Complete raw function lengths and words match after only the 11/9
source relocation fields are masked; no branch operands, registers, or
non-relocation constants are ignored. This does not authenticate masked
runtime identities. Evidence is in `build/packer-oct09/counter-proof.json`
and `counter-raw-proof.json`.
