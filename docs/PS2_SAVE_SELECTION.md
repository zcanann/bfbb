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

## Card selection follow-up

The original `zSaveLoad_CardPick` records an unsigned-byte `formatDone`
local. It tracks successful formatting and returns 11 when the subsequent
selection succeeds. The GC source had reused that variable name for UI
indices, losing the PS2 flag and completion result. The restored PS2 path
also skips formatting checks while loading and preserves its distinct
no-games prompt/result handling. UI index expressions follow the same
original conditional-argument form as GameSelect.

Across all three debug regions, CardPick's 1464-byte body improves from
55.273224% to 98.907104%. Only two prompt-loop NOP placements remain different.
`zSaveLoad_CardCheckSpace` also becomes exact (404 bytes) after restoring
the conditional UI index in its inlined prompt. These are the only two
function-score changes; other scores and the full GC report remain unchanged.
Evidence: `build/save-oct09/*-pick.json` and `pick-proof.json`.

## Autosave-status update

The previously missing PS2 `zSaveLoadAutoSaveUpdate` is restored from its
576-byte original body. It polls card status, requests card information
when idle, and selects the original saving/unformatted/no-space/changed/
failure UI. Terminal failure paths disable autosaving and stall the game.
The four persistent polling values (`ps2Result`, `ps2Formatted`,
`ps2CardType`, `ps2FreeSpace`) have named, typed DWARF records in all three
debug executables; their addresses and GP displacements are recorded in
`build/save-oct09/autosave-globals.json`.

Caching the physical slot and returning from completed status cases restores
the original lifetimes and exit branches. The function is exact in USA,
Europe and Germany, adding 576 exact bytes and one function in each. Every
other unit score and the full GC source report remain unchanged. Reports
are `*-auto.json`, with comparisons in `auto-proof.json`.

The SDK calls use the existing `libmc.h` reconstruction of `sceMcSync` and
`sceMcGetInfo`. This change adds no SDK identity records or registry entries.

## Format, space and slot checks

The original format prompt passes conditional UI indices directly into the
event helper. Restoring those expressions lets the compiler reproduce its
original inlining into `zSaveLoad_CardCheckFormatted`. Conversely, the
space-check wrapper calls the separate `CardCheckSpaceSingle_doCheck`
body; a scoped PS2 pragma preserves that original boundary.

The PS2 slot check returns 6 immediately for an unformatted target, recognizes
the corruption label/date pair, and checks startup space before offering an
empty slot. The call is specifically `xSGTgtHaveRoomStartup`, verified against
the original debug-backed callee in all three regions; it is distinct from
the ordinary room check. Lack of startup space returns 10. Restoring the
larger body also reproduces its original call from the slot-check wrapper.

These four functions become exact in each debug region: formatted check
(492 bytes), space-check wrapper (220), slot-check wrapper (248), and slot
check body (308), totaling 1268 bytes. Dispatch also improves from 34.851852%
to 68.111115%. All other scores and the full GC report remain unchanged.
Evidence: `*-checks.json`, `checks-proof.json`, and `slot-startup-calls.json`
in `build/save-oct09/`.

## Overwrite and directory checks

PS2's overwrite check shows the space prompt for result 10 and otherwise
uses the ordinary overwrite prompt. It does not run GC's filename-validity
test to choose a damaged-file prompt. Its 608-byte body is now exact.
The games check uses the game-directory result directly, preserves the
unformatted result 6, and returns immediately when no directory exists;
it does not perform GC's extra free-space check. Its 348-byte body is exact.

The 316-byte formatted-status check also becomes exact after restoring the
DWARF declaration order and its two explicit format-result cases, with zero
handled by the default assignment. The card prompt retains its original
out-of-line boundary and conditional UI arguments, improving from 73.04762%
to 98.09524%; one prompt-loop NOP placement remains different.

All three debug regions gain 1272 exact bytes and three functions. These
four scores are the only changes; the full GC report remains identical.
Evidence: `*-overwrite.json` and `overwrite-proof.json`.
