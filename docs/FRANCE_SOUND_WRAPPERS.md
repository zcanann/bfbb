# French playback wrapper cluster

`france_sound_wrappers.py` recovers three adjacent playback wrappers, adding
124 code bytes: xSndPlay3D(pos), 72 bytes at 0x20a2c0; xSndPlay3D(entity),
24 bytes at 0x20a310; and xSndPlay, 28 bytes at 0x20a330. Their direct callee,
xSndPlayInternal, is already independently proved as a complete 1,376-byte body
at 0x209d60. That existing extent is not counted or promoted again.

The full four-member original cluster spans 1,520 bytes including zero alignment
padding. Each USA, PAL and German reference must have exactly the same complete
DWARF membership, canonical linkage, order, offsets and extents. The existing
anchor's full original/French bytes and data/call inventories are rechecked
against its fixed independent registry identity. The entire masked 1,520-byte
template must have exactly one location across all loaded French file spans.
The tiny wrappers are identified within this complete unique cluster; they are
never accepted through isolated short instruction seeds.

The 72-byte position wrapper has normal frame/return control flow and two JALs
to the complete anchor. The two shorter bodies have fixed complete argument-move
inventories, one terminal J to that same anchor and their exact delay-slot word.
Neither fixed tail sequence touches SP or RA. The entity tail uses integer
argument moves; xSndPlay also moves/clears floating arguments. Original call
operands must reach the corresponding full original anchor. Only those transfer
destinations may be masked. No generic boundary checker or minimum seed rule
changes. No whole-TU or opaque-runtime claim is made.

Six original-backed tests passed in 23.6 seconds; standalone replay takes about
six seconds. Tests reject modified argument moves, tail targets, delay words,
all gap/terminal padding, complete anchor bytes, duplicate full clusters,
original owner/order/extent/membership, and missing/changed anchor identity.
Unrelated registry additions leave evidence identical; JSON round trips must
preserve the proof exactly.

All three new French source functions match exactly, adding 124 exact bytes and
three functions. All thirteen previous sound function records remain unchanged;
the selected 16-function sound unit is 100% across 3,620 bytes. These wrappers
also provide independently proved call targets for larger boss and NPC bodies.
Private evidence is `build/sound-wrappers-proof.json`,
`build/sound-wrappers-tests-final.txt` and
`build/sound-wrappers-france-pilot/report.json`. Canonical aggregate replay and
the full production source report remain integration gates.
