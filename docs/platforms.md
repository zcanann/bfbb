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

| Version | Full code-region bytes | Objdiff code-matched bytes |
| --- | ---: | ---: |
| SLUS-20680 | 2,978,560 | 224,800 |
| SLES-51968 | 2,979,712 | 224,492 |
| SLES-51970 | 2,976,512 | 224,388 |
| SLES-53623 | 2,979,968 | 44,104 |
| XBOX-US | 1,798,760 | 14,148 |
| XBOX-EU | 1,798,760 | 14,148 |

The PS2 [math alignment restoration](PS2_MATH_ALIGNMENT.md) enables complete
`xEntMotion`, `xPad`, and `xClimate` source comparisons: 34 functions / 20,040
original bytes, adding 1,224 code-matched bytes per debug version.

The subsequent [grid and drive comparisons](PS2_GRID_DRIVE.md) add 16 functions /
11,412 original bytes, including 876 independently reconstructed code bytes.

The [collision SDK foundation](PS2_COLLISION_SOURCE.md) adds the complete
36-function / 36,648-byte collision TU, with 2,236 code-matched bytes per debug
version. Its remaining functions retain their partial scores.

The complete [scene source comparison](PS2_SCENE.md) adds 24 functions / 12,384
original bytes, with 2,092 code-matched bytes per debug version.

This checkpoint includes utility, serializer, bounds, streaming, environment, light-kit,
move-point, fog, behavior-manager, binary-reader, event, volume and conditional source comparisons, plus comparisons for independently verified French function
subsets. Reports retain
standard objdiff `functionRelocDiffs=none`, as on GameCube. Code matches do not
prove relocated byte equality or a completed link; independent reconstruction
results are documented separately in the per-unit notes.

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
| USA | 5,391 | 2,107,460 | 1,490 functions / 224,800 bytes |
| Europe/Australia | 5,392 | 2,108,700 | 1,489 functions / 224,492 bytes |
| Germany | 5,394 | 2,105,512 | 1,488 functions / 224,388 bytes |
| France (reviewed and corroborated bounds) | 632 | 229,352 | 231 functions / 44,104 bytes |

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
code. France is stripped; independently reviewed and machine-corroborated
extents establish its 632-function, 229,352-byte function-only baseline. Its
44,104 matched code bytes describe that subset. The published code denominator
is the full recovered CPU text region, not this function-only subset.

Both Xbox releases have identical payloads in all 13 sections; their 532 differing
bytes are in headers/certificates. Each retains its own full-executable hash.
The `.text` inventory is 1,798,760 bytes, with 242,996 initialized `.rdata`/`.data`
bytes. Mixed SDK sections require further classification. XBE executable flags
cannot distinguish code here: `.rdata` and `.data` also have that flag. No fake
whole-section function is created to make a progress denominator.

## Compiled PS2 units

The full sound TU now compares 37 functions / 6,764 bytes, adding 15 exact
functions / 1,388 bytes after restoring the original PS2 platform types. The
19-function / 13,116-byte emitter TU also has a complete partial comparison
(50.90% fuzzy matching, no exact functions yet). See [sound](PS2_SOUND.md) and
[particle geometry](PS2_PARTICLE_GEOMETRY.md). France now compares the full
streaming-service TU through [original-only sequence recovery](FRANCE_STREAMING_RECOVERY.md),
adding 14 exact functions / 2,820 bytes. Its complete particle-command TU is also
identified by [original callback-table and sequence evidence](FRANCE_PARTICLE_COMMAND_RECOVERY.md),
adding another 15 exact functions / 2,780 bytes while retaining all 30 functions.
[Original animation sequence recovery](FRANCE_ANIMATION_RECOVERY.md) now adds
17 exact functions / 3,032 bytes, retaining all 34 animation functions.
[Entity-motion sequence and dispatch recovery](FRANCE_MOTION_RECOVERY.md) adds
five exact functions / 956 bytes while retaining the full 20-function unit.
[Math sequence and named-global recovery](FRANCE_MATH_RECOVERY.md) expands
France's math comparison from 12 to all 35 functions, adding eight matches / 728 bytes.
[Collision sequence, callback and typed-global recovery](FRANCE_COLLISION_RECOVERY.md)
adds eight matches / 2,236 bytes while retaining all 36 functions / 36,648 bytes.
[Entity sequence and typed-array recovery](FRANCE_ENTITY_RECOVERY.md) adds
23 matches / 3,580 bytes while retaining all 48 entity functions / 19,036 bytes.
[Clump-collision and spline sequence recovery](FRANCE_GEOMETRY_RECOVERY.md) adds
seven matches / 2,712 bytes across both complete units, retaining 24 functions / 17,412 bytes.
[Common and Standard NPC goal sequence recovery](FRANCE_NPC_GOAL_RECOVERY.md) adds
29 matches / 4,320 bytes while retaining all 59 functions / 18,884 bytes. Their
identities rely on unique original sequences and complete vtable/global evidence;
no rooted execution claim or missing NoManLand class layout is inferred.

