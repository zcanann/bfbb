# PS2 and Xbox bootstrap

GameCube matching builds remain on `configure.py`. PS2 and Xbox use
`tools/platform_progress.py` because their executable formats, compilers, and
relocations differ. This is verified-input and analysis infrastructure; these
platforms do not yet have reproducible source builds or whole-program matching
percentages.

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
python tools/download_tool.py objdiff-cli build/tools/objdiff-cli --tag v3.7.1
python tools/platform_progress.py report
```

On Windows the downloader/output argument is `build/tools/objdiff-cli.exe`.
Use `--orig-dir`, `--build-dir`, or `--objdiff` to override paths. Generated target
objects and extraction outputs contain original bytes and stay in ignored directories.

## CI and reporting

The Build workflow has six additional independent platform jobs. Each checks its
original executable hash, prepares available target objects, runs pinned objdiff
3.7.1, and uploads `<version>_baseline` with status and coverage metadata. The
three debug-bearing PS2 releases also upload `<version>_debug_functions` with an
actual objdiff report. These artifacts are available on staging.

They are deliberately **not** named `<version>_report` yet. That standard name is
reserved for reports ready for decomp.dev ingestion. A partial inventory must not
look like whole-game progress, and objdiff assigns 100% to some empty denominators.
GameCube's three standard reports and main-only Pages deployment are unchanged.

| PS2 baseline | Functions | Measured function bytes | Source matches |
| --- | ---: | ---: | ---: |
| USA | 5,391 | 2,107,460 | 0 |
| Europe/Australia | 5,392 | 2,108,700 | 0 |
| Germany | 5,394 | 2,105,512 | 0 |

The committed `config/platforms/<version>/symbols.json` files contain names,
source ownership, addresses, and sizes. Adjacent `splits.json` files partition
every file-backed load range into known functions and unclassified gaps. CI
regenerates and compares these registries before accepting a baseline.

These counts come from explicit retail DWARF1 function bounds, with overlap and
load-range validation. Targets retain the exact original instructions. They are
not relocation-restored link objects. Code outside those function ranges, data,
and padding remain unclassified; the whole mixed load segment is not counted as
code. France is stripped and initially has no discovered function baseline.

Both Xbox releases have identical payloads in all 13 sections; their 532 differing
bytes are in headers/certificates. Each retains its own full-executable hash.
The `.text` inventory is 1,798,760 bytes, with 242,996 initialized `.rdata`/`.data`
bytes. Mixed SDK sections require further classification. XBE executable flags
cannot distinguish code here: `.rdata` and `.data` also have that flag. No fake
whole-section function is created to make a progress denominator.

## Symbol recovery candidates

France's `symbol-candidates.json` contains 694 unique, aligned full-function byte
matches (164,824 bytes) against the debug-bearing PS2 releases. 565 have support
from all three references. These inherited names/ranges need boundary confirmation;
they are separate from the confirmed symbol registry and excluded from progress.
CI regenerates this metadata using all four authenticated originals:

```sh
python tools/platforms/france_candidates.py --check
```

The Xbox analysis uses standard Ghidra x86 analysis seeded at the original entry
point, with the original sections mapped at their actual addresses. Its candidate
functions also remain separate from confirmed symbols and reporting denominators.
The initial Ghidra 11.0.1 inventory contains 4,722 candidates covering 1,451,763
decoded instruction bytes; discontiguous bodies retain their individual ranges.
Both region registries keep independent executable identities.
See the parameterized scripts in `tools/platforms/ghidra/`. No Xbox SDK is required
for this analysis step.

## Next implementation work

- PS2: recover remaining code/data and relocation ownership; establish the exact
  `mwccps2`/`mwldps2` profile and compile the shared source with platform headers.
  The ELF comment identifies the MW MIPS compiler family, but its `2.4.1.01`
  stamp alone does not establish a particular toolchain distribution.
- PS2 France: recover independent boundaries from its stripped executable.
  Region names or similar file sizes are not evidence for copying another map.
- Xbox: recover function/data boundaries, restore i386 COFF relocations, and
  identify the retail compiler. Linked XDK libraries report 5558 QFE 1; this does
  not prove an exact MSVC compiler version.
- Promote a platform to standard decomp.dev reports once its measured scope is
  established, and only mark linked source complete after retail reconstruction
  has actually been verified.

Useful established references: [PS2 split/build/report pipeline](https://github.com/denzi-gh/crashwoc-decomp-ps2/blob/main/docs/pipeline.md),
[MW PS2 build rules](https://github.com/crowded-street/3s-decomp/blob/main/Makefile),
[objdiff MIPS support](https://github.com/encounter/objdiff/blob/v3.7.1/objdiff-core/src/arch/mips.rs),
[objdiff i386 COFF support](https://github.com/encounter/objdiff/blob/v3.7.1/objdiff-core/src/arch/x86.rs),
and [decomp.dev multi-platform versions](https://github.com/encounter/decomp.dev/issues/31).
