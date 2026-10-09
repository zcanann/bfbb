# PS2 robot source matching

`zNPCArfArf::DuploNotice` now gives each kennel loop its own counter declaration.
Reusing one counter across the two switch cases changed the second loop's
register assignment. The 168-byte function improves from 98.92857% to exactly
100% in USA, PAL, German and French originals, without a compiler patch or
platform conditional.

Full original-backed unit comparisons change only that function. USA exact
code grows from 42,496 to 42,664 bytes (178 to 179 functions), while PAL and
German grow from 42,336 to 42,504 (177 to 178). The selected French unit grows
from 1,520 to 1,688 bytes (11 to 12). All three freshly built GameCube unit
reports preserve every function record and measure.

Evidence: `build/npc-duplo-source-summary.json`, regional before/after reports,
and `build/npc-duplo-gc-check.log`. Separately tested shield Boolean casts and
corner-vector expression rewrites did not improve their targets and were
discarded.