The complete animation TU now compares all 34 functions / 17,656 bytes,
matching 17 functions / 3,032 bytes with the unchanged shipping function bodies.
See [animation and runtime-header evidence](PS2_ANIMATION.md).

The unchanged 30-function particle-command unit now has a complete source
comparison, initially matching 15 functions / 2,780 bytes. Its remaining
functions retain partial scores. See [particle commands](PS2_PARTICLE_COMMANDS.md).

The complete camera-marker unit now matches all four functions / 220 bytes.
Entity header separation also enables seven exact group functions / 440 bytes
and eleven particle-effect functions / 712 bytes. The three units retain all
original functions in their comparisons; their remaining holdouts and unlinked
status are explicit. See [camera markers](PS2_CAMERA_MARKER.md),
[groups](PS2_GROUP.md), and [particle effects](PS2_PARTICLE_EFFECTS.md).

The release `xDebug.cpp` matches all six original functions / 80 bytes in each
debug-bearing version. Its font-only GameCube helper is excluded on PS2; the
six shipping function bodies are unchanged and independently reproduce every
original byte. The camera-tweak unit adds 11 exact functions / 1,444 bytes,
and the particle manager adds two / 76 bytes. Their remaining functions stay
in the comparison as unmatched code. See [camera evidence](PS2_CAMERA_TWEAK.md)
and [particle-manager evidence](PS2_PARTICLE_MANAGER.md). France now also compares nine previously verified functions from eight complete
source units, adding 192 exact bytes. See [French source coverage](FRANCE_ADDITIONAL_SOURCE_PROFILES.md).

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

`verified-anonymous-functions.json` promotes 2,441 disjoint extents (613,110
bytes) per release after Capstone 5.0.7 re-decodes closed control flow, verifies
all body bytes are reachable, checks an incoming direct call from another
closed function, and excludes foreign interior transfers across the candidate
inventory. Reviewed extents take precedence. CI regenerates this registry from
the original; the remaining candidates stay excluded. Anonymous identifiers
establish neither original symbols nor source ownership. Together with the
115 reviewed extents, measured coverage is 2,556 functions / 635,415 bytes;
14,148 bytes match source. This is still partial coverage. The [Xbox memory initializer](XBOX_MEMORY_INITIALIZER.md) adds 103 exact source bytes using an explicitly verified data-only original binding; its recovered runtime allocator remains target-only. The [inline math boundary](XBOX_MATH_INLINE_BOUNDARY.md) adds 388 exact source bytes from Euler and the emitted xatan2 helper without changing the original denominator. The original-backed box-containment return structure adds another 84 bytes, the vector normalizer adds 163 bytes by preserving the original input-field reloads, direct axis-rotation subtraction adds 226 bytes, and ordinary random-generator inlining adds 661 bytes, and unchanged particle-list helper inlining adds 222 bytes. Ordinary allocator inlining adds another 209 bytes while preserving its existing standalone comparison in the actual particle-group source emission; that reporting group is explicitly reconstructed, not original TU ownership. Ordinary quick-cull wrapper inlining adds 193 exact bytes; all eight reviewed QuickCull functions now match, without claiming complete original TU coverage. Literal fixed-axis initialization adds 158 byte-exact X/Y/Z rotation bytes; original acceleration timing lifetime adds another 83 bytes.
The stopping routine now captures old velocity at the original update boundary, improving its partial score
from 75.53% to 92.69% without changing exact-byte totals. The KillSlow callback gains 164 exact bytes
by acquiring the particle-list root at the original boundary after command setup.

