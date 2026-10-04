# Regional builds

`configure.py --version GQPE78|GQPP78|GU4Y78` selects one build at a time,
following the SFA-Decomp workflow. USA remains the default. Ninja inputs,
extracted objects, compiled source objects, maps, and reports are separated by
version under `build/`. The root `build.ninja` and `objdiff.json` describe the
currently selected version.

## Original executables

| Version | Input | SHA-1 |
| --- | --- | --- |
| GQPE78 | `orig/GQPE78/sys/main.dol` | `306526d90b48e99894c3138f5fc8f2716d9fecf6` |
| GQPP78 | `orig/GQPP78/sys/main.dol` | `6da9022f06bfb62a203017ec38046ba2566dc0cf` |
| GU4Y78 | `orig/GU4Y78/files/Game.dol` | `aeda497067bb53e4715db5b074535645a0ea9b3e` |

GU4Y78 is the German compilation's BFBB disc. Its `sys/main.dol` is the THQ
Multipak launcher, which is outside this project's build and progress totals.
The game's code and DOL section layout are identical to GQPP78. Its loaded
payload differs only in the savegame ID string at `0x802669B0`: `GQPP` becomes
`GU4Y`. The compilation file also includes 64 zero bytes after its last section.
The GU4Y78 build preserves that 128-byte file alignment using `tools/pad_dol.py`;
the header and loaded sections are unchanged. Each version retains its own
original, full-file checksum.

## Symbols and ownership

The PAL disc ships `files/sbpeM.elf`, whose loaded sections exactly reproduce
its retail DOL. Its SHA-1 is `9741e0d932a0ce441753843d659ed5371fbdc584`.
The compilation disc includes the same ELF, before the game-ID patch.

`tools/regional_config.py` regenerates regional symbols and splits using the
retail ELFs and the curated USA ownership. It matches ELF file, section, and
symbol identities and verifies ambiguous intervals before translating them.
It does not assume one address delta for the whole executable. It also preserves
the SDK OS BSS section's 32-byte DMA alignment, required by `DriveInfo`'s source
declaration and exposed when linking regional source objects. Both ELF files
are optional local regeneration inputs, not public files or CI requirements:

```sh
python tools/regional_config.py
```

This requires `orig/GQPE78/files/sbgcM.elf`,
`orig/GQPP78/files/sbpeM.elf`, the two regional DOLs, and the downloaded DTK.
Use `--orig-root`, `--output-root`, or `--dtk` to override their locations.
Ordinary builds need only the version's original DOL and checked-in metadata.

## Completion and progress

USA's existing `Matching` flags remain USA-only. A regional
`config/<version>/matching_units.txt` lists configured unit names independently verified
for that executable (the names in `splits.txt`, which may differ from physical
source paths for MSL and MetroTRK). A high objdiff score alone does not establish that a source
object links identically: literal pools, weak symbols, and final relocations
also matter. `tools/verify_source_link.py` checks both a retail-only link and the
selected source substitutions before a unit is added to the manifest. For example,
after building Europe, verify its complete selection with:

```sh
python tools/verify_source_link.py GQPP78 --units-file config/GQPP78/matching_units.txt
```

The normal Ninja build then checks the same source selection against the retail
checksum. Use `--compilers` and `--dtk` with the verifier if those tools were
overridden during configuration.

`ninja all_source progress` builds all source objects, checks the full linked
DOL against the original checksum, and generates a deduplicated report. CI runs
this independently for all three versions and uploads `<version>_report`,
`<version>_metadata`, and `<version>_maps`. Public artifacts exclude original
executables and assets. The private build image carries only the required DOLs.

The progress site has one page per version. The root page and `api.json` remain
USA for compatibility with existing badges. Standard per-version report artifact
names allow decomp.dev to consume each version independently.
