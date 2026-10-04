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

## SDK stack and arena addresses

The regional SDK/MetroTRK holdouts used USA addresses for the application stack,
debugger stack, and arena. The linker scripts calculate their regional values:

| Symbol | USA | PAL and German compilation |
| --- | --- | --- |
| `_stack_addr`, `_db_stack_end` | `0x803D8A50` | `0x803D8E70` |
| `_db_stack_addr` | `0x803D9A50` | `0x803D9E70` |
| `__ArenaLo` | `0x803D9A60` | `0x803D9E80` |

Startup and MetroTRK's existing assembly now reference the linker symbols.
`OS.c` and `OSThread.c` retain version-specific absolute declarations because
retail embeds those C references as immediate operands. These changes make
`OS.c`, `OSThread.c`, `__start.c`, and `dolphin_trk.c` fully match in both regions.
The expanded 445-unit regional selections and the existing 472-unit USA
selection were each linked and compared byte-for-byte with their retail DOL.

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

The regional Robo goal unit also links exactly with the existing source
selection after restoring its PAL angle constants. Verify the full manifest:
a Robo-only substitution into otherwise extracted objects encounters a duplicate
`__fpclassifyf`; the normal selection already uses the source `math_ppc` object
and coalesces those helper definitions correctly. Both regions retain unchanged full retail checksums.

Regional `zMain` uses 528-line startup/save-card cameras and a 50 Hz vertical
blank rate (including the three-second startup delay and pad timestep). These
constants recover all three regional code holdouts and the data pool. The full
source selection including `zMain` reproduces both retail DOLs exactly; USA
keeps its 480-line/60 Hz behavior and its entire progress report is unchanged.

The regional Villager glyphs rotate by 3.6 degrees per frame instead of 3;
King Jelly and NPC supplement effects use a 50 Hz frame period instead of
60 Hz. Their source units reproduce retail exactly together with the existing
selection. Regional scene updating also includes the display-offset adjustment
controls and applies those offsets before rendering. Two pre-existing
holdouts still prevent marking the entire unit complete; the recovered update
and pre-render functions match.

## Camera, font, and particle timing

Direct USA/PAL object comparisons also establish these regional differences:

- Camera damping and orientation interpolation use the regional fixed step.
  `0.425f / (1.0f / 50.0f)` preserves PAL's `21.250001907` interpolation rate;
  rounding it to `21.25f` would change retail data. The same expression at 60 Hz
  reproduces USA's `25.5f`.
- Font drawing scales normalized rectangles to 528 screen lines in PAL and
  480 in USA. Texture-size normalization still uses 480 in both versions.
- Sparkle/steam animation advances every two frames (1/25 second in PAL,
  1/30 in USA), and bubble damping uses 50 or 60 frames per second.

With these constants restored, `xCamera`, `xFont`, and `zParPTank` have 100%
code and data matching in both regional reports. Adding all three to the
450-unit regional source selection reproduces both retail DOLs exactly.
USA's allocated object sections, complete report, and retail link are unchanged.


## Regional menu, UI, and display offset

The regional source selection now contains 456 units. The combined selection
including `iDraw`, `xCamera`, `xFont`, `zParPTank`, `zMenu`, and `zUI` reproduces
both complete retail DOLs exactly.

PAL menu timing uses 50 Hz and its camera is 528 lines tall. The title-menu
music-update path also sends Invisible to `mnu3 black card`. UI textures scale
480-line asset coordinates to 528 display lines; model rectangles keep their
480-line normalization. The full link check verifies these constant values and
pool order, which the object-level score alone does not guarantee.

`iDrawSetDisplayOffset` is PAL-only. It clamps the VI origins to the retail
ranges and calls `VIConfigure`. Its seven constants and alignment tail belong
to `iDraw`, not `iCollide`; the regional metadata generator derives this
additional input-section boundary from the PAL ELF.

`zGame` now has its regional camera dimensions, autosave-card dimensions,
vertical-blank timing, minimum timestep, and soak timer. Its smoothing fallback
and initial frame history still use 1/60 second in PAL, as in retail. All data
and three additional functions match; the existing screen-transition function
holdout still prevents whole-unit completion.


## Regional FMV framebuffer height

The FMV camera, projection, height scaling, and RAD image renderer use a
528-line screen in Europe and German builds, versus 480 lines in USA. A shared
`FMV_SCREEN_HEIGHT` constant restores the values shown in regional retail
instructions and constant pools. The header was already included by both units.

`ngcrad3d` now matches all 1704 code bytes and 96 data bytes in both regions.
Selecting it from source increases each regional manifest to 457 units and
reproduces the complete retail DOL, including the German trailer. `iFMV` data
matching rises from 104/152 to 152/152 bytes; its remaining `Show_frame`
instruction-order holdout matches the USA score of 96.84746%, so that unit stays
unselected. Only these two units change in the full PAL report, with no losses.

All three full source builds and strict retail SHA-1 checks pass. The complete
USA report is unchanged. Verification artifacts are under
`build/regional131/` in the RGB worktree.

## Remaining NPC timing and complete surface/OOB units

Original relocation users establish three further regional timing differences:
NPC vibration lasts three fixed frames, hazard spin/collision updates use the
regional frame period, and Dutchman's particle rates use its reciprocal.
Hazard's collision lookahead is thirty frames; the PAL product rounds to
`0.599999964f`, not the separate `0.6f` literal already used elsewhere.

The corrected Common and Dutchman units now have fully matching regional data.
Hazard's small-data section improves from 97.39884% to 98.85057%; its existing
unused `xsqrt` literal/alignment residue remains. These three units retain
shared code holdouts and are not promoted merely because timing data improved.

Surface oscillation uses 1/50-second frame counts in PAL. Textbox backgrounds
and the OOB fade use 528 screen lines, and OOB horizontal motion uses the
corresponding `640.0f / 528.0f` aspect ratio. `zSurface`, `zTextBox`, and
`zEntPlayerOOBState` now have fully matching code and data; all three together
with the previous 456-unit selection reproduce both complete regional DOLs.
All six changes leave USA allocated object sections and its full report
unchanged; its source-selected retail DOL also remains exact.