The complete particle-group source now compiles with its real particle dependency
and a hash-pinned Microsoft static runtime library. Six independently reviewed
functions / 880 bytes match in this reconstructed reporting group. The runtime helper is
excluded from reconstructed-source gains. See [particle groups and runtime
provisioning](XBOX_PARTICLE_GROUP.md). Original TU ownership/completeness remains unproven.
The complete particle-command TU also compiles with five real math dependencies.
Its original registration table identifies 21 nonempty callbacks / 4,771 bytes;
eight add 1,831 independently reconstructed bytes in both releases. All partial
callbacks remain compared, and existing source matches remain unchanged.

Three hash functions have been independently reviewed in both Xbox releases:
`xStrHash(const char*)`, its bounded overload, and `xStrHashCat`, totaling 151
extent bytes. Their CFG, caller arguments, suffix strings, and signed-byte fold
support their identities. These bounded functions have explicit i386 COFF symbols
and sizes; they do not promote the remaining analyzer candidates. The actual full shared `xString.cpp` compiles with a pinned MSVC 7.1 candidate
profile under Wine, followed by LTCG linking. Its plain hash function matches
all 48 bytes in both Xbox releases; the bounded hash and concatenating hash now
match their 55 and 48 bytes as well. See [hash loop evidence](XBOX_HASH_LOOPS.md).
The same leaf also matches with MSVC 7.0, so exact retail compiler identity is
not established. Xbox's signed-byte fold is platform-scoped; GameCube retains
its existing unsigned-byte behavior.

The independently reviewed hexadecimal parser `atox` matches all 111 bytes in both releases from the complete string source unit. See [parser evidence](XBOX_HEX_PARSER.md).

The independently reviewed string comparisons `xStricmp` and `imemcmp` add 219 exact bytes in both releases. Original caller stack cleanup supports the standard `/Gd` setting; prior matches remain intact. See [comparison and compiler-profile evidence](XBOX_STRING_COMPARE.md).

Both independently reviewed tokenizers, `xStrTok` and `xStrTokBuffer`, now match all 491 original bytes from the complete shared string source. Prior matches and recovered coverage remain unchanged. See [tokenizer evidence](XBOX_TOKENIZERS.md).

The 53-byte `icompare` also matches. Its decoded direct call resolves to the independently reviewed `imemcmp`; actual source MAP identities and explicit COFF REL32 records preserve that relationship. See [substring comparison evidence](XBOX_SUBSTRING_COMPARE.md).

The complete `xPar.cpp` also compiles and links in its own host context after
using its direct vector-header dependency on Xbox. Its independently reviewed
96-byte initializer compares at 88.030304% in both releases. This replaces an
existing anonymous identity without changing recovered coverage or adding an
exact function. See [particle evidence](XBOX_PARTICLE_INIT.md).

The Xbox pool initializer and allocator add two exact functions / 86 bytes.
Their actual PE base relocations and named source globals establish six DIR32
operands; reapplying independently reviewed original global addresses reproduces
every retail byte in both releases. The pool initializer was newly recovered;
the allocator replaces an anonymous identity. The GameCube-specific volatile
workaround remains enabled on GameCube and PS2. See
[relocation evidence](XBOX_PARTICLE_RELOCATIONS.md).

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
  using the [batch compile inventory](PS2_COMPILE_INVENTORY.md), prioritizing shared
  RenderWare/platform header blockers and validating additional source objects.
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

The complete Xbox matrix source compares 30 reviewed functions / 5,164 bytes,
including 14 exact functions / 2,114 bytes. See [matrix evidence](XBOX_MATH3.md).

