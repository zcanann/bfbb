# French Sandy Sit goal

`france_sandy_sit.py` identifies the complete 2,336-byte
`zNPCGoalBossSandySit::Process` body at `0x32db20`. USA, PAL and German original
DWARF must agree on owner, linkage and complete extent. Every instruction,
closed frame/return/control-flow boundary and the unique full-body template
is checked with only the exact reviewed operand masks. No whole-unit claim
is made.

The eight changed data operands follow fully authenticated paths: three
loads of `globals.player.ent.model`, one load of
`globals.player.carry.grabbed`, the second element of `sBone[13]`, the address
of `sNFSoundValue[6]`, and two complete terminated sound-name strings. The
independently proved complete CalcNewDir body pins the full globals object;
original carry/player/entity layouts establish the newly used pointer path.
Both array bounds and signed/unsigned element types come from their original
declarations. The entire initialized bone table must match byte for byte;
complete BSS ranges are checked for globals and the sound array. No data
extent is promoted.

All fourteen direct calls resolve to complete independently proved functions,
including the recovered damage-effect leaf and sound playback wrapper. Fixed
dependencies prevent later registry additions from changing the evidence.
Negative tests cover instructions, targets, strings, the far end of the bone
table, duplicate full-body candidates, original owner/extent, complete callees,
missing/altered dependencies, array bounds/declarations and carry-member paths.
The proof must round-trip through JSON without changes.

The newly selected body matches French source exactly, adding 2,336 matching
bytes and one function. All sixteen prior Sandy records remain unchanged.
Private evidence is `build/sandy-sit-proof.json`, `build/sandy-sit-tests.txt`
and `build/sandy-sit-france-pilot/report.json`. Canonical aggregate regeneration
and the full production report remain integration gates.
