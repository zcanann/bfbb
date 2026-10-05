# PS2 and Xbox builds and progress

GameCube matching builds remain on `configure.py`. PS2 and Xbox use
`tools/platform_progress.py` because their executable formats, compilers, and
relocations differ. This includes verified inputs, analysis infrastructure, and compiled PS2/Xbox
source comparisons. Function recovery and retail executable links remain incomplete. The reporting
scope is described below.

## Verified originals

Every supplied disc matches the official Redump size, CRC32, MD5, and SHA-1.
The complete values and reference provenance are in
[`config/platforms/versions.json`](../config/platforms/versions.json).

| Version | Release | Public record | Private input |
| --- | --- | --- | --- |
| SLUS-20680 | PS2 USA | [Redump 32423](http://redump.org/disc/32423/) | `orig/SLUS-20680/boot.elf` |
| SLES-51968 | PS2 Europe/Australia **v1.01** | [Redump 82840](http://redump.org/disc/82840/) | `orig/SLES-51968/boot.elf` |
| SLES-51970 | PS2 Germany | [Redump 14785](http://redump.org/disc/14785/) | `orig/SLES-51970/boot.elf` |
| SLES-53623 | PS2 France | [Redump 26550](http://redump.org/disc/26550/) | `orig/SLES-53623/boot.elf` |
| XBOX-US | Xbox USA | [Redump 49757](http://redump.org/disc/49757/) | `orig/XBOX-US/default.xbe` |
| XBOX-EU | Xbox Europe | [Redump 25597](http://redump.org/disc/25597/) | `orig/XBOX-EU/default.xbe` |

The private `zcanann/bfbb-build` repository/image holds only the boot executables,
plus the existing GameCube inputs. Full disc images and game assets are not added.
The original PS2 debug sections are retained for symbol recovery. The public
repository contains hashes, tools, and address/name metadata, never binary payloads.

```sh
# Recheck expected disc hashes against the current official public DATs.
python tools/platform_progress.py verify-references

# Scan per-version subdirectories, hash each entire disc, then extract only its
# boot executable. Filenames are not trusted as version identities.
python tools/platform_progress.py extract --iso-dir orig

# Or extract one selected version from a directory containing all six images.
python tools/platform_progress.py extract --iso-dir orig --version SLUS-20680

# CI uses the already extracted, hash-locked originals from the private image.
python tools/platform_progress.py verify
python -c "from pathlib import Path; Path('build/tools').mkdir(parents=True, exist_ok=True)"
python tools/download_tool.py objdiff-cli build/tools/objdiff-cli --tag v3.7.1
# Required for Xbox boundary validation outside the private image.
python -m pip install capstone==5.0.7
python tools/platform_progress.py report
```

On Windows the downloader/output argument is `build/tools/objdiff-cli.exe`.
Use `--orig-dir`, `--build-dir`, or `--objdiff` to override paths. Generated target
objects and extraction outputs contain original bytes and stay in ignored directories.

## CI and reporting

The Build workflow checks all nine releases. Each PS2/Xbox job authenticates its
original, builds the supported shared-source units, runs pinned objdiff 3.7.1,
and uploads `<version>_report/report.json` for decomp.dev. The private images
supply original executables and compilers; public artifacts contain JSON only.

The standard reports use full verified **code-region byte denominators**, including
unresolved function ranges and original alignment. Actual objdiff function scores
supply every match; unresolved bytes contribute zero. Function counts still cover
only recovered bounds, clearly identified by the "Known functions only" category.
No translation unit or retail link is marked complete on these platforms yet.

`<version>_functions` retains the original function-only objdiff baseline for
analysis. `<version>_baseline` contains original verification, symbols, splits,
region/proof metadata and `section-coverage.json`, which explains the aggregation.
The standard report is a schema-v2 aggregation adapter: objdiff 3.7.1's generator
counts only symbol-covered code bytes, so it cannot include symbol-free gaps in
its denominator. The adapter includes those authenticated bytes without creating
fake functions, changing function scores, or counting comparison-container padding.
It removes percentage fields with empty denominators. Unit/category sums are
checked, and the standard objdiff parser accepts the result.

| Version | Full code-region bytes | Exact source-match bytes |
| --- | ---: | ---: |
| SLUS-20680 | 2,978,560 | 1,616 |
| SLES-51968 | 2,979,712 | 1,616 |
| SLES-51970 | 2,976,512 | 1,616 |
| SLES-53623 | 2,979,968 | 1,276 |
| XBOX-US | 1,798,760 | 48 |
| XBOX-EU | 1,798,760 | 48 |

PS2 reports partition the original load into CPU text, VU upload packets and
initialized data, plus independently proven runtime BSS. Original linker/VU
DWARF anchors, every known function/data anchor, all 41 DMA/VIF packets and the
startup zeroing loop corroborate this layout. The complete VU packet block is
unique and byte-identical in France, establishing its stripped boundaries too.
The 173,504 packet-storage bytes are data in the CPU executable; they do not count
as matching R5900 code. Each release has 1,109,120 runtime BSS bytes. The original
load's virtual end equals BSS start; file offsets include the ELF header and must
not be mistaken for virtual addresses.

Xbox reports cover the authentic `.text` section and initialized `.rdata`/`.data`.
Mixed XDK sections, embedded images/audio and virtual zero-fill remain explicitly
outside this report's scope. Category names identify that exclusion.
GameCube's three reports, badges and main-only Pages deployment are unchanged.

The narrower function-only baselines currently contain:

| PS2 baseline | Functions | Measured function bytes | Source matches |
| --- | ---: | ---: | ---: |
| USA | 5,391 | 2,107,460 | 17 functions / 1,616 bytes |
| Europe/Australia | 5,392 | 2,108,700 | 17 functions / 1,616 bytes |
| Germany | 5,394 | 2,105,512 | 17 functions / 1,616 bytes |
| France (reviewed and corroborated bounds) | 326 | 89,856 | 12 functions / 1,276 bytes |

The `address-anchors.json` registries also recover over 2,500 named data addresses
and 628 function declarations in each debug-bearing version. Addresses do not
imply object sizes, and declaring source files do not establish definition ownership.
Function declarations are distinguished from data even when MW encodes them with
a global-variable tag.

The adjacent `data-extents.json` registries recover 733 nonoverlapping object
ranges per debug-bearing release using explicit concrete-type `AT_byte_size`
attributes: 26,208 file-backed bytes and 120,832 zero-fill bytes. Primitive,
pointer, and array sizes are not inferred. `_rwDMAFlipData` has conflicting
976-byte and 816-byte declarations and remains unresolved. These are partial
layout records, with no definition-TU ownership or matched-data claim.

The committed `config/platforms/<version>/symbols.json` files contain addresses
and sizes, with names and source ownership where established. Anonymous Xbox
extents explicitly have no original name or source ownership. Adjacent `splits.json` files partition
every file-backed load range into known functions, explicitly sized data, and
unclassified gaps. CI
regenerates and compares these registries before accepting a baseline.

These counts come from explicit retail DWARF1 function bounds, with overlap and
load-range validation. Targets retain the exact original instructions. The `xBase` call relocations are restored and verified by inverse reconstruction;
other target objects are not relocation-restored link inputs. Code outside those function ranges, remaining data,
and padding remain unclassified; the whole mixed load segment is not counted as
code. France is stripped; sixteen individually reviewed extents and 310
machine-corroborated extents establish its function-only baseline. Its
1,276/89,856 matched bytes describe that subset. The published code denominator
is the full recovered CPU text region, not this function-only subset.

Both Xbox releases have identical payloads in all 13 sections; their 532 differing
bytes are in headers/certificates. Each retains its own full-executable hash.
The `.text` inventory is 1,798,760 bytes, with 242,996 initialized `.rdata`/`.data`
bytes. Mixed SDK sections require further classification. XBE executable flags
cannot distinguish code here: `.rdata` and `.data` also have that flag. No fake
whole-section function is created to make a progress denominator.

## Compiled PS2 units

The unchanged shared `src/SB/Core/x/xBase.cpp` compiles with the hash-pinned
`mwcps2-3.0b38-030307` profile under unmodified Wibo 1.2.0. All five functions
(276 bytes) match all four PS2 releases. Compiler 3.0.1b74 is an independent
negative control: its Save/Load bodies differ, leaving only three exact functions.
This establishes a working profile for this TU, not the compiler for every SDK.

The same profile compiles the full shared `xordarray.cpp` for all four releases. Seven of eight functions match (1,000 of 1,368 code bytes). PS2-specific
MAX expressions and an indexed search loop preserve GameCube's existing build.
`XOrdSort` remains nonmatching. Its source comparison stays in the report.

`xRMemData.cpp` adds five exact functions (340 bytes) in the three debug-bearing
releases, using unchanged function bodies. Retail DWARF identifies PS2 `size_t`
as unsigned int; the corresponding platform typedef and minimal standard header
fix leave the earlier compiled units byte-identical. The `memset` call target is
independently identified from its byte-fill semantics and nonzero-value callers;
its authentication span does not add library-function progress.

```sh
python tools/verify_ps2_runtime.py
```

The same profile compiles the entire unchanged `xBehaviour.cpp`: `GetOwner`,
`Clear`, `PreCalc`, `EvalRules` and `Process` match all four releases (184 bytes).
Only the PS2 header dependency changes: `xScene` is forward-declared because
this header uses it solely as a pointer. GameCube retains the original include.
These five code bodies require no relocation restoration. France identities and
boundaries are independently corroborated by the unique, identical 220-byte
retail neighborhood in all three DWARF-bearing originals, individual named
extents, return delay slots and padding. `verify_reviewed.py` rechecks that proof.
Compiler-created RTTI, vtables and weak bodies remain outside this function
comparison; the unit is not marked complete or retail-link verified.

The unchanged `xBehaveGoalSimple.cpp` compiles as a whole unit and matches all
eight original code bodies (680 bytes) in the three debug-bearing releases.
Its profile covers the five callbacks, factory constructor/destructor and type
registration; no nonmatching factory code is omitted. Original DWARF identifies
all external calls, callback addresses and three vtable addresses. Five call
relocations and eight HI16/LO16 pairs per version reproduce the retail bytes.
The opt-in interleaved-pair validator checks each intervening instruction's
register reads/writes and incoming direct control-flow edges using immutable
retail bytes. A verified JAL immediately before LO is allowed because LO executes
in its delay slot; carrying the high half in `$ra` is rejected. Unsupported
instructions or other control transfers fail validation. The stripped France
version remains excluded from this profile pending independent data identities.
Matching these code bodies does not establish complete RTTI/vtable ownership,
exception metadata or an executable link, so completion remains false.

`xVolume.cpp` also matches all five functions (128 bytes) in the three
debug-bearing releases with unchanged source bodies. Its PS2 header path uses
an `xCollis` forward declaration and standard math API declarations; the existing
PowerPC-only `xsqrt` implementation remains excluded on PS2, where the shared
function is declared but not implemented here. This unit emits no math calls:
its only external references are `xBaseInit`, `xBaseReset`, `xBaseSave` and
`xBaseLoad`, independently named in retail DWARF. The latter three use verified
MIPS tail jumps. Original GameCube include paths and inline bodies are unchanged.
No RenderWare layouts or replacement math implementations are introduced.

`config/platforms/ps2-toolchain.json` records archive/binary hashes, flags, and
explicit symbol/call mappings. Three `R_MIPS_26` call relocations per version are
restored only after identifying retail serializer targets independently. Reapplying
each destination reproduces its original instruction. `xordarray` adds three
validated calls and one `R_MIPS_GPREL16` access: the retail ELF `.reginfo` supplies
GP, and DWARF independently identifies `gActiveHeap`. Inverse reconstruction
checks its signed offset before restoring the relocation. Source objects are never
used to infer target bytes. Original code stays private.

To run the source comparison on Linux (or inside WSL), with the private image's paths:

```sh
python tools/platform_progress.py report --ps2-compilers /ps2-compilers --wibo /usr/local/bin/wibo-ps2
```

PS2 region verification authenticates all four originals; keep all four boot
executables under `orig/` when generating a standard report outside CI.
Use local compiler/runtime paths when outside the image. Windows native compiler
invocation is not the validated runtime; Wibo avoids requiring a separate native
license configuration. Without these options, the command produces target-only
baselines. `complete` remains false: `.exceptix`, `.mwcats`, full layout, and retail
executable relinking have not been reconstructed.

## Symbol recovery candidates

France's `symbol-candidates.json` contains 694 unique, aligned full-function byte
matches (164,824 bytes) against the debug-bearing PS2 releases. 565 have support
from all three references. These inherited names/ranges need boundary confirmation;
unresolved candidates remain separate from the confirmed registry and progress.
Sixteen extents have been reviewed independently in `reviewed-functions.json`.
A further 237 extents (60,028 bytes) are in `corroborated-functions.json`: each
has an authenticated, uniquely occurring named reference body, a direct JAL
entry witness reached by static control flow from the stripped executable entry,
and closed local control flow with balanced stack and return-address handling.
CI regenerates these proofs from the originals; static reachability is not a
claim that a path executes in game. Both sets enter the partial France baseline. A further 73 functions (27,620
bytes) in `relocation-corroborated-functions.json` preserve every non-transfer
bit and prove each changed J/JAL destination using internal offsets or an
acyclic chain to already identified callees. These are boundary/identity proofs,
not compiled-source matches. Serializer entry
and allocator identities used for call relocations are in `reviewed-call-targets.json`.
France's `reviewed-data-anchors.json` records the `gActiveHeap` address from named
reference declarations and actual GP-relative accesses. Startup code proves its
runtime-cleared BSS range; the stripped ELF's load header omits that memory, so
this evidence does not invent an ELF section.
CI rechecks reviewed evidence and regenerates candidates using all four authenticated originals:

```sh
python tools/platforms/verify_reviewed.py
python tools/platforms/france_candidates.py --check
```

The Xbox analysis uses standard Ghidra x86 analysis seeded at the original entry
point, with the original sections mapped at their actual addresses. Its candidate
functions begin as candidates, separate from confirmed symbols and reporting denominators.
The initial Ghidra 11.0.1 inventory contains 4,722 candidates covering 1,451,763
decoded instruction bytes; discontiguous bodies retain their individual ranges.
Both region registries keep independent executable identities.
See the parameterized scripts in `tools/platforms/ghidra/`. No Xbox SDK is required
for this analysis step.

`verified-anonymous-functions.json` promotes 2,515 disjoint extents (627,372
bytes) per release after Capstone 5.0.7 re-decodes closed control flow, verifies
all body bytes are reachable, checks an incoming direct call from another
closed function, and excludes foreign interior transfers across the candidate
inventory. Reviewed extents take precedence. CI regenerates this registry from
the original; the remaining candidates stay excluded. Anonymous identifiers
establish neither original symbols nor source ownership. Together with the
three reviewed functions, measured coverage is 2,518 functions / 627,523 bytes;
only 48 bytes match source. This is still partial coverage.

Three hash functions have been independently reviewed in both Xbox releases:
`xStrHash(const char*)`, its bounded overload, and `xStrHashCat`, totaling 151
extent bytes. Their CFG, caller arguments, suffix strings, and signed-byte fold
support their identities. These bounded functions have explicit i386 COFF symbols
and sizes; they do not promote the remaining analyzer candidates. The actual full shared `xString.cpp` compiles with a pinned MSVC 7.1 candidate
profile under Wine, followed by LTCG linking. Its plain hash function matches
all 48 bytes in both Xbox releases; the other two functions remain nonmatching.
The same leaf also matches with MSVC 7.0, so exact retail compiler identity is
not established. Xbox's signed-byte fold is platform-scoped; GameCube retains
its existing unsigned-byte behavior.

Source boundaries come from reachable decoded control flow in the linked PE,
starting at the compiler's MAP symbols. Original target sizes are not used to
truncate source code. A small host link context retains the functions; its entry,
CRT support and floating-point marker are excluded from code/data coverage.
No complete TU or retail executable link is claimed. The private `:xbox` image
contains the pinned compiler/runtime and Capstone 5.0.7; its Wine prefix is
isolated on Linux storage.

```sh
python tools/platform_progress.py report --version XBOX-US --xbox-compilers /xbox-compilers --wine /usr/lib/wine/wine
```

On Windows, use the same compiler directory without `--wine`.

```sh
python tools/platforms/verify_xbox_reviewed.py
```

### Xbox shared-source compile coverage

A native MSVC 7.1 inventory of the 62 core C++ files smaller than 16 KB
compiles eight complete translation units with the existing Xbox headers:
`xMath2`, `xString`, `xCurveAsset`, `xordarray`, `xBase`, `xSurface`,
`xRMemData`, and `xFactory`. The last four became compilable after separating
save-game API types from platform-dependent save-game storage, using the
compiler's standard `<new>` header on Xbox, and spelling one binary mask in
hexadecimal for this older compiler. No Xbox timer or save-game ABI is assumed.

These additional objects are compile coverage, not newly matched code. Actual
LTCG link attempts still require the real serializer and memory-manager
implementations: `xSerial::Read_b1`/`Write_b1`, `gActiveHeap`, and allocation
routines. They are not replaced with stubs. All 224 GameCube game/engine source
objects compile after the include cleanup, with allocated section bytes and
sizes identical to the previous build.

## Next implementation work

- PS2: recover remaining code/data and relocation ownership; expand compilation
  beyond xBase/xordarray/xRMemData and validate toolchain profiles against additional source objects.
  The ELF comment identifies the MW MIPS compiler family, but its `2.4.1.01`
  stamp alone does not establish a particular toolchain distribution.
- PS2 France: recover independent boundaries from its stripped executable.
  Region names or similar file sizes are not evidence for copying another map.
- Xbox: recover function/data boundaries, restore i386 COFF relocations, and
  identify the retail compiler. Linked XDK libraries report 5558 QFE 1; this does
  not prove an exact MSVC compiler version.
- Expand Xbox reporting to mixed SDK sections once their code/data ownership is
  established. Only mark source linked after retail reconstruction is verified.

Useful established references: [PS2 split/build/report pipeline](https://github.com/denzi-gh/crashwoc-decomp-ps2/blob/main/docs/pipeline.md),
[MW PS2 build rules](https://github.com/crowded-street/3s-decomp/blob/main/Makefile),
[objdiff MIPS support](https://github.com/encounter/objdiff/blob/v3.7.1/objdiff-core/src/arch/mips.rs),
[objdiff i386 COFF support](https://github.com/encounter/objdiff/blob/v3.7.1/objdiff-core/src/arch/x86.rs),
[objdiff symbol-based report accounting](https://github.com/encounter/objdiff/blob/3cebee67667d440fa0fe1b64026c35db53ac40d6/objdiff-cli/src/cmd/report.rs#L288),
and [decomp.dev multi-platform versions](https://github.com/encounter/decomp.dev/issues/31).