PS2 complete hangable and combo source comparisons add 1,136 matched bytes
per debug region. See [gameplay evidence](PS2_HANGABLE_COMBO.md). Xbox geometry
adds 2,485 bytes per version; see [geometry evidence](XBOX_GEOMETRY.md).

The complete PS2 entity source adds 3,580 bytes per debug region; see
[entity evidence](PS2_ENTITY_SOURCE.md). Xbox quick-cull adds 974 bytes per
version; see [quick-cull evidence](XBOX_QUICK_CULL.md).

The PS2 [complete allocator declarations](PS2_RW_ALLOCATORS.md) enable spline,
INI, update-culling and clump-collision comparisons: 22,568 bytes compared and
3,032 additional matched bytes per debug region.

Four further complete PS2 gameplay units add 1,600 bytes per debug region;
see [gameplay object evidence](PS2_GAMEPLAY_OBJECTS.md). Xbox math adds 702 bytes
per release with a verified genuine runtime callee; see [math evidence](XBOX_MATH.md).

Complete PS2 laser and decal comparisons add 1,736 matched bytes per debug
region, all independently raw-exact; see [rendering evidence](PS2_LASER_DECAL.md).

The complete PS2 platform unit adds 1,100 matched bytes per debug region;
see [platform evidence](PS2_PLATFORM.md).

Complete PS2 particle-tank and pickup source comparisons add 3,188 matched
bytes per debug region; see [frame and rendering evidence](PS2_FRAME_PTANK_PICKUP.md).

Xbox `find_char` adds 1,024 matched bytes per release with independently
verified switch destinations; see [string switch evidence](XBOX_STRING_SWITCH.md).

Complete PS2 Robo goal source comparison adds 9,112 matched bytes per debug
region; see [Robo evidence](PS2_ROBO_GOALS.md).

Complete PS2 common and standard NPC goal comparisons add 4,320 matched bytes
per debug region; see [goal evidence](PS2_COMMON_STANDARD_GOALS.md). Xbox utilities
add 532 bytes per release; see [utility evidence](XBOX_UTIL.md).

Complete PS2 model and simple-shadow comparisons add 3,952 matched bytes
per debug region; see [renderer evidence](PS2_MODEL_SHADOW.md).

The complete PS2 NPC common source adds 5,212 matched bytes in USA and 4,904
in Europe/Germany; see [NPC common evidence](PS2_NPC_COMMON.md). Xbox fast
collision adds 260 bytes; see [curve/collision evidence](XBOX_CURVE_FAST_COLLISION.md).

Complete PS2 effects source comparison adds 3,596 matched bytes per debug
region; see [effects evidence](PS2_FX.md).

Complete PS2 Robot source comparison adds 23,392 matched bytes per debug
region. Its corrected original hazard layout also adds 204 bytes in Robo goals;
see [Robot layout evidence](PS2_ROBOT_LAYOUT.md).

Complete PS2 Plankton source comparison adds 5,420 matched bytes per debug
region; see [Plankton evidence](PS2_PLANKTON_SOURCE.md).

Complete PS2 Tiki source comparison adds 768 matched bytes per debug region;
see [Tiki evidence](PS2_TIKI_SOURCE.md).

Complete PS2 screen-effects comparison adds 3,416 matched bytes per debug
region using original Sky2 vertices; see [screen-effects evidence](PS2_SCREEN_EFFECTS.md).

Complete PS2 Tiki goals add 424 matched bytes per debug region; the complete
boss-goal factory is also retained as a partial source comparison.

Complete PS2 SB2 source comparison adds 5,008 matched bytes per debug region;
see [SB2 evidence](PS2_SB2_SOURCE.md).

Complete PS2 Dutchman source comparison adds 2,624 matched bytes per debug
region, with separately verified USA and PAL original profiles.

Complete PS2 Sandy and Prawn comparisons add 10,652 matched bytes per debug
region; see [Sandy render-array evidence](PS2_SANDY_RENDER_ARRAY.md) and
[Prawn platform evidence](PS2_PRAWN_SOURCE.md).

