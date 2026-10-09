# PS2 save selection and call boundaries

The original USA, Europe and Germany save-selection routines preserve calls
to `CardtoTgt`, `zSaveLoad_CardCheckSingle`, `zSaveLoad_CardCheck`, and
`zSaveLoad_CardCheckValid`. The latter also calls the separate games/space
checks. Scoped PS2 `dont_inline` pragmas retain these authenticated boundaries;
otherwise the compiler expands several checks into their callers.

`zSaveLoad_GameSelect` also needs these PS2 behaviors:

- Round file sizes to 1 KB blocks and add one block, instead of GC's 8 KB
  rounding and three additional blocks.
- Count empty slots, then select the target again and call
  `iSGIsGameCorrupt(svinst->isgsess, 0)`. When all three slots are empty but
  game files remain, display the original `Corrupt Game File\n\n` label/date
  and enable the first load slot.
- Check formatting only when saving, retain the original card-validity
  prompt/reset flow, and handle slot results 6/10 as cancellation.
- Preserve the wrong-device prompt in Germany. Its original routine is
  2448 bytes; USA/Europe omit that branch and are 2288 bytes.

The original GameSelect DWARF records `done`, `i`, `svinst`, `use_tgt` and
`emptyCount`. The restored code uses that counter and the original branch
conditions. Conditional UI indices passed directly into the event helper
also restore the original temporary lifetimes, including the games check.
All changes are PS2-only; GC source behavior and generated code are preserved.

Validation compares all functions in each regional unit, authenticates the
original helper-call addresses against the debug symbol table and ELF words,
and checks the complete USA GC source report for equality.

| Region | GameSelect before | GameSelect after | Unit exact bytes before/after |
| --- | ---: | ---: | ---: |
| USA | 49.791958% | 99.65035% | 1760 / 2312 |
| Europe | 49.791958% | 99.65035% | 1760 / 2312 |
| Germany | 51.7451% | 99.346405% | 1760 / 2312 |

Each region gains two exact functions: `zSaveLoad_CardCheck` (148 bytes)
and `zSaveLoad_CardCheckGames` (404 bytes). Ten function scores improve and
none regress. USA/Europe GameSelect's remaining difference is the placement
of one NOP within an inlined prompt. No compiler patch is proposed.

The current profile has no France coverage for this unit, so no France gain
is claimed. Private reproducible evidence is in `build/save-oct09/`:
regional before/after reports, `prove.py`, `proof.json`, retained solo builds,
and `GC-after.json`. The full GC build/report check passes unchanged.
