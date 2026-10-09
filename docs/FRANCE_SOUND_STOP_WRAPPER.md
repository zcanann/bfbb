# French stop wrapper anchored between complete bodies

`france_sound_stop_wrapper.py` proves the eight-byte `xSndStop` at `0x209cc0`
inside a complete 176-byte original cluster: the already proved 64-byte
`xSndParentDied`, the stop wrapper and the already proved 88-byte
`xSndIDIsPlaying`, including every alignment byte. Neither existing anchor
is promoted or counted again.

All three debug originals must have exactly the same owner, canonical member
linkages, complete extents, order, offsets and cluster membership. Both anchor
bodies are rechecked against their independent complete original/French hashes
and data/call inventories, including their closed control flow. The complete
masked 176-byte template must have exactly one match across all loaded French
file spans. The eight-byte tail is never an independent search seed.

Its entire instruction inventory is exactly `J iSndStop; nop`; the target is
the independently proved complete 184-byte platform stop body. Only that
transfer destination is masked. Both original and French tails preserve SP,
RA and every argument register. This scoped proof changes no generic boundary
checker, uniqueness threshold or decoder. It claims no whole translation unit.

Seven original-backed tests pass, covering fixed tail/delay/padding words,
complete anchor and callee bytes, duplicate full clusters, original member
order/extent/owner/completeness, missing or altered dependencies, unrelated
registry additions and exact JSON round trips. Private evidence lives in
`build/sound-stop-wrapper-proof.json`, `build/sound-stop-wrapper-tests.txt`
and `build/sound-stop-wrapper-france-pilot/report.json`.

The wrapper matches source exactly, adding eight exact bytes and one function.
All sixteen prior core sound records are unchanged; the selected core sound
unit remains 100% across 3,628 bytes. This closes a direct sound dependency
for larger original boss routines. Full canonical aggregate regeneration and
the production source report remain integration gates.