Complete PS2 [player source](PS2_PLAYER_SOURCE.md) and
[Bungee/CruiseBubble states](PS2_PLAYER_STATES.md) add 17,636 matched bytes /
166 functions per debug region. All 429 original functions remain represented;
Germany retains its larger original player function extents.

The complete [King Jelly comparison](PS2_KING_JELLY_SOURCE.md) adds 4,976 matched
bytes / 17 functions per debug region with unchanged source. All 69 original
functions retain their regional extents and partial scores.

Complete PS2 [Villager, Villager goals and Hazard comparisons](PS2_VILLAGER_HAZARD_SOURCE.md)
add 9,288 matched bytes / 65 functions per debug region. All 191 original
functions remain in the comparison; Germany keeps its distinct goal extents.

The complete [Patrick comparison](PS2_BOSS_PATRICK_SOURCE.md) adds 8,300 matched bytes /
35 functions per debug region with only a PS2 render-header include. All 63
original functions remain represented.

Complete [PS2 Supplement comparison](PS2_SUPPLEMENT_SOURCE.md) adds 3,808 matched
bytes / 13 functions per debug region. Authenticated version defines select the
original 50 Hz timing in Europe/Germany; USA retains 60 Hz. The remaining source
profiles and compiler settings are preserved.

The complete [thrown-object, boulder and shrapnel comparisons](PS2_THROWN_BOULDER_SHRAPNEL_SOURCE.md)
add 2,752 matched bytes / 24 functions per debug region without source changes.
All these matches independently reconstruct the original bytes.

The complete [cinematic effects comparison](PS2_CINEMATIC_SOURCE.md) adds 2,544
matched bytes / 21 functions per debug region using only PS2 include selections.
All 77 original functions retain their extents and partial scores.

The complete PS2 camera and lasso source comparisons add 24 matched functions /
3,472 bytes in each debug region, all independently verified against original
bytes after relocation. Unmatched functions remain in both unit totals. See
[camera and lasso evidence](PS2_CAMERA_LASSO_SOURCE.md).

The PS2 TalkBox, DiscoFloor and original goo-rendering paths add another 53
standard matched functions / 6,404 bytes per debug region. See [TalkBox and
DiscoFloor](PS2_TALKBOX_DISCOFLOOR_SOURCE.md), [PS2 goo](PS2_ZFX_SOURCE.md), and
[regional timing](PS2_LASER_KING_REGIONAL_TIMING.md). French comparisons add
17 matched functions / 2,816 bytes using already-confirmed identities; see
[French source coverage](FRANCE_ADDITIONAL_CORE_PROFILES.md).

The complete debug-region [UI source comparison](PS2_ZUI_SOURCE.md) adds 11
matched functions / 2,144 bytes per version. Eight further French source
comparisons add nine matches / 1,236 bytes, keeping all 26 confirmed members
and unknown references: [French effects and camera coverage](PS2_FRANCE_EFFECTS_CAMERA_SOURCE.md).

The original PS2 [OOB rendering path](PS2_OOB_SOURCE.md) adds 12 exact functions /
1,012 bytes for USA and Europe, and 11 / 908 for Germany. All these matches also
reconstruct the original bytes after relocation. Twelve additional French source
profiles add six standard matches / 552 bytes while retaining all confirmed
members and unknown references: [remaining French coverage](PS2_FRANCE_REMAINING_SOURCE.md).

The [ambient/spawner](PS2_AMBIENT_SPAWNER_SOURCE.md) and [NPC support](PS2_NPCSUPPORT_SOURCE.md) comparisons add 51 normal matches / 5,648 bytes per debug PS2 region. The [French grid recovery](PS2_FRANCE_GRID_UI.md) adds one verified 1,984-byte function and enables the three previously known UI members; these improve comparison coverage without new exact matches.

The [remaining NPC and Player animation-table comparisons](PS2_REMAINING_NPC_PLAYER_TABLES.md) add 42 normal matches / 6,432 bytes per debug PS2 version. The six animation-table functions retain their original header ownership and are compared using a complete Player source compilation; their symbols do not overlap the existing Player profile.

