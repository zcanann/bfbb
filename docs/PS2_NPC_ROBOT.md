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

Two timer conditions now preserve the original PS2 integer predicate stage:
ShieldUpdate (608 bytes, 96.57895% to 100%) and NPCBlinker::Update (140 bytes,
86.85714% to 100%). TU-private inline predicates return S32. Returning bool
folded away the original Boolean materialization; returning U8 still differed.
The S32 form restores the original comparison, zero extension and branch
without assembly, volatile access or compiler changes. The private helper
names describe the reconstruction; original inline names are unknown.

The change is PS2-only because using that form for GameCube changes its code.
Full source comparisons in all four PS2 regions show exactly those two function
changes: +748 exact bytes and two exact functions per region. Combined Robot
and Support exact bytes grow 46,596 to 47,344 in USA, 46,436 to 47,184 in PAL
and German, and 2,804 to 3,552 in the selected French profiles. All six GameCube
unit comparisons (two units across three versions) remain unchanged.
Evidence: `build/npc-timer-source-summary.json`, its regional reports,
`build/npc-timer-robot-gc.log` and `build/npc-timer-support-gc.log`.

The same PS2 S32 timer predicate also restores Sleepy::SnoreNZeez (708 bytes,
95.64972% to 100%) and the timer portion of Slick::Damage. Reordering its
three equivalent switch labels completes Damage (624 bytes, 97.03846% to
100%). Tubelet::Chk_IsBonked (368 bytes, 97.98913% to 100%) needs an increment
for the first hurt count and logical negation for its final dead predicate.
The label, increment and negation changes preserve all three GameCube units
without conditionals. The timer helper remains PS2-only.

Tubelet::Process also improves from 85.8% to 89.26667% through the timer
predicate in its inlined Chk_NonAlertBonk. Full Robot comparisons change only
these four functions in USA, PAL and German: +1,700 exact bytes and three
functions each. USA grows from 43,272 to 44,972 exact bytes (180 to 183), and
PAL/German from 43,112 to 44,812 (179 to 182). The selected French profile
contains Chk_IsBonked and gains its 368 exact bytes (2,548 to 2,916; 14 to 15
functions). All three GameCube unit reports retain every function and measure.
Evidence: `build/npc-timer-followup-summary.json`, its regional reports, and
`build/npc-timer-followup-gc-shared.log`.
