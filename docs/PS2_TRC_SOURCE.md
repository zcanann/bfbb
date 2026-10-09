# PS2 compliance-message source recovery

Restore the PS2 controller-message handlers missing from `xTRC.cpp`, retaining
the existing GameCube and Xbox paths. The complete nine-function PS2 unit now
compares as follows:

| Version | Before | After | Newly exact bytes/functions |
| --- | ---: | ---: | ---: |
| SLUS-20680 | 35.65547% | 78.952736% | 624 / 3 |
| SLES-51968 | 35.65547% | 78.864426% | 624 / 3 |
| SLES-51970 | 35.217445% | 79.12408% | 664 / 3 |

Every previously exact function, other function score, and data control is
preserved. GU4Y78 retains its complete 12 exact functions / 1,436 bytes. France
does not currently select this translation unit in the proven-source profile.

## Recovered behavior

`pad_message_valid` follows the original mode, menu, loading, bus-stop,
out-of-bounds, room, and player-control checks, including their calls to
`zGameStall`. Germany adds a scene-null guard before the HB10 test and a stall
path for control-off flag `0x20`. This helper becomes exact at 468 bytes in
USA/Europe and 508 bytes in Germany.

`DisplayMessage` follows the original 13-entry state dispatch, cutscene stop
condition, and optional standalone-camera rendering. Its strings preserve the
actual regional controller wording: the German release uses a localization
token, Europe and USA use distinct DUALSHOCK strings. Retail's otherwise
unexpected Scooby-Doo and GameCube disc-message wording remains intact.

`xTRCPad` handles only port zero, stores the pad state, coordinates pause and
HUD behavior, and checks the four original autosave-failure entities before
resuming a stalled game. Both this function and `DisplayMessage` retain the
original named zero-initialized color locals, including their emitted byte-zero
loops. Their remaining scheduling/alignment differences stay counted:
`xTRCPad` reaches 97.75281%; `DisplayMessage` reaches 96.77419% in USA,
96.201614% in Germany (Europe's existing report is recorded separately).

`xTRCInit` recovers the original pad-ID store order, becoming exact at 100
bytes. The 56-byte memory-card helper keeps the existing negative-availability
normalization and writes the original two globals, then uses the single PS2
generic message key. The GameCube-specific alternate message selection remains
on its existing platform path.

The original `yellow` object is an `iColor_tag` with bytes `ff e6 00 ff`.
Restore that type on PS2, correcting the little-endian interpretation of the
GameCube packed integer and improving `render_message` from 56.020306% to
61.85279%. Remaining textbox-copy differences are not addressed by this change.

## Original evidence and validation

Private artifacts under `C:/Projects/bfbb-agent-ps2-oct08/build`:

- `trc-originals.py` / `.txt`: every original instruction and named local for
  all three versions.
- `trc-evidence.py` / `.txt`, `trc-layout.py` / `.txt`: original call identities,
  globals, strings, and types. In particular, `zGlobals::player` is at `0x700`,
  its `ControlOff` member at `0x10f8`, `cmgr` at `0x2044`, `sceneCur` at
  `0x2048`, and the suppression flag at `0x6f0`. The cutscene pointer/time/stop
  and 12-byte pad-info layout agree with the current source headers.
- `trc-raw-proof.py` / `.json`: independently relocates actual objects and
  reproduces every byte of all three newly exact functions in each original.
  Calls use original DWARF linkages or the existing reviewed `memset` anchor;
  globals use original locations. The generic text key has a unique original
  loaded-data occurrence, verified independently of the compiled operand.
- `trc-data-proof.py` / `.json`: original state-table case destinations, all
  nine message strings per region, all four autosave names per region, and the
  actual compiled yellow bytes.
- `trc-final-comparison.py`, `trc-final-after.py`, `trc-final-comparison.json`,
  `trc-final-changes.json`: complete before/after source reports against
  `0fc2d45c6`, including the final German behavior.
- `checktrc-gc.py`, `trc-gc-before.json`, `trc-gc-after.json`: complete unchanged
  GameCube comparison.

The stripped RenderWare camera destinations remain unresolved identities.
The source restores their camera operations without registering speculative
SDK names. No header, compiler, comparison rule, target, or evidence registry
was changed. The two larger restored functions remain partial matches.