Restoring the original [four-voice PS2 stream limit](PS2_SOUND.md#original-stream-voice-limit) adds two standard matches / 488 bytes per debug region. After restoring the original direct platform stop calls, StreamUnlock (72 bytes) and StreamLock (416 bytes) both independently reproduce the original bytes in all three debug versions.

The original [update-cull initializer statement order](PS2_RW_ALLOCATORS.md#update-cull-initializer-instruction-order) adds one standard match / 1,484 bytes per debug PS2 version. Only two adjacent instructions change; all other source instructions, relocations and regional function scores remain unchanged.

The ordinary Xbox world-transform helper boundary improves four partial matrix
and bounds functions while retaining every prior exact match. Its newly reviewed
98-byte original extent was already counted anonymously; its 94-byte source
body honestly scores 97.36842%. See [matrix evidence](XBOX_MATH3.md).

The [French QuickCull recovery](PS2_FRANCE_QUICKCULL.md) adds two initializer matches / 280 bytes, with independent relocated-byte equality. Ten of its eleven recovered functions now match; the remaining packed-byte intersection implementation stays partial. The [PS2 stop-fade loop exit](PS2_SOUND.md) adds one standard match / 396 bytes per debug version; its unresolved platform callee differences remain documented.

The [extend/retract expression](PS2_MATH_ALIGNMENT.md) adds 680 exact reconstructed bytes in all four PS2 versions. The [animation bone-count expression](PS2_ANIMATION.md) adds another 1,616 standard matched bytes per version; its unresolved runtime call remains documented. The [French SKB sequence](PS2_FRANCE_SKB.md) establishes two more original bounds / 1,512 bytes without adding source matches.

[Camera-tweak](PS2_CAMERA_TWEAK.md) now matches all 12 code functions / 1,684 bytes in the three debug PS2 versions, preserving the public Reset function. Four unresolved math-call addresses prevent a raw-link claim. The [complete French FFX comparison](PS2_FRANCE_FFX.md) adds six exact functions / 172 bytes; all eleven matched FFX functions independently reproduce 712 original bytes after relocation.

The [move-point unsigned zero tests](PS2_MOVEPOINT.md) add 620 exact bytes / two functions per debug PS2 version, with independent relocated-byte equality. Eight of the nine original move-point functions now match.

The complete [PS2 platform geometry source comparison](PS2_IMATH3.md) adds eight exact functions / 3,652 bytes in every version; independent relocation reconstruction reproduces all eight original bodies. Its remaining seven functions retain their partial scores. [Jaw evaluation](PS2_MATH.md) adds 308 matched bytes in the three debug versions, bringing that two-function unit to 100% code match while retaining its unresolved runtime call addresses.

Original debug-scope [string scan counters and staged hexadecimal conversion](PS2_STRING_SCANS.md) add 1,848 exact source bytes and two functions per PS2 version. [French string boundaries](PS2_FRANCE_STRINGS.md) now cover all twelve original members, including the independently reconstructed twelve-entry dispatch table. These are function matches, not whole-executable linking claims.

The original PS2 signed angle-clamp return flow adds one 132-byte standard code match per version. Its runtime call destination remains unresolved, so this does not add a raw-byte reconstruction or retail-link claim. Xbox speed-limited acceleration improves from 92.24% to 98.12% through the original arithmetic order; its remaining spill difference stays unmatched.

The PS2 volume initializer uses its original U16 count directly, adding 224 code-matched and independently reconstructed bytes per debug release. The existing French volume subset is unchanged. Xbox particle aging adds another 49 literal byte-matched bytes through the original root-acquisition order.

[French bounds, particle initialization, and update-culling coverage](PS2_FRANCE_BOUNDS_CULL.md) adds ten independently corroborated functions / 3,548 known bytes. The three new update-cull comparisons add 1,620 standard matched bytes; all prior function scores and full executable denominators remain unchanged. The other newly covered bodies retain their measured partial scores.

The original PS2 volume event callback now adds another 316 matched and independently reconstructed bytes per debug version, bringing zVolume to four of five matched functions. Xbox box-sphere intersection improves to 98.82%, and endpoint acceleration to 89.08%; both remain partial comparisons.
