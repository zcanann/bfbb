# duplotron

Experimental branch. **Not intended to be merged into `main`.**

Goal: drive the decompilation to 100% matching, using a scheduler-patched
CodeWarrior (`GC/2.0p1a`, derived at build time by `tools/patch_compiler.py`)
and whatever else it takes. Upstream is not interested in the patched
toolchain, so this branch tracks `main` one-way and never flows back. Library
code (MSL, Dolphin SDK, rwsdk, bink, MetroTRK) is in scope here even though it
is off-limits for upstream PRs.

## Ground rules

- `main.dol` sha1 must stay `306526d90b48e99894c3138f5fc8f2716d9fecf6`.
- `report.json` is the only metric. No change lands without a real build.
- No regressions: `matched_functions` never goes down.
- When a function is blocked purely on instruction scheduling, extending the
  compiler patch is fair game.

## Status

| metric | at branch point | now (2026-08-14) |
|---|---|---|
| matched functions | 6491 / 10147 | 8051 / 10147 |
| complete units | 195 / 543 | 232 / 543 |
| **game code exact** | — | **67.750%** (bytes) |
| **game code fuzzy** | — | **95.778%** (6882 / 7673 functions) |
| **game units linked** | — | **91 / 224** |
| SDK code | — | **90 / 90 units, 100.000% fuzzy — complete** |

Game code is tracked separately because it is the part that is actually
being worked, and the project figure understates it badly: Renderware and
Bink contribute 1375 functions at 6.6% and 2.4% and have never been
touched. By category: SDK 99.484%, **game 94.775%**, MSL 79.045%,
Renderware 6.629%, Bink 2.371%.

**Report exact and fuzzy together; they have decoupled.** `matched_functions`
counts a function **only at exactly 100.0%** — not at >=99% (verified: `zFX`
matched 54, functions at 100.0 = 54, functions at >=99.0 = 59). So near-miss
work is nearly invisible in fuzzy: the 2026-08-12 batch converted 24 functions
from 99.9% to 100.0% and moved fuzzy **+0.014** while moving exact **+0.61**.
The inverse also holds — the `xShadow` pass the same day moved that unit's
fuzzy +3.14 and exact by **zero**, because nothing crossed 100.0.

Three different denominators answer three different questions, and the
flattering one is the least useful:

| measure | value | what it means |
|---|---|---|
| functions exact | 86.76% (6657/7673) | flatters: unmatched functions average 654 b, matched ones 146 b |
| **bytes exact** | **59.70%** | the honest headline |
| units linked | 88 / 224 | what a port actually needs — one bad function poisons a unit |

**A fourth number, and it is the one that bites: `complete_units` counts
`Object(Matching, ...)` markers in `configure.py`, not units that reach 100% in
`report.json`.** On 2026-08-12 `zMenu` hit 14/14 functions at 100.0 with
`report.json` scoring **100.0 on every section, code and data** — and a real
link still moved the DOL, because `.sbss` is laid out in declaration order and
ours was `menu_fmv_played, card, sInMenu, corruptFileCount, time_last,
time_current, sAttractMode_timer` against the target's `menu_fmv_played,
time_last, time_current, sAttractMode_timer, card, sInMenu, corruptFileCount`.
Reordering three declarations fixed it and the unit linked.

So finishing a unit's functions is **necessary but not sufficient**. The closing
sequence is always: `tools/symorder.py <unit>` until it says "every section
matches the target's symbol order", then `tools/fliptest.py --test <unit>`, then
`--apply`, then a real `ninja` with the DOL sha1 checked. Skipping it means the
work does not count where it matters.

Note the gap between game fuzzy (91.07%) and game `matched_code`
(~55%). That is the near-miss effect, and a large and growing share of it
is *known unfixable* rather than pending: ghost-literal pool
displacements cap ~30 functions in `zEntCruiseBubble`, ~27 in
`zNPCTypeBossPatrick` and 11 in `zNPCFXCinematic` (that last one priced
exactly, with a throwaway probe that was measured and then removed).
Do not read the remaining 9% as 9% of remaining work.

Merged `bfbbdecomp/main` (d226f0ae..24d388c4) at `297ce59f`.

See **docs/PCPORT.md** for the PC-port follow-up plan, and specifically for why the
port's gate is "all of `src/SB/**` written and correct" rather than this table.

**`report.json` credits near-misses, so it understates stub work.** The xFX
wave moved seven functions from ~0.5% to 72-100% and promoted six neighbours
to exact, and `report.json` recorded **+1** for the unit — because it was
already counting those six at 99.3-99.9%, while the newly-filled bodies at
72-96% are still below its bar. Same for zEntPlayer: six stubs filled, +3
recorded. Judge stub waves by `solo.py` non-matching counts; `report.json`
only sees the crossings.

## Where the remaining functions are

Classified with `tools/classify.py`. This counts the
2681 non-matching functions in units objdiff can compare; the project
total of 2957 also includes units with no diffable object at all.

| class | count | meaning |
|---|---|---|
| MISSING | 1029 | not written yet — no symbol in our object |
| OTHER | 799 | same count, different code |
| SIZE | 654 | different instruction count — real source difference |
| POOL | 144 | **identical code**, only anonymous `@NNN` literal/template pool composition differs |
| SCHED | 35 | identical instruction multiset, different order |
| REGS | 20 | identical mnemonics, different register numbers |

`POOL` functions are byte-identical apart from which anonymous constant they
reference. Biggest clusters: `xFont` (13), `zEntCruiseBubble` (13), `xMath` (9),
`iMath3` (7), `zNPCGoalRobo` (7), `zNPCTypeVillager` (7), `zNPCSupport` (7),
`zNPCTypeKingJelly` (7), `zNPCSupplement` (7). See **Settled** below for why
this is not the cheap bucket it looks like.

**objdiff compares relocations by target offset, not by symbol name.** This
corrects the earlier assumption that anonymous names are cosmetic.
`sStripVert$2188` vs `sStripVert_2188` is *not* flagged — the name differs but
the target is the same object. `@958@sda21` vs `@256@sda21` *is* flagged,
because those two literals sit at different `.sdata2` offsets. So what matters
is pool **layout**, not pool numbering, and layout is something source order
can actually control.

That makes POOL partially reachable, and it cuts both ways within a file.
Writing `DrawRing` at the target's position in `xFX.cpp` seeded `.sdata2` with
the target's first nine constants in the target's own order, and six unrelated
neighbours went from 99.3-99.9% to exact as a side effect. The inverse is the
standing hazard: one unwritten function that owns an early pool slot shifts
everything after it. `xFXRenderProximityFade` owns slot 0x2C (255.0f), so
nothing in `xFX` can align past 0x28 until it exists — which is the current
ceiling on four other functions in that unit.

Practical consequence: when a unit has several near-100% functions and one
large unwritten function early in the file, write the big one first. Filling
small stubs around it just re-shuffles a pool that is going to move again.

58 game units are within 3 functions of being complete — finishing those is the
fastest route to raising `complete_units`. See the roadmap below.

## Roadmap to 100% game code

Written 2026-08-12 against that day's build. Re-derive the numbers before
trusting them; the shape is what matters, not the digits.

**1,017 functions and 663,320 bytes remain, across 221 game units of which 87
are fully matching.** The distribution is barbell-shaped, and that dictates the
order of everything below:

| band | count | note |
|---|---|---|
| 99-100 | 185 | 168,944 b — each 1-3 instructions out |
| 90-99 | 455 | the big middle |
| 0-90 (nonzero) | 278 | broken bodies |
| absent | 99 | 98 with no symbol + 1 wrong `operator=` |

**58 units are 1-3 functions from complete**, while **30 units hold 623 of the
1,017 functions** (61%), zEntPlayer alone holding 88.

### Phase 1 — convert units, not functions

Clearing every unit 1-3 away costs ~118 functions and takes fully-matching
units **87 -> 145**. Nothing else has that leverage, because a unit only
becomes `Matching` — the DOL linking *our* code — when every function in it is
exact. Order: the 24 one-away units first, then the 13 harder ones, then 2-away
(18 units), then 3-away (16). Batch ~4 small units per agent; they share idioms.

**Measured correction (2026-08-12, batch 1 of 8 one-away units): the ordering
above is inverted, and the yield is far lower than "~118 functions" implies.**
Eight of the eleven units whose single blocker was *already above 99%* were
attempted. **One converted** (`zMenu`, and only after the `.sbss` reorder
above). The other seven were all compiler-class and unreachable from source:

| unit | blocker | class |
|---|---|---|
| `zMenu` 99.622 | `zMenuLoop` | **fixed** — volatile static read into a local |
| `iPad` 99.883 | `iPadUpdate` | SCHED — two adjacent independent `lis` swapped |
| `zCollGeom` 99.770 | `zCollGeom_Init` | operand canonicalisation; 12 shapes measured |
| `xEntMotion` 99.694 | `xEntMotionDebugDraw` | REGS — allocator tie-break |
| `zEntButton` 99.429 | `zEntButton_Init` | SCHED — rotation of four `addi` |
| `iAnim` 99.581 | `iAnimBlend` | REGS — copy base coalesced r21 vs r12 |
| `zNPCTypeBoss` 99.565 | `ZNPC_AnimTable_BossSBobbyArm` | REGS — r4/r5 on a 2-word `@sda21` copy |
| `zAnimList` 99.488 | `zAnimListInit` | 2b reload-after-store |

**The lesson is that a tiny residual is evidence *against* source-reachability,
not for it.** A function 2-3 instructions out has usually already had its shape
solved by whoever left it there; what remains is the allocator or the scheduler.

**Refined again after batch 2 — rank by absolute differing-instruction count,
not by percentage.** Percentage conflates two unrelated things: a *small*
function a few instructions out, and a *large* function with real bugs.
`zGameModeSwitch` reads 94.872% because it is 78 instructions and 4 of them
differ — a SCHED interleave, unreachable. `MorphCommon` reads 94.608% because it
is 498 instructions with genuine source bugs, and it yielded three of them. Same
percentage, opposite prospects. Of batch 2's first four, only the one genuinely
large function carried real bugs; two were a single scheduler idiom repeated at
six sites (`zTextBox`/`zUIFont`, the same float-literal-versus-stack-store shape
as `xFont`'s `render_fill_rect`, which already carries a "float scheduling"
comment).

So work the one-away units in *ascending* percentage — the 13 sitting at 84-98%
(`xHudFontMeter` 84.6, `zGoo` 90.7, `xClimate` 92.4, `zEGenerator` 92.4,
`zUIFont` 93.3, `zGameState` 94.9, `zTextBox` 94.9, `iMorph` 94.6, `xSkyDome`
96.5, `xNPCBasic` 98.6, `xstransvc` 98.6, `iTRC` 98.5) are where real source
bugs still live. Skip `xSFX` 72.9 — see "Will this reach 100%?".

Three of the seven have **provably identical instruction multisets**, so they
are exactly the population a scheduler/allocator patch converts. Best test case
on the board: the two-word `@sda21` aggregate copy adjacent to a call taking two
null arguments, with three witnesses (`ZNPC_AnimTable_BossSBobbyArm` 99.565,
`ZNPC_AnimTable_NightLight` 99.545, `ZNPC_AnimTable_Tubelet` 99.583) against two
controls that match at 100.0 because they address a three-word table with
`lis/addi` (`ZNPC_AnimTable_SleepyTime`, `ZNPC_AnimTable_BossSB1`).

**Batch 3 (2026-08-12, post-E3n): the five never-investigated one-away units
are also compiler-class. Retire them.** Ranked by differing rows rather than
percentage — which is the right heuristic and still did not find source work:

| unit | blocker | rows | class |
|---|---|---|---|
| `zEntHangable` | `zEntHangable_UpdateFX` | 2/65 | **branch form** — see below |
| `xSkyDome` | `xSkyDome_AddEntity` | 4/78 | SCHED |
| `xstransvc` | `XST_unlock` | 5/22 | REGS (r5<->r6) |
| `xMovePoint` | `xMovePointGetNext` | 10/75 | REGS (r4<->r6) |
| `xNPCBasic` | `Init__9xNPCBasic` | 11/127 | SCHED |

`zEntHangable` is a **new residue class worth naming: codegen branch form, not
scheduling.** Verified against raw target bytes in `build/GQPE78/asm/`. The
target's `case 2:` body is *two* instructions, so CW picks its 1-instruction
inverted dispatch and falls through; ours is one instruction, because CW drops
the unreachable end-of-case branch, so it picks the 2-instruction
`beq case / b default` form instead. ~30 shapes measured: every `switch` form
gives 99.769 with the same 2 rows, every `if`/`goto`/loop form collapses to a
single `beq` (96.769), and extra arms flip CW into its tree form (90-93%). No
source shape keeps the dead end-of-case branch alive.

**Boundary on the E3n `const` lever, measured on four units: it does not reach a
store made *through a pointer parameter*, only stores into a declared frame
object.** Top-level `const` on a pointer parameter (`xEnt* const ent`) does not
change the CW mangled name, so it is free to try without touching a header — and
it moves nothing. Do not retry it.

**`dwarf/` is not always right.** It lists `xMovePointGetNext`'s locals as only
`rnd, idx, previousOption`; writing it that way measures **92.467%** against the
current 99.333%. Keep the locals. Use DWARF as evidence, not as an oracle.

**Retail bug, faithful, flagged for PCPORT:** `xSkyDome_AddEntity`'s first loop
is `for (i = 0; i > sSkyCount; i++)` — it never executes, so the duplicate-entity
guard is dead and an entity can be added to `sSkyList` twice. The target emits
`cmpw`/`bgt`, so retail shipped it. A PC port will want `i < sSkyCount`.

**Verify completeness with `tools/symorder.py`, not `report.json`.** It scored
`zSurface` 28/28 while `solo.py` had a function at 99.733% and our object
emitted a weak `xVec3::operator=` the retail link deduplicated. Five units sit
at 100% and still cannot be marked Matching for this class of reason (see
Open leads).

### Phase 2 — the structural blockers

These block dozens of functions each and decide whether 100% is reachable.

**2a. Deadstripped-literal pool displacement.** A function the retail link
removed left `.sdata2`/`.rodata`/`.sbss2` literals behind that nothing
surviving references, so every later pool offset shifts. Priced on 2026-08-12
at **44 functions across just `zNPCFXCinematic` and `zNPCTypeDutchman`** (both
measured with throwaway probes that were then removed), and it is the same
mechanism as the eleven ghost `.rodata` templates in 15+ units and the
`.sbss2` object pinning `ZDSP_elcb_event` at 99.984%. **Fabricating bodies is
not allowed** — the objects *are* referenced, which fails condition 1 of the
`__deadstripped_<unit>()` exception. The legitimate attack is `dwarf/`: if a
deadstripped function can be *named* there, its real body is recoverable and
the problem dissolves honestly. Highest-value investigation on the board and
not yet run against this question.

**2026-08-14: `zPlatform` is the fourth and most informative 2a witness, and it
re-confirms the zero pricing the hard way.** Mid-investigation `report.json`
and `solo.py` appeared to *agree* at 21/24, which looked like a counter-example
to the blindness. It was not. `zPlatFM_Update` was being held off 100.0 by a
single **non-pool** row — the target stores `tmrs[i] = 0.0f` *before* the
`flags &= ~(1 << i)` read-modify-write, we did it after. Swapping those two
statements moved `solo.py` 99.431 -> 99.883 and flipped `report.json` to a
clean **100.0 while 21 pool rows remain in the diff**, banking all 3588 bytes.

Lesson, and it is the practical one: **when `report.json` and `solo.py`
disagree about a unit, the delta is the pool bucket and you should ignore it;
when they agree, do not conclude the pool is being counted — look for the one
non-pool row hiding among the pool rows and fix that instead.** Sorting a
function's diff rows into pool vs non-pool before touching anything is worth
more than any amount of pool archaeology.

The two functions still short here (`zPlatformEventCB` 3192b at 99.098,
`zPlatform_PaddleCollide` 888b at 99.865) are blocked by SCHED/REGS residue,
not by the pool: a 180.0f/`toParam[1]` load permutation at six sites and an
f3/f5 two-cycle respectively. So the displacement below is worth **zero
`matched_functions`**, as originally priced. It is recorded because the
fingerprint is the best one yet, not because it is billable.

Both sections hold the *same 30 objects with the same values*; the whole
difference is that the target creates `3.0f` (`@974`) and `1e-5f` (`@976`)
between `zPlatform_PaddleStartRotate` and `zPlatform_PaddleCollide`, while we
create `3.0f` last of all (`@1187`) and `1e-5f` inside `zPlatFM_Update`
(`@696`). Seed those two, in that order, at that point and every other object
in the section lands on the target's exact offset — head and tail both.

What makes this witness better than the `zThrown`/`zShrapnel`/`zNPCTypeRobot`
`0.5f` ones: it is **two constants, two ids apart, and it reuses a `2.0f` that
already exists** at `@968`. That is a fingerprint, not a single float.
`EASE()` in `xMathInlines.h` is `rhs * ((rhs*3.0f) - (rhs*2.0f)*rhs)` — it
creates exactly `3.0f` and reuses `2.0f`. `1e-5f` occurs only in
`xVec3NormalizeMacro`/`xfeq0`. So the missing construct plausibly eases a
scalar and then normalises a vector, in that order.

`zPlatform_PaddleStartRotate` is at **100.0%** for us and its last pool
reference is `@875`, so the creator is not in its body — it sits textually
between that function and `PaddleCollide` and emitted no surviving code, i.e.
a link-deadstripped file-local. `dwarf/` has no `zPlatform` file and
`solo.py --missing` reports 0, so the honest route is still blocked. **Do not
fabricate a body here**: the constants are referenced, so condition 1 of the
`__deadstripped_<unit>()` exception fails, exactly as for the other three.
(Contrast `xCamera`, same day: there the ghost `.rodata` block *is* clean of
inbound relocations — only `@405` is referenced, addend 0, as the materialised
section base — so `__deadstripped_xCamera()` was legitimate and took `.rodata`
to layout-identical. The two cases differ on condition 1, nothing else.)

Helper: `scratchpad/zplat_pool.py` compiles a unit via `solo.py`'s own
machinery and prints the `.sdata2` slot map with values — reusable for any 2a
investigation.

**RUN 2026-08-12. Repriced: 2a is worth ZERO `matched_functions`, because
`report.json` is blind to literal-pool displacement.** Across the game units,
545 functions differ *only* by a relocation whose target symbol name differs in
the compiler-assigned `@NNN` id — and **all 545 already read exactly 100.0 in
`report.json`**. Verified directly on `zNPCFXCinematic`: `solo.py`/objdiff says
30 non-matching of 93, `report.json` says 16 of 93, and the delta is exactly the
14 pool-only functions. `NCIN_MaryBoom` measures 99.894% under objdiff and
**100.0000** in `report.json`; `report.json` scores that unit's `.sdata2` 100.0
while the target section is 0xc0 bytes and ours is 0xb8.

This also resolves the standing "`solo.py` and `report.json` disagree and both
are right" puzzle — **the delta *is* the pool bucket.**

**Worse: the blindness is not limited to anonymous ids. `report.json` will score
a function 100.0 while it references an entirely different named global.**
`ZNPC_AnimTable_ThunderCloud` read `g_strz_roboanim` where the target reads
`g_strz_cloudanim` — a real behavioural bug, the thunder cloud playing robot
animations — and `report.json` called it 100.0 both before and after the fix,
while objdiff moved 99.420 -> 100.000. Whenever a relocation *target* is the
only difference, the relocated field is zero in both objects and the byte
comparison passes. Five game functions currently counted as matched reference
the wrong symbol: `HurtThePlayer` and `WipeIt` (`zNPCHazard`), `Subscribe`
(`zNPCSpawner`), `ParseINI` (`zNPCSleepy`), and `ThunderCloud` (now fixed).

Three of those five pointed at a device that has now been removed. **RESOLVED
2026-08-12** (`87692902`, `0c6050f7`): `zNPCHazard`, `zNPCSpawner`, `zScene` and
`zNPCTypeRobot` declared externs that nothing anywhere defined —
`_958_Hazard // 0.0f`, `_959_Hazard // 1.0f`, `_1041_Hazard // -1.0f`,
`_805_Spawner // 5.0f`, `_1250`, `_1251`, `_2013`, `_2014`, `byte_803D0884`, and
`zNPCSleepy::init`. They were a pool-slot device, and **the names literally
encode the target's anonymous pool ids** — the target references `@958`, `@959`,
`@1041`, `@805`, `@1250`… at exactly those sites. Someone read the numbering off
the target and named externs after it instead of writing the constants.

The `zNPCHazard`/`zNPCSpawner` values came from the comments; `zScene`'s four
carried no comments and were recovered from the target object
(`@1250` = `0f 0f 0f 00`, `@2013` = `00 00 00 ff`, `@1251`/`@2014` zero, in
`.sdata2`/`.sbss2` respectively). Nine undefined symbols eliminated; those
objects can now in principle link.

**Removing a placeholder can convert its neighbours.** One edit in
`zNPCSpawner` — `_805_Spawner` to `5.0f` — took six functions to 100.0, five of
them untouched ones sitting at 99.8-99.93%. In `zNPCHazard` nothing shifted at
all, because those literals reused slots that already existed. Both outcomes are
normal; measure the whole unit either way.

Two related placeholders of the *defined* kind were also removed: `xFont`'s
`_1107` (unused dead `.rodata`) and `zScene`'s `_2098_0` (a hand-written
288-byte jump table duplicating the compiler's own switch table, displacing
every later `.data` object by 288 bytes). After both, each unit's `.data` and
`.sdata2` match the target's layout exactly.

**Expect the metric not to notice.** That batch made 16 functions byte-exact and
`report.json`'s `matched_functions` moved by **1**, because pool-only and
relocation-target differences already scored 100.0 there. The work is still
real: byte-exactness is what `Matching` requires.

So: **price pool work in units linked, never in `matched_functions`.** The
honest price of 2a is **9 units, 0 functions** — 9 of the units that are 1-3
report-functions from complete carry ≥1 pool-only function and will therefore
fail `fliptest` even at `report.json` 100%: `zDiscoFloor` (14), `iMath3` (7),
`xClimate` (4), `xHudMeter` (3), and `xTRC`, `xCM`, `zAssetTypes`, `iCamera`,
`iScrFX` (1 each). 52 game units carry at least one.

**And `dwarf/` does not help the two units 2a was priced on.** A sweep of all
198 `dwarf/` files that map to a unit with a target object found 29 dwarf-only
function definitions across 13 units (22 of 24 checked are absent from
`config/GQPE78/symbols.txt`, i.e. from the whole retail DOL) — but
`zNPCFXCinematic` (77 dwarf defs) and `zNPCTypeDutchman` (98) have **zero**.
Point `dwarf/` at the 13 units that do: best are `zDiscoFloor` (3 away, 14
pool-only, names `clip_render`, `sphere_hits_screen`, `compare_buckets`,
`insert_atomic`) and `xTRC` (small, 3 away, `DisplayMessage`,
`pad_message_valid`). Caveats: absent-from-DOL has three causes (deadstripped,
GC-inlined, PS2-only — `xShadowReceiveShadowFastPS2` and `strtosjis` are plainly
the third), and `dwarf/` is DWARF from a *linked* PS2 ELF, so anything the PS2
linker also dropped is invisible there too.

**Two framing errors corrected.** A name-based orphan scan is mostly false
positives: CodeWarrior materialises one `.rodata` section base and addresses
later constants by displacement, so in `zNPCFXCinematic` the "orphans" at
`+0x170`/`+0x17c`/`+0x188` are all reached by `NCIN_SleepyLamp_AR` off a single
`@405` base pair. `zVar` has 18 `.rodata` objects with 2 named; `zNPCHazard` 53
with 14. Treat the project-wide name-based count of 323 as an upper bound, not a
population. Second, the eleven-ghost-template id range is **not** stable across
units (`@612`-`@618` here, `_617`-`_623` in `zVar`), so the "shared range is the
clue" lead below is wrong. `zNPCFXCinematic`'s actual defect is pool *ordering*,
not orphans: both pools hold the same 40 objects with the same values, but the
target creates `3.0f` and the u32→double magic at `+0x18`/`+0x20` ahead of
`3.141593` while we create them at `+0x60`/`+0x80`, and the unit has no stubs
and no MISSING functions left, so no unwritten body can explain it.

One genuine orphan does remain there: `@1756` = `(0.25f, 0.0f, 0.0f)`, 12 bytes,
whose id places its owner between `NCIN_SleepyDRay_AR` and `NCIN_FodProd_Upd` —
so one of `MaryBoom`, `PeteBonk`, `FireSpiral_Upd`, `FireSpiral_AR`,
`ShieldPop`, `OilHazard` should declare a 12-byte aggregate initialiser with
that value. Four of the six are 88-99%.

**2b. The reload-after-aliasing-store defect.** Retail's `mwcceppc` reloads a
value after a possibly-aliasing store; this branch's compiler forwards it.
One rule, and no source form reaches it short of `volatile`, which is wrong and
was already rejected on `zNPCHazard::Discard`. This is a compiler patch, which
is what this branch is for. Prior art: the float-meme alias patch priced at
+55/+77 functions.

**CENSUSED 2026-08-12 — and it does not price like the float meme.** Across all
542 units with both objects:

| tier | what it is | functions |
|---|---|---|
| NAMED | surplus load of a *named* global; symbol identical in both objects, so the comparison is exact | **90** (58 game) |
| MEMBER | surplus load of a struct member `0xN(reg)`; registers normalised, so collisions possible | 96 |
| ANON | surplus load of an anonymous pool literal; ids differ across objects, so per-literal attribution is impossible | 87 |

**The number that matters is 8.** Of the 58 game NAMED candidates, only 8 have a
match percentage consistent with the reload being their *sole* cause (comparing
the surplus load count against the observed deficit). The other 50 have other
differences too, so fixing 2b moves them but does not convert them:
`iModelStreamRead` 99.490, `zParPTankSteamUpdate` 98.919, `zLightningUpdate`
98.802, `zParPTankSparkleUpdate` 98.776, `zUIRenderAll` 98.652,
`PlayerMountHackUpdate` 94.545, `xCMupdate` 91.500, `xSerialShutdown` 80.000.
Distribution of the 58 by band: 2 at 99-100, 31 at 90-99, 16 at 50-90, 9 below.

**The "seven witnesses in `xFX`" claim below is stale** — re-measured after the
2026-08-12 `xFX` work, `DrawRing` (93 loads vs 93, `Im3DBufferPos` 4 vs 4),
`xFXShineRender` (55 vs 55) and `xFXStreakRender` (33 vs 33) have *identical*
load counts and are not reload cases at all. `zAnimListInit` is also excluded,
because the `volatile` device means we now emit the reload; its residual is the
`mr` copy. The surviving verified witnesses are `activate_ribbon`,
`xFXAuraUpdate` and `NPCHazard::Discard` (NAMED), plus `xFXRingCreate` and
`LightResetFrame` (ANON, float-literal reloads).

### A SECOND witness for "what creates a literal before its first .text use?"

`zNPCTypeBossPlankton::update_move_orbit` (752 b, 99.936) is the cleanest
instance yet of the project's sharpest open problem, and it was measured to
the byte.

Every differing row is a `lwz rN, 0xNNN(r31)` whose DISPLACEMENT differs --
the code is otherwise byte-identical. Both objects anchor at `.rodata + 0`,
so those are absolute offsets for four zero-templates.

  * The target opens `.rodata` with **13 all-zero anonymous templates nothing
    references** -- `@405 @406 @410 @441` (0x0C), `@607..@613` (0x28), `@781`
    (0x0C), `@842` (0x10) = **356 bytes**. Note `@405/@406/@410/@441/@607-613`
    are the SAME ids already reproduced in `zNPCTypeRobot`.
  * Adding the sanctioned `__deadstripped_` block lands `sound_assets`,
    `beam_ring_curve`, `beam_glow_curve` and all `say_*` at the target's exact
    offsets (`beam_ring_curve` 0x340, `say_set` ending 0x42c -- both exact).
  * We are then **exactly 12 bytes short**: our templates land at 0x438,
    retail's at 0x444. Confirmed by throwaway probe -- one 12-byte dummy
    immediately before the function takes it to **100.000**. Probe removed.

Where the 12 bytes come from, and why it is blocked: the target's `.rodata`
order is NOT its definition order. `ring_to_world_vel` (`.text` 0x3834) and
`world_to_ring_loc` (later still) have their `xVec3 out = {...}` templates
created BEFORE `update_move_orbit`'s, though that function sits at `.text`
0x32c0; `register_tweaks`'s `@896` is created early and its body emitted at
`.text` index 25. Ours is definition order, and so is `zNPCTypeRobot`'s, so
mwcc is order-consistent and retail is not.

**This extends the open problem in a useful direction: it is not only
`.sdata2` scalars, it is LOCAL AGGREGATE TEMPLATES too**, and in every case
here the function's body is emitted at a call site rather than at its
definition. That is the signature of an implicitly-inline (in-class or
`inline`-keyword) definition -- which would explain `register_tweaks` if
retail defined it inside the class body, with the class placed between
`sound_assets` and `beam_ring_curve` (dwarf's declaration order agrees). It
does NOT explain `ring_to_world_vel`, whose emitted position matches its
definition position in both objects.

The `__deadstripped_` block was REMOVED per the "must move the number" rule --
its contents are verified and recorded in the agent report, one paste from
being re-added the day the construct is understood. One loose end: we emit an
unexplained 12-byte template `@254` at `.rodata` 0 that `zNPCTypeRobot` does
not have, created during header parsing and referenced by nothing.

### `zThrown_Update` cluster A: two ANTI-CORRELATED halves, 34 variants deep

3,784 bytes, 20 rows, and the most thoroughly bounded REGS case in the file
after `_xCameraUpdate`. Cluster A (16 rows, the bounce/friction reflection
loop) is a pure colouring difference that splits into two halves which cannot
be satisfied at once:

  * **GPR half (6 rows) -- SOLVED in principle.** Writing the negated headings
    as three named locals in x,y,z order (`F32 nx = -collis.colls[i].hdng.x;`
    ...) makes all six GPR rows byte-identical, including retail's
    `addi r4,r1,0x224 / addi r5,r1,0x220 / addi r3,r1,0x228` and the three
    `lfsx`.
  * **FP half (10 rows) -- then gets WORSE.** With `nx/ny/nz` as real two-use
    locals mwcc gives them fresh f6/f7/f8 and recycles the dead load registers
    for the `vel` loads; retail does the exact opposite (negations recycle
    f3/f6/f2 in place, including an in-place `fneg f2,f2`, and the vel loads
    take fresh f8/f9). The BASELINE spelling gets the negation colours exactly
    right and misses only the 3-cycle `{vel.y,vel.x,pz}`.

Net: baseline 99.794, named-locals 99.730. Thirty-four variants measured; the
two halves are anti-correlated in every partial hoist (99.736 gets the GPRs
right but commutes the `fmuls`). Retail hands its `{f7,f8,f9}` pool to the
LONGEST-lived value first (`pz`); we allocate in definition order. That is a
priority-vs-linear tie-break in the allocator, same family as
`_xCameraUpdate`.

**Two negative results from this pass that close off searches:**

  * **The residual is decided LOCALLY.** Adding an extra FP temp upstream (in
    the swept-sphere block) changed that block and left every cluster-A row
    bit-for-bit unchanged. So the "an allocator cursor set earlier in the
    function rotates everything downstream" reading is DEAD -- do not go
    hunting upstream for this class.
  * **The `zThrownCount` alias defect is NOT escape analysis.** Adding
    `U32* probe = &zThrownCount;` at file scope changed nothing at all, in any
    of the four functions. mwcc is not reasoning "this static's address is
    never taken"; it simply does not treat a store to an sda21 static as
    killing a pointer load. Note also that the same defect surfaces as
    *scheduling* in `zThrown_LaunchVel` and as *CSE* in `zThrown_AddFruit`.

Also settled here: the six `px/py/pz/tx/ty/tz` temps are real and must all be
computed before the first store (dropping them is 98.330; computing `t`
between the stores is 98.646), but their statement ORDER is completely inert
-- mwcc canonicalises it, so spend no measurements there.

### The register allocator is NOT patchable -- and here is its actual mechanism

**Do not re-open "patch the FP colour tie-break". There is no tie-break.**
Investigated 2026-08-22 by locating `Coloring.c`'s assertion strings
(`0x5bcbe8`) and following their three cross-references into the module at
roughly **`0x508680`-`0x508c60`**.

Map of the module:

  * `0x508680` -- the colouring driver. Loops over the five register classes
    (`cmp byte [esp+4], 5`), class in `byte [0x5ea299]`, per-class register
    counts at `[cls*4 + 0x5e9800]` and node counts at `[cls*4 + 0x5e9b04]`.
  * `0x508a20` -- **simplify**. Walks nodes in INDEX order, repeating to
    fixpoint; a node with degree (`word [n+0x12]`) < k goes on the stack
    (flagged `or word [n+0x16], 2`) and its neighbours' degrees are
    decremented; otherwise it goes on the spill-candidate list. If that list
    is non-empty it computes the classic Chaitin ratio at `0x508ad2`
    (`fild [n+0xc]` / `fild degree`, `fdivrp`) to pick a spill.
  * `0x508900` -- **select**. Builds the free mask by clearing each coloured
    neighbour's bit (`mov eax, 0xFFFFFFFE / rol eax, cl / and edx, eax`),
    then:

        xor ecx, ecx
        mov eax, 1 / shl eax, cl / and eax, edx
        jne -> mov word [node+0x14], cx      ; assign
        inc ecx / cmp ecx, numregs / jl

    i.e. **scan colours from 0 upward, take the first free one.** No
    preference, no coalescing hint, no cost term. There is nothing to flip.

And retail was built with this SAME binary (unpatched GC/2.0p1), so the
algorithm cannot be the difference -- only the input graph can be.

**Our patches are not the cause either, they are a large help.** The
`_xCameraUpdate` witness form measures **99.949 with GC/2.0p1a and 98.416
with unpatched GC/2.0p1**; clause C+/V/E3n are worth 1.5 points on that one
function.

### THE RULE: colour order = SOURCE DECLARATION ORDER

**This supersedes the interference-degree hypothesis below, which was wrong.**
Established 2026-08-22 by closing `_xCameraUpdate` (3,560 b) from 99.792 to
**exactly 100.0, 0 differing rows of 890**, with a rule that predicted every
one of ~270 builds.

In these blocks `k` for the FP class is far above every node's degree, so
simplify pushes every node in ONE pass in index order and select pops LIFO.
The net effect is simply:

    Values are coloured in the order they are DECLARED in the source, each
    taking the lowest colour not blocked by an already-coloured neighbour.

So the method for any REGS-class residual is mechanical, not a search:

  1. Read the target diff and note which physical register each value should
     get.
  2. Sort those values by target register ASCENDING.
  3. That is the order they must be DECLARED in.

`_xCameraUpdate` wanted `ppv`->f4, `vax`->f5, `vay`->f6, `dpv`->f7,
`vaz`->f3, so the declaration order had to be `ppv, vax, vay, dpv` with
`vaz` last (it is coloured last and takes the lowest colour the f0/f1/f2 load
temps leave free). We had been declaring `dpv` first, which pinned it to f5.

**The corollary that makes otherwise-impossible orders reachable:
DECLARATION order beats DEFINITION order.** A bare `F32 vax, vay;` declared
early but ASSIGNED later still colours in declaration order. That matters
because `vax = at.x * dpv;` cannot be *defined* before `dpv` -- but `vax` can
be *declared* before it. Use bare declarations to place a value early in the
colour order without moving its computation.

Measured ladder on this one function, all predicted correctly in advance:

    ppv, dpv, vay, vax   -> dpv=f5 vax=f7   99.949   6 rows
    ppv, dpv, vax, vay   -> dpv=f5 vax=f6   99.916  10 rows
    dpv, ppv, vay, vax   -> dpv=f4 ppv=f5   99.927   9 rows
    vay, ppv, dpv, vax   -> vay=f4 dpv=f6   99.893  12 rows
    ppv, vax, vay, dpv   -> 4,5,6,7 retail  100.000  0 rows

**Fidelity caveat, unresolved.** `dwarf/` lists this block as
`dpv, hpv, ppv, vax, vay, vaz`, and the dwarf order IS declaration order
elsewhere in this same file (it reproduces our already-matching `wcvx..psv`
and `it, ot, T_inv` orders). The 100% form contradicts it on `dpv`'s
position, and 20 builds failed to reconcile the two: with `dpv` declared
first the colouring pins at f4 or f5 across every structural spelling. Under
the rule the target permutation and the dwarf permutation are disjoint, so
no source with `dpv` first can produce retail's registers. Either the dwarf
order is not declaration order for this block, or there is one more input to
the node index. **A direct oracle exists if anyone wants it**: dwarf also
annotates `dpv // r4, hpv // r7, ppv // r1, vax // r5, vay // r7, vaz // r4`,
which are NOT physical registers (`hpv`/`vay` share, `dpv`/`vaz` share) and
so look like pre-allocation VIRTUAL register numbers. Emitting DWARF from our
own build and comparing vreg numbers would settle the node index directly.

Note the change also CORRECTS the rounding: the old
`right.x * ppv + at.x * dpv` rounds the `right.x` product and fuses the
other; retail's `fmuls f5, f0, f7` / `fmadds f5, f2, f4, f5` rounds
`at.x * dpv` and fuses `right.x * ppv`. The new source reproduces retail's
rounding exactly.

### OPEN CONFLICT: the GPR ordering key is not settled

Two passes measured the GPR key and got OPPOSITE answers. Do not treat either
as settled; test both handles on any new function.

  * `PlayerCollsSelectDepen` (zEntPlayer, 2026-08-22): a bare
    `xCollis* c; xCollis* cend;` declared early and assigned later was
    **BIT-IDENTICAL** to baseline, while moving the INITIALISER moved the
    colour exactly as predicted. Reads as: GPR key = definition point.
  * `PipeForAllSceneModels` (zScene, same day): hoisting
    `U32 remainSubObjBits;` -- a BARE declaration, no initialiser -- to the
    outer-loop top **DID** move its colour (to index 6) and put `model` on
    retail's r24, 99.176 -> 99.412. Reads as: GPR key = lexical declaration
    point.

The difference between the two experiments is that zScene's hoist crossed a
SCOPE boundary (into the enclosing loop) while zEntPlayer's stayed in the
same block. That is the obvious hypothesis -- a bare declaration may only
move a GPR's colour when it changes scope -- but it is UNTESTED. Whoever
touches this next should test it directly; it would resolve the conflict and
make the GPR case as predictive as the FP case.

**Sub-rule, measured and useful on its own:** a loop counter declared in a
`for`-init occupies the LAST slot of the declaration-ordered set. Declared
anywhere else -- outer-loop scope, if-block scope, or function scope, with or
without an initialiser -- it is EJECTED past later values (in
`PipeForAllSceneModels`, past `pipeCB` and an anonymous byte-offset temp,
from index 8 to index 10). So such a counter has exactly TWO reachable
colours. That is what makes `PipeForAllSceneModels` unreachable: retail needs
`k` at index 6, which requires it declared before `model` while STILL being a
for-init declaration -- a contradiction.

### Scoping note: check whether the function can reach 100 AT ALL first

`zSceneSetup` (3,196 b) has an FP cluster that looked like a good colouring
target. It is not worth attacking, because even with that cluster solved the
function still carries the `gCurEnv` store-then-reload row, whose only known
fix is the volatile read that is banned here (it banks zero and masks the
compiler defect). **A function with a second, independently-blocked residual
is worth zero no matter how tractable its first residual looks.** Enumerate
ALL clusters before starting.

### The alias patches do NOT cause zScene's load/store residuals -- measured

A pass suggested `zSceneInit`'s two clusters (a load moved across a store
that retail treated as a barrier) might be CAUSED by clause V / E3n rather
than merely uncured by them, which would be a cost line against those
patches. Measured directly, same source, both compilers:

    zSceneInit    GC/2.0p1a 98.203   vs  unpatched GC/2.0p1 94.729
    zSceneSetup   GC/2.0p1a 99.618   vs  unpatched GC/2.0p1 99.293
    PipeForAll…   identical in both

The patches are worth +3.5 points on the very function that raised the
suspicion. Claim disproven; do not re-open it.

**Scope note added 2026-08-22.** That disproof is about `zScene`, and it is
sound there. It is NOT a general exoneration of the patch clauses, and it had
started to be read as one. Clause E3n costs `zEntPlayer_AnimTable` **23,820
bytes** -- the largest single-function patch cost on record, and 92% of
everything removing E3n would win back. The honest framing: E3n is strongly
net-positive by function count while being byte-negative in its largest
individual case. Always price a clause both ways.

### CORRECTION: FP orders by DECLARATION, GPR orders by DEFINITION

**This corrects the corollary stated below.** "Declaration order beats
definition order" was measured on the FP class in `_xCameraUpdate` and it is
true THERE. It is FALSE for the GPR class, measured on
`PlayerCollsSelectDepen` (2026-08-22):

    bare `xCollis* c; xCollis* cend;` declared before colls/mat,
        assigned after            -> 99.818, 15 rows, BIT-IDENTICAL to baseline
    swap the two INITIALISED declarations colls/mat
                                  -> 98.662, registers swapped as predicted
    swap the two INITIALISED declarations c/cend
                                  -> 99.486, registers permuted as predicted

Moving a bare declaration is inert; moving the INITIALISER moves the colour.
So the ordering key differs by class:

    FP class   -> DECLARATION point   (bare-declaration loophole available)
    GPR class  -> DEFINITION point    (loophole NOT available)

**This reconciles the `xShadowSimple_Add` result** recorded below as "the rule
does not reach callee-saved GPRs at all": that pass moved sixteen
DECLARATION shapes, which for GPRs is exactly the inert dimension. The two
findings agree -- GPRs order by definition point.

**The GPR colour-index table is also not what you would assume.** Derived and
confirmed against two independently predicted permutations:

    colour index:  0    1    2    3    4    5    6    7
    register:     r27  r29  r28  r26  r30  r31  r25  r24

Values are coloured in definition order, each taking the lowest-index free
colour. It is NOT descending r31, r30, ... Anyone reasoning about GPR colours
must use this table, not intuition.

**`PlayerCollsSelectDepen` (1,868 b) is ARITHMETICALLY UNREACHABLE, proven.**
Target wants `c`=r25, `cend`=r26, `idx`=r30; we get r26, r30, r31. Sorted
ascending, the target order is `c, cend, idx` -- which is ALREADY our
definition order. It is a uniform shift within the free list, not a
permutation. The loop-1 trio interferes only with `{ent, colls, mat}`
(r27/r28/r29, identical on both sides), and everything else holding
r24/r25/r26/r30/r31 has a disjoint live range and can never block it. So the
trio always takes colour indices 3,4,5 = {r26, r30, r31} in whatever order
their definitions appear. Retail's {r25, r26, r30} = indices 6,3,4 SKIPS
index 5 (r31), which requires a coloured neighbour holding r31 -- and no such
neighbour exists in our graph. **Retail's source creates one extra
interference across loop 1 that ours does not.** That is an interference-graph
difference, not an ordering one, and it is the crisp falsifiable statement of
what retail's source must do: keep a value live across loop 1 that shares r31
with the loop-2 iterator.

### THE RULE, REFINED TWICE (2026-08-22, xShadowSimple)

Two refinements from closing `xShadowSimple_CalcCorners` (484 b) to
**100.000, 0 of 121 rows** and from failing on `xShadowSimple_Add`. Both
change how to apply the rule, so read them before the LIMIT section below.

**1. NAMING AN ANONYMOUS TEMP PULLS IT INTO THE DECLARATION-ORDERED SET.
This is the escape from the limit.**

`CalcCorners` wanted `dydz`-CSE=f5, `bx`=f6, `dydx`-CSE=f7; we had `dydx`=f6
and `bx`=f7. Declaration placement of `bx`/`by` was byte-identical in four
separate spellings, because they colour AFTER two anonymous merge-block
scratch temps. The value actually out of place was the anonymous
`cache->dydx` CSE temp. Binding it to a named local:

    F32 dydz = cache->dydz;
    F32 dydx = cache->dydx;
    ...
    ay = ax * dydx + az * dydz;

pulled it into the ordered set, and `bx`/`by` then snapped to retail's f6/f7
BY THEMSELVES (99.752, 6 rows). The rule then predicted the last step
arithmetically -- retail wants `dydz` below `dydx`, so declare `dydz` first --
and that measured **100.000 first try**.

So when the mis-coloured value is an anonymous CSE temp, do not permute the
named locals around it: NAME IT. That is also a faithful change in its own
right (a member read repeatedly is a plausible local in the original).

Note this also corrects the LIMIT's wording below: the obstructing temp here
came from a LATER statement yet was coloured EARLIER, so "an earlier
statement" is not the right test. The right test is simply whether the
mis-ordered value is anonymous -- and the answer is now to name it.

**2. THE RULE GOVERNS VOLATILE REGISTERS, NOT CALLEE-SAVED GPRs.**

`xShadowSimple_Add` (1,176 b) needs a permutation of six values across
r26-r31 -- both objects save exactly r26-r31. **Sixteen source shapes moved
NOT ONE callee-saved register**: `shadowWas` first, `castOnEnt` first, `j`
first, a full 14-position sweep of `j`, dwarf declaration order, `U8`->`U32`
on `moved`, `shadowWas` scoped into its own branch, all else-branch locals
scoped, and a named `xShadowSimpleQueue*` element pointer.

In the SAME function the volatile GPRs obey the rule exactly: `vert` and the
polygon-loop `j` trade r7/r6 purely on declaration order, earlier taking the
lower. That is what splits the `j` sweep (positions 0-3 give 99.541/25 rows,
4-13 give 99.320/37, the twelve extra rows being only `vert`/`j` swapping).

So callee-saved GPRs are assigned by a different mechanism that declaration
order does not reach. **Before starting a REGS residual, check which register
class the mis-coloured values are in.** FP and volatile GPRs are workable;
a permutation confined to the callee-saved GPR set is not, and
`xShadowSimple_Add` is the measured witness.

### THE RULE'S LIMIT: it orders NAMED LOCALS only, not anonymous temps

Established on `zThrown_Update` (3,784 b) over ~320 builds, 2026-08-22. The
function did NOT close and was reverted; the boundary condition is the
deliverable, and it tells you when to stop.

**The rule was confirmed here**, including the bare-declaration loophole:

  * `F32 px, py, pz;` declared x,y,z but ASSIGNED z,x,y is byte-identical to
    plain x,y,z -- declaration order beats definition order.
  * The rule correctly predicted `d`->f1, `px`->f3, `py`->f6 from the
    baseline declaration order.

**But its domain is named locals among themselves.** Anonymous expression
temporaries created by an EARLIER statement are ordered separately and always
precede the named locals of later statements. No declaration placement
reaches them:

    3 bare decls before `d`, assigned after          99.794, cluster A = 16
    same, comma-declared                             99.794, A = 16
    same, at FUNCTION scope                          99.794, A = 16
    named vx/vy/vz after the p's, used later         99.794, A = 16

all bit-identical to baseline. The confirmation that the named locals stay
behind: if they were coloured first, `px` would take f0; it takes f3 in every
build, because `OPB(f0)`, `d(f1)`, `nZ(f2)` are always coloured first.

Why that blocks this function. Cluster A is NOT "two anti-correlated halves"
as previously recorded -- it is ONE rotation in two register classes. Retail's
FP order is
`OPB(f0) .. d(f1) .. nZ(f2) .. nX(f3) .. OMF(f4) .. VZ(f5) .. nY/py(f6) ->
pz(f7) -> vel.y(f8) -> vel.x(f9)`; ours is identical except `pz` sits AFTER
the two `vel` temps instead of before. That single swap produces all 16 rows,
GPR bases included. To fix it `pz` must be declared before the two
`thrown->vel` temps -- but those are anonymous temps created inside the `d`
expression, and `pz = -hdng.z * d` cannot exist before `d`. **The required
order is not expressible.**

Two escapes were measured and both fail: naming the vel values and declaring
them after `pz` (CSE binds them to the temps `d` already created --
bit-identical), and removing `d` as a variable so its temps belong to `px`'s
statement (still ahead of `py`/`pz`; that is the 99.804 / A=14 form).

Also settled here: retail's `pz` is NOT computed in place (`fneg f2,f2` then
`fmuls f7,f2,f1`), which proves `nz` and `pz` are separate values in retail
and that the p/n fusion forms are wrong.

**Practical test before spending a session on a REGS residual:** work out the
required colour order, then ask whether every value in it is a NAMED LOCAL
whose declaration you can move. If the order requires placing a named local
ahead of an anonymous temp created by an earlier statement, stop -- it is not
expressible.

### Superseded: the interference-degree reading

Because select is lowest-free in stack order, a value can only receive a
HIGHER colour than another if the lower colour is **already taken when it is
coloured**. So when the target gives value X a higher register than we do,
retail coloured its competitor Y FIRST. Colouring pops LIFO off the simplify
stack, so Y was **pushed LATER** -- meaning Y kept degree >= k through more
simplify rounds, or sits later in node index order.

Concretely for `_xCameraUpdate`: retail has `dpv`->f7, `vax`->f5; we have the
reverse. So retail colours `vax` before `dpv`, i.e. **`vax` is pushed later
than `dpv`**. The lever is therefore `vax`'s INTERFERENCE DEGREE, not its
statement position -- which is why a ~950-build sweep over statement
orderings, operand orders and accumulate masks bottomed out at 6 rows without
touching it. Lengthen `vax`'s live range, or shorten `dpv`'s, so `vax`
survives more simplify rounds.

That rule applies to every REGS-class case in this file, including
`zThrown_Update` cluster A (retail hands `{f7,f8,f9}` to `pz` first; we
allocate in definition order) and `zThrownCollide_ThrowFruit`.

### `_xCameraUpdate`: 3,560 bytes behind ONE binary FP colour tie-break

The single cheapest compiler-side witness currently known. `_xCameraUpdate`
(3,560 b) sits at 99.792 in the tree; a clean, non-contorted source form
reaches **99.949 with exactly 6 differing rows**, and those 6 rows are a
literal two-register swap: retail colours `dpv`->f7 and `vax`->f5, we get
`dpv`->f5 and `vax`->f7. Everything else in the block is byte-identical --
same mnemonics, same operand positions, same instruction count, same branch
shapes, same frame, same callee-saved set.

`dpv` and `vax` interfere. After excluding the colours pinned by `at.x`(f0),
`at.z`(f3), `right.x`(f2), `right.y`(f1), `ppv`(f4) and `hpv`(f6), the only
two left are f5 and f7. Their live ranges, interference degrees and use
counts are identical on both sides, so there is no source-visible
discriminator -- it is one bit in the allocator's tie-break.

The search that establishes this is not a spot check: 288 builds sweeping
pv-order x va-order x accumulate mask, 144 more adding both operand-order
flips, and 500 random topological orderings of all 15 statements in the
block. **The floor of ~950 builds is 6 rows.** A separate probe also proved
that a change AFTER the block cannot alter the block's colouring, which
bounds where any fix could live.

The 99.949 form is NOT in the tree: it banks zero (only 100.0 counts) and it
contradicts `dwarf/` on two declaration orders, so it would trade fidelity
for nothing. But if anyone extends the compiler patch to the FP colour
tie-break, this is 3,560 bytes for one bit, with the source form recorded in
the agent transcript.

### Entry 4: the decisive test is whether SOURCE ORDER MOVES THE ROWS AT ALL

Two more entry-4 confirmations in `xCollide` (2026-08-22), and one of them
supplies the cleanest diagnostic yet for telling entry 4 apart from an
ordinary source-order problem.

`xSphereHitsOBB_nu` (99.849): **writing the three stores in the source as
`y, x, z` produces BYTE-IDENTICAL output.** The scheduler emits `y, x, z`
whatever the source says, so no spelling can reach it. That is the test to
run first on any transposed-store pair -- permute the source statements and
see whether the emitted rows move. If the output is bit-identical under
permutation, the reorder is happening below the source level and you are
looking at entry 4; stop. If the rows DO move (as in `xParabolaHitsEnv`,
where swapping made it 98.569 and moved a different row), you are at least
in contact with the scheduler, though it may still be unreachable.

Also measured and rejected on these two: binding `xVec3& N = data.N;` across
the whole block (byte-inert), and `(o)->assign(0.0f, 1.0f, 0.0f)` (96.109).

### `xSweptSphereToBox`: SOLVED as a diagnosis -- not expressible, and here is why

2,448 b, 99.158, 29 rows of 614. Re-worked 2026-08-22 with the colouring
rule. It does not close, and two things previously recorded here were WRONG.

**The real emission rule for this block** (replaces the old "defect 1 +
defect 2" reading, and every measured variant fits it):

    The nine (load, fmuls) pairs are emitted in ASSIGNMENT-statement order,
    with the value belonging to the FIRST STORE statement moved to the END
    of that list.

  base (assign asc, first store `aXx`) -> emitted `aXy..aZz, aXx`   99.158/29
  assignments reversed, stores asc     -> emitted `aZz..aXx`        99.098/31
  store `boxaZ.z` first                -> ascending, no move        99.163/24
  stores fully reversed                -> ascending, no move        99.144/26
  declaration list rotated one left     -> BIT-IDENTICAL to base

**CORRECTION 1: "temp assignment order is irrelevant (inert)" was FALSE.**
Reversing all nine assignments measures 99.098 / 31 rows and reverses the
emitted load order. The old "inert" reading came from moving only `aXx` to
last, which happens to produce the identical emitted list under the rule
above.

**CORRECTION 2: "defect 2 (a one-instruction interleave offset) exists" was
FALSE.** It was an artifact of the odd store order in the `boxaZ.z`-first
probe. With the stores fully reversed, the loads, the `fmuls`, the f8->f0
product colours AND the load/fmuls interleave are ALL byte-identical to
retail -- the entire computation half of the block matches. Only the nine
stores (in that variant's reversed order) and `dy`/`dz` differ.

**CORRECTION 3: "`dy`/`dz` in f22/f23 is downstream of the rotation" was
FALSE.** In the fully-reversed variant the rotation is gone and the products
carry retail's f8..f0, yet `dy`/`dz` are STILL f22/f23 against retail's
f26/f27. It is an INDEPENDENT second defect. Both sides save f21-f31 with
`rad`=f26 and `radsqr`=f25 identically, so retail has four values coloured
before `dy` that interfere with it and not with `dx`; nothing in our graph
does. That is the `PlayerCollsSelectDepen` shape -- an interference-graph
difference, not an ordering one.

**Why it is NOT EXPRESSIBLE.** Two constraints collide. (1) Emitted store
order tracks source store order, adjusted only for value-readiness --
demonstrated directly, since fully reversing the source stores fully reverses
the emitted stores. Retail's emitted stores are
`0x54,0x58,0x5c,0x48,0x4c,0x50,0x3c,0x40,0x44` and the values become ready in
exactly that order, so retail's source store order is the natural ascending
one with `boxaX.x` first. (2) The rule then moves the first-stored value to
the end of the load/fmuls list -- the rotation. To cancel it the first-stored
value must be `aZz`, but making `boxaZ.z` the first store forces its `stfs`
to be emitted third, or forces the whole store block into reverse. The
scheduler cannot defer a store that is first in source order and ready. So
retail's store order and a non-rotated load order are mutually exclusive
under this compiler.

**The DWARF form is the real source and confirms the diagnosis.**
`dwarf/` lists `dx, dy, dz, rad, radsqr, testdist, invZ, boxPos, boxaX,
boxaY, boxaZ` and NO `a??` temps at all. It measures 96.842 with a fully
serial `lfs/fmuls/stfs` chain through f0 (binding `const xVec3&` to the three
matrix rows is byte-identical to it, so that lead is closed too). Retail's
compiler hoisted nine loads over nine stack stores from that source; ours
will not. **The nine temps are a workaround for the load-hoist alias defect,
and the workaround is what injects the rotation.** If that predicate is ever
widened, start from the fully-reversed-stores variant, then delete the temps
entirely and use the DWARF form.

Note the colouring rule's LIMIT explains why declaration order is a live
lever for the products' colours but a dead one for emission order: these
single-assignment single-use locals are copy-propagated away, so the ordered
set that matters is the anonymous one -- and there is no anonymous temp left
to name, because naming them is exactly what the baseline already does.

### symorder's "SAME SET, WRONG ORDER" is not evidence on its own

`tools/symorder.py zPlatform` reports `.sdata2: SAME SET, WRONG ORDER`, and a
2026-08-21 pass read that as a fourth witness of the "dead constants created
early" problem alongside `zThrown`, `zShrapnel` and `zNPCTypeRobot`, with the
conclusion that the unit "will not link byte-identically even at 24/24".

**That conclusion is contradicted by the DOL hash and must not be repeated.**
`build/GQPE78/main.dol` is byte-exact against
`306526d90b48e99894c3138f5fc8f2716d9fecf6`. The shipped image therefore
already contains the correct `.sdata2` bytes for this unit; a real pool defect
here would make the whole DOL differ.

Two things make the report misleading for a unit like this, and both are
visible in the same output:

  * `.text: different sets` -- our object also carries inline/weak functions
    retail dead-strips (`__as__5xVec3`, `xVec3SMulBy`, `xModelGetFrame`,
    `xEntERIs*`, `xBoundCenter`, ...). Those extras own pool entries, so the
    two objects' pool ORDINALS are not comparable directly.
  * the only ordering difference is where the single 8-byte magic double sits
    among the 4-byte entries, and the 8-byte object is 8-aligned, so the
    alignment padding absorbs the difference and both land at the same offset.

objdiff agrees: the agent confirmed the relocation targets are identical
(`.sdata2` offset 0x58 on both sides). So references resolve to the same
place.

Before treating a symorder pool warning as real, check the DOL hash first,
and check whether `.text` sets differ. It is genuine evidence only for units
whose object is otherwise a set-match.

### The switch-tree "pivot convention" was WRONG. It is missing `case` labels.

**DISPROVEN 2026-08-22, and this is the correction that matters most in this
file.** `zEntPlayerEventCB` was closed to 100.0 by adding four `case X:`
labels that fall straight to `break`. There is no pivot defect and no
compiler patch to write -- do not spend a session on one.

What the earlier pass got right: our emitted case values, the case-body
order, and the body offsets were all identical to retail, and all 70
differing rows were in the search tree. What it got wrong was the inference.
It concluded both compilers partition identically and differ only in choosing
max-of-lower vs min-of-upper as the pivot. The real cause is that **retail's
case LIST was longer than ours**. Cases whose body is just `break` emit no
body and no `cmpwi` of their own, so they are invisible in a value-set
comparison -- but they change the shape of the tree CW builds.

The tell was there all along and was read past: **the target's dispatch was
one instruction LARGER than ours** (108 vs 107). A pure pivot-selection
difference cannot change the instruction count. A longer case list can.

Diagnostics, now that the shape is understood -- use these on any large CW
switch whose value set, body sizes and body order match but whose tree does
not:

  * the target's dispatch is BIGGER than yours -> you are missing labels.
  * a pivot `cmpwi V` with **no** `beq`: CW elides the equality test when the
    pivot tops a consecutive run of default-mapped cases and emits the
    boundary as `max+1`. So V-1 is the top of a run you have not written.
  * a **four**-instruction leaf `cmpwi / beq body / blt default / b default`
    where you emit three: that node has an all-default sibling child.
  * a lone default-mapped case shows up as a stray `beq <default>`; pairing
    it with its neighbour removes that row.

Add one probe case at a time and watch which subtree snaps into place. On
`zEntPlayerEventCB` fourteen case-set combinations localised it to exactly
{51, 52, 284, 285}; a single case at 285 scored 99.488 and at 286 scored
99.489, both carrying the stray `beq`, and only the two-element run [284,285]
removed it.

**Source POSITION of an empty case has zero effect on codegen**, so it cannot
be recovered from the object -- put such labels wherever reads best.

Still open on this shape: `zPlatformEventCB` (3,192 b). Note its residual was
separately measured and is NOT this defect -- zero of its rows are in the
search tree; all 12 are in case bodies, one motif across the six
`eEventRot*` cases. So the two big EventCB functions had two different
causes, which is exactly why the "deep switches cost us everywhere" reading
never held up.


Weigh that against the cost: locating median selection in a 6 MB stripped
binary with no debugger is open-ended, unlike clause E3n which was a
one-byte change at an address an agent had already pinpointed. Left as a
lead, not scheduled.

One thing it DOES settle: `xEvent.h`'s enum is correct. The case values match
retail exactly, so any future "the event enum is shifted" reading of that
diff is wrong and must not be acted on.

### A THIRD alias lead: pointer loads killed by a store to a file-scope static

`zThrown_LaunchVel` (91.934) and `zThrown_AddFruit` (88.657) share one root
cause, and it is the cleanest compiler-side witness pair currently on the
board. Both do:

```c
newThrown = &zThrownList[zThrownCount];
zThrownCount++;
newThrown->killTimer = stats->carry->killTimer;
```

Retail issues the `lwz` of `stats->carry` **after** the `stw` of
`zThrownCount` -- the store to the static kills the load. Our mwcc proves
`zThrowableModels` and `zThrownCount` cannot alias, and then either hoists
the load above the store (`LaunchVel`) or reuses the value already computed
for the earlier `stats->carry != &c_fruit` test (`AddFruit`). Every other
differing row in both functions -- the r7/r8 and r8/r9 renumbering, the
li/lis scheduling swaps, the extra live register -- is downstream of that
single decision.

This is adjacent to but OUTSIDE clause V. Clause V kills the literal pool
across a store to a small static; what is wanted here is killing
**pointer-dereference loads** across a store to a file-scope static.

Two things make this a good candidate if anyone extends the alias predicate:
856 bytes across two functions, and **no masking to unwind first** -- a
whole-variable `volatile` on `zThrownCount` is firmly disproven by the
target (it breaks six other functions in the unit and creates four new
non-matching ones), so nothing has been papered over here.

### Clause H: the THIRD query site FOUND AND PATCHED -- LICM's alias query (2026-08-25)

**The "second query site in the code generator's redundant-load path" /
"gFrameCount third site" hunt is over.** Alias.c holds FOUR dispatch tables
in a row, and the float-meme work only ever found the last one:

| table | consumer | callers |
|---|---|---|
| `0x5bd068` (3 entries) | `0x511a30` VN store-kill | ValueNumbering.c — clauses V, F |
| **`0x5bd074` (3x3)** | **`0x511cb0(instr, expr)`** | **CodeMotion.c ONLY — clause H** |
| `0x5bd098` (3x3) | `0x511e10` must-alias/full-overlap | CodeMotion.c |
| `0x5bd0bc` (3x3) | `0x511fc0` scheduler may-alias | Scheduler.c + CodeMotion.c — clauses A/B/C/C+/E3n |

`0x511cb0` is "may this loop store alias this hoist candidate's memref".
Stock entry 0 is reference identity, so an `lfs` of a `.sdata2` literal was
invariant in any loop storing to a static array and LICM hoisted it to the
preheader. Retail answers may-alias and keeps the load in the loop — which
is upstream of the WHOLE `zEntPlayer_SNDInit` residual: in-body reloads,
remainder-loop reloads, and the unroll factors (the kept load doubles the
body, so retail's 0.65f fill is a 16-wide loop where ours was 56 straight-line,
and 0.77f x48 is retail 2x21 vs our full unroll).

Clause H (shipped, see `patch_compiler.py` for the mechanism — it is a
pre-dispatch hook covering all nine entries, living in a NEW second cave
created by growing .text's raw size one page):

    store base expr word == 5  AND  candidate base expr word == 5
    AND candidate size <= 4  ->  may-alias

Measured 2026-08-25, full ninja, DOL intact: **matched_functions 8403 ->
8409 (+6/-0), zero sub-100 functions down anywhere.** GAME exact 80.226 ->
80.349, fuzzy 99.1546 -> 99.2130. Gains: xScrFXGlareUpdate, xSndDelayedInit
(59.459!), cruise_bubble::update_hud, PlayerTeeterCheck (the long-stuck
78.649), zFXGooEventMelt, NPCS_SndTimersReset (56.984). Also up without
crossing: xFXAuraUpdate 86.0->95.4, zEntPickup_SceneUpdate 96.3->99.5,
**zEntPlayer_SNDInit 94.126 -> 99.188**, NPCS_SndTimersUpdate 86.8->98.4,
zEntPlayer_Update 96.6->96.8. New compiler sha1
`5c6862b641adb8845f0fc09a6569902df068a83f`.

**Method that found it, because it generalises:** frida (pip-installed, works
on the native Windows mwcceppc.exe; bypass sjiswrap and spawn the compiler
directly so the hooks land in the right process) hooking `0x511a30`,
`0x511fc0`, `0x511cb0` with caller addresses turned every "which path does
this store take" argument into a five-minute measurement. Forcing a candidate
predicate's answer in the running compiler (Interceptor onLeave
`rv.replace(1)`) measures a clause's exact tree effect BEFORE spending any
bytes; the byte implementation then reproduced the instrumented objects
bit-for-bit. Scripts in the session scratchpad (`probe_alias.py`,
`probe_cm3.py`, `force_cm.py`, `battery.py`, `fsolo.py`, `mkclauseH.py`).
Pitfall for the next user: a frida `InvocationReturnValue` is only valid
inside its callback — copy it (`rv.add(0)`) before storing, and capture
recursive helpers like `0x512e20` at depth 0 only.

Ruled out as frida A/Bs over the shipped clause (each measured as the delta
of only the widening, all inert on the units that clause H moved): candidate
size <= 8; dropping the store-side static gate; admitting declared-frame
candidates. Forcing the SCHEDULER table (0x5bd0bc) entries 2/6, 1/3, or all
nine to may-alias does not touch this population at all.

Stale attributions this supersedes: "zEntPlayer_SNDInit is not a clause-V
win waiting to happen" stays true, but its residual WAS one compiler defect
after all — this one. `xFXAuraUpdate`'s "different sub-case, the killing
store is indirect" reading was partly this too (+9.4 from clause H). The
`ourAnims`/`ZNPC_AnimTable_BossPlankton` "literal load hoisted over an
INDEXED frame store" candidate clause should be re-measured against this
compiler before anyone chases it — the store there is into a FRAME array,
which clause H's static gate excludes, so it is probably still open.
`zEntPlayer_SNDPlayStreamRandom` (99.108) did not move and its "no
intervening store, global CSE across blocks" reading stands; likewise
`Process__10zNPCBSandy` and `zCameraTweakGlobal_Reset` did not move.

### The store-to-load forwarding defect: FIXED IN THE COMPILER (clause F, 2026-08-25)

**This whole section is now historical.** Clause F (see `patch_compiler.py`)
suppresses store-to-load forwarding for whole plain-static objects: on the
value-numbering entry-0 store-kill path, a store whose base expression is a
plain static (word 5) gets a FRESH value number instead of recording the
stored value's, so the following load reloads exactly as retail does. Four
bytes (`xor esi, esi` on clause V's static discriminant path); the stock
0x50a2c0(memref, value) records when value != 0 and kills when value == 0,
and the entry-0 handler passes esi as value.

Measured 2026-08-25 (full ninja, DOL sha1 306526d9... intact):
matched_functions **8386 -> 8403 (+17 / -0)**, GAME exact 79.830 -> 80.226,
GAME fuzzy 99.1068 -> 99.1477, 20 sub-100 functions up, ZERO down, 12 units'
matched-count up, none down. Clause F ALONE was +16/-15 with the DOL broken:
under stock forwarding a source re-read of a just-stored static and a use of
the stored expression's value compile identically, so 15 functions were
spelled with a re-read where retail's source used the expression value or a
local (`cnt--; if (cnt == 0)` vs `if (--cnt == 0)`, chained assignments,
call-result temps). All 15 were respelled the same session — the census's
prediction that "every one of these must be reverted first or the measurement
will read as zero" was exactly right, in both directions.

The volatile-device census below was swept: devices removed from iSnd (5),
zNPCTypeTiki (2), zNPCHazard, zEntPlayer (5 incl. bbash_tmr and
sTongueDblSpeedMult), zScene (3), iModel (2), zCombo (5), zAnimList,
isavegame, xutil, xMath (rndseed), xFFX (3) — every beneficiary function
still byte-exact, and zAnimListInit ("no source form reaches 100%") crossed
to 100.0 once its device stopped fighting the clause. TWO devices survive
and are still load-bearing: `xPar`'s pool volatile (it also pins store
ORDER, which is scheduler territory) and `zMovePoint_GetMemPool`'s volatile
return (that forward happens upstream of value numbering — evidence that
retail's own change sits earlier in the pipe than clause F's reconstruction
of it). `zNPCTypeBossSandy`'s `sElbowDropThreshold` masks LICM, not this,
and stays.

Everything below this line is the pre-clause-F record, kept for the method.

### (historical) The store-to-load forwarding defect: isolated, and DEPRIORITISED

A 30-line repro reproduces it standalone with the shipped 2.0p1a compiler and
the project's own cflags. All three field shapes fall out of it:

```c
S32 g_cnt, g_max, g_out, g_arr[64];
void a(void) { g_cnt++; if (g_cnt == g_max) g_out = 1; }     /* Tiki Process */
void b(void) { g_max--; g_out = g_arr[g_max]; }              /* iSndSceneExit */
extern S32 find(void); S32* g_ptr;
void c(void) { g_ptr = (S32*)find(); if (g_ptr == 0) g_out = 2; } /* InitFX */
```

We emit, for `a`, `lwz r3,g_cnt / addi / stw r3,g_cnt / lwz r0,g_max / cmpw
r3,r0` -- the compare uses the forwarded `r3`. Retail reloads `g_cnt`. In `c`
the compare is even hoisted ABOVE the store (`bl find / cmplwi r3,0 / stw
r3,g_ptr`), where retail is `stw / lwz / cmplwi`.

Flag sweep on the repro: **no named `-opt no*` switch disables it** --
`nocse`, `nopropagation`, `nolifetimes`, `noglobal_optimizer`, `nopeephole`,
`noschedule`, `nodeadcode`, `nostrength`, `noloopinvariants` all still
forward. It turns on at the `-O2` threshold (`-O0`/`-O1` reload, `-O2`
upward forward). So it is gated by opt level inside the optimizer, not by a
flag, and there is no cheap toggle to bisect it with.

**Deprioritised on cost/benefit, not on difficulty.** Nearly every known
witness is ALREADY matched via the `volatile` device -- Tiki's `Process` and
`zNPCTiki_InitFX`, `NPCHazard::Discard`, and the iSnd sites. Fixing the
compiler would mostly let those `volatile` qualifiers be deleted, which is a
source-fidelity gain rather than exact bytes. Before anyone spends a session
on it, re-check the census below and count how many functions would actually
CROSS 100.0 as a result; when that count was last taken it was approximately
zero. Note also `xFXAuraUpdate` (85.294) is a *different* sub-case -- there
the killing store is indirect (`0x4(r31)`), not a plain static.

**Census of `volatile` sites installed for the store-then-reload defect.**
These are all reproductions of a reload retail genuinely performs, each
evidenced against the target, and each is file-local. But they MASK the
compiler-side fix: if the value-numbering path is ever widened to cover this
class, every one of these must be reverted first or the measurement will read
as zero. Keep this list current.

    iSnd.cpp          ua_stream_buffer, stream_buffer  (file-scope globals)
    iSnd.cpp          sinfo_array_max                  (file-scope global)
    iSnd.cpp          snd_id, strm_id                  (function-local statics)
    zNPCTypeTiki.cpp  cloudEmitter                     (file-scope static)
    zNPCTypeTiki.cpp  numTikisOnScreen                 (pointer-cast at the use)
    zNPCHazard.cpp    g_cnt_activehaz                  (file-scope static)
    zEntPlayer.cpp    sPlayerIgnoreSound, bbash_start_ht, idle_tmr
    zScene.cpp        oldOffsetx, oldOffsety, scobj_idbps (pointer-cast)
    zNPCTypeBossSandy sElbowDropThreshold -- masks a DIFFERENT defect: it
                      suppresses cross-block constant CSE (LICM), not
                      store-to-load forwarding. Kept because it is the closer
                      form (99.364 with, 99.349 without) and Process is
                      blocked by entry 4 regardless, so neither form banks
                      bytes. Revert it before measuring any LICM fix.

**ADDED 2026-08-22: `zEntPlayer.cpp` `bbash_tmr`**, at the single use site in
`zEntPlayerJumpUpdate` (`if (*(volatile F32*)&bbash_tmr >= 0.0f)`). It
qualifies -- it takes that function (1,460 b) all the way to exactly 100.000%
-- and it must be reverted before any measurement of a compiler-side fix.

**ADDED 2026-08-22: `iModel.cpp` `sEmptyAmbientLight`** (in `iModelInit`, 192 b)
**and `instance_camera`** (in `iModelStreamRead`, 628 b). Each takes its
function all the way to exactly 100.000% and each carries an in-source comment
saying it is a matching device. Three honest spellings were measured against
each (`*&`, cast, temp-then-assign) and all were bit-identical to baseline.

Two sites deliberately NOT added in the same unit, both by the same rule:
`zEntPlayerFloorUpdate`'s three store-then-reload clusters (`surfSlickTimer`,
`surfSlipTimer`, `surfFriction`), because a fourth entry-4 subrange x subrange
cluster at `0xa8/0xac(r1)` caps that function below 100.0 regardless; and
`zEntPlayer_Init`'s `drybob_anim_count`, where the device does not even reach
the site -- introducing an index local, with or without a volatile read, is
**bit-identical to baseline** because mwcc copy-propagates it away.

**ADDED 2026-08-25 to the not-added list: `zThrown.cpp` `sSNDLandTimer`**, at
the single site in `zThrown_Update`. Measured: the plain form is 99.794, and
`*(volatile F32*)&sSNDLandTimer < 0.0f` reproduces retail's reload exactly and
moves it to 99.915 -- but the residue is register allocation (r3/r4/r5 and
f7/f8/f9 permuted around the `lfsx` cluster), so the function is capped below
100.0 regardless and the device would bank zero bytes. Declined by the rule
below. Three other spellings were measured first and were all bit-identical to
baseline: `x = x - dt` instead of `x -= dt`, a named temp assigned then read
back, and the two combined -- mwcc copy-propagates all of them away, so the
volatile lvalue is the only form that reaches this defect.

One site was deliberately NOT added: `zSceneSetup`'s `gCurEnv`. Reading it
through a volatile lvalue does reproduce retail's `stw`/`lwz` pair and moves
the function 99.618 -> 99.743, but a second, scheduler-class cluster still
blocks it from 100.0, so the device would bank no bytes while adding to the
masking problem. The rule that follows: install this device only where it
takes a function ALL the way to 100.0.

Clause V does not fire on these. Clause V kills the *literal pool* across a
store to a small static; this defect is the compiler forwarding the *stored
value itself* to a following load of the same static. That is store-to-load
forwarding, a different transform, and it is reachable from neither patched
dispatch table. Eight-plus witnesses across four units now, which is the
argument for finding it rather than qualifying more variables.

**The store-then-reload shape appears three times in `zNPCTypeTiki` alone**
(`numTikisOnScreen` in `Process`, `cloudEmitter` in `zNPCTiki_InitFX`, and a
literal-inside-an-unrolled-loop variant in `SetCarryState`). Fourteen
non-volatile source spellings and eleven compiler-flag settings were measured
against it; nothing but `volatile` reproduces it. Note also that for
`numTikisOnScreen` a *whole-variable* `volatile` is disproven by the target --
it takes `zNPCTiki_PickTikisToAnimate` from 100.0 to 97.806, because the
volatile store stops mwcc sinking `li r0, 0` -- so the pointer-cast spelling is
the minimum-blast-radius form of the same fact, not a weaker one. Whatever the
real construct is, it is neither a statement-level rewrite nor a flag.

**`NPCHazard::Discard` re-measured 2026-08-21, after clause V shipped: the
`volatile` on `g_cnt_activehaz` is still load-bearing.** Removing it drops
`Discard` from 100.0 to **90.161** — mwcc collapses the
`subi r0,r3,1 / stw / lwz` triple into a single `subi r3,r3,1` and reorders
the store past the `srawi`. Retail genuinely stores and reloads the counter,
so this site is outside clause V's reach and the device stays. It also has no
measurable effect on `zNPCHazard_ScenePrepare`/`SceneFinish`, whose own
residual (a `stw` to the same symbol sinking past 24 unrolled null stores) is
a separate, unsolved issue.

Most-reloaded symbols, in case the trigger is narrower than "any global":
`__ctype_map` (7), `RwEngineInstance` (6), `cb_bink_sound` (6), `globals` (5),
`cb_bink_IO` (4), `gTRKCPUState` (3).

**Method warning — four separate attempts failed the same way**, all worth
knowing before writing any cross-object comparison: objdiff row *alignment*
hides a surplus when it pairs the extra instruction against one of ours (2/9
recall); raw operand text never matches because anonymous pool ids differ
(`@1171` left, `@531` right) and registers differ (0/9); collapsing all
anonymous ids to one token merges every distinct float literal in a function
(9/9 recall but 43% of all non-matching functions flagged); `relocation.
target_symbol` is an **index into that side's symbol list**, not a name, and the
indices differ between objects; and symbols of the form `name$1234` are
function-local statics whose ids also differ. Normalise all five before
counting, and require that *we* do not load a named symbol the target lacks —
that last check is what separates the defect from a plain wrong-symbol source
bug. The census script is `cen_2b_v7.py` (scratchpad), validated at 5/5 recall
on the re-verified witnesses.

**Byproduct worth mining: 30 functions reference a different named symbol than
the target does.** Some are naming artifacts, but the first one checked was a
real bug — `ZNPC_AnimTable_ThunderCloud` used `g_strz_roboanim` where the target
uses `g_strz_cloudanim`, fixed in `504aebf3` for 99.420% -> 100.000%. Triaging
the rest is cheap, high-yield source work.

**2c. Weak/deduplicated symbols. REWRITTEN 2026-08-12 — the old framing was
wrong.** It said our surplus `operator=` instantiations block units from
Matching. They do not. Most of them are harmless and always will be.

**The rule: a surplus or misplaced symbol blocks `Matching` only if this unit
is the symbol's owner in the retail link.** Every TU that used an `xVec3`
assignment emitted a weak copy of `__as__5xVec3FRC5xVec3`; the retail linker
kept exactly one and discarded the rest, so dtk's extracted objects cannot show
it anywhere except its owner. Ours emits it for the same reason theirs did, and
the linker drops it for the same reason. `symorder.py` will report that surplus
for every unit that assigns an `xVec3`, forever, and it means nothing.

Use **`tools/symowner.py <symbol>`** to resolve a name through
`config/GQPE78/symbols.txt` to an address and then through `splits.txt` to the
owning unit. `__as__5xVec3FRC5xVec3` lives at `.text:0x8000B264 scope:weak`,
inside `SB/Core/x/xBound.cpp`.

Proof the surplus does not block: `zEnt.cpp`, `zEntTrigger.cpp`,
`zPendulum.cpp`, `xSurface.cpp`, `xHudText.cpp` and `zVar.cpp` are all
`Object(Matching, ...)` today and all emit surplus `.text` symbols their target
objects lack — `zEnt` emits `__as__5xVec3FRC5xVec3` itself, mid-`.text`, and
`zVar` emits a **strong global** `__deadstripped_zVar__Fv` at position 0.

Re-triaged against that rule:

| unit | actual blocker |
|---|---|
| `xParSys` | `using_ptank_render`, which it owns — **fixed, unit now links** (`98560c47`) |
| `xClimate` | none. Its only symbol difference is xBound's. The blocker is `UpdateRain` at 92.405% |
| `zSurface` | `.sdata2` ordering of `@900`, the signed int->float magic — a 2a deadstripped artifact, see below |
| `xDebug` | `__as__10iColor_tagFRC10iColor_tag`, which it owns, one position too early |
| `xModel` | owns `__as__11RwMatrixTagFRC11RwMatrixTag` and misplaces it — but it also has two broken functions, so symbols are not the first problem |

**`xParSys` is the worked example.** It sat at 0 non-matching of 22 and could
not link because CodeWarrior emits a header-defined `inline` at end-of-TU while
the target has `using_ptank_render` at `.text` position 3, right after its first
caller. Moving the definition into the `.cpp` between `par_sprite_update` and
`render_par_sprite` put it exactly there and the unit linked, with the three
foreign-owned weak symbols still present and still harmless.

**`__deadstripped_<unit>()` does NOT make a unit unlinkable** — an earlier note
here claimed it did, on the basis of `xDebug`. `zVar` and `xHudText` both carry
the idiom and both link. In `xDebug` the stub is load-bearing for a different
reason: it is a call-forcing stub, not the `.rodata` template kind, and it is
the only thing causing ten weak inlines that `xDebug.o` *owns*
(`0x80017DA4`-`0x80018064`, ~704 bytes) to be emitted at all. Removing it loses
all ten. Keep it. `zFX`'s copy is likewise not a problem.

**`zSurface`'s real blocker** is that `@900` (`43 30 00 00 80 00 00 00`, the
signed int->float magic) sits after `zSurfaceUpdate`'s four floats in our
`.sdata2` and before them in the target's. The constant is allocated at the
TU's *first* signed int->float conversion in source order; in retail that
happened inside a function between `zSurfaceGetSlideStopAngle` and
`zSurfaceUpdate` that no longer exists. Proven with a throwaway probe: inserting
such a conversion into `zSurfaceGetSlickness` moved the constant to the target's
exact slot, `.sdata2` matched completely and `zSurfaceUpdate` reached 100%.
Reordering the `switch` cases does not move it and costs 38 points. So this is a
Phase 2a deadstripped-code artifact, the `__deadstripped_` exception does not
cover it (condition 1 fails — the object *is* referenced), and `dwarf/` lists no
zSurface function we lack.

**Method warning for header blast-radius sweeps.** An include-path overlay does
not work: mwcc's `-gccinc` own-directory rule resolves `#include "xParSys.h"`
from `src/SB/Core/x/` before any `-i` path, so the overlay is silently ignored
and the sweep appears to prove no change. Copy the whole tree and patch it.
Always run an `#error` positive control first.

### Another scheduler patch — measured 2026-08-12, and the answer is NO-GO

Sized before building anything, because the population had never been measured.

**First, a measurement artifact that invalidated the starting numbers.** objdiff
prints a local branch's target as a section-relative address, so every branch in
a function reads as a difference whenever the function sits at a different
`.text` offset. `classify.py` compared raw text and so inflated `OTHER` roughly
2x while hiding ~500 already-byte-equal functions inside it. Fixed in
`eebc2025`; buckets went OTHER 914 -> 443, POOL 150 -> 493, SCHED 26 -> 83,
REGS 23 -> 94. **The old "SCHED 26" floor was an artifact.** Anyone quoting
`classify.py` numbers from before that commit should re-run it.

**The real population**, with the two streams realigned independently and pool
ids, local-static ids, resolved relocation names and branch deltas all
normalised:

| class | project | game | converts on a scheduler patch? |
|---|---|---|---|
| SCHED sole blocker | 89 | 86 | yes |
| SCHED-modulo-registers sole | 87 | 85 | only if the reorder is pre-RA — unproven |
| SCHED **plus** a separate REGS window | 42 | 41 | **no** — needs both |
| REGS sole blocker | 88 | 87 | no |
| byte-equal (pool/placement only) | 545 | 543 | already 100.0 in report.json |

That third row is a class nobody had named. A naive count scores those 42 as
gains and they do not convert; carry the distinction so the next patch is not
over-priced.

**The filter that kills it.** A patch at the `0x511fc0` alias oracle is an alias
*edge* — it can only stop one memory reference crossing another. Of the 218
scheduling-sole functions, **78 contain a motion that crosses no memory
operation at all** (an `addi` sinking past an `addi`, an `mr` past an `fmuls`).
Those are pick-order/tiebreak, and every tiebreak mutation is already measured
and dead. At most 140 are reachable by any alias predicate, before asking
whether one predicate covers them.

**No cluster has a head.** Reducing each permutation to its motions gives 673
events across 218 functions; the largest single shape is 23 events / 18
functions and it is one of the unreachable ALU-past-ALU ones. The top 32 shapes
cover 292 of 673.

**The best candidate rule is clause D, which is already dead.** "A literal load
may not hoist over a stack store" converts **11** functions (14 if three
register fixes come free) — `zNPCGoalRobo` x3, `xFX::eval_joint`,
`xRayHitsSceneFlags`, `LeafNodeBoxPolyIntersect`, `KickOilGlobby`,
`auto_tweak::load_param<iColor_tag,i>`, `CollidePyramidBoxTop`, `NCIN_Zapper`,
`xFont::render_fill_rect`. Scanning all 7,251 functions currently at exactly
100.0, it puts **243 of them at risk across 821 sites — 17x the gain**, and
`xFont::get_texture_size` is the recorded witness of clause D knocking a
function off 100%. Broader rules are worse (the full-memory-barrier form reaches
33 and is the unconditional-may-alias experiment already measured at -144/-330);
narrower ones reach 4-7.

**REGS is not approachable, and the allocator is now mapped so nobody has to
ask again.** 88 sole blockers, of which 31 are a single register 2-cycle; the
biggest cluster is `f0<->f1` at 6 functions and the `ZNPC_AnimTable_*` family
is 3.

The allocator, read from `.text` on 2026-08-12 — previously unmapped:

- **RA/schedule driver `0x508680`**, once per function. Five-iteration loop over
  register classes (current class at `[0x5ea299]`); per class it compares live
  ranges `[0x5e9b04+class*4]` against physical count `[0x5e9800+class*4]` and
  runs build → simplify → select → spill.
- **Allocatable-mask builder `0x4fe4d0(class)`**, from `.bss` tables `[0x5e3b68]`
  (slot → register bit) and `[0x5e5c78]` (reserved flags).
- **Simplify/degree `0x508a20`**, iterating the live-range array `[0x5e9858]` by
  index — i.e. IR creation order — degrees at `[lr+0x12]`. Classic
  Chaitin-Briggs; this fixes the select-stack order.
- **Color/select `0x508900`**, the decisive one: it clears each interference
  neighbour's colour bit, then scans from bit 0 upward and assigns the
  **lowest-numbered free register**.

So the `r4`/`r5` difference is **not a tie-break knob** — it is coloring order.
Whichever live range is coloured first takes r4 and the other takes r5, and that
order comes from creation index. Changing it is a whole-program change with the
same blast radius as the tie-break experiments already measured dead
(ties-keep-earlier −1204). There is no narrow gate. That is *why* REGS is a
wall, not merely that it is one.

**The dependency-graph framing is mechanically impossible**, so that door is
closed too. The store builder `0x508350` creates WAR/WAW edges by walking only
the **backward** pending lists `[0x5e0866]`/`[0x5e0862]`. The discriminating
reload in `get_texture_size` is a *forward* instruction, five after the store, so
it is not in any list at edge-creation time — it only becomes visible later as an
outgoing RAW edge at the RAW builder `0x508100`. An earlier note here claimed the
builder "is given that information"; it is not. A reload gate would need a new
forward block-scan or a post-DAG cleanup pass, neither of which exists. And
empirically it would not have separated the populations anyway: gains are 32%
reloaded (10/31), losses 40% (36/89) — statistically indistinguishable.

**One untried framing, flagged not recommended.** The discriminator between
clause D's gains and losses is whether the stored slots are reloaded in the same
block. The alias predicate is not given that information — but the dependency
graph builder at `0x508100`/`0x508350`, which walks the pending load/store lists
at `[0x5e0866]`/`[0x5e0862]`, is. Installing the predicate there rather than at
`0x511fc0` is the only unexplored option. It is a much larger RE job, the
ceiling is still ~14 functions, and nothing about it has been measured.

Better uses of a session, by the same measurement: 98 game functions have no
symbol at all (39,896 bytes), 394 game functions are genuine source differences,
and `zNPCGoalRobo` and `zNPCHazard` each carry 17 scheduling-sole functions plus
real source work.

### Phase 3 — the near-miss sweep (~185 functions)

~~Cheap per function, but **run it after 2a**. A large share of the 185 are
pool-shift victims no source edit can fix.~~

**Corrected 2026-08-12: Phase 3 does not depend on 2a and can run now.** Pool
victims already read 100.0 in `report.json`, so by construction *none* of the
185 near-misses is a pool-shift victim — they are a disjoint population. What the
batch-1 measurement does say is that the near-miss pool is much thinner than 185
suggests: 7 of the 8 near-misses attempted were REGS/SCHED/2b and unreachable
from source. Triage by class before assigning, and expect a low hit rate at the
top of the percentage range.

### Phase 4 — the long tail (30 units, 623 functions, 410,192 b)

Real decomp work: `zEntPlayer` (88), `zNPCTypeRobot` (42), `zNPCGoalRobo`
(39), `zNPCHazard` (38), `xFX` (34), `zNPCTypeBossPlankton` (25). zEntPlayer
needs a dedicated multi-run campaign, never a shared batch — at 146 KB with 54
already-matched functions, a pool disturbance risks a lot at once. Plus the 98
absent bodies, concentrated in `zEntPickup`, `iParMgr`, `xShadowSimple`,
`zNPCGoalStd`, `xSnd`.

### Phase 5 — the queued shared-header changes

Deliberately last, applied one at a time with a full sweep after each. High
blast radius: `xVec3.h` reaches 188 TUs, `xClumpColl.h` 169, `xFX.h` 74. See
"Queued shared-header changes" below.

### Will this reach 100%?

**Not from source alone.** `xSFXUpdateEnvironmentalStreamSounds` had five
errors proved against the target and its control flow brought to an exact
match; eight variants measured, all *below* the untouched baseline. REGS-class
and SCHED residues have resisted every source shape tried across many
functions. `LassoNotify` is one unreachable branch instruction. These are
documented dead ends, not open puzzles.

The realistic path is **two tracks in parallel**: source work clears Phases 1,
3 and 4, while the compiler track (2b, plus whatever 2a's DWARF work reveals)
closes the last stretch. If the compiler track fails, the branch plateaus —
high, but short of 100%.

Recommended order: **Phase 1 first** (best leverage, lowest risk, visible as
whole units), with **2a's DWARF investigation alongside** — it is research
rather than edits, and it gates Phase 3.

## The playtest build (2026-08-24)

`python configure.py --non-matching && ninja` links **all 224 `src/SB` units
from our source** and every library unit (MSL, Dolphin, Runtime, rwsdk, bink)
from the retail objects, producing a runnable `build/GQPE78/main.dol`. The
object selection lives in `tools/project.py`, guarded on `config.non_matching`,
so the matching build and its sha1 are untouched. Plain `python configure.py`
switches back.

Without that guard `--non-matching` links only the 108 units marked `Matching`
or `Equivalent` and takes the other 116 from retail, so most of what runs is
not our code.

Getting it to link surfaced **22 defects that no amount of matching work would
have found**, because a symbol that is declared, called, and defined nowhere is
invisible until the whole game links at once. Four classes, all fixed:

| class | examples |
|---|---|
| declared-but-never-defined | `xVec3::normal`, `xNPCBasic::DBG_IsNormLog`, `xListItem<T>::Remove`, `gClimate`, `gPlayerPad`, `mRumbleList`, `dutchman_reticle_center` |
| overrides retail never had | eleven in `zNPCTypeRobot.h`; the vtable slots belong to `zNPCCommon` |
| undefined protected destructors | `~zNPCCommon`, `~zNPCGoalCommon`, `~xGoal` — all three "prevents implicit destructors" hacks |
| wrong call target | `xHudMeter` C++-mangled `printf`; `xpkrsvc` `char*` vs `const char*` |

Plus one symbol collision that only exists in this configuration:
`math_api.h` handed every TU an inline `__fpclassifyf` while `math_ppc.c` owns
the real one, so `xBound.o`/`xCollide.o` emitted competing weak copies.

**None of it regressed the matching build** — DOL sha1 held, zero units lost a
function, and Game Code went 79.2789 → 79.2852% exact (7193 → 7195 functions),
because several of these were genuine fidelity fixes rather than plumbing.

Two data-size mismatches were found by comparing symbol sizes against the
retail objects, one fixed and one still open:

- `sDummyEmptyJSP` was `xJSPHeader` (24 b) where retail reserves 32 — it is a
  `xJSPHeaderGC`, and `JSP_Read` already reported its size to callers as 32.
  **Fixed.**
- **OPEN:** `colls_grid` is 52 bytes for us and **156** in retail, so `xGrid`
  is missing 104 bytes of trailing members. Nothing reads past 0x30 today
  (`xGrid.o` is 14/14 at 100%, and every reference just passes `&colls_grid`),
  so it is latent rather than live, and the missing members are not guessable
  from the code we have. `xGridBound` is correct at 20 bytes.

Also noted, not fixed: the target defines `gDebugPad` in `xPad.o` immediately
before `gPlayerPad`; we define it in `xEntMotion.cpp`.

The `.bss` of the playtest DOL is 42 KB larger than retail's and `.text` is
6.7 KB smaller; entry point and boot section addresses are identical.

## First boot of the playtest build (2026-08-24)

It runs. The apploader loads our DOL (the `DVDRead` sizes in Dolphin's log match
our section table, not retail's), the Dolphin SDK initialises, and the game
reaches `main`.

**How to debug it.** Dolphin's GDB stub is the tool: launch with
`-C Dolphin.General.GDBPort=2159` and talk the remote protocol from a script.
Two things to know. The stop reply carries the registers you usually want --
`T05` **immediately followed by** `40:<pc>;01:<r1>;` with no separator before
`40:`, which is easy to mis-parse. And the stub queues an extra packet after an
interrupt, so a naive client reads every answer one query late; drain before
each query. With that, `p<n>` (67 = LR), `m<addr>,<len>` and `Z0` all work, and
the back chain at r1 unwinds into a full symbolised call stack against
`build/GQPE78/main.elf`. Breakpoints did not fire under JIT64.

**Bug 1, fixed (86856842).** `iFile::async_cb` treated every completed DVD read
as a failure, so no multi-chunk async load could finish. The boot stalled after
the first 32-byte read of `font.HIP`. With it fixed the DVD trace matches the
retail disc read-for-read -- same offsets, same DMA buffers, same lengths --
through all 222,400 bytes.

**Bug 2, open -- and narrowed hard.** The boot stalls one step later, before
retail reads `sb.ini`. It is a *true* infinite loop: five minutes, log frozen,
CPU climbing. PC sampling puts 28 of 31 samples inside MSL's `__timesdec`, and
the call stack (every symbol range checked, no mis-attribution) is:

    main -> zMainShowProgressBar -> zMainMemCardRenderText -> RenderText
      -> xtextbox::temp_layout -> layout::refresh -> layout::calc
      -> parse_next_jot -> parse_next_tag_jot -> parse_tag_height
      -> xatof -> atof -> __strtold -> __dec2num -> __num2dec_internal
      -> __two_exp -> __timesdec

`__timesdec` is retail MSL here, so the corruption is in what we hand it.
`parse_tag_height` does `xatof(ti.value.text)` on a substr that is *not*
null-terminated, which is safe only while the surrounding text is well formed.
The string in memory at the hang is well formed --
`{font=0}{h*2}{w*2}{color=FFFFFFFF}Loading...{~:c}` -- and `parse_tag_height`,
`parse_split_tag`'s `find_char` overloads and `xtextbox::set_text` are all at
100%, so the bad pointer comes from somewhere else in the layout walk. Prime
suspects, all non-matching and all on this path: `xString` at 93.841%
(`xStricmp` **56.10%**, `atox` 90.45%, `find_char(substr,substr)` 97.36%) and
`xtextbox::find_entry` at 86.89%.

Note `set_text(const char*)` passes `0x40000000` as the size deliberately, so
the walk is bounded only by the NUL terminator -- a pointer that escapes the
buffer runs a long way.

**What the live state says, and what it rules out.** Read over the GDB stub at
the hang:

- `atof` receives **the correct string**. The stack holds `0x803d89f7` ->
  `"2}{w*2}{color=FFFFFFFF}Loading...{~:c}"`, i.e. exactly the `2` of `{h*2}`.
  The textbox itself is intact: `tb$247.text.text` -> the full well-formed
  `{font=0}{h*2}{w*2}{color=FFFFFFFF}Loading...{~:c}`, size `0x40000000`.
- The nested `__two_exp` frames hold negative exponents that **double** with
  depth -- `-33, -66, -133, -267, -535` -- and each frame ends in `7ff00000`,
  the high word of `+inf`. So MSL is evaluating `10^huge` and overflowing.
  `f0` sits at `3.28e-307` with the same mantissa as `f1` (`0.92`), i.e. a
  value being scaled toward denormal. For the input `"2"` the exponent should
  be `0`.
- MSL is **retail code** in this build, so it is not the thing that is wrong.

Eliminated, each by measurement rather than reasoning:

| hypothesis | verdict |
|---|---|
| bad string / bad `substr` pointer | ruled out -- pointer and text read back correct |
| stack overflow into `.sdata2` | ruled out -- retail's SP sits in the same region (`0x803d8658`) |
| small-data 16-bit reach overflow | ruled out -- `.sdata2` is 15,884 b, `.sdata` 2,136 b |
| FPU state (rounding / non-IEEE) | ruled out -- retail's FPSCR is also `0x14`, MSR also `0xa932` |
| `xStricmp` / `imemcmp` / `icompare` | `xStricmp` was badly broken and is fixed (d27efcae); `imemcmp` differs only in register allocation |
| `find_char`, `set_text`, `parse_tag_height`, `parse_split_tag`, `calc` | all 100% matching |
| under-sized `xGrid` corrupting neighbours | not this hang -- `zGridInit` runs during scene load, much later |

So the remaining suspects are the non-matching functions still on this path:
`get_bounds` (**49.42%**, the worst in `xFont`, called per character via
`xfont::bounds`), `parse_next_text_jot` (94.20%), `find_entry` (86.89%),
`yextent` (98.35%). A garbage width from `get_bounds` would make `calc`'s
`line.bounds.w >= tb.bounds.w` test fire on every character; that is the
next thing to look at.

**Breakpoints do not work** in this Dolphin's GDB stub under JIT64 *or* the
cached interpreter -- `Z0` returns OK and never fires, even on `main`. Use PC
sampling and stack reads instead.

**Dolphin setup.** The `GQPE78.ini` game patch "EFB Copy Fix" pokes a byte at
`0x803CD04C` every frame. That address is in retail's `data7`; our data sections
moved, so it lands in our `.bss`. It must be disabled for playtesting -- but
Dolphin keys game INIs by game ID, so disabling it also disables it for a retail
copy, where it is a legitimate fix at above-1x internal resolution.

## Tooling

The tools that earned their keep now live in `tools/` and are documented in
`tools/README.md`: `solo.py` (compile+diff one unit without touching the
build), `symdump.py` (read a static table out of the target object),
`classify.py` (bucket the remaining functions by why they differ), `smoke.py`
(compile everything, report only failures), `flagsweep.py` (find a library's
real compiler flags).

**`tools/solo.py` counts differ from `report.json` counts, and both are
right.** For `zNPCTypeRobot`, `solo.py` reports 129 non-matching where
`report.json` reports 86 — but running `objdiff-cli` directly on the object
`ninja` built gives 129 as well. `solo.py` is faithful to the build; dtk's
report counts matched functions by a different rule. So never compare a
`solo.py` number against a `report.json` number. Use `solo.py` for
before/after *within* a unit, and `report.json` as the project metric.

Still scratch-only, because they are either superseded or too tied to this
machine to commit:

- `vary.py <src> <obj> <unit> <sym> <variants.py>` — patch a source snippet,
  rebuild just that object, report the percent, restore. Most REGS-class fixes
  here were found by feeding it 3-4 expression shapes.
- `pools.py <unit-frag>` — target vs ours data-symbol layout, for diagnosing
  POOL mismatches. `tools/symdump.py` covers most of what this was for.
- `candidate.py <unit-frag> <src-path> <cand1> <cand2>` — swap each candidate
  version of a source file into the tree, compile that one unit, and report
  exact-match counts plus which functions are exact in only one of them.
  Always restores the original. Written for merge-conflict resolution: it
  answers "is upstream's version of this file better than ours" with a number
  instead of an opinion. See "Merging upstream" below.
- `gh.sh <out.c> <func>...` — Ghidra headless decompilation of the
  symbol-bearing `sbgcM.elf`, ~5s per batch, and the only source of a starting
  point for the MISSING bucket. Not committed because it hardcodes a local
  Ghidra install and project path. Use
  `ghidra_11.3.1_PUBLIC_20250219`, not `ghidra_11.3_DEV` (too old for the
  project file). Batch 10-20 names per invocation; one call costs the same as
  twenty.

## The alias patch: clause C+, and what the predicate cannot reach

Shipped 2026-08-21. **Clause C+ on dispatch entry 0: +19 functions to exactly
100.0, 9,400 bytes, zero functions lost, DOL unchanged.** Game exact
73.214 -> 73.786 in one change.

Clause C excluded these sites through the **size test and the flags mask**,
not the storage gate. Of the four bits it excludes (0x08/0x10/0x20/0x40) only
**0x20** unlocks anything, and every site it unlocks is an indirect store such
as `stw r0, 0x4(r31)` whose memref reports the *pointee's* size rather than
the access width -- so `flags & ~0x86 == 0` and `sizeof <= 4` each
independently reject the pair. Clause C+ tolerates 0x20 and drops the size cap
on the store side only, keeping both static-storage gates and the
differing-opcode test.

**Entry 0 only.** On entries 1/3 as well it is +19/-3, and one loss is
`zPickupTableInit` in a *complete* unit, which breaks the link.

Space came from **deduplication**: the two literal copies of clause C are now
one shared body reached by a 10-byte `call` stub per entry, so free `.text`
padding went from 25 bytes to 43. The refactor was validated before any
semantic change -- the deduped *strict* clause C produced 8060 exact and not
one changed function across all 451 units.

### Clause V: the value-numbering store kill (SHIPPED)

The site above was found and patched, 2026-08-21. **+25 functions to exactly
100.0, zero lost, DOL unchanged, game exact 74.511 -> 75.732** -- the largest
single change in this project's history.

**How it was located, since the method generalises.** `mwcceppc.exe` still
contains its `__FILE__` strings (`Alias.c`, `ValueNumbering.c`, `Scheduler.c`
...) for the `CError_FATAL` call sites. Scanning `.text` for the *address
constants* of those strings gives a module map; a naive `E8`-scan call graph
then answers "who calls what, from which module". That separates two Alias.c
entry points:

| routine | table | called from |
|---|---|---|
| `0x511fc0` | 3x3 at `0x5bd0bc` | Scheduler.c, CodeMotion.c -- clauses A/B/C/C+/E3n |
| `0x511a30` | 3-entry at `0x5bd068` | **ValueNumbering.c only** -- clause V |

`0x511a30(memref, instr)` is the **store kill for local value numbering**: it
dispatches on the stored memref's kind byte (`+0x2c`) and stamps a fresh value
number via `0x50a2c0`. Stock kills only the stored object and its precomputed
alias sets, so a `.sdata2` constant survives the store and the next statement
reuses it. Verified under gdb (wibo maps the PE at 0x400000; sjiswrap
relocates to 0x110000): the repro shows nine loads in value-numbering pass 1
and five in pass 2 -- the literal loads die there, not in the scheduler.

**Clause V, entry 0 only:** when the store's base expression is a plain static
object, walk the value-number list and also kill every object that is itself a
plain static of size <= 4. Both halves are load-bearing; each was measured
tree-wide:

| variant | result |
|---|---|
| shipped (static base + static <=4 kill) | **+25 / -0** |
| gate the store on size <=4 instead of static base | +11 |
| kill the whole list (`call 0x511a00`) | +26 / **-24** |
| kill every static regardless of size | +10 / -0 |
| filter killed objects on size alone | +25 / **-5** |
| filter on the kind byte instead of base expression | +25 / **-1** |
| entry 1 (subrange stores) | +1 / **-30** |
| entry 2 | inert |

Entry 1's -30 is the compiler agreeing with the large-object contrast above:
retail deliberately does *not* kill on large-object stores, and killing there
changes CodeWarrior's unroll factor.

The cave was re-assembled as one 413-byte position-independent block at
0x57ea4c (68 new bytes did not fit in 45 fragmented free ones): clause B is
now a single body reached by `CALL` from both handlers, and the two stock
answers share one tail. **The refactor was validated before clause V was
added** -- rebuilt from clean `GC/2.0p1`, all 451 units produced identical
object SHA-1s and not one symbol moved. 27 of 451 objects change and none
belongs to a complete unit.

`PATCHED_SHA1 = 918652d8063c37ff4d172244f4fcbfa88e0ea062`.

### Clause V does NOT need a same-object size exemption -- measured, inert

`xFXAuraUpdate`'s `sAuraPulseAng[0] += ..; [1] += ..; if ([0] > k)` looked like
a clause-V gap: retail reloads `[0]` after the store to `[1]`, the array is
`static F32[2]`, and clause V only kills statics of size <= 4. **The premise is
false.** Both `[0]` and `[1]` are *subrange* memrefs (kind 1), so the store
dispatches to VN table **entry 1**, and clause V (entry 0) is never consulted.
Proved by pointing each VN entry in turn at a kill-all stub on a 20-line repro:
only entry 1 changes the output. Adding a same-object exemption to clause V
measures **+0/-0, zero of 451 objects changed**.

The correctly-targeted version is worse. Stock entry 1 at `0x511b0e` already
walks the other subranges of the same object and kills each one whose
`[offset,size)` **overlaps** the store; `[0]` and `[1]` do not overlap, so it
declines -- correctly, by C semantics. Retail kills anyway. Defeating that
overlap gate (two bytes, `je` -> `nop nop` at `0x511bb3`) measures
**+0/-47**, and `xFXAuraUpdate` itself drops 85.294 -> 80.537. Losses include
`xShadowReceiveShadow`, `xBoxInitBoundOBB`, `zEntPickup_GivePickup`,
`zGridInit`, six `xFont` functions and five `xClumpColl`/`xScene` routines.

And even that is only half: the stored value is still forwarded. The other
half lives on **scheduler entry 4** (subrange x subrange). Entry-1 no-overlap
plus entry 4 answering may-alias *does* reproduce retail's sequence exactly on
the repro -- and measures **+16/-560** tree-wide, matching the docstring's
existing -199 warning for entry 4. `xFXAuraUpdate` still does not reach 100.
**This direction is dead.**

**`gFrameCount` hoisting is a THIRD site, reachable from neither table.** With
all nine scheduler entries answering may-alias unconditionally *and* all three
VN entries killing everything, the `lwz r4, gFrameCount@sda21` is still hoisted
out of the 8-unrolled loop, byte-identical to stock. It is decided in
loop-invariant motion or global CSE. Anyone chasing it needs to find that site
first, the way `ValueNumbering.c` was found.

### VERIFIED: `zEntPlayer_AnimTable` is source-correct; clause E3n breaks it

Measured 2026-08-22, and then re-measured independently by me rather than
taken on report. 23,820 bytes, the largest single non-exact game function.

    zEntPlayer_AnimTable__Fv   tree (GC/2.0p1a)     97.249%
                               stock GC/2.0p1      100.000%
                               entry-3 ablated     100.000%
                               control (chk)        97.249%

The control is the whole patch rebuilt from pristine `2.0p1` by
`patch_compiler.py`'s own constants; it reproduces `PATCHED_SHA1`
`19480c5d...` byte-for-byte, so the ablations differ from the shipped
compiler by exactly one dispatch entry and nothing else.

**Nothing in `zEntPlayer.cpp` needs to change.** All 452 differing rows sit in
blocks that store a call result into the frame arrays `tranTbl1`/`tranTbl2`
(`stw r3, 0x18(r1)`, `0x1c(r1)`, ...). E3n pins that `stw` ahead of the
block's `lfs @NNN@sda21` / `lwz 0(rN)` static loads. Retail sinks it thirteen
instructions, to just before `mr r3, r31`, which keeps **r3 live** across the
whole argument setup; retail's allocator therefore cannot use r3 as scratch
and takes r4/r5/r6/r9, while ours frees r3 immediately and takes r3/r4/r5/r8.
One root cause, 452 rows.

**Re-priced tree-wide** (all 224 `main/SB/*` units, `2.0p1a` vs entry-3-on-
clause-C, solo basis): removing E3n is **+6 functions / +25,924 bytes** and
**-107 functions / -77,224 bytes**. `zEntPlayer_AnimTable` is 23,820 of the
25,924 -- 92% of the entire win. Other winners: `MoveNormal__14zNPCGoalPatrolFf`
716 b, `BasisBspline__FPA4_fPf` 576 b, `Process__18zNPCGoalJellyBirth` 348 b,
`xBoxFromCircle` 256 b, `get_bounds` (xFont anon) 208 b. Biggest losers:
`Process__12zNPCBPatrickFP6xScenef` 6040 b, `_xCameraUpdate` 3560 b,
`ConfigHelper__9NPCHazardF9en_npchaz` 2940 b, `Process__8zNPCTikiFP6xScenef`
2380 b.

**E3n stays.** But the narrowing is now the single largest identified win on
the board. NOT YET ACHIEVED: it means writing new x86 into the cave against
struct fields nobody has identified, and a guessed gate recorded as a rule is
worse than no rule.

**Both obvious gates are DEAD. Measured, three witnesses:**

    zEntPlayer_AnimTable     stw  into a 32-byte frame array     over-fires
    zLightningFunc_Render    stfs into a stack array element     over-fires
    turning__12zNPCDutchmanCFv  stfs/stw into an 8-byte xVec2    over-fires
    E3n's 19 motivating wins stfs to a scalar frame local        CORRECT

- **Store-opcode class (float vs integer) is dead:** the over-fire cases span
  both `stw` and `stfs`.
- **Frame-object size is dead:** the third witness is an 8-byte aggregate,
  which sits *between* the scalar wins and the arrays. No threshold at 4 or 8
  separates them. I promoted this gate on two witnesses and the third refuted
  it -- recorded so nobody re-promotes it.

**That hypothesis is dead too, and for an instructive reason.** I proposed that
the over-fire cases are PARTIAL writes to a multi-field frame object where the
motivating wins write a WHOLE scalar. Measurement: `[memrefA+0x2c] == 1`
(subrange) and `[memrefB+0x2c] == 0` (whole) at **every** E3n site, winner and
loser alike. Partialness is already fixed by the dispatch entry -- entry 3 *is*
subrange x whole -- so it cannot possibly discriminate within entry 3. A
whole-scalar store never reaches clause E3n at all.

**THE NARROWING ATTEMPT FAILED, and the failure is conclusive rather than
incomplete.** A dedicated session read the actual field values out of the
compiler's structures rather than guessing:

- **Frame-object size: FALSIFIED, not merely unfound.** Repros with declared
  frame arrays of 8, 12, 16, 24, 32 and 64 bytes plus 12- and 32-byte structs,
  reading every dword of the store's base-expression node from +0x00 to +0x68
  by bisection: **all eight shapes read identically in every field.** The
  descriptor carries no size, no element count, no array flag, and no
  reachable `Type*`-with-size.
- **Store opcode class: dead, and worse than thought.** A "FP stores only"
  gate (`opcode >= 50`) costs four zEntPlayer winners on its own
  (`CheckObjectAgainstMeleeBound`, `zEntPlayer_Damage`, `update_camera`,
  `WallJumpCallback`). Three need E3n to fire on `stb`/`sth`; `WallJumpCallback`
  needs it on **`stw`** -- the same opcode as the over-fire.
- 204-probe grid over every dword of memrefA/memrefB/insnA/insnB in both
  directions at six thresholds; 64-probe scan of the base node 0x00-0xa4; a
  full opcode ladder on both operands; and a sibling-alias-list-length gate.
  **Not one probe reached loser=100 with the winners kept.** The alias list
  length is exactly 1 at every site, winner and loser alike.

Both populations are real: stock `GC/2.0p1` keeps **0 of 11** zEntPlayer winners
while giving AnimTable 100.000. One rule must split them and **nothing in the
state clause E3n is given does**.

#### What the six unpromotable units actually are

Diagnosed 2026-08-31. The blocker is **weak out-of-line copies of inline
functions**, not function bodies -- every function in all six is already
100%.

CodeWarrior emits an out-of-line copy of an inline (`operator=`, a static
helper, a template member) into whichever object first needs it, and the
linker keeps ONE copy across the whole link. dtk carves the target objects out
of the linked dol, so a copy the retail linker resolved elsewhere is simply
absent from the target object. When our object emits that symbol, or emits it
at a different position, OUR copy wins the link, its address moves, and every
`bl` to it in every other unit moves with it. That is why promoting one unit
perturbs `xFont.o`, `xFX.o`, `xScrFx.o` and `xTRC.o`.

Comparing definition order has to be restricted to symbols present in BOTH
objects or the extras alone make every unit look reordered:

| unit | order of common symbols | extras in ours |
|---|---|---|
| `zAnimList` (control, promotes) | OK | 0 |
| `iCamera` | **OK** | 1 (`__as__6RwRGBA`) |
| `zSurface` | **OK** | 1 (`__as__5xVec3`) |
| `xDebug` | one symbol displaced | 1 |
| `xParEmitterType` | differs from #15/27 | 7 |
| `zShrapnel` | differs from #15/37 | 5 |
| `xHudMeter` | differs from #1/15 | 4 |

`iCamera` and `zSurface` have correct order, so their defect is content or the
extra symbol, not layout.

**`xDebug` is diagnosed and is NOT source-reachable.** Promoted alone it is
120 differing bytes, and the whole cause is that
`__as__10iColor_tagFRC10iColor_tag` sits at emission index 12 instead of 10
(target: `create__5xfont`, `__as__10iColor_tag`, `NSCREENY`, `NSCREENX`; ours
swaps the operator= to last). Two source attempts inside
`__deadstripped_xDebug` failed to move it: adding `iColor_tag c; c = col;`
(dead-store eliminated, no reference emitted) and then feeding `c` to
`xfont::create` so the assignment survives. Neither changed the emission
index, so the position is not controlled from that function. The reason is
almost certainly that all six real functions in `xDebug.cpp` are `// Redacted.
:}` empty stubs -- it was retail's actual bodies that fixed where the operator=
copy got queued, and a synthetic `__deadstripped_` helper reproduces the symbol
SET but not the ORDER. Do not spend another session on xDebug's ordering
without first recovering those bodies.

**`__deadstripped_` helpers are not themselves a blocker.** They link `UNUSED`
(confirmed in `main.elf.MAP` for `__deadstripped_zVar__Fv` and
`__deadstripped_xHudText`), so they never reach the dol.

#### Unit promotion is an audit, and six of seven units fail it

`tools/promotable.py` lists units whose every function is 100% but which
configure.py still links from the extracted object. Promoting them is the only
check that ever looks at the whole object: `report.json` pairs symbols by name
and normalises pool ordinals, so it cannot see definition order, data contents
or section sizes. The DOL sha1 can.

Tested one unit per build on 2026-08-31, from a green `306526d9`:

| unit | result |
|---|---|
| `SB/Game/zAnimList.cpp` | promotes clean, now `Matching` |
| `SB/Core/x/iCamera.cpp` | DOL -> `7cbf6455` |
| `SB/Core/x/xDebug.cpp` | DOL -> `8a32ae8b` |
| `SB/Game/zSurface.cpp` | DOL -> `9591ca65` |
| `SB/Core/x/xHudMeter.cpp` | breaks (in the combined run) |
| `SB/Core/x/xParEmitterType.cpp` | breaks (in the combined run) |
| `SB/Game/zShrapnel.cpp` | breaks (in the combined run) |

So six units are 100% on every function and still are not the target object.
That is a real defect class, and the only one `report.json` structurally cannot
report. Each is worth a session on its own terms.

**Two traps, both hit while measuring this.** Promoting several units at once
and then mapping the differing DOL bytes back through `main.elf.MAP` gives
CONFIDENT AND WRONG attribution: the first unit to change size shifts every
address after it, so the positional byte diff blames whatever happens to sit at
those offsets. The first attempt named three units this way and the real answer
was six. Promote one unit per build. And do not try to substitute a direct
comparison of our `.o` against `build/GQPE78/obj/**.o`: dtk reconstructs those
from the linked DOL, so weak inline out-of-line copies, `.comment` and `.text`
section accounting differ for reasons that are not defects.

#### Re-measured 2026-08-31 on the C-sourced patch, with the load-size gate

Two things settled, now that `tools/variant.py` makes an ablated compiler a
40-second edit rather than a hand-assembly job.

**E3n's exact ledger is +115 / -5, not +82 / -5.** Ablating E3n alone
(`tools/variant.py`, then `tools/patchcost.py --stock GC/<variant>` both ways)
gives 115 functions / 80,696 bytes that E3n earns and 5 it costs. The five are
`xBoxFromCircle`, `BasisBspline`, `MoveNormal__14zNPCGoalPatrol`,
`Process__18zNPCGoalJellyBirth` and `zEntPlayer_AnimTable`. The sixth function
in the patch's -6 is `Setup__11zNPCFodBzzt`, which E3n ablation does not move
and which is entry-0/clause-C+ over-fire.

**The load-size gate is the last untried field, and it fails hardest of all.**
E3n's operand gates are asymmetric -- store `> 4`, load `> 8` -- which reads
like it was meant for 8-byte double loads. All five victims load 4 bytes, so
`AL_SIZE(B) != 8` recovers every one of them. It also costs **115 functions /
80,696 bytes**: E3n's entire yield is the 4-byte case, and `-sz8` is
indistinguishable from `-noE3n` on an 11-function panel. Do not re-try a size
gate on the load operand.

**The victims' source is not the problem, and this is provable rather than
argued.** All five are 100.000% under stock `GC/2.0p1` with the source
untouched. A function that reproduces retail byte-for-byte under the stock
compiler cannot be a source-shape error, so "our source is written differently
from retail's" is ruled out for these five specifically -- no source edit is
available or wanted. Reading the diffs agrees: every one is the same single
decision, an `@NNNN@sda21` load that retail hoists above a run of frame-local
stores and E3n pins below them. `zGooCollsBegin` is the mirror image, retail
keeping the load below the store with E3n correctly preventing the hoist, and
it is structurally identical at the query site. Same operands, opposite correct
answers.

**Per-unit compiler selection would buy +2, and is a fit rather than a
finding.** E3n's winners and victims are not evenly spread: `xMath3` and
`xSpline` contain a victim and no winner, so compiling just those two units
with an E3n-ablated compiler is +2 functions. `zNPCGoalStd` and
`zNPCGoalAmbient` are 1-for-1 washes; `zEntPlayer` is +1/-13 and must keep
E3n. This is recorded as available, not recommended: retail was built with one
compiler, per-unit *flags* have a real counterpart in the original build but
per-unit *compiler binaries* do not, and adopting it makes "what does E3n
cost" globally incoherent. Worth doing only if someone decides the branch's
number matters more than the patch staying interpretable.

**Tooling note.** `tools/patchcost.py` had a bug that made every explicit
`--stock X` sweep silently measure zero units: `X` does not start with `-`, so
it was also read as a unit-name filter. Any pre-2026-08-31 result of the form
"variant Y costs nothing" that came from `patchcost.py --stock` is void and
needs re-running. Per-function `solo.py --mw` results are unaffected.

**So any real fix must change WHAT THE PREDICATE IS GIVEN, not what it tests.**
Two openings, neither attempted: put the gate in the dependency-graph builder
at `0x508100`/`0x508350`, which unlike `0x511fc0` can see the pending
load/store lists; or extend the base-object node, which is a compiler-wide
allocation change rather than a cave patch.

**Source-side mitigation that works today:** `const` on the read-only frame
aggregate cleared the gate entirely on the third witness -- 87.870% patched
before, 99.259% under every compiler after. Verified by checking out the
pre-fix blob and measuring both ways.

### zLightning: BOTH functions blocked. Do not send another agent at this file.

`RenderLightning` (2,948 b, 99.028%) and `zLightningFunc_Render` (1,580 b,
96.886%) were worked hard on 2026-08-22 and neither is source-reachable.
**Identical instruction counts on both sides** in both functions (737/737 and
395/395) -- there is no missing `case`, no wrong operand, no fused-vs-rounded
multiply anywhere. Every differing row is register renaming or ordering.

- `RenderLightning`: 106 rows in four clusters, the largest (63 rows) being a
  single cyclic rotation of the volatile register pool by two positions.
  **The decisive fact:** the function's FIRST `for (i = 1; i < last; i++)` loop
  is byte-exact, including `srwi. r0` for `flip` and `clrlwi. r5` for `i & 1`.
  The SECOND loop is the same source shape, and retail allocates it
  *differently from its own first loop* (`srwi. r3`, `clrlwi. r0`) while our
  compiler makes the same choice in both. Retail's compiler is not consistent
  with itself across two structurally identical loops in one function, so no
  single source spelling can satisfy both -- and we already match the first.
- `zLightningFunc_Render`: a callee-saved GPR permutation (r16/r17 and
  r24/r25/r26), which `xShadowSimple_Add` already established is not reachable
  from declaration order, plus an entry-4 same-array subrange store reorder.

Twelve source variants measured, all <= baseline; the two best were
bit-identical and the rest lost ground (worst: removing the `cr/cg/cb` temps,
-5.7 points).

**E3n over-fires here too** (measured with `verify_mw.py`): `zLightningFunc_Render`
is 96.886% patched and 98.635% under both stock `2.0p1` and `2.0p1a-no3`,
with entries 0, 1 and V all inert. It does not reach 100 either way so it is
not bankable, but it is the SECOND witness that E3n's over-firing is real and
recurring -- and its store site is `stfs` into `param[]`, an element of a stack
**array**, matching zEntPlayer's `stw` into the `tranTbl` **array**. Both
over-fire sites are frame arrays; E3n's motivating shape is a frame **scalar**.
That is the evidence behind the frame-object-size gate proposed above, and it
is evidence *against* a store-opcode-class gate, since one witness is an
integer store and the other a float store.

### The GPR scope hypothesis is now TESTED, and the answer is NO

The "OPEN CONFLICT" section below asks whether a bare declaration moves a GPR
colour *when it crosses a scope boundary* -- `PlayerCollsSelectDepen` said no,
`PipeForAllSceneModels` said yes -- and marks it untested.

It is now tested, from the other direction. Moving three initialised `U8`
declarations in `RenderLightning` from **function scope into an inner block** --
a real scope change with the definition point held fixed -- is **byte-identical**:
same percentage, and every objdiff relocation index unchanged, with only
anonymous pool ids renumbering. **A scope change on its own does not move a GPR
colour.** Whatever produced the `zScene` result depends on something else, and
the scope-crossing hypothesis should not be carried forward as the explanation.

### CAVEAT: naming an anonymous temp does not always pull it into the ordered set

These notes record "give the value a name" as *the* escape from the rule's
limit, on the strength of `xShadowSimple_CalcCorners`. It is not unconditional.
In `RenderLightning` the mis-coloured values `cg`/`cb` are **already** named
locals; the rule says defining them first should order them first; and defining
them first (two separate variants) moves only `cr` into the low pool while
`cg`/`cb` stay at r23/r24. So mwcc's live range for a local that is just a load
from a global does **not** reliably start at its source definition point -- it
behaves as though rematerialised at the use. Naming is necessary, not sufficient.

### Do not optimise on DIFFERING ROW COUNT. Use the percentage.

objdiff gives partial credit per instruction, so the two disagree. A measured
case: a `zLightningFunc_Render` variant produced **fewer** differing rows (43 vs
45) at a **lower** percentage (96.486 vs 96.886). Row counts are for cluster
bookkeeping only; `match_percent` is the thing report.json is built from.

### More dwarf counter-evidence (add to "dwarf is not an oracle")

For `zLightningFunc_Render`, dwarf lists a single function-scope `signed int i`
where four separate `for`-init counters measure 0.46 points better, and its
declaration order `numVerts, u, aVal` measures 0.05 points worse than our
`alpha, tex, nvert`. Separately it lists **no** `cr/cg/cb` locals at all, yet
removing ours costs 5.7 points -- so the GameCube build needs temps the PS2
DWARF has no name for. On the other side of the ledger, `dwarf/SB/Game/zLightning.cpp`
gives a third and fourth witness for the `RwRGBA* _col` macro form, listing
exactly one `class RwRGBA * _col;` per `RwIm3DVertexSetRGBA` invocation (12 and
4 respectively), matching our call sites one-for-one.

**Retail quirk, verified faithful rather than ours:** `RenderLightning`'s second
`for` loop has no `else { lastdir = dir = up; }` arm where the first loop does,
so in the `flags & 0x200` case it computes `flip` from values left over from the
first pass. Instruction counts match exactly, so retail shipped that asymmetry.
Flagged for PCPORT, not to be "fixed" here.

### The load-hoist-over-`stw` defect IS source-reachable. Retry it.

These notes file `xFX::eval_joint` as compiler-track: "predicted to flip and
did not... the surviving defect is presumably the `stw`, so the store side of
the clause declines there". `zCameraUpdate` has the same shape -- a small-static
`lfs` hoisted above the `stw r0,0x30(r1)`/`stw r3,0x34(r1)` pair of mwcc's own
int-to-float conversion scratch -- and **it is reachable from source**.

Binding the conversion result to a named local blocks the hoist:

    F32 dp = (F32)(MAX(32, MIN(x, 110)) - 32);
    dp = 0.016666668f * (dp * zcam_pad_pyaw_scale);

Four sites, **+0.95 pp**. The `fmuls` operand order corrects as a consequence.
**Retry this anywhere an `lfs <named global>` sits at the head of a clamp block
instead of at its use site.** Do not assume the store side of a clause
declining means the function is compiler-track.

**REFUTED FOR `eval_joint` SPECIFICALLY (2026-08-22, same day).** I predicted
the transfer in this very section and it does not hold. Binding the conversion
results to named `F32` locals in `eval_joint` is **bit-identical** (same diff
sha1), as are `(F32)` casting, bare-decl-then-assign, and adjacency changes.
Structural permutations do move rows (82.865, 64.250), so it is not entry 4 --
the baseline is simply already the best form.

The reason is worth more than the lead was. `eval_joint` is 105 rows,
byte-identical except that the `lfd` of the u32->float magic double sits after
`stw r5,0x14(r1)` in retail and five instructions earlier in ours. The pair is
(8-byte **whole** `.sdata2` load) x (**whole** 4-byte declared frame local
`alpha`), which dispatches to the **whole x whole** entry -- and E3n lives on
entry 3. The old note guessed "the store side of the clause declines"; the
truth is the clause is never *consulted*. Precisely-shaped candidate: **E3n's
rule applied to the whole x whole entry**, worth 416 b.

Generalise the method, not the fix: check which dispatch entry a pair actually
reaches before predicting that a source lever will move it.

### The E3n `const` lever is a GAIN lever, not only loss-recovery

These notes frame `const`-on-a-frame-local as the thing that recovered E3n's
ten regressions. It is more general. `const xVec3 tran_accum = cam->tran_accum;`
moved `zCameraUpdate` **+0.6 pp** with no E3n regression anywhere in sight --
and the `const` is correct on its own merits, since the local is never written,
only read at five sites. The idiom to scan for is `xVec3 local = <expr>;`
sharing a block with `.sdata2` literal loads, regardless of E3n history.

### TWO PRECONDITIONS on the "name the anonymous temp" escape

Found independently by two agents on the same day, in different units. The
escape (from `xShadowSimple_CalcCorners`) is recorded here as *the* way out of
the rule's limit. It has preconditions, and both were measured:

1. **The named temp must be genuinely multi-use.** In `zCameraFreeLookSetGoals`
   the mis-coloured value is a single-assignment, single-use accumulator;
   naming it is copy-propagated away and is byte-inert (two spellings, measured
   twice).
2. **Naming is necessary, not sufficient.** In `RenderLightning` the
   mis-coloured values are *already* named locals, and defining them first
   moves only the first of the three into the low pool. A local that is only a
   load from a global behaves as though rematerialised at its use, so its live
   range does not reliably start at its source definition point.

### REFINEMENT: the bare-declaration loophole needs a SINGLE live range

"DECLARATION order beats DEFINITION order... use bare declarations to place a
value early in the colour order" did **not** hold for `zCameraFreeLookSetGoals`'s
`newPitchGoal`: four bare-declaration positions, one of them crossing a scope
boundary, are all byte-identical. The variable is reassigned on every path, so
it splits into separate single-def values that colour at their definitions --
i.e. it behaves like the GPR case.

Corrected wording: *the bare-declaration loophole reaches a local with a single
live range. A local reassigned on every path colours at each definition and the
loophole is inert.*

### 2b census correction: `zCameraUpdate` would NOT cross 100.0

These notes carry a standing instruction to re-count how many functions would
actually **cross** 100.0 before spending a session on the store-to-load
forwarding fix. `zCameraUpdate` (3,892 b) is a five-site 2b witness and must be
counted as a **no**: a `volatile`-read probe on all five sites measures 99.681%
with 19 rows left, and those 19 are four REGS clusters that survive the fix
(an `f2`/`f3` transposition in the `dlerp` block, `r4`/`r5` between
`lassocam_enabled` and the pad byte, a uniform +1 colour shift in the lassocam
d/h lerp, and one `fmuls` destination). The probe was removed rather than left
installed at sub-100, per the install-only-at-100.0 rule.

### Refuted: clamp-literal early colour is NOT CSE with an earlier literal

The hypothesis that a clamp literal gets its early colour by sharing a pool
entry with an identical literal earlier in the function is **wrong**.
Substituting a wholly distinct literal (`1e-30f`, a different pool entry) in
`zCameraFreeLookSetGoals` left the colouring bit-identical.

### dwarf is wrong about `zCamera`'s pad locals, in a checkable way

`dwarf/SB/Game/zCamera.cpp` lists the pad stick locals as `signed int x` /
`signed int y`. The GameCube target keeps the raw byte in a register and
re-issues `extsb` at each use, which is `S8` behaviour, not `S32`. **Our `S8 x`
is right and DWARF is not.** Add to the "dwarf is not an oracle" list.

### zCamera: what is left, and why

`zCameraUpdate` 98.988% -- blocked, see the 2b census entry above.
`zCameraFreeLookSetGoals` 99.714% (7 rows) -- unfinished, not proven blocked.
Byte-identical until the first `fmadds` of the velocity dot product: retail
accumulates in place (`fmadds f0,f3,f2,f0`, dest = third operand) and gives the
`0.0f` clamp literal f7->f2, while we colour the literal f0 first, pushing the
accumulator to f2 and shifting everything downstream. **20 spellings measured**
(accumulate form, named chain, `MIN` macro, `if`-clamp both polarities,
`0.0f - x`, `*= -1.0f`, right-association, term reorder, reciprocal,
`const xVec3&` binding, fresh local, three `newPitchGoal` positions); every
faithful one is bit-identical, every distorted one is worse.
`zCameraFlyStart` 94.850% (4 rows) -- our scheduler hoists `lwz r0,0x1c(r1)`
from the frame local `info` above `stw r0, zcam_flypaused@sda21`. Frame-load x
static-store is exactly what clause C's static-storage gate excludes on purpose.
NOT entry 4 -- source permutation does move the rows -- so it is in contact with
the scheduler, but no faithful spelling reaches it.

### SECOND patch-cost witness, on entry 0: `xFXRenderProximityFade`

Verified by me, not taken on report. 1,612 bytes, `src/SB/Core/x/xFX.cpp`:

    tree (2.0p1a)      95.273%      2.0p1a-no1     95.273%
    control (chk)      95.273%      2.0p1a-no3     94.814%
    stock GC/2.0p1     99.504%      2.0p1a-noV     95.273%
    2.0p1a-no0        100.000%

Note the shape, which is unlike `zEntPlayer_AnimTable`: it reaches 100.0 ONLY
with **entry 0 ablated and the rest of the patch still on**. Stock is 99.504
and the full patch is 95.273, so the other clauses are worth +0.5 here while
entry 0's clause costs the last stretch. **Do not touch this `.cpp`** -- the
source is correct.

**Do not ablate entry 0 either.** Within xFX.cpp alone that trade is +1/-10
(losing `activate_ribbon`, `xFXShinyRender`, `xFXRingUpdate`,
`xFXFireworksUpdate`, `render_strip`, `xFXBubbleRender`, `xFXAuraAdd`,
`xFXShineUpdate`, `xFXStreakStart`, `xFXStreakUpdate`). Entry 0, like E3n, is
a **narrowing** target.

So there are now two independent dispatch entries with measured over-fire
costs. When a near-100% function resists source work, run the full matrix --
`tree / stock / no0 / no1 / no3 / noV / chk` -- not just stock.

Clause V has two cost witnesses in this unit as well, neither bankable:
`xFXShineRender` 90.425 -> **98.219** and `xFXStreakRender` 65.481 -> **93.415**
under `noV`.

### xFX: what is blocked, with the stop tests run

Six functions in this unit are completely patch-insensitive (identical under
all seven compilers) and therefore pure source-track: `RenderRotatedBillboard`,
`eval_joint`, `tri_data::init`, `MaterialSetEnvMap2`, `get_normal`, and both
`xFXanimUV*SetAngle`.

- **`xFXanimUVSetAngle` / `xFXanimUV2PSetAngle`** (92 b each, 83.478). One
  instruction moved: retail issues `stfs f1, xFXanimUVRotMat0@sda21` into the
  `icos` return shadow, before both `li` address materialisations; ours issues
  the ready `li`s first. **All 24 store permutations measured**; the natural
  `abcd` order is what the source has and the best distorted form is `dcba` at
  90.870. Same multiset, same registers. SCHED, blocked. Fixing one fixes both.
- **`tri_data::init`** (164 b, 97.439). Callee-saved GPR permutation (r27-r30).
  `vi`-before-`v`, no-reference and bare-pointer forms are all bit-identical to
  each other -- inert, confirming the `xShadowSimple_Add` rule that declaration
  order does not reach callee-saved GPRs.
- **`MaterialSetEnvMap2`** (180 b, 95.444). 3 rows: two adjacent independent
  instructions swapped (`mr r31,r4` vs `lis r4,@stringBase0@ha`), the r5-vs-r4
  choice a consequence of r4 still being live. Same shape as the recorded
  `iPadUpdate` SCHED case.
- **`RenderRotatedBillboard`** (1,440 b, 99.389). The **entire** residual is 44
  rows of 360, **all `lbz`/`stb`**: the four colour bytes get r0,r3,r4,r5
  ascending in retail and r5,r4,r3,r0 in ours, at six sites. All 16 subsets of
  naming the macro arguments were swept. Naming `_r` is always inert
  (copy-propagated). **Naming `_a` alone -> 99.694**, fixing alpha *and* green
  and leaving only r<->b. Naming all four fixes the RGBA cluster but trades it
  for a whole-function callee-saved permutation (98.181). This is direct
  evidence on the open question of whether the real `RwIm3DVertexSetRGBA` macro
  binds `_a` to a temp: **it does something that binds `_a`.** That is a
  shared-header change in `include/rwsdk/rwcore.h` and needs a tree-wide sweep
  before anyone believes it.

### `get_normal`: a REAL numerical defect, kept even though it banks zero

`xFXRibbon::get_normal` (432 b) 90.046 -> 99.769, from two genuine source
defects, both confirmed against the target's own asm:

**Retail rounds each square to single before adding; we fuse.**

    8002AABC  fmuls f2, f8, f8
    8002AAC0  fmuls f0, f7, f7
    8002AACC  fadds f5, f2, f0

Our `dir.y * dir.y + dir.z * dir.z` compiles to `fmuls` + `fmadds`, keeping the
first product at full internal precision. That is a real difference in shipped
arithmetic. Naming the two products forces the rounding. **Flag for PCPORT, and
do not "simplify" the named products back into one expression** -- they are
load-bearing for numerical fidelity, not for the percentage.

Also: retail loads `dir.x/y/z` into f9/f8/f7 and reuses them **in arm 1 only**
(arms 2-3 reload in retail too, and binding them in all three arms measures
78.935). The final +0.46 came from the bare-declaration loophole, `F32 dz, dy,
dx;` then assigning `dx,dy,dz` -- the FP rule predicted the target's ascending
dz=f7, dy=f8, dx=f9 exactly.

**This was initially reverted on the "install only at 100.0" rule and that was
wrong.** That rule exists to stop *hacks* being left in at sub-100 (volatile
probes, dead code to shift a pool). It does not cover faithful source. A change
that makes our arithmetic bit-match retail's belongs in the tree whether or not
it banks a function -- correctness outranks the percentage, in both directions.

Blocked at 4 rows: an f5/f6 swap between the anonymous `-a` temp and the
`dy2+dz2` sum. Naming the sum is *worse* (98.935, three spellings), and
operand permutations (`(dy2+dz2) * -a`, `(0.0f-a)*...`) are bit-identical --
canonicalised.

### HAZARD: a stale `.bak` in the shared scratchpad silently reverts a file

This actually happened on 2026-08-22 and cost real time, so it is written down
in full. `src/SB/Core/x/xFX.cpp` was found mid-session holding the blob of
commit `4e30d04`, **two commits behind HEAD**, silently discarding `e1b2c66`
and `ae45307`. Measured cost while it was in that state: 16 non-matching
instead of 12, with `DrawRing` knocked from 100 to 89.674.

**Cause.** A variant harness stored its baseline at
`<scratchpad>/xFX.cpp.bak` -- a generic name in a directory the agent believed
was session-private -- behind this guard:

    BAK = ".../scratchpad/xFX.cpp.bak"
    if not os.path.exists(BAK):
        shutil.copyfile(SRC, BAK)      # <-- silently reuses a STALE file

`xFX.cpp.bak` already existed, left by an **earlier session's** xFX agent whose
work predates those two commits. The guard declined to overwrite it, so the
harness's "baseline" was two-commit-old content, and its first `restore()`
wrote that over HEAD's.

**Why it is nasty:** a reverted file shows in `git status` as an ordinary
` M src/...`, indistinguishable from a live edit. Nothing warns you.

**Two mitigations, both cheap:**
1. Harnesses must derive their baseline from `git show HEAD:<path>`, never from
   a filesystem snapshot taken at unknown time.
2. Namespace scratch files per agent (`scratchpad/<agent>/`), never a bare
   `<unit>.cpp.bak` at the top level.

The scratchpad currently contains this exact hazard for several other units --
`xCollide.cpp.bak`, `zScene.cpp.bak`, `iSnd.cpp.orig`, `iSnd.cpp.base`,
`zLightning.cpp.base`, `zLightning.cpp.orig`, `zNPCTypeRobot.cpp.orig`,
`zCamera.cpp.orig`, `zThrown.cpp.orig`, `xFX.cpp.bak` -- all of unknown vintage.
**Assume every one of them is stale.**

**Gating rule this forces.** When gating an agent's work, do not accept that a
file changed; read the diff and confirm it is the change the agent described.
A silent revert and a real edit look identical in `git status`, and only the
diff distinguishes them. Also attribute report.json deltas to units: a build
picks up every modified file in the tree, including other agents' in-flight
work, so a gain measured after a build is not necessarily the gain of the unit
you are gating.

### The scratchpad is SHARED between concurrent agents, not per-session

Two agents running in the same session get the same scratchpad directory, and
one overwrote another's `vary.py` mid-run. **Namespace scratch files** under a
per-agent subdirectory. Brief agents accordingly.

### `const` on a read-only local aggregate: a lever that keeps paying

It paid **five separate times in one unit** (zNPCTypeDutchman), worth +3.5 to
+11 points each, and it cleared an E3n over-fire outright. These notes framed
it as the 12-byte three-register `xVec3` copy form. It is broader:

- it applies to **8-byte `xVec2`** as well;
- it applies to aggregates whose initialisers are **non-constant expressions**,
  where CW still copies an anonymous zero template in before overwriting.

**Scan for `xVecN local = {...};` that is never written afterwards.** The
`const` is correct on its own merits in every such case, so this is a free
correctness-and-percentage lever, not a trade.

### Permuting sibling `const T&` binding declarations is a cheap mechanical lever

Six builds, ~15 seconds, and it closed two functions. `play_sound` went
96.458 -> 100.000 by swapping two of three reference declarations (2 of the 6
permutations hit 100), and `Initiate::Enter` 99.394 -> 100.000 by moving one
reference below two others. Worth trying before any deeper analysis on a
function whose residual is REGS and whose head is a run of reference bindings.

### CORRECTION: `LassoNotify`'s dead branch IS source-reachable

These notes list it as "one unreachable branch instruction... documented dead
ends, not open puzzles". Too strong. Adding `case LASS_EVNT_BEGIN: break;`
**does** produce the second `b` and takes it 96.429 -> 99.393. It still does not
close, because that case also reshapes the pivot tree (`cmpwi 3/beq/bge/cmpwi
2/bge` becomes `cmpwi 2/beq/bge/cmpwi 0/beq`). Correct framing: *the dead branch
is reachable, but not simultaneously with the target's tree.* The change was
reverted because the label is invented and it banks nothing.

### RE-PRICE the `xVec2::create` header change before spending a session on it

The `.sbss2` entry names five functions "whose *entire* residue is that shift",
`clip_outside_circle` among them. **`clip_outside_circle` (xVec2 overload) was
closed to 100.000% from source alone, with no header change.** So whatever holds
`create__5xVec2Fff` (44 b, 63.636%) back is not a shift that also gates its
neighbours, and the "five functions unblocked by one header change" pricing is
wrong. Re-derive it.

### Lead: retail's `xatan2` may return `double`

`update_turn__12zNPCDutchmanFf` (260 b, 94.308%, 8 rows): the target emits an
extra `frsp f0, f31` narrowing `angle` before `angle + diff`; we forward `f31`
unrounded. Eight source shapes measured, all <= baseline.

A redundant `frsp` is what CW emits when the value's producer is **double**-typed.
So the lead is that retail's `xatan2` returns `double`, not `F32`. The mangled
name `xatan2__Fff` encodes only parameters, so the return type is invisible to
the linker and this is testable without breaking anything. `xMathInlines.h` is
shared, so it needs a tree-wide sweep. **Unfinished, not blocked.**

### dwarf, both directions, in a single unit

Decisive and CORRECT three times in zNPCTypeDutchman: `update_wave` has no
`tanx`/`tanz`, `update_flames` no `gx`/`gz`/`tanx`/`tanz`, `Initiate::Enter` no
`ox`/`oz` -- inlining each was part of every fix. WRONG twice in the same unit:
`add_spray`'s dwarf omits `mult`, yet naming `mult` *and declaring it first* is
exactly what reaches 100.000%; and `check_player_damage`'s dwarf lists no `xBox`
locals at all though the GC target plainly has two on the stack. Use it as a
hypothesis generator, never as an oracle.

### THE DISPATCH TABLE, DECODED. This settles which clause can ever see what.

Read out of `0x511fc0` rather than inferred. Given two instructions:

    memrefA = [insnA+0x18]        memrefB = [insnB+0x18]
    index   = [memrefA+0x2c]*3 + [memrefB+0x2c]
    kind byte at +0x2c:  0 = whole object,  1 = subrange

So the table at `0x5bd0bc` is indexed by operand *wholeness*, and the entries mean:

    entry 0 = whole    x whole
    entry 1 = whole    x subrange
    entry 3 = subrange x whole
    entry 4 = subrange x subrange

**Consequences, several of which correct these notes:**

- **Clause E3n's store operand is ALWAYS a subrange**, never a whole object.
  Both this file's "an `stfs` to a **scalar** declared frame local" and
  `patch_compiler.py`'s "an `stfs` to a declared frame local" are wrong. A
  whole-scalar store cannot reach E3n.
- **E3n's winners are not all `stfs`.** At least three zEntPlayer winners need
  it on `stb`/`sth` and one on `stw`.
- **`eval_joint` sits on entry 0** (whole 8-byte `.sdata2` load x whole 4-byte
  frame local), confirmed from the formula rather than surmised. "E3n's rule on
  the whole x whole entry" is therefore a genuinely NEW clause, not a
  relocation of this one.
- **Entry 4 being permanently blocked now has a structural reading**: it is
  subrange x subrange, i.e. two partial accesses, which is exactly the
  same-array-different-elements shape.

**memref layout** (from stock entry-4's overlap test at `0x512012` and clause
V's list walk):

    +0x00  value-number list next     +0x1c  value number
    +0x08  sibling-list head          +0x24  alias bitmap
    +0x0c  computed-address flag      +0x28  alias bit index
    +0x10  base-object node           +0x2c  kind byte (0 whole / 1 subrange)
    +0x14  subrange offset
    +0x18  access size

**instruction layout:** `+0x10` latency, `+0x14` flags, `+0x18` memref,
`+0x20` opcode id (16-bit).

**The opcode id table is at VA `0x5c3070`** -- 791 entries, 20-byte stride
`{char* mnemonic, u32 id, char* operand_format, u32 mask, u32 encoding}`.
`lwz`=34, `stb`=40, `sth`=44, `stw`=49, `lfs`=142, `stfs`=150, `stfd`=154.
Verified twice over: the encoding fields match real PPC primary opcodes, and a
threshold ladder's flip points land exactly on 49 and 150.

### AVAILABLE BUT NOT INSTALLED: 24 free cave bytes

Cave space is the binding constraint on every future clause and only 8 bytes are
free. Three cave blocks re-implement code the stock compiler already contains:
`stock0` open-codes `0x511FF2`, `may` open-codes `0x512081`, and `e13h`'s tail
open-codes `0x511FFF`. Replacing them with jumps takes the cave **428 -> 412,
freeing 24**.

Measured: all 224 `main/SB/*` units compile to **byte-identical objects** under
the shipped `2.0p1a` and the compacted build (0 differing, 0 failed). Compacted
compiler sha1 `bf72c2e4180ebef769deb949c63cad4ecd7c6f24`; bytes in
`scratchpad/e3n/compact_cave.hex`.

**Deliberately NOT installed.** It changes `PATCHED_SHA1`, which invalidates
every agent's variant compilers and forces a re-patch mid-flight, and there is
no clause waiting on the space. Install it when a clause actually needs the
room, not before.

### Reusable compiler-RE assets in `scratchpad/e3n/`

Do not rebuild these from scratch:
`cave.py` (symbolic re-assembler for the whole cave, validated to reproduce
`CAVE_BYTES` byte-for-byte), `mkvar.py` (variant builder), `probe.py`/`scan.py`
(zEntPlayer oracle -- one compile yields both the loser and 11 winners),
`read.py`/`fieldread.py` (sub-second repro oracle that reads real field values
by bisection), `mnem.py` + `opcodes.json` (the decoded opcode table), and
`hashsweep.py`.

Also confirmed here: rebuilding from pristine `74bc177b...` twice gives
`19480c5dcb2c3de3b870c1fb29db73f14f7b2889` both times, so `PATCHED_SHA1` is
correct and the patch is deterministic; and the recorded E3n price of
+6/-107 functions and +25,924/-77,224 bytes reproduces exactly.

### `solo.py`'s no-symbol mode returns EVERY symbol, not just non-matching ones

A harness that assumed otherwise gave a false reading for an iteration. Filter
on `match_percent < 100.0` yourself if you write against it (this is what
`verify_mw.run(..., None)` returns too).

### NEW LEVER: `j = i, i++` -> `j = i++`. It LOOKS like SCHED and is not.

`PointWithinTriangle` (672 b) closed to 100.000% on this alone, at three sites.
The loop was `for (i = 0, j = 2; i < 3; j = i, i++)`; the target emits
`addi i,1` **before** `addi ptr,4` and we emitted them the other way round.
The comma form cannot reach it; `j = i++` can.

**Why this matters beyond one function:** the residual presents as *two adjacent
independent instructions swapped*, which these notes elsewhere tell you to
classify as SCHED and stop on. That guidance is right in general and wrong
here. **Before writing off a transposed `addi` pair in a loop increment as
blocked, try re-spelling the increment.**

### dwarf's PS2 sibling for iCollide: right twice, wrong three times

`dwarf/SB/Core/p2/iCollide.cpp` exists and is the PS2 counterpart of the GC
file. Decisive-CORRECT on `iSphereHitsModel3` and `sphereHitsEnv3CB` (local sets
match ours exactly). Decisive-WRONG on `FindNearestPointOnLine` and both ray
functions: it lists no `dx/dy/dz`, no `sx/sy/sz` and no `RwV3d temp` in
`iRayHitsEnv`, yet the GC target's asm proves all of them must exist. Another
entry for "dwarf is not an oracle" -- hypothesis generator only.

### iCollide: what is left, and why

The unit is patch-insensitive where it matters: `PointWithinTriangle`,
`FindNearestPointOnLine`, `iRayHitsEnv` and `iRayHitsModel` are identical under
all of `- / 2.0p1 / no0 / no3 / noV`. `sphereHitsEnv3CB` and `iSphereHitsModel3`
*gain* 4-5 points from the patch. Nothing in the unit reaches 100 under any
variant, so it is pure source track.

- **`iSphereHitsModel3`** 98.913% (920 b) -- BLOCKED. All 9 rows are in the
  64-bit `collide_rwtime += t1 - t0` accumulate: same multiset, different
  registers, and the target interleaves `stw` between `addc` and `adde`.
  `= a + (b-c)`, `+= named delta` and reversed operands are all bit-identical
  (canonicalised); split-into-two is 96.174. dwarf's local list matches ours.
- **`sphereHitsEnv3CB`** 97.427% (1,228 b) -- three residual shapes. (a) A
  `mr r31, rN` at the three `idx = X = cbnumcs++` sites: retail keeps the load
  in a scratch and copies into `idx`'s callee-saved register, we coalesce;
  three spellings bit-identical. (b) Retail reloads `NEXT2` after storing it and
  reloads `cbnumcs` after `cbnumcs--` where we forward + `clrlwi` -- the
  store-to-load forwarding defect, `volatile`-only, not installed. (c) Two
  f0/f1/f2 transpositions.
- **`FindNearestPointOnLine`** 97.903% (248 b) -- UNFINISHED, one FP tie-break.
  Naming `dx/dy/dz` fixed the load order; also naming `sx/sy/sz` snapped both
  register groups onto the target's (`dx,dy,dz`->f4,f5,f6; `sy,sx,sz`->f7,f8,f9)
  and `dx * mu` fixed the operand order. What remains is `mu` getting f1 where
  retail gets f2, with both candidates dying at the `fsubs` -- the
  `_xCameraUpdate` one-bit shape. Four declaration positions byte-identical.
- **`iRayHitsEnv`** 94.139% / **`iRayHitsModel`** 91.824% -- ONE shared root
  cause, blocked. 51 of 58 rows in the former are a callee-saved permutation:
  we materialise `&isx.t.line.end` early (`addi r31, r1, 0x20`) and hold it
  across six calls; retail materialises it at the swap. In `iRayHitsModel` that
  costs a fifth callee-saved register (`stmw r27` vs the target's four) because
  retail reuses r28 for `mat` and then `&end`. **The `addi` is hoisted across
  `bl` instructions, so it is IR-level, not the scheduler.** Seven source shapes
  measured; baseline is best.

### WARNING: the `const` lever can INVERT an E3n verdict. Re-measure old ones.

Measured on `DiscoRender__10zNPCSleepyFv` (zNPCTypeRobot):

    before `const vec_ray`:   tree 72.850   stock 76.904   no3 76.904
    after  `const vec_ray`:   tree 81.053   stock 75.016   no3 75.016

Before the fix this is a textbook E3n over-fire. After it, **the over-fire is
gone and E3n is +6 points on the same function.** The `const` removes the
declared-frame-object store that E3n was gating on, so the clause stops firing
there at all.

**Consequence: any E3n over-fire recorded BEFORE the const lever was applied to
that function is suspect and must be re-measured.** A patch-or-source verdict is
only valid for the source as it stood when the matrix was run. Re-run the matrix
after every source change that touches a frame aggregate.

### SECOND entry-0 over-fire witness: `Setup__11zNPCFodBzztFv`

Verified here, not taken on report. 288 b, `src/SB/Game/zNPCTypeRobot.cpp`:

    tree 95.347 | stock GC/2.0p1 100.000 | no0 100.000 | no3 95.347 | noV 95.347

**Source-perfect; do not touch it.** Note it is NOT quite the same shape as
`xFXRenderProximityFade`, which needed `no0` *specifically* (stock gave only
99.504). This one reaches 100 under stock and `no0` alike, so it is a plain
entry-0 over-fire rather than a patch-interaction case.

Ablating entry 0 is not free even locally: within `zNPCTypeRobot` alone the
trade is +1/-2 (`RendConeOfDeath` 89.515->87.767, `RendConeRange`
87.884->87.054). Entry 0 remains a narrowing target, not an ablation target.

A **fourth E3n witness**, not bankable but worth the file: `RendConeOfDeath`
(808 b) is tree 90.777 / no3 92.762 / stock 91.163. Callee-saved GPR
permutation underneath, so it never reaches 100 either way.

### METHOD: compare the instruction MULTISET before reading the diff at all

The single most productive move of a recent session. Normalise registers and
pool ids, then compare the multiset of instructions on each side:

- multisets **equal** -> pure REGS/SCHED. Go straight to the allocator and
  scheduler sections; do not read the diff line by line looking for a source
  bug that is not there.
- multisets **differ** -> a real source difference, and the *difference itself*
  names it (an extra `fmadds` against a `fmuls`+`fadds` pair; a missing switch
  dispatch; an extra `mr`).

It separated three real-source cases from six pure REGS/SCHED cases in about a
minute each, and it surfaced a fused-multiply defect that reading the diff
top-to-bottom had missed.

**DO NOT USE `scratchpad/robot/ms.py`. It is unsound and gave false verdicts.**
I recommended it in three agent briefs before this was caught; the method is
right and that implementation is not. Two defects, both confirmed by reading it:

- **It never normalises registers.** Its `norm()` strips only `@N@` pool ids and
  `$N` local-static suffixes, so **every REGS case reads as "multisets
  DIFFER"** -- exactly the verdict that sends you hunting for a source bug that
  is not there.
- **The column split is a fixed `line[3:50]` / `line[50:]`.** When a
  left-column instruction exceeds 47 characters `solo.py` pushes the right
  column further right, and the parser slices a mangled name in half. That is
  where phantom entries like `ONLY IN OURS: {'s': 1}` and `{'ertex': 1}` come
  from.

On one function it fabricated an instruction-count difference and simultaneously
**hid a real one-instruction difference** (`li 0x1f00` against our `li 0x1200`,
a genuine wrong-flag bug) behind a bogus branch-target normaliser.

Use `scratchpad/npcsup/{dj.py,ms.py}` instead: they read objdiff's JSON directly
rather than parsing formatted columns, and report both reg-sensitive and
reg-normalised multisets so you can tell the two classes apart.

### OPEN LEAD: the construct that keeps a fully-degenerate switch alive

`DoAliveStuff__11zNPCTubeletFf` (384 b, 91.667%) is a SIZE difference with a
understood cause and an unknown cure. The target emits a live 6-instruction
switch dispatch (`lbz 0x84(r3)` / `cmplwi 1,beq` / `cmplwi 2,beq` / `cmplwi 4`)
whose case bodies are **all empty** -- every arm branches to the instruction
after the switch. The switched-on value is dead in retail too (the `AdjustHome`
third argument is a `.sdata2` literal, not `wid`). Our mwcc deletes the entire
switch once dead-store elimination empties the bodies.

Ruled out: writing the cases explicitly empty (91.667, unchanged), adding
`default:`, and grouping BOX/OBB. The two extra `lwz` reloading
`drv_data->driver` afterwards are a *consequence* -- the live switch splits the
basic block and kills the CSE. **The construct that keeps a degenerate switch
alive in CW is unknown.** Whoever finds it also gets the reload behaviour free.

### More dwarf counter-evidence (zNPCTypeRobot)

Wrong twice: `zNPCTubelet::ParseChild`'s dwarf lists a `zNPCTubeSlave* slave`
local, and writing it **costs** 1.07 points (96.395 -> 95.326).
`zNPCSleepy::RendConeRange`'s dwarf entry is a *different function* -- it has
`rad_fadeinInner`/`rad_fadeinOuter` statics and none of `pos_top`/`pos_bot`/
`rgba_*`, so the PS2 and GC bodies genuinely diverge there. Correct once, and
decisively: `TurnThemHeads`' missing `pos` local is what closed it.

### zNPCTypeRobot: remaining classification

- `Unbonk` 116 b 99.586 -- **BLOCKED**, the known epilogue `lwz r31`-before-
  `lwz r0` class; 3 permutations, baseline best.
- `ParseChild` 344 b 96.395 -- **BLOCKED**, callee-saved permutation
  (`xShadowSimple_Add` class), 4 shapes all <= baseline.
- `NightLightUVStep` 200 b 60.700 -- **BLOCKED**, entry 4: `lfs uv_nightlight[1]`
  hoisted over `stfs uv_nightlight[0]`, i.e. subrange x subrange. Rewriting
  `+=` as `x = x + y` measures 65.800 and narrows it to that one hoist, but was
  NOT installed: that is not a fidelity question and it banks nothing.
- `ConeOfRange` 324 b 95.802 -- unfinished. We hoist an extra `lfs 0.0f` to the
  head of the `MAX(0.0f, MIN(pct,1.0f))` clamp to fill the `fdivs` latency;
  retail loads it once at the use site into f31 after `rad2` dies, serving as
  both compare operand and result. 4 spellings, none moved it.
- `DiscoUpdate` 468 b 95.769 -- unfinished, same shape (two
  `uv_discoLight[i]` loads hoisting above `stb rgba_discoLight.alpha`),
  patch-insensitive across all five compilers.
- `RendConeOfDeath` 808 b 90.777 -- near-blocked. FP set is a clean descending
  declaration-order sequence except `sn`, defined inside the loop, colours
  third; bare-declaration hoists are byte-inert exactly as the rule predicts for
  a value redefined every iteration. Confirmed again here: naming the four RGBA
  macro arguments does NOT buy retail's all-loads-then-all-stores order when the
  temps are single-use -- they are copy-propagated away.
- `RendConeRange` 964 b 87.884 -- unfinished. `pos_vtx` and `vec_ray` occupy
  each other's frame slots (target 0x10/0x1c, ours 0x1c/0x10) but the
  reverse-declaration lever does NOT reach it: three placements cost 4.5 points,
  scoping buys +0.05, `const pos_bot` is -2.0. 8 shapes.
- `DiscoRender` 748 b 81.053 -- unfinished, improved 8.2 points. Remaining is
  the setup block's load order plus retail keeping `mem` and `vert_list` in two
  registers (`mr r26,r27`) where we coalesce.

### NEW COMPILER-TRACK SHAPE: rematerialise-vs-copy (`mr`)

Four independent witnesses in `zEntPlayer.cpp`, each otherwise byte-identical.
Retail keeps a value in a register and copies it with `mr`; our compiler
rematerialises it instead:

- `GetPatrickTarget` -- retail `mr r23, r27` vs our `li r23, 0`, with
  `li r27, 0` present and live in **both** objects
- `zEntPlayer_Update` -- retail `li r14,0 / mr r16,r14` vs our two `li`
- `get_reticle_bound` -- retail holds `addi r30,r29,0x94` across a call vs our
  two displacement loads
- `SpatulaGrabCB` -- retail `mr r4,r31` vs our `addi r4,r1,0x14`, and retail
  saves one more callee-saved GPR as a result

**This is neither the alias/reload defect nor the scheduler.** No source form
measured reaches any of the four (eight spellings on `get_reticle_bound` alone
are bit-identical or worse). Name it and count it before anyone spends a
session trying to spell around it.

### CORRECTION: E3n's store side is ALWAYS a subrange, so subrange-ness proves nothing

`zEntPlayer_Render`'s residual (8 rows) is a `lwz` of the 4-byte static
`gPTankDisable` hoisted above three `stfs` into a declared frame `xVec3` --
and E3n declines on it, while being worth +1.17 points elsewhere in the same
function (`no3` measures 96.849).

The agent reporting this read it as evidence for the "whole vs partial memref"
hypothesis, on the grounds that the store side is a subrange (`center.x`)
rather than a whole scalar. **That inference is invalid.** Per the decoded
dispatch formula, entry 3 *is* subrange x whole -- being a subrange is what
gets a pair TO clause E3n in the first place, so it cannot discriminate within
it. The hypothesis stays dead.

The *observation* is still a real puzzle and worth keeping: E3n is consulted
here and declines for a reason not yet identified, on a pair that satisfies
every clause condition as documented. Whoever revisits the clause should start
by finding out why.

### `PlayerTeeterCheck`: clause V's blind spot is VN table entry 1

444 b, 78.649%. A fully-unrolled 4-iteration loop where retail reloads
`0.424264f`, `0.2f` and `0.0f` per iteration and we hoist all three. The
killing stores are `stfsx`/`stfs` into the `floor_tmr[]` **array** -- subrange
memrefs, which dispatch to **VN table entry 1**, and clause V patches only
entry 0. Matches the documented `xFXAuraUpdate` finding exactly. Compiler-track,
and a concrete second site for anyone extending clause V.

### CAVEAT on the `MAX(k, expr)` signature entry

These notes carry a flat reading from `zEntHangable` that "no source shape keeps
the dead branch alive". In `CalcCombinedDepen` the target's second clamp emits
`ble L / b L2 / L: fmr` -- an empty then-arm -- and **`MAX(0.25f, dot2)` does
produce that two-branch form**. So the form is reachable and the flat reading is
too strong. But it produces it with the operands and destination register
transposed, while `if (...) {} else {...}`, `if (a < k)`, `if (a <= k)`,
`!(a > k)` and the self-ternary all collapse to a single `bgt`/`bge`. Correct
framing: *the two-branch form and the target's register map are individually
reachable and mutually exclusive here.*

### Lead: the callee-saved FP ordering key is not the volatile-FP rule

`CalcJumpImpulse_Smooth` (680 b, 88.147%): instruction multisets identical,
frame identical, callee-saved set identical (f18-f31). A pure REGS/SCHED
permutation on **callee-saved FP** registers. Notably **our ascending register
order is the exact reverse of our declaration order**, which does not match the
volatile-FP declaration-order rule at all. Worth a dedicated pass by anyone
testing whether callee-saved FP has its own ordering key.

### zEntPlayer: other classifications

- `zEntPlayer_SNDPlayStreamRandom` (1,076 b, 99.108) -- a `0.0f` literal we keep
  across basic blocks / hoist out of a loop where retail reloads it. **No
  intervening store**, so it is global CSE / LICM, not clause V. The existing
  source comment already had this right.
- `zEntPlayer_Update`'s empty `for (U32 i = 0; i < sc->num_npcs; i++) {}` --
  retail does not unroll it and reloads `sc->num_npcs` each iteration, with a
  second `i*4` induction variable surviving; we hoist the bound and unroll by 8.
  `S32` counter, `while` form, dead element load and dead element-pointer are
  all bit-identical to baseline. An unrolling/LICM decision, not source.
- `zEntPlayer_Init` (3,392 b, 95.660) -- 140 rows dominated by the unrolled
  `drybob_anim_count` loop, retail reloading the static after its own increment
  store where we forward.
- **`zEntPlayer_SNDInit` re-measured on the current tree**: 91.947 with the tree
  compiler and *worse* under every ablation (stock 88.457, no0 91.592, no3
  90.005, noV 90.519). Consistent with the correction already in this file, but
  the numbers are now current. It is not a clause-V win waiting to happen.
- The whole unit is patch-insensitive in the sense that matters: the tree
  default is best or tied for **every** function in it. Nothing here is
  already-correct source -- except `zEntPlayer_AnimTable`, which is.

### NEW LEVER: the `fmr` copy trio -- "modify the original FIRST, the copy second"

Three unexplained `fmr` instructions in the target next to an in-place
`fadds`/`fsubs` pair on the same value mean the source **copies a variable,
then modifies the ORIGINAL first and the COPY second**:

    ax = tx;  ay = ty;  az = tz;
    tx -= dx; ty -= dy; tz -= dz;   /* must come FIRST */
    ax += dx; ay += dy; az += dz;

**The statement order is load-bearing.** Writing the `+=` before the `-=` lets
copy propagation fold `ax = tx; ax += dx` back into `ax = tx + dx` and all three
`fmr`s vanish -- 86.136 against 93.491 on the same function.

Worth 9.1 points on `iRenderPushQuadStreak` and 8.1 on two more in the same
unit. It presents as **extra instructions in the target**, so the
instruction-multiset test catches it, but the fix is not obvious from the diff.

### The target's FRAME SIZE is evidence for declaration order

`iParMgrRenderParSys_Ground`/`_Flat`: declaring the `at`-row products
(`zdx,zdy,zdz`) bare and first, ahead of the `right`-row products, took Ground
97.488 -> 99.477. In definition order we spill an extra callee-saved FPR and
**the frame grows 0x90 -> 0xa0**. When our frame is larger than the target's,
the spill is the tell and declaration order is the lever -- check frame size
before hunting registers.

### Our allocator colours anonymous store-value groups in REVERSE source order

Three witnesses in one unit, and it looks like a single mechanism:

- `iRenderPushQuadStreak`'s `{px-dx, py-dy, pz-dz}` -- we give f5,f4,f3 where
  retail gives f3,f4,f5
- `iParMgrRenderParSys_Ground`'s six `{v0.xyz, v1.xyz}` store values -- exactly
  reversed
- `iRenderPushFlat` -- likewise

**In all three the middle element matches and the outer pair swaps.** No source
spelling reaches it: naming fails the multi-use precondition, and statement
permutation only makes it worse (~35 spellings measured on QuadStreak alone,
including all 120 declaration permutations and all 6 store-component
permutations; the floor is 4 rows). A good target if anyone re-opens the
allocator.

### The multi-use precondition, confirmed sharply -- same file, same day

In `Ground`/`Flat`, naming `px - xdx` (used **twice**) **paid**. In
`QuadStreak`, naming `px - dx` (used **once**) was **bit-identical** in five
declaration positions. Opposite outcomes on use count alone. This is now the
third independent confirmation; treat single-use naming as inert.

### Another mutually-exclusive sibling pair (the `RenderLightning` situation)

`iParMgrRenderParSys_Streak` and `_InvStreak` were diffed **against each other
in the target**: the two retail bodies are byte-identical except an f6/f7 swap
on the y lane. Retail's own compiler allocated two identical sources
differently, so **no single source can close both**. Same class as
`RenderLightning`'s two loops. The pre-existing source comment saying so is
verified against raw bytes.

### dwarf is decisively WRONG for `iRenderPushQuadStreak`

`dwarf/SB/Core/p2/iParMgr.cpp` lists the only float local as `size` -- no
`px/py/pz`, no `tx/ty/tz`, no `dx/dy/dz`. It **does** record register-allocated
locals elsewhere in the same function, so this is not a recording artifact.
Writing the function that way -- every position expression inlined, CSE-only --
measures **62.665%** against 99.900%. The GC target's three `fmr`s prove the
named values must exist.

### iParMgr: patch behaviour, and what is left

The patch is strongly positive here and there is **no new patch-cost witness**.
E3n is worth +1.00 pp on QuadStreak, +0.78 on Ground, +0.95 on Flat. On
`iParMgrInit`, clause V is worth **+15.1 pp** and entry 0 **+21.0 pp**
(tree 70.040 / stock 54.960 / no0 49.069 / no3 70.040 / noV 54.960).

- `QuadStreak` 99.900 (4 rows) -- REGS, the `_xCameraUpdate` one-bit tie-break:
  `(px-dx)` wants f3 and `(pz-dz)` wants f5, we produce the reverse. ~35
  spellings; floor is 4 rows.
- `Ground` 99.709 (16 rows) / `Flat` 99.452 (19 rows) -- same class, a
  permutation of anonymous store-value temps across f6-f11 / f4-f11.
- `Streak` 93.491 / `InvStreak` 93.082 -- structure now matches; residual is the
  `5.0f` literal colouring f1-vs-f3 plus z-lane scheduling, and the pair is
  mutually exclusive as above.
- `Sprite` 87.688 -- one finding left on the table: retail computes **all twelve
  `fmadds` into twelve distinct registers and then stores them all**, in
  component-major compute order with vertex-major store order. Reproducing that
  with 12 named temps works structurally but measures **85.795**, below the
  inline form, because our allocator then places them badly. Not shipped.

### `--relocs` IS NOT EVIDENCE ABOUT THE POOL. Read section sizes instead.

`iModel.cpp` carried `__deadstripped_sdata2_hack()`, a **fabricated** dead
function seeding `0.0f` that a previous pass had installed. Under
`solo.py --relocs`, removing it made five extra functions non-matching, which
reads convincingly as real pool work.

**It was not.** `readelf` shows `.sdata2` is 0x24 **with and without** it
(target 0x28). It moved anonymous *numbering* only -- cosmetic by this file's
own layout-not-numbering rule -- and banked zero under report.json semantics,
while emitting a surplus strong global the target does not have. Removed; the
DOL is unchanged by its removal, which confirms it was dead weight.

**Rule: to test a pool hypothesis, compare section sizes with `readelf`, never
`--relocs` row counts.** And a fabricated function that does not move the number
must come out, however convincing the wrong measurement looks.

### `>=` has its own branch signature (sibling to the `a <= b` note)

    MAX2(a,b) with `>=`  ->  fcmpo / cror eq,gt,eq / bne
    MAX2(a,b) with `>`   ->  fcmpo / ble

**A `cror` in a max-chain means the source used `>=`.** This caught a real
defect: `iModelCull` selected the max scale with `MAX` (`>`, from `macros.h`)
where the target uses the `>=` form of the file-local `MAX3` -- and its own
sibling `iModelCullPlusShadow` already used `MAX3` and matched that block
byte-for-byte.

### NAMED SHAPE: "the target unrolls a loop and we do not"

`iModelCullPlusShadow` sat at 74.45% because our loop body contained the entire
shadow-test block, so mwcc would not unroll it. Retail's `bgt` jumps **out** of
the loop to a block placed after the loop's `return 0`, leaving a small body it
unrolls 3x. Rewriting the `if` as `goto shadow_test;` with the block after the
loop was **+23.5 points in one edit**.

**The tell is `li r0,<n/3> / mtctr` in the target against our `li r0,<n>`**, and
the branch polarity says which side is cold. When you see it, ask whether the
`if` body inside our loop belongs outside it.

### COUNTER-EXAMPLE: a tiny residual is NOT evidence against source-reachability

These notes advise working one-away units in ascending percentage. In `iModel`
two of the three easiest closes were the **highest**-percentage entries --
`iModelVertEval` (99.831, 2 rows) and `iModelStreamRead` (99.522) -- and both
were genuine source bugs. The ascending-percentage heuristic would have
deprioritised exactly those. It is not reliable in `Core/gc`.

### `iModelMaterialMulCB`: another entry-0 pair, same shape as `eval_joint`

244 b, 95.738%, patch-insensitive. 4 rows of 61, all in the first of three
`U8_COLOR_CLAMP` blocks, where our scheduler hoists the u32->double magic `lfd`
above `stw r4,0x8(r1)` (the `col` struct copy). That pair is an 8-byte **whole**
`.sdata2` load x a **whole** 4-byte frame object = **dispatch entry 0**, so E3n
(entry 3) is never consulted. Second witness for the "E3n's rule on the
whole x whole entry" candidate clause, after `eval_joint`. Source permutations
do move rows (94.180, 90.738), so it is in contact with the scheduler, but
baseline is the best form.

### iModel: E3n witnesses in both directions, in one file

Over-fire, neither bankable: `SkinXform` (520 b) tree 96.600 / `no3` 98.138;
`SkinNormals` (632 b) 96.424 / 97.177. Counterweight from the same file:
`iModelAnimMatrices` is a strong E3n **winner** -- 95.107 tree against 83.467
stock and `no3`. A reminder that E3n's price is per-pair, not per-unit.

### iModel: the five left

- `iModelCullPlusShadow` 636 b, 97.906 -- **unfinished, not proven blocked**,
  patch-insensitive. Retail CSEs only `shadowVec->x` into loop 2 and reloads
  `->y`/`->z`; we hoist all three (f9/f10/f11) and the register map cascades.
  10 spellings all bit-identical. Naming all three reaches 98.000 but invents
  locals for +0.09, so not installed.
- `SkinXform` / `SkinNormals` -- **BLOCKED**. Multisets identical (pool id
  only); callee-saved GPR rotation (r26-r29), the `xShadowSimple_Add` class,
  plus two ALU-past-ALU swaps.
- `iModelAnimMatrices` 300 b, 95.107 -- **BLOCKED**, three clusters: a
  callee-saved r27/r28 swap (six declaration positions, all <= baseline, two
  bit-identical), a `stw` of `matrixStack[0].flags` scheduled among nine `stfs`
  to the same aggregate (**entry 4**), and an `addi`-past-`addi` pick swap.

Unit `.sdata2` remains 0x24 against the target's 0x28 (`.sbss` 0x2c vs 0x30) --
a genuine Phase-2a shortfall with **no honest route available**: nothing is
missing from `.text` and dwarf names no extra function (`solo.py --missing` = 0).

### A retail out-of-bounds read, reproduced deliberately (PCPORT)

`iModelCullPlusShadow`'s first frustum loop is unrolled 3x by mwcc, and mwcc
updates the secondary induction variable (`numPlanes`, r4) **wrongly**:
`subi r4,r4,1` sits after copy B's exit branch and `subi r4,r4,2` at the latch,
so at the six exits r4 is 5,5,4,2,2,1 where the correct remaining-plane counts
are 5,4,3,2,1,0. **On four of six exits the second loop runs one iteration too
many and reads `cam->frustumPlanes[6]`, past the array.** Verified against raw
bytes at `800C8484`-`800C84EC`. Our code reproduces it exactly, because it must.
Flag for PCPORT.

### ANSWERED: the callee-saved FP ordering key is REVERSE declaration order

The open lead is settled, with a prediction made *before* measuring. In
`NPCC_aimVary` the four callee-saved FP locals `dst_toFake, mag_vary,
mag_updown, mag_toFake` get f31, f30, f29, f29 from us against retail's f29,
f30, f31, f31 -- an exact reversal. **Reversing the declaration list landed the
target's map first try** (99.676 -> 99.912, zero non-pool rows), and a
24-permutation sweep confirms only the four orders ending in `dst_toFake`
reach it.

**The boundary is sharp.** The identical 24-permutation sweep on `RenderCone`'s
`u_tip/v_tip/u_base/v_base` is completely inert (99.056 or 98.869, never
better). Those are single-assignment locals that get copy-propagated.

*The reverse-declaration-order rule reaches callee-saved FP values with real
multi-block live ranges, and nothing else.* Note this is the OPPOSITE direction
from the volatile-FP rule, which is forward declaration order.

### The "rematerialise-vs-copy (`mr`)" class IS source-reachable in one shape

These notes record four `zEntPlayer` witnesses with "no source form measured
reaches any of the four". At least one shape does. In `RenderCone` retail keeps
`mem` and `vert_list` in two registers (`mr r25, r29`) where we coalesce them.
**Passing the other alias of the same pointer to the final call** --
`xMemPopTemp(vert_list)` rather than `xMemPopTemp(mem)` -- defeats the coalescer
and reproduces retail's copy exactly: 91.537 -> 94.100, dragging a callee-saved
GPR permutation into place with it. **Retry this on the four zEntPlayer
witnesses.**

### A recorded E3n over-fire is evidence of a SOURCE BUG, not only a patch fault

Strengthening the earlier warning. `NPCC_LineHitsBound` was a textbook
over-fire (tree 93.846, stock and `no3` both 96.038). Swapping
`ray.max_t`/`ray.min_t` in the source reaches 96.038 and **all five compilers
now agree** -- the over-fire is gone, not masked.

Conversely `NPCC_aimVary`, `NPCC_HaveLOSToPos` and `RenderCone` moved from
E3n-neutral to E3n-**positive** after source work (`no3` is now 1.2, 0.8 and 2.4
points worse). So the matrix moves in both directions under source change.

**Read an over-fire as "there is probably a source defect in this function"
before reading it as "the patch is at fault."** Two of the three functions that
looked patch-blocked in this unit were source bugs.

### The POOL bucket does NOT exempt rows in report.json -- measured

`NPCC_aimVary`'s only remaining rows are five 12-byte `.rodata` zero templates
at target `+0x210` against our `+0x18`, a 504-byte pool displacement. The
expectation from this file's POOL-bucket note was that such rows would not be
scored and the function would read 100.0. **It reads 99.912.** Checked directly
in `build/GQPE78/report.json`. Pool displacement rows are scored like any other.

### CLAMP evaluates its middle argument twice, and that is a lever

`CLAMP(x,a,b)` = `MAX(a, MIN(x,b))` evaluates `MIN(x,b)` **twice**. With `x` an
inline expression mwcc CSEs the branchy `MIN` into a single `fmr`; with `x` a
named local it does not, which is retail's shape. Binding the ratio to a local
took `Firework::FlyFlyFly` 89.318 -> **100.000**. Look for a lone unexplained
`fmr` next to a clamp.

### The bare-declaration loophole failed again, on its documented precondition

Third independent confirmation. Hoisting `F32 rat;` out of `NPCBlinker::Render`'s
loop to three different scopes is byte-identical in all three, because `rat` is
reassigned every iteration -- the "local reassigned on every path colours at
each definition" case. Stop re-testing this one.

### dwarf in zNPCSupport: right once, wrong twice

CORRECT and decisive on `NPCC_chk_hitEnt` -- its local order is exactly ours,
which is what stopped a +0.09 pp reorder that would have contradicted it.
WRONG on `NPCC_GenSmooth`: it lists `i, u, u3` with **no `u2` and no row
pointer**, yet `u2` must be a named local declared first to get retail's f3,
and `F32* pre = prepute[i]` is worth +2.0 pp and is what removes the `stfsu`
re-basing. WRONG on `NPCC_aimVary`: it lists `dst_toFake` first, where the GC
target's colouring requires it declared **last**.

### THE CHEAPEST QUESTION IN THIS PROJECT: patch, or source?

Before spending a session on any near-100% residual, compile the unit with
**stock `GC/2.0p1`**. If it hits 100.0 there, the source is already correct
and the work is in the patch, not the `.cpp`. This costs one compile.

`scratchpad/verify_mw.py <unit> <mw_version|-> <symbol>` does it: it reuses
`build.ninja`'s own rule and flags but substitutes a chosen `$mw_version`, and
compiles into a private temp dir -- so it is safe to run while other agents are
building, and it never touches the shared compiler. Variant compilers live at
`scratchpad/compilers/GC/2.0p1a-{no0,no1,no3,noV,e3c,chk}`; `no3` ablates E3n,
`chk` is the control. Building a variant takes seconds; sweeping all 224 SB
units takes ~35 s at 8-way parallel.

### `solo.py`'s LEFT COLUMN IS THE TARGET. RIGHT IS OURS.

Stated in solo.py's own docstring (`left` comes from `-1 <target_path>`) and
got read backwards anyway, twice in one session, by me. It inverted the
diagnosis both times: on `zEntPlayer_AnimTable` it is **retail** that
accumulates `@stringBase0` through three registers and **ours** that does it
in place (the direction is the entire finding -- retail has more registers
occupied, not fewer), and on `xSpline`'s `Tridiag_Solve` it is **ours** that
takes `b,c,d` in plain parameter order and the **target** that scrambles.
Check the orientation before writing down a conclusion.

### The `int ourAnims[2]` idiom: 3 witnesses, not source-reachable

`ZNPC_AnimTable_NightLight` (176 b), `_Tubelet` (192 b) and `_BossSBobbyArm`
(184 b) are 5 rows each, a pure r4<->r5 transposition on the 8-byte
`@sda21`->frame copy of `int ourAnims[2]`. The trigger is exactly the
2-element array: `grep 'ourAnims\[2\]'` returns these three and nothing else,
and every 3-element sibling (`SleepyTime`, `BossSB1`) matches.

Only one allocator decision is involved: the copy's scratch temp is r4 for us
and r5 for retail, and the `li 0` the scheduler hoists above the copy is then
forced to be *the other* argument register, producing all five rows. For
retail to pick r5, r4 must be live across the copy -- retail's IR materialises
arg 2 before the array copy and ours does not.

**Stop test run and passed.** Six spellings -- array-first, `table`-declared-
first, array-then-bare-`table`-then-assign, `S32` vs `int`, literal `0` vs
`NULL` for arg 2, and a braced scope around the call -- are all bit-identical
(same 99.545%, same SHA-1 of the full diff text). Only two things move it and
both move it the wrong way: declaring the array after the call gives 79.636%,
making it 3 elements gives 76.091%. **Do not re-open.** This supersedes the
older note framing these three as "the best test case on the board".

### NEW CANDIDATE CLAUSE: literal load hoisted over an INDEXED frame store

`ZNPC_AnimTable_BossPlankton` (2,472 b) and `_BossSB2` (3,384 b) have
*identical* 17-row clusters at the closing `NPCC_BuildStandardAnimTran` call.
Both compile identically under stock `2.0p1` and patched `2.0p1a`, so the
patch is exonerated here. Root cause is a literal-load hoist, not a register
problem:

    target: addi r5,r1,0x18 / slwi r0,r17,2 / li r4,0 / lis r3,g_strz_bossanim@ha
            / stwx r4,r5,r0 / addi r4,r3,@l / mr r3,r18 / li r6,1
            / lfs f1,@1657@sda21 / bl
    ours:   lis r3,@ha / addi r5,r1,0x18 / addi r4,r3,@l / slwi r0,r17,2
            / li r3,0 / lfs f1,@437@sda21 / stwx r3,r5,r0 / mr r3,r18
            / li r6,1 / bl

Our scheduler hoists `lfs f1, @NNN@sda21` and the `lis @ha` **above** the
`stwx` into the frame array; retail does not. The r3-vs-r4 choice is a
consequence, not a cause. This is the clause-D shape that these notes already
price as net-negative -- except that the store here is **indexed** (`stwx`,
computed address), which is why clause C's `memref+0x0c == 0` gate and E3n's
declared-frame-object gate both decline. Precisely shaped candidate: *a small
static literal load may not hoist above an indexed store to a frame array.*
Worth 5,856 bytes across these two.

Source stop test on Plankton: `anim_list[anim_size++]`, adjacency of the
terminator write, explicit `0` for `ANIM_Unknown`, `&anim_list[0]` vs
`anim_list`, and binding the 0.2f to a named `F32` local are all bit-identical.
Moving the terminator write earlier *does* move the rows (97.508%), so unlike
the `ourAnims[2]` family there is real contact with the scheduler -- but the
baseline ordering is already the best one and the residual is the hoist.

### Two corrections this forced

- **`zEntPlayer_SNDInit` was not the prize.** It was dispatched as a
  10,160-byte function whose residual was "about 400 of 403 rows missing `lfs`
  reloads"; clause V moved it 90.519 -> 91.947 and `PlayerTeeterCheck` not at
  all. **That is the second per-unit attribution to collapse on contact with
  the compiler** (the first was the 1.750pp alias-predicate estimate). Agent
  reports are good at *characterising* a residual and bad at predicting what a
  compiler change will pay. Verify before promising.
- **The docstring's warning against E3n on scheduler entry 3 was stale.**
  Reverting entry 3 to clause C measures **+6/-88** (superseded: a full
  224-unit sweep on 2026-08-22 measures **+6/-107 functions, +25,924/-77,224
  bytes**), so E3n is worth +82 net
  in the current tree, not the "+22/-18" recorded against it.

**Every unit's residuals now need re-measuring.** Attributions recorded before
2026-08-21 were made against a compiler that has since changed twice.

### The redundant-load path is a SECOND patch site, and it is untouched

`zMainParseINIGlobals` (8,980b, 99.276%) is capped by a defect the installed
patch cannot reach, and the third pass on it pinned down why.

A 20-line standalone repro (`extern F32 a1..a6;` + `if (u) { a1 = DEG2RAD(a1);
... }`) reproduces the **entire** residual under this unit's exact flags:

- **One statement alone compiles to retail's sequence exactly**, register roles
  included (`lfs f2,@PI / lfs f1,val / lfs f0,@180 / fmuls / fdivs / stfs`).
  The arithmetic, operand order and allocation are already right; the sole
  defect is that *consecutive* statements share the two literal loads.
- The reuse is **strictly basic-block-local** -- a `goto`/label between two
  statements reloads both literals and restores retail's roles at zero
  instruction cost. The retail block is one basic block of 36 straight-line
  instructions, so there is no free second boundary to exploit.
- **It is not the global optimizer.** `#pragma opt_common_subs off` and
  `#pragma global_optimizer off` change nothing (both verified accepted with
  `#pragma warn_illpragma on`). It is the **code generator's redundant-load
  elimination**.
- **It is governed by an alias query.** Inserting `*p = 1.0f;` through an
  `F32*` parameter between two statements makes the next statement reload both
  literals and revert to retail's exact `f2/f1/f0` roles. The query exists; our
  compiler simply answers "no alias".
- **The contrast is inside the same function.** The three
  `globals.player.g.*SlideAngle = DEG2RAD(...)` statements sixty lines earlier
  share their literals and are byte-identical to ours. The nine `zcam_*`
  statements do not share. The only difference is the store: a 4-byte
  `@sda21` scalar with an opaque `extern` definition versus a member of a
  large named object reached through a base register. Retail answers
  **may-alias** for (`lfs` of a <=4-byte anonymous `.sdata2` constant) x
  (`stfs` to a <=4-byte opaque named static) and no-alias for the large-object
  store; we answer no-alias for both. A `v_big` probe reproduces retail's
  large-object behaviour exactly, so **only the small-static case is wrong**.

That is the clause-A/clause-C predicate shape -- two <=4-byte statics,
differing opcodes, plain load/store -- but applied in the **code generator's
redundant-load path**, not the instruction scheduler's may-alias predicate.
`patch_compiler.py` only redirects the scheduler dispatch table at 0x5bd0bc,
which is exactly why clause C+ moved 19 functions tree-wide and moved this one
by zero. Whoever extends the patch next should look for the second query site.

Ruled out beyond the earlier lists: an `inline F32 d2r(F32)` helper (emits a
real `bl`; the unit is `-inline off`), per-statement braced temps, unused
labels (stripped before the optimizer), `extern F32 a[]` with `a[0]`, the
`(&a1)[0]` spelling, and a double-precision spelling (wrong shape entirely).

**`zMainMemCardSpaceQuery`'s pool ceiling is real but is NOT its blocker.**
Our object does emit out-of-line `NSCREENX`/`NSCREENY` bodies (declared
`inline` in `xFont.h`, not inlined because the unit is `-inline off`) which
intern 1/640 and 1/480; retail's zMain.o has neither, and across the six
target objects referencing them only `xDebug.o` defines them. But
`solo.py --relocs` costs just 0.049 pp here and `report.json` does not count
relocation rows at all -- the blocker is register allocation. Our allocator
always gives r31/r30 to the two block-scope values and r29..r22 to the
function-scope locals in declaration order; retail puts `workArea` and
`startBytes` *above* those, which no declaration permutation can reach. An
automated hill-climb over ~600 declaration orders plateaus at 97.803% and
never reaches 100.

**Independently re-derived 2026-08-22, from a cold start, by an agent that had
not read this section** (the checkout had been rolled back by a container reset).
It reached the same conclusion by the same route -- small-data store kills a
cached `.sdata2` literal in retail and not in ours, with the large-global store
as the negative control -- and cost ~80 minutes to do it. The finding is
therefore replicated, not a one-off. It also independently hit the r30/r31 pin
on `zMainMemCardSpaceQuery`'s two anonymous temps (15 orders sampled, versus the
~600-order hill-climb above; same plateau, same cause).

**So: DO NOT SEND ANOTHER AGENT AT zMain.** Both of its functions are
diagnosed and both are blocked on compiler-track work, not source work.
`zMainParseINIGlobals` needs the second alias query site found in the code
generator's redundant-load path; `zMainMemCardSpaceQuery` needs an allocator
that can lift block-scope values above function-scope locals, which the select
routine at 0x508900 cannot express (lowest-free-colour, no preference term).

### Clause E3n's load-side size bound widened to 8 (SHIPPED 2026-08-21)

Clause E3n reads "an `stfs` to a declared frame local may not be crossed by a
*later* small static load". Its load-side test was `sizeof(B) <= 4`, which
declined on the one object that matters most for this shape: the
unsigned-int-to-float magic double `0x4330000000000000`, an **8-byte**
`.sdata2` object. With the clause declining, our scheduler hoisted that `lfd`
above a run of stores to declared frame locals; retail leaves it at its first
use.

The fix is one byte in the cave -- `cmp ecx, 4` -> `cmp ecx, 8` at `0x57eb75`,
same instruction length, no reflow. Tree-wide measurement against the
otherwise identical build:

    main.dol   306526d90b48e99894c3138f5fc8f2716d9fecf6  (unchanged)
    GAME exact 76.523 -> 76.639   (+0.116)
    GAME fuzzy 98.9078 -> 98.9106 (+0.0028)
    exact functions +3, -0

Gains: `xFX::DrawRing`, `zNPCTypeKingJelly::load_param<iColor_tag,int>`, and
`zNPCBalloonBoy::PlatAnimSet` (48.419 -> 100.0). Only `DrawRing` was
predicted; the other two were found by the sweep, which is the usual argument
for measuring these tree-wide rather than on the witness unit alone.

Three functions get *worse* in fuzzy without crossing 100 -- `xFont::get_bounds`
61.827 -> 49.423, `cruise_bubble::add_trail_sample` 97.661 -> 91.516,
`zUI_Render` 91.348 -> 90.159 -- so they cost no `matched_code` and the net is
+3/-0 on the metric that counts.

**`xFX::eval_joint` was predicted to flip and did not** (98.077, unchanged).
Its `lfd` crosses `stfs f0, 0x8(r1)` *and* `stw r5, 0x14(r1)`; the surviving
defect is presumably the `stw`, so the store-side of the clause is what
declines there. Do NOT widen A's test to match -- that is the symmetric rule
the clause-C notes measure at -50 exact functions. Only the load side moved.

### Measured NO-GOs, so nobody re-opens them

- **Relaxing the static-storage gate on the store side: -80 (+29/-109), and
  none of the intended functions move.** The premise -- "the load side
  qualifies, the store side is pointer-based and fails the gate" -- is FALSE.
  Two probes prove it: allowing the store side only when its base expr is not
  a frame object, and only when it has no base expr, **both change nothing
  anywhere**, so those pointer-based stores never reach clause C at all. The
  only population the relaxation admits is stack traffic, i.e. exactly what
  the gate exists to exclude (the docstring's -50).
- **The whole avenue is bounded.** With **all nine dispatch entries answering
  "may alias" unconditionally**, only 5 of the 21 functions this project had
  attributed to the alias predicate reach 100.0, and most get *worse*. A
  1.750pp attribution built from per-unit agent reports did not survive
  contact with the compiler. Treat "blocked on the reload defect" as a
  hypothesis to test, not a diagnosis.
- Also measured full-tree and rejected: store side gated on computed-address
  -5; on object-link no gains; dropping the differing-opcode test +21/-7;
  requiring the load side to be a whole object +19/-8; requiring load-side
  offset 0 +19/-7.

**Method note worth copying.** The agent hashed every compiled object before
and after: 28 of 451 changed and none belonged to a complete unit, which is
how it predicted the DOL would survive before any link was run. That is a
better proxy than percentages, because objdiff pairs symbols by name and is
blind to definition order.

## Patterns that keep working

- **A same-value expression used as both an allocation size and a copy size
  gets CSE'd into a callee-saved register and wrecks the register map.**
  Retail's is *signed* at one site and unsigned at the other:
  `(S32)sizeof(xVec3) * numVertices` for the `memcpy`, plain
  `sizeof(xVec3) * numVertices` for the `xMemAllocSize` argument. Different
  result type, different tree, no CSE -- retail recomputes `mulli`/`slwi` at
  each site. This took `zFXGooEnable` from 93.296 to exactly 100 (1,000 bytes)
  and was the only thing that did.

- **Bind each argument of a store macro to a named temp first.**
  `RwIm3DVertexSetPos`/`SetRGBA` written with expressions inline emits
  load/store/load/store; retail emits all loads then all stores. Our compiler
  will not reorder a load across a store through a pointer, so the temps have
  to impose the order from the source side. Worth ~9 points on
  `zFXGooRenderAtomic`, whose position blocks became byte-exact.

- **Declaration ORDER alone can be the entire residual.** In
  `zNPCBSandy_BossDamageEffect`, `S32 j;` before `S32 i;` -- with every use
  unchanged -- flipped which subscript mwcc materialises with `slwi` and which
  rides the induction variable, and took the function from 97.971 to 100.
  Cheap to try, and it costs nothing to be wrong.

- **`x = !(flags & bit)` narrows to 8 bits because C++ `!` yields `bool`.**
  Written as an explicit `if/else` assigning 1 and 0, mwcc if-converts to the
  same `cntlzw`/shift pair with no `bool->int` conversion node, giving
  retail's full-32-bit `srwi`/`extlwi`. Keep the variable `S32`: making it
  `bool` fixes the shift and breaks every later use of it.

- **Retail does not always fold a literal multiply.** Its inliner emits
  `fmadds f0, <1.0f>, (a-b), b` for `LERP(1.0f, b, a)`; written as a literal,
  mwcc folds `1.0f*x` to `fadds` and folds the `0.0f` case away entirely.
  Routing the blend factor through a local reproduces the unfolded form.

- **CAVEAT on the const-aggregate rule below: it is not unconditional.** It
  did not fire on `zFX`'s `validate_popper`, where the 12-byte copy still used
  two scratch registers with `const` applied; an intermediate model local was
  also measured and rejected. Try it, measure it, drop it if it does nothing.

- **`const` on a read-only local aggregate changes the copy expansion.** A
  12-byte `xVec3` copy-init emits `lwz/lwz/stw/stw/lwz/stw` with two scratch
  registers when the local is non-const, and `lwz/lwz/lwz/stw/stw/stw` with
  three when it is `const` -- the latter interleaves freely with surrounding
  FP work, which is what retail does. Same for `xVec2`. In
  `zNPCTypeBossPlankton` this was the *sole* change needed for
  `update_follow_camera` (83.784 -> 100) and `Enter__22zNPCGoalBPlanktonFlank`
  (94.435 -> 100), and it carried four more functions. Cheap to test on any
  unit with local vector copies.

- **An array element bound to a reference addresses differently.** Retail
  emits `mulli / addi <member offset> / lwzx` for `territory[i].timer`; the
  plain subscript emits `mulli / add / lwz <disp>`. Writing
  `territory_data& t = territory[active_territory];` then `t.timer`
  reproduces retail's form, and was the only change `stun` needed to reach
  100.0.

- **Locals that never existed are the single most productive find, every
  time.** A pointer temp (`xMat4x3* mat = ent->model->Mat`) the original did
  not have changes register allocation and suppresses the reload the target
  performs. `dwarf/` lists the real set. The inverse matters as much: a local
  that *should* exist, e.g. `F32 fadeDist = 0.0f;`, because mwcc folds a
  literal `0.0f + x` and the target does not fold.

- **Cross products take `xVec3` struct operands, not six `F32` scalars.**
  Scalars give the right relative register order rotated by one
  (`ax=f4..bz=f3` instead of `f3..f8`). Worth 1,924 bytes on
  `xShadowReceiveShadow`.

- **`x / 2.0f` is not `x * 0.5f`.** mwcc canonicalises a multiply so the
  constant loads first (`fmadds f0, 0.5, x, y`); a divide by an exact power of
  two folds to the same pool entry but cannot commute, giving retail's
  `fmadds f0, x, 0.5, y`. All three multiply spellings were measured and
  produce the wrong order.

- **`if (len)` is not `if (len != 0.0f)`.** Written against a literal, mwcc
  emits `fcmpu cr0, const, len`; the implicit test emits `fcmpu cr0, len,
  const`, which is retail's order.

- **`x OP= c` is not `x = x OP c`.** mwcc evaluates the constant first for the
  second form and the destination first for the compound assignment. If the
  target loads the memory operand before the literal in an `fadds`/`fsubs`,
  the source used `+=`. Timers are the usual site.

- **A three-way ladder, not a two-way one: `x = c * x` / `x *= c` / `x = c
  * expr`.** The refinement of the note above, measured on
  `NPCHazard::DeathStar`. With `F32 spd = 0.4f * this->custdata.typical.rad_max;`
  mwcc issues `lfs <0.4f>` before `lfs <member>` and lands the product with
  `fmuls f1, f1, f0`. Splitting into a load then `spd *= 0.4f;` fixes the load
  order but keeps the wrong destination register (99.565). Only
  `F32 spd = member; spd = 0.4f * spd;` gives both, and it is 100.0. So the
  choice is not merely which operand issues first -- it also decides which
  register the result is written to, and the two are set independently.

- **mwcc evaluates the RIGHT operand of a binary `*` first when both sides
  have side effects.** This is why folding a `spd_factor` temp back into one
  expression in `NPCHazard::KickBlooshBlob` left the emitted `xurand()` call
  order untouched while flipping `fmuls f31, f31, f0` into the target's
  `fmuls f31, f0, f31` (99.868 -> 100.0). Useful whenever the only diff is a
  commuted `fmuls` and the operands are calls: fold the temp away rather than
  swapping the operands in the source, which is inert. The inverse move --
  hoisting a *named* temp out of such an expression -- sinks the multiply past
  the call and is much worse (93.649 measured).

- **The `const`-aggregate lever does not fire on aggregates that already emit
  the three-register form**, so apply it one declaration at a time and keep
  only the ones that move. In `NPCHazard::Render` three of five candidates
  paid (93.330 -> 94.930) and two were inert.

- **A member load and an adjacent aggregate copy sharing a base register get
  clustered, and then load order and copy position cannot both be had.**
  Measured across `Upd_OilOoze`, `Upd_OilGlob`, `TarTarLinger`,
  `Upd_ChuckBloosh` and `StagColGeneral`: declaring the scalar before the
  vector gives retail's multiply-before-copy position but the wrong load
  order; swapping gives retail's load order but sinks the multiply below the
  copy. No source form yields both. When the copy source is a `.rodata` base
  instead (a *different* register, as in `DeathStar`) there is no clustering
  and the asymmetry inverts -- which is why `DeathStar` is solvable and these
  five are not. Treat this shape as blocked, not as unfinished work.

- **`MAX(k, expr)` has a distinctive signature** and is often mistaken for an
  `if`-clamp: the literal is loaded into the *result* register before the
  compare, the compare is `fcmpo cr0, <literal>, value` (literal first), and
  there is an `ble L / b L2 / L: fmr` pair with an empty then-arm. If the store
  comes from the literal's register, the source used the macro. Worth
  94.245 -> 94.858 on `thunderCountCB` by itself.

- **A two-statement fract idiom tells you which variable retail assigned to.**
  `x = x - (F32)(S32)x;` emits `fsubs f5, f5, f3` (writes back into its own
  register); assigning into a *different* variable emits `fsubs f3, f5, f3`.
  Read the overwritten register in the target and you know the destination.
  Same family as the `x OP= c` note, applied to the destination rather than
  the operand order.

- **When the only diff is a commuted `fmuls`/`fmadds` and NEITHER operand has
  side effects, swapping the source operands is not the fix** -- it moves the
  evaluation order too and just trades one wrong row for another. Make one
  operand already-computed instead: `factor = xurand() - 0.5f;` then
  `ePos.x += (1.0f - gfactor) * factor;`. This is the other half of the
  right-operand-first note above, which covers only the side-effecting case.

- **Binding store-macro arguments to named temps works, but the temps must be
  INTERLEAVED so only two are live at once.** This is the rule that took
  `SandyLimbSpring::SpringRender` 92.635 -> 100.0 (844 b), and the failed
  intermediate is the instructive half. `RwIm3DVertexSetPos` is a
  three-statement macro; with the products written inline each `->y =`/`->z =`
  is its own statement, mwcc gives each one `f0`, and it therefore reloads the
  `0.9f`/`1.1f` literal per component. Retail loads the literal once and keeps
  two products live. Declaring **all four** temps up front only reaches 97.370:
  with sin/cos in `f2`/`f3` and the first pair in `f0`/`f1` there is no scratch
  left, so mwcc re-materialises the second pair at the use site and the second
  vertex keeps the reload/interleave shape. Declaring the second pair *below*
  the first macro call is the last 2.6 points. So when this lever half-works,
  the fix is usually to move declarations down, not to add more of them.

- **The `const` lever does NOT extend to scalar locals -- there it can HURT.**
  Measured on `lightning_ring::set_ring_segments`: a `const F32 angle_step =
  PI / 32.0f;` inside the init loop was the function's ENTIRE residual,
  because the const pulled the constant's `lfs` up into the `icos` return
  shadow. Replacing it with `angle += PI / 32.0f;` took the function to
  100.0. The lever is specifically about read-only local AGGREGATES, where it
  buys the three-register copy form; on a scalar there is no copy form to
  buy and all it does is move a load.

- **CW numbers same-scope locals in REVERSE declaration order.** So when the
  target's frame slots are the giveaway, the local that wants the HIGHER slot
  must be declared LAST. `apply_wave_damage` needed `xSphere inner; xSphere
  outer;` to put `outer` at 0x14 and `inner` at 0x24, and closed on that
  alone.

- **Declare-first-assign-later is a distinct lever from move-the-computation.**
  In `generate_zap_particles` retail's colouring wanted `points` created
  before `emitted`. Moving the whole `points` computation first measured
  **92.135** (it forces the source object into a callee-saved register early);
  declaring `S32 points;` first and assigning it after `emitted` measured
  100.0. The virtual register is created at the DECLARATION; the load stays
  where the assignment is.

- **Retail's `bne .+8 / b exit` is the shape CW emits for the LAST operand of
  an `||` chain**, not for a standalone `if (cond) return;`. Seeing that
  degenerate two-branch form means two guards in the original were one
  condition. Merging them closed `repel_player`.

- **One scheduler rule accounts for most of what is left in zEntPlayer:
  our scheduler always fills a float-load latency slot with an already-ready
  store, where retail leaves the stall.** Confirmed as the same single site in
  `BoulderRollCB`, `BoulderRollDoneCB`, `SlideTrackUpdate`,
  `zEntPlayer_SpringboardFX` and two of `zEntPlayerFloorUpdate`'s clusters --
  **3,232 bytes of otherwise-clean functions**. The canonical shape is three
  constant stores to adjacent members where retail emits
  `lfs f1,<0> / stfs f1,x / lfs f0,<3> / stfs f0,y / stfs f1,z` and we hoist
  `stfs f1,z` into the `lfs f0` shadow. Roughly ten source spellings have been
  measured against it across two passes; none reaches it.

  **This is scheduler entry 4, and entry 4 is DEAD -- do not re-open it.**
  The two reordered stores are subranges of the SAME aggregate
  (`info.vel.y` vs `info.vel.z`), which is exactly the subrange x subrange
  query on entry 4, measured **+16/-560 tree-wide** above. The same shape was
  independently reached from `xCollide` (`xParabolaHitsEnv` 576 b,
  `xSphereHitsOBB_nu` 956 b), so the true witness set is at least seven
  functions and about 4,764 bytes -- all of it behind a predicate that costs
  560 exact functions to satisfy. Recognise this shape and STOP: it is not an
  opportunity, it is the single largest confirmed dead end in the project.
  It was briefly written up as "the highest-value single shape on the board"
  before the entry-4 connection was made; that reading was wrong.

- **`zEntPlayer_SpringboardFX` is the sharpest `volatile` near-miss recorded.**
  Marking its function-local `static F32 sLastSpringboardBubbleEmit` volatile
  takes it from 12 differing rows to **2** -- fixing both the reload and an
  f30/f31 swap -- but scores 98.361 rather than 100.0, because the moved store
  scores worse than the substitutions it replaces. Reverted under the
  install-only-at-100.0 rule. Its last row is the scheduler shape above, so it
  closes for free if that is ever fixed.

- **Two independent agents have now reconstructed the SAME
  `RwIm3DVertexSetRGBA` from dwarf, so the header form is probably real.**
  `dwarf/` lists, for both `xFX::RenderRotatedBillboard`/`DrawRing` and
  `xShadowSimple_AddVerts`, exactly one `class RwRGBA* _col;` local per
  invocation of that macro -- and none for `SetPos`/`SetUV`. The stock
  RenderWare form introduces that temp:

      RwRGBA* _col = (RwRGBA*)&((_vert)->r);
      _col->red = (_r); _col->green = (_g); ...

  Our `include/rwsdk/rwcore.h` has the flattened `(_vert)->r = _r;` form.
  **Measured, hand-expanded in xFX: the `_col` form alone is byte-for-byte
  INERT.** So on its own it is a fidelity/naming change for zero match gain,
  reached by two routes. The open question is whether the real macro also
  binds `_a` to a temp, or is an inline FUNCTION (whose argument evaluation
  would hoist the `lbz` naturally) -- `xShadowSimple_AddVerts` needed six
  explicit `alpha = cache->alpha;` re-reads to reproduce retail, which an
  inline function's argument evaluation would give for free. That variant has
  NOT been measured tree-wide and is the thing to test if anyone opens this.

- **Distribute a constant to the USE site rather than folding it into the
  expression -- the constant's register is decided by which temp's live range
  STARTS first.** `zNPCGoalBossSandyLeap::Enter` 99.586 -> 100.0 (532 b) on
  exactly this. Retail allocates `10.0f`->f0 and `1.0f`->f2 so `fdivs` writes
  into the *numerator's* register; we allocated the reverse. Written as one
  statement, `mag = 10.0f * (1.0f / xsqrt(mag));`, mwcc evaluates the
  call-bearing operand first, so the `1.0f` temp is created first and takes
  f0 -- and once f0 holds a value dying at the `fdivs`, the divide targets f1
  and every downstream register follows. The fix:

      mag = 1.0f / xsqrt(mag);
      endX = endX * (10.0f * mag);
      endZ = endZ * (10.0f * mag);

  Now the `10.0f` temp is created after the reciprocal is already a plain
  variable, so it takes f0 and pushes `1.0f` to f2. Eleven other spellings
  measured, including every obvious split; ALL of them scored 99.586 or
  worse. Note especially that `mag = 1.0f / xsqrt(mag); mag *= 10.0f;` gives
  99.624 with retail's operands REVERSED -- higher number, wrong code.

  **The control that proves it**: `zNPCTypeKingJelly::get_away` is 100.0
  today and contains literally
  `F32 scale = 0.70710677f * (1.0f / xsqrt(dist2));` -- the folded spelling --
  and compiles to OUR pattern. So the folded form is provably not what
  Sandy's retail source had. When two call sites of the same idiom want
  different register maps, the difference is where the constant lives.

- **Retail's `fmadds` are usually IN PLACE -- write `base` then `+= a * b`,
  not one fused expression.** Written as `vax = right.x * ppv + at.x * dpv;`
  mwcc gives the product temp and the result different colours. Written as
  `F32 vax = at.x * dpv;` then `vax += right.x * ppv;` they coalesce, which is
  what retail emits (`fmadds f5, f2, f4, f5` -- destination and third operand
  the same register). On `_xCameraUpdate` this plus one declaration-order
  constraint removed 16 of 22 differing rows (99.792 -> 99.949). Look for a
  target `fmadds` whose destination equals one of its source registers; that
  is an accumulate in the original, not a fused expression.

- **`A + B + C` emits `fmuls(B) / fmadds(A) / fmadds(C)`** -- mwcc evaluates
  the RIGHT operand of the inner `+` first, so the MIDDLE term's multiply
  issues first. Source order `y,x,z` gives `fmuls(x)` first; right-associating
  as `A + (B + C)` gives `fmuls(z)` first. If the target's first `fmuls` is
  the middle term of a three-term dot product, your source order is already
  right -- measured on `zThrown_Update`, where only `x,y,z` reproduces
  retail's `fmuls f1,f6,f8`.

- **`c ? K-1 : K` is not `K - (c != 0)`.** Retail materialises the boolean
  SIGN-extended (`neg / or / srawi 31`, giving 0/-1) and ADDs it to a
  separately materialised constant (`lis/addi` then `add r3, r0, r3`); the
  subtraction form gives the LOGICAL 0/1 (`srwi`) plus `subf`. The
  ternary-with-two-constants is what makes mwcc build the constant as a value
  and fold the delta into a 0/-1 mask. Closed `zNPCBPlankton::next_goal`
  (95.185 -> 100.0). **Write `K - 1` literally** -- spelling it as the
  enumerator with the same value measured 87.407, and
  `K + -(c != 0)` measured 85.926 because the constant then folds into
  `addis/addi`.

- **A countdown loop on the parameter** (`while (numTriangles--)`) rather than
  an index loop: the index form costs a callee-saved register and shifts the
  whole file. Worth 83.611 -> 100.0 on `shadowCacheLeafCB`.

- **One shared loop counter per function**, not one `S32 i` per `for`-init.
  mwcc allocates a fresh register per declaration; this took five functions to
  100.0 in `zNPCHazard` alone.

- **Evaluate all components into temporaries, then store.** The target
  computes three sums into `t0x/t0y/t0z` and stores after; a per-component
  load/add/store makes the store to `.x` kill the cached `.y`/`.z`.

- **Read the stack frame before guessing at the body.** The `stwu r1, -N`
  in the prologue is a hard measurement of how much local storage the
  original declared, and every `r1`-relative offset in the diff is a slot
  map you can solve. `start_detaching` was 0xe0 against our 0xd0; the
  missing 0x10 is exactly what `xMat4x3` adds over `xMat3x3` (`pos` plus
  its padding), and because `xMat4x3 : xMat3x3` the `right`/`up`/`at`
  offsets are shared, so nothing else in the function moved. Changing the
  one declaration erased the whole prologue/epilogue diff. Before this,
  look for the *smallest* type change that accounts for the delta — a
  derived type, a bigger array bound — and only then consider an unused
  local.
- **CW numbers same-scope locals in reverse declaration order.** Slots go
  up as declarations go back: the last-declared local gets the lowest
  `r1` offset, the first-declared the highest. Given a target slot map
  you can therefore read off the original declaration order directly and
  reorder to match. In `start_detaching` the target's `eulerVec` at 0x50
  and `world_loc` at 0x5c proved `eulerVec` was declared *after*
  `world_loc`, i.e. down at its first use rather than at the top.
- **Bind a `&` to an aggregate the target keeps in a callee-saved
  register.** When the target computes an address once, holds it across a
  call, and reaches everything through `rN+offset` while we recompute
  `globals@ha`/`@l` at each use, the original bound a reference.
  `xMat4x3& cam = globals.camera.mat;` took `start_detaching` from
  94.097 to 99.172. Declaration point matters: placed before the call it
  matches, placed after it costs 4 points.
- A float compared against a literal zero: `if (speed)` gives
  `fcmpu speed, 0.0` — `if (speed != 0.0f)` gives the operands the other way.
- Reusing a parameter as the destination (`f2 = tmp - f2;`) pins the result to
  that parameter's register.
- Collapsing two statements into one expression changes which temporaries are
  live at the same time, and therefore the register numbering.
- A helper whose return value is never used is usually `void` in the original;
  a non-void return forces the result into `f1`/`r3`.
- **The vtable pointer goes where the first `virtual` is declared.** CW does
  not force it to offset 0. `struct A { void* p; S32 n; virtual void f(); };`
  puts the vptr at 8, and a derived class stores its vtable there. If the
  target stores the vtable at 0, move the virtual declarations above the data
  members. This is also how to tell a *base* that has virtuals from a derived
  class that introduces them: adding `virtual` only to the derived one puts
  the vptr after the base's members.
- Container index parameters are **`u32` (`unsigned long`)**, not `U32`
  (`unsigned int`) — `CFUl` vs `CFUi` in the mangled name. This keeps coming
  up; check it before assuming a body is wrong.
- **`a <= b` on floats gives `cror eq,lt,eq` then `beq`; `!(a > b)` gives
  `ble`.** They are semantically identical and compile differently. Rewriting a
  condition under negation took Dutchman's `turning() const` from 91% to 98.9%.
- **Hoist a `const&` out of an if/else.** When the target computes an address
  *before* the branch and both arms use it, the original bound a reference
  first rather than indexing inside each arm. `const sound_asset& asset =
  sound_assets[which];` ahead of the `if` took `kill_sound` from 52% to 100%
  and `play_sound` from 3% to 96%.
- **`x / 2.0f` is not `x * 0.5f`.** Both emit a single `fmuls` against 0.5f,
  but the division form emits `fmuls rD, var, const` and the multiplication
  form `fmuls rD, const, var`. Operand order is otherwise canonicalised by CW
  and unreachable, so this is one of the few ways to choose it. (`zNPCGoalRobo`
  `LaunchRoboBits` -- it was the entire residue.) Note the trick does not
  generalise to other constants: for `fv * 2.0f` neither `2.0f * fv` nor a
  named temp moves the operands.
- **`S32 flag = (cond) ? 1 : 0; if (flag)`** is the only shape that reproduces
  CW's `li 1 / b / li 0 / cmpwi` boolean materialisation. A bare `if (cond)`,
  an `S32`/`bool` temp assigned from the comparison, and a two-statement
  `if (c) flag = 1;` all get folded away -- all four measured.
- **The narrowing type of a boolean temp picks the compare.** `S32 x =
  (bool)(a && b);` gives `clrlwi` at definition plus *signed* compares at use;
  plain `bool` gives `li 0/1` and `clrlwi.`; `U8` gives `clrlwi` + `cmplwi`;
  `S8` gives `extsb`. Four distinct emissions from one expression.
- **`F32 x = expr; x *= k;` versus `F32 x = expr * k;`** decides whether the
  multiply lands before or after an intervening call.
- **`arr[i++]` in the body with `arr[i]` in the condition** reproduces a
  non-CSE'd double load plus `lwzx base, offset` addressing, where a hoisted
  pointer will not.
- **Reading a `U8` flag field as its declared signed type emits `extsb.`;
  retail often wants the plain byte** (`cmplwi`), i.e. `*(U8*)&field`. Same
  result for a 0/1 flag, but it changes register allocation across the whole
  function -- worth 1.7 points on one `zNPCGoalRobo` function.
- **CodeWarrior inlines a same-TU callee only if it is defined *earlier*.**
  So forward-declare the helper and put its body *after* the caller when the
  target emits a real `bl`. Conversely, a static helper defined above its only
  caller will be inlined whether you want it or not.
- **A table that is uninitialised in our source lands in `.bss`; the target
  has it in `.rodata`/`.data`.** Every relocation against it then mismatches,
  which can make a dozen unrelated functions look broken. `tools/symdump.py`
  and a real initialiser fix all of them at once.
- **Position in the file decides pool index.** Anonymous literals are
  allocated in codegen order, so a function sitting too early in the file
  steals the low pool indices from whatever should own them. Moving
  `register_tweaks` after `ParseINI` in Dutchman was worth a whole cluster.
  The corollary is the most productive move found so far: **write the missing
  pool-contributing functions in the target's source order.** In
  `zNPCSupplement` that realigned pool indices 0-36 and cascaded thirteen
  unrelated functions to 100% for free. Get the target's source order from its
  `.text` symbol order, and the authoritative pool contents from
  `tools/symdump.py` plus `dtk elf disasm`.

  **But first-use order is not the whole rule**, and this is the sharpest
  open problem in the project. `zThrown` is now the reduced test case: its
  pool has exactly the target's 22 entries with exactly the target's values,
  and differs by the position of **one object**, `0.5f`. Inserting a single
  unused `static F32 probe(F32 x) { return 0.5f * x; }` between
  `zThrown_Setup` and `zThrown_AddTempFrame` takes the unit from 18
  non-matching to **9** - nine functions flip at once. That is measured, not
  theorised, and it was removed again because shipping dead code to shift a
  pool is not decompiling.

**Scope check, 2026-08-21: zThrown's REMAINING functions are not pool-blocked.**
The unit is down to four non-matching from the eighteen that motivated the
probe experiment, and every diff row in all four was re-examined: the
anonymous `.sdata2` rows (`@844`, `@257` and friends) all render as
*identical*, so pool ordering is not what holds any of them back. Their
causes are register colouring (`ThrowFruit`, `zThrown_Update` cluster A),
store-to-load forwarding (`zThrown_Update` cluster B) and the alias question
below. So do not reach for the pool explanation here by reflex -- it was true
of the unit as a whole once and is not true of what is left.

**Seen in three units now.** `zThrown` (`0.5f`), `zShrapnel` (`0.5f`), and
`zNPCTypeRobot`, whose `.rodata` opens with **eleven zero-filled objects we do
not produce**, ten of which have no relocation pointing at them from any
section. Inserting eleven dummies proves they are worth exactly 11 functions.
Dead constants created early and referenced late or never is the recurring
shape, and it is unlikely to be three separate accidents.

### What is known about the missing construct

- `.sdata2` section order is ascending `@NNN` order - the compiler's object
  id, assigned at creation - in the target as well as ours. This is not a
  sorting question. It is: *what creates a literal before its first `.text`
  use?*
- **A function's new literals are always contiguous in our output.** In the
  target, `zFruit_Update` uses ids 842, 847 and 932. Non-contiguous, so it
  did not create `0.5f`; it reused one created elsewhere.
- **`fruitPattern`'s static-local suffix pins the boundary.** Ours is `$279`
  with body literals `@293/@294` - the static's id comes *first*. The
  target's is `$863` with body literals `@844/@845` - the static's id comes
  *last*. So `0.5f`, `0.0f`, `1.0f` and `1e-5f` were all created before
  `zFruit_ColorFade` was parsed at all.
- **Fingerprint.** The construct sits immediately after the `airTime`
  computation in `zThrown_Setup`, is compact (ids 1-2 apart), uses
  `0.5f, 0.0f, 1.0f, 1e-5f` in that order, and emits nothing -
  `zThrown_Setup` is byte-count-identical to ours and `AddTempFrame` matches
  100%.
- `zShrapnel` has the same shape (`0.5f` created early, used only late),
  which suggests one shared construct - a header inline or a debug/assert
  macro - rather than a per-unit accident.

Mechanisms ruled out by direct experiment (introduce a novel constant at the
top of a unit, consume it at the bottom, see where it lands;
`poolorder.py` in the scratchpad dumps any object's section symbols in
address order):

- an `inline`-keyword function defined early, called late - lands at the
  **call site**, not the definition; weak inlines compile after `.text`
- a `static` non-`inline` function defined early - lands at index 0, but it
  is also *emitted* there, so `.text` and pool order still agree
- a file-scope `static const F32`, a **default argument** value, and a
  **class static member function** - all land at the use site
- dead code: `F32 unused = 0.5f;` and `if (0) { ... 0.5f ... }` are folded
  before pool allocation
- a member declared in a class body in a header, and an out-of-class `inline`
  member defined in a header - both land at the end of `.sdata2`
- **section splitting.** The target objects have many `.text` sections (18 for
  `zEntCruiseBubble`) where ours have one, which looked like the answer: weak
  inlines in their own COMDATs would explain early literal ids with no
  main-`.text` presence. Sweeping 32 flags found the mechanism — **`-sym on`
  produces 12 `.text` sections** — but it changes no literal ordering and
  yields **zero** additional matches on `zEntCruiseBubble`, `zThrown` or
  `zNPCTypeRobot`. Section layout and pool allocation are independent.

**Unverified, from the `zEntCruiseBubble` agent, and it contradicts what is
written above:** that objdiff pairs anonymous symbols **by name when the
`@NNN` ids coincide**, not purely by ordinal. Its evidence is that
`state_player_halt::update` reaches 100% because our `0.0001f` happens to get
id `@1721`, matching the target's, and that adding an unrelated function which
shifts our id numbering knocked it straight back to 99.904%. If true, aligning
the *count* of preceding objects matters as much as their order. Worth
confirming before anyone builds a strategy on either model.

## Settled

- **`xVec2::create` brace-init is the strongest-evidenced open header change,
  and it is still UNVERIFIED.** `zNPCTypeDutchman` reports `create__5xVec2Fff`
  at 63.636%: the target loads an 8-byte all-zero `@512`, stores it to the
  return slot, then overwrites both words; we go straight to two `stfs`. The
  agent slot-mapped `.sbss2` on both sides -- target 19 slots, ours 18, in
  exact 1:1 owner order with a uniform 8-byte offset -- and named five
  functions whose *entire* residue is that shift and which should reach 100.0
  with the one change: `clip_outside_circle(xVec3)` 99.143, `update_eye_glow`
  99.787, `calc_beam_loc` 99.859, `Process__PostFlame` 99.915, and `create`
  itself.

  **Do not apply it on that evidence alone.** The identical change to
  `xVec3::create` was equally well evidenced, made both overloads byte-exact,
  and still came out +2/-5 with a broken DOL on a full rebuild, because it adds
  an object to `.sbss2`/`.rodata` in *every* TU that instantiates it and
  `.sbss2` ordering is per-object. Measure with `find src -name "*.cpp" -o
  -name "*.c" | xargs touch && ninja`, a per-function `report.json` diff, and
  the `306526d9...` DOL check before believing either direction.

- **`xVec3::create` brace-init: measured, and it BREAKS THE DOL. Do not apply.**
  The target's `create__5xVec3Ffff`/`create__5xVec3Ff` load a 12-byte all-zero
  `.rodata` template, copy it to the frame and then overwrite all three words,
  so `xVec3 v = { 0.0f, 0.0f, 0.0f };` reproduces them exactly and takes both
  from 50.000% to byte-identical. It is still a no-go. A full build gives
  **+2 functions, -5 functions, and `main.dol` sha1
  `e81045024e60853b3c12a37cc9bf5b1682b60bea`** instead of `306526d9...`.

  The losses are the point: `xMath3Init`, `xParEmitterEmitSphereEdge`,
  `get_triangle_area` (`zFX`), `FodBombBubbles` (`zNPCHazard`) and
  `zVarGameSlotInfo` (`zVar`) each fall off 100.0, and several sit in units
  that are `Matching`, which is why the link moves.

  Two lessons. First, a header change that is provably right *for one function*
  can still be wrong for the tree -- this one is genuinely what retail's
  `create` did, and it still cannot land while other units depend on the
  current shape. Second, and more useful: **"most of the damage is POOL-class,
  so it is free" is not a safe inference.** That was the reasoning used to
  re-open this after it had been rejected on `solo.py` counts, and it was
  wrong -- the regressions were real `OTHER`/`SIZE` losses, not pool ordinals.
  Re-price on `report.json` *and* check the DOL before believing either
  direction.

  The same brace-init question is open for `xVec2::create`, which is written up
  further down as a recommended fix. It has not been measured this way. Do that
  before applying it.

- **`xVec3::cross` zero-template: same family, same result, DO NOT APPLY.**
  In the target, `cross` copies its 12 zero bytes from a *local anonymous*
  `.rodata` template (`@410`), not from the external `m_Null__5xVec3`, so
  `xVec3 v = { 0.0f, 0.0f, 0.0f };` instead of `= xVec3::m_Null;` makes the
  function byte-exact: `cross__5xVec3CFRC5xVec3` 59.355 -> **100.0**.

  A full build then gives **-4 functions, exact 71.410 -> 71.304, and DOL
  `ef8df9bd7b548636bb5b2380faeb61969725c308`**. Five functions fall off 100.0
  and they are *the same five* the `xVec3::create` entry above names --
  `xMath3Init`, `xParEmitterEmitSphereEdge`, `get_triangle_area` (`zFX`),
  `zVarGameSlotInfo` (`zVar`), `FodBombBubbles` (`zNPCHazard`) -- plus six more
  regressions in `xCamera`, `zNPCGoalRobo`, `zNPCTypeRobot`, `zNPCHazard`,
  `xCM`. Adding an anonymous template to *any* `xVec3` inline reshuffles
  `.sbss2`/`.rodata` in every TU that instantiates it, and the victim list is
  a property of the class, not of which method you touched.

  **The blast-radius estimate that justified trying it was wrong in an
  instructive way.** It was measured as "`cross__5xVec3CFRC5xVec3` appears in
  exactly one unit in `report.json`, so only that object changes". That counts
  where the *out-of-line body* is emitted; it does not count where the inline
  is *instantiated*, which is every TU that calls it. For a header change,
  grep the call sites, never the symbol table.

- **Making `xsqrt` an `inline` in `xMathInlines.h` does not compile.** The
  hypothesis is well-evidenced -- `xsqrt__Ff` is STB_WEAK in retail's
  `xBound.o`, and the four literals it creates (0.5, 3.0, 100000, 1e-5) are
  exactly the group missing from six of our objects (`xBound`,
  `zNPCTypeBossSB2`, `zNPCTypeBossPlankton`, `zEntCruiseBubble`,
  `zEntPlayerBungeeState`, `zNPCTypePrawn`). But `isinf` lives in MSL's
  `math_api.h`, which `xMathInlines.h` does not include, so ~250 TUs fail with
  `undefined identifier 'isinf'` (10 of 451 units before smoke.py aborted).
  Adding that include shifts include order, and therefore anonymous pool
  serials, project-wide. Anyone retrying this needs an answer for `isinf`
  first.

- **Stripping the dead aggregate initialisers out of `xLaserBolt.h:160`
  (`xVec3 temp = { 0, 0, 0 }`) and `zNPCTypeBossSB2.h:287` (`xVec2 cur = {..}`)
  is wrong.** Those templates leak into every TU that includes the headers and
  shift `.rodata`/`.sbss2`, which is real -- `zScene.o` carried both -- but
  removing them costs **exact 70.659 -> 70.648, -1 function**, and breaks the
  two functions that own them: `perturb_dir` 100.0 -> 80.889 and `turning`
  87.87 -> 65.833. The initialisers are what retail wrote. Fix the *consumer*
  by dropping the unnecessary `#include` (see `zScene.cpp`), not the header.

- **`solo.py`'s `@NNN` ordinals are not comparable to the target's, and a
  large finding was built on the assumption that they are.** solo compiles into
  a private temp dir where mwcc numbers anonymous literals differently from the
  real build. An agent measured "32 functions / 28,840 bytes in `zEntPlayer`
  blocked by a two-slot `.sdata2` misalignment", proposed a third `float_fix`
  shim to create the slots, and reported solo going 80 -> 48 non-matching. On a
  real build the payoff was **exactly zero**: `zSandy_AnimTable` (14,188b),
  `zEntPlayerVelUpdate`, `CheckObjectAgainstMeleeBound` and `zEntPlayer_Damage`
  were already at 100.0 in `report.json`, and `matched_code`/`matched_data`
  were unchanged to four decimal places.

  Rule: **use `solo.py` for code shape, never for pool questions.** To test a
  suspected pool mismatch, diff the object the real build produced:
  `objdiff-cli diff -1 build/GQPE78/obj/<unit>.o -2 build/GQPE78/src/<unit>.o`.
  A useful cross-check is that `matched_code` equals the sum of the sizes of
  the functions at exactly 100.0 in `report.json`.

- **A second compiler defect, sibling to 2b: a small loop bound that retail
  keeps in a callee-saved register, mwcc folds into `cmpwi`.**
  `iSndWaitForDeadSounds` wants `li r31, 0x8c` / `cmpw r0, r31`;
  `iSndSceneExit` wants `li r30, 0x190` / `cmpw r0, r30`. Seventeen source
  spellings were measured (plain/`const`/`register`/`static` locals, separate
  assignment, `for`-init, `do/while`, `goto`, reversed and negated compares,
  `long`/`U32` types, redundant self-assignment, extra break test) under
  GC/2.0p1, GC/2.0p1a and GC/2.6 -- **every one folds**. The `i = 0x8c;`
  re-assignment hack currently in `iSndWaitForDeadSounds` buys 95.455 against
  80.636 for the honest form and no matched function.

- **`ninja` does not track every header dependency. Incremental builds after a
  header edit can be measured on stale objects.** `zNPCGoalRobo.cpp` includes
  `xEnt.h` on line 5; `ninja -t deps` lists 180 dependencies for its object and
  `xEnt.h` is not among them, so after editing that header `ninja` reported
  "no work to do" and left a stale object behind. 376 of 542 objects predated
  the edit, most legitimately (bink/rwsdk do not include it), but not all.

  This matters because it silently changed a conclusion. The incremental build
  of the inline-helper fix reported **0 flipped, 0 lost**; a forced full
  rebuild of the same tree reported **0 flipped, 1 lost**. The regression was
  real and the incremental build hid it.

  **After any shared-header change, force a full rebuild before believing the
  number**: `find src -name "*.cpp" -o -name "*.c" | xargs touch && ninja`.
  Every header change measured before 2026-08-13 was measured incrementally and
  may be understated. HEAD as of `eeacf61b` has since been re-verified with a
  full rebuild -- DOL sha1 intact, 8029 functions -- so the committed state is
  sound; it is the *rejected* experiments whose costs may have been larger than
  recorded.

  **RESOLVED, and `solo.py` was the one telling the truth.** The apparent
  `solo.py` / `report.json` disagreement on `check_hide_entities` was not a
  compiler or flags problem: solo's private compile and ninja's object are
  **byte-identical** (verified by hashing both, plus a third compile with
  ninja's exact `-o <dir>` and `-MMD` form -- all three sha1-equal), and solo
  derives its 77 flag tokens from `build.ninja` with no divergence at all.

  The function's 172 bytes already matched the target. What differed was
  **binding**: ours `GLOBAL`, the target's `LOCAL`. objdiff pairs by name and
  scored it 100.0; dtk's report pairs local symbols by ordinal, so when the
  inline-helper change altered the object's local-symbol set it mispaired this
  function against a different one and reported 91.047%. Adding the missing
  `static` (`58dcb963`) fixed the binding, and the inline-helper change is now
  measured at **zero cost** -- no function gained or lost tree-wide, DOL
  unchanged.

  Two things worth carrying forward. `report.json` can report a *false*
  regression when a unit's local-symbol set changes, so a surprising drop on a
  function you did not touch is worth checking against the raw bytes before
  believing it. And a `GLOBAL` symbol that should be `LOCAL` is not cosmetic --
  it makes that unit's scores fragile against unrelated edits.

- **RETRACTED: "we emit inline helpers in dozens of objects, retail emits one".
  That was a methodological error, and the method is the lesson.** The target
  objects under `build/GQPE78/obj/` are not retail's compiler output. They are
  reconstructed by decomp-toolkit from the *linked* DOL, where `mwld` had
  already collapsed every weak duplicate into a single copy. So a target object
  set can only ever show **one** definition of any weak or inline-emitted
  symbol, no matter how many the original compile produced.

  Measured across three unrelated symbols, all showing the same shape:

  | symbol | target objects | ours |
  |---|---|---|
  | `__as__5xVec3FRC5xVec3` | 1 | 73 |
  | `__as__4xBoxFRC4xBox` | 1 | 7 |
  | `__as__7xSphereFRC7xSphere` | 1 | 7 |

  **Counting definitions of a weak symbol across target objects measures the
  linker, not the source.** Do not draw conclusions from it. Reference counts
  are still meaningful -- ours 856 against the target's 893 for `__as__5xVec3`
  says our call sites broadly agree -- but definition counts are not.

  Two changes were made on the strength of the bad reading before it was
  caught. `8211ea95` moved `xEntGetPos`/`xModelGetFrame` out of their headers
  into single `WEAK` definitions; it measured at zero cost with the DOL intact,
  but the premise was false, an `inline` one-line accessor in a header is what
  the original source almost certainly had, and `zLight` still fails
  `fliptest` without it. **Reverted in `be71d261`.** The second, giving `xVec3`
  a user-declared `operator=` with one out-of-line definition, was far worse
  and never landed: it makes `xVec3` non-trivially-copyable, so every struct
  containing one gets a member-wise implicit `operator=`, and a full rebuild
  measured **+1 function, -39** -- `__as__9xEntFrame` fell to 0.000%,
  `__as__13zThrownStruct` to 54%, `__as__5xBBox` to 50%. This is the exact
  inverse of the `xCollis::tri_data` finding below: a user-declared `operator=`
  on a widely-embedded value type is poison, in both directions.

  `zLight` remains unlinked and its actual blocker is **unknown**. It is 17/17
  functions at 100%, its object carries `__as__5xVec3` where the target's does
  not, and removing the other two surplus symbols did not help -- so the
  surplus-weak-symbol theory does not explain it either. Note `zVar` is
  `Matching` while carrying a surplus `__deadstripped_zVar`, so surplus symbols
  are evidently tolerable in at least some cases. Start there.

- **`xDebug` is blocked on something else and is *not* covered by the above.**
  Its 16 functions are at 100% and `fliptest` still fails. The set difference
  is our surplus `__deadstripped_xDebug` carrier -- tolerable in itself, since
  `zVar` is `Matching` carrying the same device -- plus one ordering
  difference: the target emits `__as__10iColor_tag` immediately after
  `create__5xfont`, its only caller, while we emit it two slots later, after
  `NSCREENY`/`NSCREENX`. Emission order is otherwise exactly reverse order of
  first use on both sides. Tried and failed: adding an explicit `iColor_tag`
  assignment between the NSCREEN calls and `xfont::create` in the carrier, on
  the theory that retail's deadstripped function had one -- CW folds the
  trivial copy, emits nothing, and the order does not move. This looks like a
  difference in when the compiler flushes a nested instantiation dependency,
  not something reachable from source shape.

- **A user-declared `operator=` on a union member was holding `xCollis` back,
  and it was never legal.** `xCollis::tri_data` carried a hand-written
  `operator=`, which makes `xCollis` non-trivially copyable, so
  `__as__10xEntCollis` emitted a loop calling `__as__7xCollis` where the target
  does a flat `lwz/lwzu + stw/stwu` copy of 180 8-byte units. It sat at
  **0.000%**. `tri_data` is used as a union member, and C++98 forbids a union
  member with a non-trivial copy assignment operator -- CodeWarrior accepted it
  anyway, which is why it survived. Removing it: 0.000 -> 100.000, no
  regression anywhere in a full build. **Worth sweeping for the same shape
  elsewhere**: a nested type with a hand-written `operator=` that is also used
  inside a union is both illegal and a matching blocker.

- **`xSndIsPlayingByHandle` should stay `U8`. Measured and rejected.** The
  reading that it should be `U32` -- because the adjacent `xSndIsPlaying` is
  `U32` over the same `bool`-returning `iSnd*` call, and because we emit a
  `clrlwi` retail does not -- is wrong. Changing it does not even fix
  `xSndIsPlayingByHandle` itself, and it costs `zEntPlayer_SNDPlayStream` and
  `zEntPlayer_SNDStopStream` their 100.0. Net -2. Do not retry.

- **"POOL is worth zero" has a corollary that is worth a great deal: driving a
  REAL row *into* the POOL bucket is a full `report.json` win.** These are two
  different moves and it is easy to conflate them. Realigning the pool under
  rows that are *already* POOL-only buys nothing, because they read 100.0
  already. But taking a function whose instructions genuinely differ and fixing
  the source until the only remaining difference is a pool ordinal moves it
  from below 100.0 to exactly 100.0. Measured on `zNPCGoalRobo`: one function
  reached byte-exact and twelve more became POOL-only, and the build scored
  **all thirteen**. So the instruction to agents is "do not work on POOL rows",
  never "do not let a function end up POOL-only" -- POOL-only is a finished
  function.

- **CodeWarrior emits `__declspec(section)` functions in REVERSE order of
  definition.** `Runtime/__mem.c` defined `memset, __fill_mem, memcpy` and the
  object came out `memcpy, __fill_mem, memset` against a target of `memset,
  __fill_mem, memcpy`. Reversing the definitions produced the target order,
  `symorder.py` went green, `fliptest` passed and the unit is now `Matching`.
  Declaration order in the header is *not* the driver -- that was changed first
  and moved nothing, so `__mem.h` is untouched. Worth trying wherever a
  `.init`/`.ctors`/`.dtors` section is in the right set but the wrong order.
  Note this was undiagnosable until `4647a07f` restored the `__declspec` guard
  and moved these three functions out of `.text`; it is the second unit that
  one-character fix has unblocked.

  **Do not generalise this to `.bss`.** Function-local statics are laid out in
  **ascending** declaration order, the opposite way round, and the two are
  separate mechanisms. Measured on `zEntPlayer_Init`'s four `drybob` arrays:
  declaring them `chgData, oldData, chgTime, oldTime` (matching the target's
  `.bss` order at `r31+0x6ac/0x7ac/0x8ac/0x9ac`) gives **94.829%**, reversing
  them gives **94.513%**. This entry originally claimed the reverse rule
  covered both and `52461655` acted on that; `b1d360cf` corrects it. The
  ascending order is also what `dwarf/` reports, so `dwarf/` is a usable
  cross-check for `.bss` layout but says nothing about `__declspec(section)`.

- **Never buy pool alignment with an explicit template instantiation.** In
  `zNPCHazard`, `xUtil_choose<int>` is instantiated by the target at the call
  site, so its int->float magic constant owns `.sdata2` 0x120; our build defers
  instantiation to end of TU and parks it at 0x150. Adding
  `template S32 xUtil_choose<S32>(const S32*, S32, const F32*);` at the call
  site realigned the whole `.sdata2` tail and took the unit **39 -> 33** in
  `solo.py`. It was still the wrong trade, for two reasons, and it was reverted.

  First, an explicit instantiation gives the symbol **GLOBAL** binding where the
  target's is **LOCAL** (`readelf -sW`). objdiff does not compare binding, so
  `solo.py` and `report.json` are both blind to it -- but `symorder`,
  `fliptest` and the real link are not. This is the same axis that surfaced the
  dead `__declspec`, and it is worth remembering that a device invisible to the
  metric can still be a genuine object difference.

  Second, and decisively: **it bought zero.** Measured directly by building both
  ways -- with the line, Game Code 6780 functions and 1038976 bytes; without it,
  6780 and 1038976, identical to the byte. The ten `solo.py` rows it moved were
  all already 100.0 in `report.json`, because they were pool rows. This is
  Phase 2a doing exactly what 2a was repriced to do in the entry below, and it
  is the second time a `solo.py` gain of this shape has evaporated on the
  metric. **Price data-layout work against `report.json` before accepting it,
  never against `solo.py`.**

  The corollary cuts the other way too, so check rather than assume: `xFX`'s
  missing `.rodata` strings looked like the same trap and were not.
  `xFXRibbonSceneEnter` read 99.947 in `report.json`, not 100.0, so the
  `__deadstripped_xFX` carriers bought a real function.

- **`__declspec` was dead tree-wide, and the fix completed the SDK category.**
  `include/types.h` had `#ifdef __MWERKS__ / #define __declspec(x)`. The
  original commit `06a3f860` wrote `#ifndef` -- stub the attribute for the host
  and IDE compilers that cannot parse it -- and `50c8ffa7` flipped it, which
  inverts the meaning: the attribute became a no-op under CodeWarrior, the one
  compiler where it carries information, and stayed live for the compilers that
  choke on it. Dead as a result: 24 `section` attributes, 19 `weak`, and the 23
  uses of the `WEAK` macro, which expands through `__declspec(weak)`.

  Verified against the compiler, not assumed: `__declspec(section ".ctors")`
  works unaided, an undeclared section name is a hard error, and `.ctors`,
  `.dtors`, `.init`, `.sdata2` -- every name the tree uses -- are accepted.
  `.bss`/`.sbss`/`.sbss2` are rejected, and `.ctors`/`.dtors` accept data but
  not code. So the attribute was never unsupported; it never arrived.

  Restoring `#ifndef` moved `memset`/`memcpy`/`__fill_mem` and
  `__init_hardware`/`__flush_cache` from `.text` to `.init`, moved the three
  `_reference` objects from `.sdata2` to `.ctors`/`.dtors`, and changed 32
  symbols from GLOBAL to WEAK, in every case toward the target. DOL sha1
  unchanged; 7924 -> 7935 functions; four units became linkable and
  `complete_units` went 227 -> 231. **SDK Code is now 90/90 at 100.000%
  fuzzy.**

  The general lesson: a macro that neutralises a compiler attribute is
  invisible to every diff tool here, because the object is self-consistently
  wrong. `tools/` gained nothing to detect this; the way it surfaced was
  comparing **symbol binding and section** between target and ours, which no
  existing tool did. That comparison is worth keeping -- it still reports
  ~2044 mismatches, of which 1033 are WEAK-in-target/GLOBAL-in-ours from
  header-defined functions and are a separate, unexamined lever.

- **A "cluster of near-identical functions at 99.8%" is a pool-alignment
  symptom, not a codegen problem.** `xFont` showed 14 `parse_tag_*` and 17
  `reset_tag_*` functions all at 99.7-99.9%, which reads like one missing
  source idiom repeated. It was one bad line in an unrelated function: a
  `typedef __typeof__(((struct font_asset){ 0 }).char_pos[0])` in
  `get_tex_bounds` emitted a 404-byte anonymous `.rodata` object referenced by
  nothing. Those bytes shifted every later `.rodata` offset, and since objdiff
  pairs anonymous symbols **by ordinal within a section**, every `cb$` callback
  table after it mispaired. Removing it flipped ~50 functions at once.
  `xFont` went 67 -> 9 non-matching.

  So: when many functions in one unit sit just under 100% with a single
  differing relocation each, look for a surplus or missing *data* object
  earlier in the section before looking at any of the functions. Same shape as
  the `.sbss2` entry below, and the same shape as `zNPCTypeRobot`'s `.bss`
  ordering pass.

  Note the metric consequence: report.json already scored most of those 50 at
  100.0 (it is blind to anonymous pool ids), so a 58-function solo.py gain
  showed up as far less on the project figure. **Always quote the report.json
  delta, not the solo.py delta.**

- **`classify.py` was misfiling pool-only functions as OTHER.** `norm_reloc`
  collapsed `@NNNN` but not the `$NNNN` suffix CodeWarrior appends to a
  function-local static (`npcmsg$1475`, `skipstates$1647`) -- a per-TU counter
  that differs between the two objects for the same variable. 178 instructions
  in `zNPCTypeRobot` alone carry one, and 11 of the rows the tool ranked as
  that unit's highest-value work were pool-only. Fixed in `d695e5d9`. Any
  ranking produced by `classify.py` before that commit over-states OTHER.

- **An aggregate initialiser in an inline function seeds a junk `.sbss2`
  object -- this was the anonymous-literal mystery.** CodeWarrior creates an
  anonymous 4-byte all-zero object at *parse* time for an aggregate
  initialiser inside an `inline` function, in every TU that includes the
  header, whether or not the function is ever instantiated. Minimal repro
  under the real flags: `inline grid_index f(U16 a, U16 b) { grid_index i =
  {a, b}; return i; }` emits `@1 4 bytes .sbss2`; rewritten as member
  assignments it emits nothing. `xGrid.h`'s `get_grid_index` was doing this,
  which is where `@148` in `zDispatcher` and `@150` in `xModelBucket` came
  from, and it shifted every later `.sbss2` operand by four bytes in about
  140 units. Fixed in commit 134129c2.

  **But the shape is not wrong everywhere -- measure, do not pattern-match.**
  The only two other instances in the tree, `zNPCTypeDutchman.h`'s
  `xVec2 facing = {0,0}` and `xLaserBolt.h`'s `xVec3 temp = {0,0,0}`, are
  both correct: removing them costs `xLaserBolt` a matched function and drops
  `zNPCTypeDutchman`'s fuzzy. In those two the original really did write an
  aggregate initialiser. Both were measured on a full build and reverted.

  **Correction: retail's `get_grid_index` did have the initialiser too.** The
  claim previously recorded here -- that `xGrid.h` was the one case where the
  original did not write one -- is refuted by the target itself. Disassemble
  `get_grid_index__FRC5xGridff` and the first thing it does is
  `lwz r5, @587@sda21` / `stw r5, 0x8(r1)`, filling the whole 4-byte
  `grid_index` local from a constant before either field is assigned, and
  `@587` is `.sbss2:0x803D0818`, `size:0x4`, `scope:local` -- precisely the
  junk object. So removing the initialiser was still the right call on the
  numbers (it was worth ~140 units), but the reason cannot be "retail did not
  write one". Something else about our header's reach differs: retail emitted
  that object in *fewer* TUs than our `xGrid.h` does, so keeping it added four
  bytes where retail had none.

  The consequence for anyone in `xScene`: `get_grid_index` sits at **60.558%**
  and **cannot cross 99%** while the initialiser is deliberately absent -- the
  missing `lwz`/`stw` pair alone is more than 1% of a 43-instruction function.
  Treat it as a priced ceiling, not a lead. There is a second, independent
  defect in it that *is* reachable if anyone wants the fuzzy: retail computes
  both products before the first `bl` and parks the z product in `f31`, where
  we park the raw `z` argument and recompute after the call. Hoisting both
  `(v - min) * inv_csize` products into locals should recover it -- but it is
  a shared header with a ~140-unit blast radius and it buys no function, so
  measure the sweep before spending the time.

  This is the general answer to "what creates a literal before its first
  `.text` use", and it is worth checking other headers for the same shape
  before assuming a pool ordering is unreachable. `iMath3` and `iScrFX` are
  both blocked on exactly that: their target pools are seeded with constants
  that no function in the `.cpp` materialises, and their anonymous indices
  start around 555 and 527 against our 68 and 37, i.e. the original created
  far more anonymous objects during header parsing than we do.
- **`complete_units` is a `configure.py` marker, and `report.json` cannot
  tell you when to set it.** A unit reaching 100% in `report.json` does not
  raise `complete_units`; that number counts `Object(Matching, ...)` entries
  (`Matching = True`, `NonMatching = False`, `Equivalent = config.non_matching`,
  configure.py:401). Flipping a marker makes dtk link *our* object instead of
  the extracted original, so it is only safe when our object would link to
  the same bytes -- and `report.json` scores things 100% that would not. It
  called `zDispatcher` 23/23 with 100% matched data while the built object's
  `.data` was 92 bytes against the target's 96.

  The reliable test is a real link, and it is cheap: the objects are already
  built, so flipping one marker and running `ninja` costs about nine seconds.
  `tools/fliptest.py --test` flips each candidate on its own and reports
  PASS/FAIL; `tools/symorder.py` explains the failures. Do not bisect --
  test every candidate singly, because the failures are independent.

  The trap worth knowing: **objdiff matches symbols by name, so it is blind to
  function order**, while the linker lays functions out in definition order.
  Two units scored a flat 100% on every symbol and still moved the DOL.
  `abort_exit.c` defined `abort` before `exit` and the target has `exit`
  first -- swapping the definitions was the entire fix. `mem.c`'s `.text` was
  the *exact reverse* of ours, which is the `-inline deferred` signature; the
  earlier whole-tree flag sweep could not have found it, because reversing
  the order does not move the objdiff percent. It is worth re-checking other
  units for that signature.
- **Compiler patch clause C — +41 on its own, and it kills the zThrown float
  meme.** Dispatch entries 1 and 3 (whole object vs subrange, both operand
  orders) only tested "same base object", so a load of a small global or a
  float literal hoisted across a store to a *different* small global —
  `stfs c_fruit` vs `lfs globals.throwHeight` in `zThrown_Setup`. Clause C
  answers may-alias for those entries iff both base expressions have word 0
  == 5, both sizes are <= 4, the opcodes differ, and both instructions are
  plain loads/stores. Word 0 == 5 is the **static-storage gate**: frame and
  stack objects carry `0x00010005` and are excluded. That gate is the whole
  safety property — ungated, the same predicate pins integer-conversion stack
  traffic (`stw` frame slot vs `lfd` magic double) and costs 50 exact
  functions (+84/-50). Full details, including which variants were measured
  and rejected, are in the `tools/patch_compiler.py` docstring and commit
  message. `zThrown_Setup` 85.50% -> 99.35%; the residue is an r6/r7/r8
  allocation permutation, i.e. REGS, not SCHED. Three sub-100 functions
  wiggle down (`NPAR_TubeSpiralMagic` 98.9 -> 81.8, `VFXSmokeStack`
  83.4 -> 77.6, `zEntPlayerTSlideUpdate` 94.4 -> 94.2) against ~84 that
  improve.
- **Clause C had to be taught about volatile — +11 more.** A volatile access
  sets instruction flag bit `0x80`, so the plain-load/store test `flags & ~6
  == 0` rejected every volatile reference before it reached the clause. That
  is why `zMenu`, whose timers are `static volatile F32`, kept hoisting a
  literal across a store to a different small static *with* clause C
  installed. Widening the mask to `~0x86` and adding the clause to entry 0 as
  well, both behind the static-storage gate, measures 7279 -> 7290 with zero
  units regressed and the DOL sha1 unchanged. Widening entry 0's *clause A*
  the same way is not safe — it pins volatile frame locals and measures -4.
  Patched compiler sha1 is now
  `7d3ff244fb371e3b15b0becd41ac04b627869ae8`.
- ~~**Clause D is dead — do not refit it.**~~ **REFITTED AND INSTALLED
  2026-08-12 as E3n (`3316f9f0`, `e88c7360`). This entry was stale and cost the
  project real progress; read the correction below before believing anything
  that follows it.**

  Re-measured on today's tree the same clause is **+59/−10**, not +22/−18, and
  with eight of the ten losses recovered from source it is **+63 net**: Game
  Code 6673 → 6736 functions, exact 60.038% → 62.235%, DOL sha1 unchanged,
  `complete_units` unchanged at 89. Nothing about the compiler changed. The
  *tree* changed — most functions clause D used to damage have since been
  rewritten or converted by ordinary source work.

  **The generalisable lesson: a shelved patch verdict is a property of the patch
  TIMES the tree it was measured against. Re-measure before trusting any of
  them, including E3n itself.** Two separate investigations this month reached
  "no-go" on this clause by reasoning from the recorded numbers instead of
  re-running them.

  **The lever that recovered the losses: `const` on the destination frame
  local.** E3n's gate requires the store side to be a *declared* frame object
  (`base word0 == 0x00010005`, `[base+0x18] != 0`); `const` clears that field,
  no alias edge is created, and the retail schedule returns. Every loss was the
  same idiom — `xVec3 local = <expr>;`, a compiler-generated three-word struct
  copy sharing a block with a `.sdata2` literal load. It is semantically
  accurate wherever the local is never modified, so it is a faithfulness
  improvement rather than a hack. It does **not** work on by-value call
  arguments, nor where the local is written later (`BasisBspline`'s `Ntemp`,
  `nearestTrackCB`'s `pdx[]`/`pdz[]`).

  Two losses remain and are the standing price: `xMath3::xBoxFromCircle`
  (77.875%; recoverable to 99.375% only via an aggregate initialiser that drops
  the target's 12-byte zero `.rodata` template `@441`, so the faithful shape is
  kept) and `xSpline::BasisBspline` (96.264%).

  Also settled, so nobody re-runs it: the losses were **not** "false matches"
  that only worked by accident under the old scheduler. `ColTestCyl`'s split
  multiply and `dampen_velocity`'s comparison ladder look contorted but are
  faithful — the natural fused forms measure 82.5% and 47.8%. Roughly 45 source
  shapes were measured across the ten before the `const` lever was found.

  The original entry, preserved because its measurements were honest at the
  time: a directional rule (an `stfs` to a
  declared frame local may not be crossed by a *later* small static load,
  entry 3 only) hits exactly the motion four otherwise-finished functions
  need, and measures +22 functions to 100% against 18 whose percent drops.
  The gain and loss populations are indistinguishable in every field the
  predicate can see — same opcodes, sizes, overlapping offsets, same storage
  classes, same base-expression words. stfs-only, the declared-local gate,
  entry-3-only, offset thresholds and a literal-only static side were all
  measured; none separates them. Recovering the losses from the *source* side
  was then tried on four of them (`dampen_velocity`, `BoundAsRadius`,
  `get_texture_size`, `xBoxFromCircle`): statement reordering, operand swaps,
  binding the literal to a local, and initializer restructuring all left the
  percent unchanged. The instruction *set* already matches; only the order
  differs, and with the edge added the list schedule is deterministic, so
  source shape has no purchase on it.
- **Edge latency is not the lever either — the zero-latency lead is
  refuted.** The suspicion was that clause D's drops came not from adding the
  alias edge but from the edge carrying normal store-to-load latency, where
  retail's placement looked like what a zero-latency edge would produce. The
  mechanism is real and now fully read out of the binary. `0x5084f0` is the
  edge builder, `cdecl(from, to, flag)`: `flag != 0` takes the latency from
  `word[from+0x10]` and, when bit 0 of the to-side access flags is set, adds
  `word[[0x5e0850]+8]` from the machine model; `flag == 0` gives latency 0.
  It has ten call sites; exactly three are the may-alias sites (`0x5081fd`,
  `0x508376`, `0x5083ab`, each `call 0x511fc0` / `test al,al` / `je` /
  `push 1`), and the barrier, volatile and branch chains already pass 0. The
  same call sites confirm operand A is the earlier instruction
  (`push [later]+0xc` then `push [current]+0xc`). Measured against the
  installed build: zeroing only the new clause-E edges takes it from
  +20/-17 to +10/-20; zeroing the shipped clauses A/B/C costs 108 functions;
  zeroing every alias edge costs ~1000. And the premise is simply false —
  `xFont.o` and `zNPCTypeCommon.o` come out byte-identical either way,
  because an `stfs`'s `word[insn+0x10]` is already 0, so those edges never
  carried latency to begin with. Nothing to install.
- **MSL_C compiler flags were wrong - +51.** `configure.py` built
  `MSL_C.PPCEABI.H` at `-opt level=0 -inline off`. Sweeping against the target
  objects: level 0 matches *nothing* anywhere, level 4 is best or tied for
  every unit (+40), and `-inline on` beats `-inline off` in seven more (+11).
  Nothing regressed. Two Runtime units want `-inline deferred` (+1).
- **dolphin and SB flags are right.** A 42-flag sweep over all 90 dolphin
  units and a 15-flag sweep over all 164 SB units produced no verified gain.
- **bink is built with ProDG (SN gcc), not CodeWarrior.** CodeWarrior `asm`
  blocks do not compile there; use GNU inline asm. `binkngc`'s time-base
  readers are single asm blocks with hardcoded r11/r9/r0.

- **`xVec3::operator=` — done, +11.** The hand-written definition was blocking
  every implicitly generated assignment operator of a struct containing an
  `xVec3`. CodeWarrior inlines an *implicit* member `operator=` into the
  enclosing implicit one, but emits a call when the member's is *user-declared*.
  Deleting both declaration and definition lets CW generate it; it is still
  emitted out of line in `xBound.o` and still matches byte for byte. The
  apparent contradiction (`__as__5xVec3FRC5xVec3` exists in the target objects)
  was the clue misread: that symbol *is* the compiler-generated one.
- **POOL is not cheap after all.** Investigated `xMath`: our `.sdata2` pool has
  the same *contents* but the double literal used by `xurand` lands at index 1
  instead of 8, because the functions ahead of it in the file are stubbed and
  allocate one float literal where the original allocates seven. Aligning a
  pool needs the whole unit reconstructed, not a local edit. Treat POOL as a
  *symptom* of unit incompleteness rather than an independent bucket.
- **`xVec3 v = { 0, 0, 0 }` in `xVec3::cross`** matches the target's anonymous
  rodata template and takes `cross` from 36% to 91%, but it adds that template
  to the `.rodata` of every unit that includes `xVec3.h`, which shifts `zVar`'s
  pool and **breaks the DOL**. Reverted.

## Working the MISSING bucket

`zNPCTypeDutchman` went 59 -> 86 matching in one pass. What worked:

1. Ghidra headless (`gh.sh`) on a batch of the smallest missing symbols at
   once. The one-liners (`get_orbit`, `get_center`, `get_facing`,
   `enable_emitter`, `emit_particles`, `PRIV_GetLassoData`, `IsAlive`, ...)
   came out matching first try, 11 for 11.
2. Reading struct offsets straight out of the Ghidra output against the
   headers - `this+0x24` is `xEnt::model`, `model+0x4c` is `Mat`, and so on.
3. For anonymous-namespace tweak structs, computing field offsets from the
   declaration and looking up the one the target loads (`tweak+0x170` turned
   out to be `damage.slime_time`).
4. **Template members only appear when something calls them.** Explicit
   instantiation (`template struct static_queue<T>;`) is silently ignored by
   this compiler - CodeWarrior still only emits used members. Writing one real
   method (`update_slime`) pulled in nine container functions with it.

A second pass added `update_hand_trail`, `refresh_reticle`, `halt`,
`turning`, `get_eye_loc` and the Nil/Disappear/Reappear goal entry points.
`zNPCTypeDutchman` is now 94 / 227.

Six of those sit at 99.4-99.8% blocked on one thing: the target's `.sdata2`
float pool starts `@1603, @1604, @1605 (0.0f), @1606 (1.0f)` while ours has
thirteen entries ahead of `0.0f`. Literals are allocated in first-use order,
so the pool only lines up once the functions ahead of them in the file are
written. `@1603`/`@1604` are used by the no-argument `turning() const`, which
suggests that function sits near the top of the original file.

`static_queue::init` was also simply wrong: `_max_size` is the rounded-up
power of two and `_max_size_mask` is that minus one, not the shift count and
the power. Fixing it also fixed the instantiations in `xDecal` and
`xLaserBolt`.

## The STUB bucket

Found by `tools/stubs.py`. A **stub** is a function whose symbol exists in
both the target and our object but whose body in our source is an empty `{ }`
placeholder, so our object has a bare `blr`. This is a distinct bucket from
MISSING (no symbol at all) and from the near-100% classes (real code, wrong
details), and it was not being tracked at all until now.

Started at **74 functions, 37640 bytes, across 20 units**; down to **52
functions, 32116 bytes, across 15 units** after the first wave and the
24d388c4 merge. These are among the cheapest work left: the target
disassembly is complete and readable, and because the symbol already exists
and is already correctly placed in `.text`, you are filling in a body rather
than deciding where a definition goes.

Current state — re-run `python tools/stubs.py` rather than trusting this table:

| unit | stubs | bytes |
|---|---|---|
| `zNPCFXCinematic` | 20 | 8200 |
| `xFX` | 10 | 8512 |
| `zEntPlayer` | 6 | 3704 |
| `xCollide` | 3 | 200 |
| `zMain`, `xParCmd` | 2 each | 2136 / 1112 |
| `zLasso`, `xCutscene`, `zNPCTypeKingJelly`, `xSnd`, `xLaserBolt`, `iFX`, `xParEmitterType`, `xHudFontMeter`, `xHud` | 1 each | 4048 / 2444 / 648 / 352 / 340 / 164 / 108 / 88 / 60 |

Note the two singletons with outsized bodies: `zLasso_Render` at 4048 bytes
and `xCutscene_Render` at 2444 are each worth more code than most whole units
in the table.

**Look for a matched sibling before reading any stub's asm.** Several stubs are
near-copies of a function already written in the same file, and the sibling is
a far better starting point than the disassembly:

- `xQuickCullForRay`/`xQuickCullForBox` in `xCollide.cpp` are the same
  one-line forwarder as `xQuickCullForBound` in `xBound.cpp:399` —
  `xQuickCullForX(&xqc_def_ctrl, q, x)`. Both went straight to 100%.
- `xHudFontMeter::load` is the `init_base` + placement-new idiom shared by
  `xHudText`, `xHudModel` and `xHudUnitMeter`. Its comment claimed it was
  stubbed because the real body "caused a build failure" — the actual cause
  was that the definition named its third parameter `size_t` (of type `u32`),
  shadowing the type so `new` would not parse. Renaming it fixed the build and
  the function matched 100% immediately. **Treat "this does not compile"
  comments as unverified.**
- `xParCmd_AlphaInOut_Update` is `xParCmd_SizeInOut_Update` with `custAlpha`
  for `custSize`, writing `p->m_cfl[3]` and then `p->m_c[3] = (U8)p->m_cfl[3]`.
  0.990% -> 87.525% from the sibling alone.

`xParCmd_SizeInOut_Update` (90.542%) and `_AlphaInOut_Update` (87.525%) now
share the same two residuals, both already flagged in the SizeInOut source: the
clamp is not the `CLAMP` macro, and `0.33333334f` is cached before the loop
rather than reloaded. Crack that idiom once and both functions move —
`xParCmd_Shaper_Update` (708b, still a stub) reuses it a third time.

Worked so far: the `init_sound`/`play_sound`/`kill_sound` family across
Plankton, SB2 and Prawn (the same idiom in three files — crack one and the
rest follow), Ambient's Jelly/Neptune group, and `xFont`'s four `parse_tag`
colour channels.

Still open and worth taking in one pass because the members are siblings:
`xFont`'s remaining group, `zNPCFXCinematic` (20 stubs, and note
`zNPCB_SB2::singleton` and `zNPCFXCutscenePickTable` become available as a
by-product — `g_cutmap` does not exist in our source at all), and `xSnd`'s
entire fader subsystem (`xSndStopFade` is a stub; `update_faders`,
`fade_data::operator=` and `xSndPlay3DFade` are all at 0%, and there is no
`faders[]` array in the source — it needs `fade_data` moved above line 130).

Two cautions learned filling these:

- A filled body can emit new anonymous literals that shift *other* functions.
  Implementing `zNPCNeptune::AnimPick` in place added two `@12` `.rodata`
  entries ahead of `PlayWithAnimSpd`'s array and knocked it off 100%. Moving
  the definition to the target's position fixed it. Run `symorder.py` after
  every stub.
- Filling a stub in a unit whose pool is already misaligned gets you
  instruction-identical code that still does not reach 100% under `solo.py`.
  All four `xFont` `parse_tag` functions are in that state, as is the
  pre-existing correct `parse_tag_yspace`.

## xFX / tier_queue

`xFX` went 109 -> 80 non-matching. `tier_queue`, `tier_queue_allocator` and
the ribbon update/insert path are written; what remains there is the render
side (`render`, `render_strip`, `eval_joint`, `refresh_joint`, `get_normal`)
plus `init`, `set_texture` and `xFXRibbonSceneEnter`.

Type signatures carry real information here:

- Container index parameters are **`u32` (`unsigned long`)**, not `U32`
  (`unsigned int`). The mangling says so: `CFUl` vs `CFUi`. Getting this wrong
  costs every caller a mismatched `bl` target.
- `tier_queue::empty` reads `_size` directly; going through `size()` costs a
  call. `static_queue::empty` *does* go through `size()` - the two containers
  genuinely differ, so do not assume symmetry.
- `xFXRibbon::need_update` returns `bool` (caller tests with `clrlwi.`);
  `render_compare` returns `S32` (caller tests with `cmpwi`). One instruction
  in the *caller* is the only tell.

## Header-declared helpers are free wins

A whole class of missing functions are small helpers the headers *declare* but
nobody ever defined - `xVec3::create`, unary `operator-`, `safe_normal`,
`up_normal`, `xVec2::create`/`operator*`/`operator+=`/`operator*=`. In the
target they are weak per-TU symbols, so defining them inline in the header
makes them appear everywhere they are used at once. Cheap, and it also removes
unresolved externs.

**`xVec2::create` is now diagnosed exactly** (2026-08-12, second independent
derivation). Mapping every `.sbss2` object in the `zNPCTypeDutchman` target to
its referencing function pins the one we are missing: `@512` at `+0x000`, an
8-byte all-zero template owned by `create__5xVec2Fff`. The target body loads that
template, stores it to the frame, overwrites both words with `f1`/`f2`, and
reloads — so `src/SB/Core/x/xMath2.h:75` should read `xVec2 v = { 0.0f, 0.0f };`,
not `xVec2 v;`. Ours sits at **63.636%**, and target `.sbss2` is 19 objects
(0x98) against our 18 (0x90). `create__5xVec2Fff` is emitted in exactly one
target object project-wide, so the *symbol* blast radius is one unit — but the
parse-time `.sbss2` template risk is header-wide and must be swept.

Related, from the same sweep: only 31 units have `.sbss2` at all and 14 disagree
in size — short by 8 in `xCutscene, xScene, rpptank, zScene, zNPCTypePrawn,
zNPCTypeBossPlankton, zNPCTypeDutchman`, short by 4 in `iTRC, zDispatcher,
zEntPlayerBungeeState, zEntPlayerOOBState, zMenu, zGame, zMain`, long by 8 in
`zAssetTypes, zNPCTypeSubBoss, zNPCTypeCommon, zNPCGoals`. Note `zDispatcher` is
now 4 bytes **short**, not long — the sign flipped after the `xGrid.h` fix
(`134129c2`), so the `ZDSP_elcb_event` note in Open leads is stale.

Still unwritten in this class, worth doing next: `basic_rect<F32>` accessors,
`xSCurve`, `xQuickCullForSphere`, and `auto_tweak::load_param<T1,T2>` (that
last one is a template with per-type specialisations - `<f,f>` calls
`zParamGetFloat`, `<i,i>` calls `zParamGetInt`, `<xVec3,i>` calls
`zParamGetVector` - so it needs explicit specialisations, not one body).

## Running several agents in parallel

Naive parallelism does not work here: every agent running `ninja` in one
checkout clobbers the others' objects and `report.json`. `tools/solo.py`
removes the contention - it compiles a single unit into a private temp directory with the
exact flags from `build.ninja` and diffs that against the target object, ~2s,
writing no shared state. So N agents can share one checkout as long as:

- each owns a different `.cpp` (+ its own `.h`),
- nobody runs `ninja`, `configure.py`, or touches git,
- nobody edits a shared header - they report the change they want instead.

The integrating session runs the real build once at the end.

**Dispatch about two at a time, not eight.** Eight concurrent agents burned
through the session usage limit and all eight died mid-edit at the same
moment, leaving eight half-written units and nothing verified. They resume
cleanly from their transcripts, so it is recoverable, but a stalled fleet
still costs an integration cycle. Two at a time finishes the same work with
the failure surface of two.

An agent that dies mid-edit has left a file that may not compile. Resume it
with an explicit instruction to run `tools/solo.py` on its units and fix any
compile error **before** writing anything new.

First run of this, six agents on six units, +138 functions verified by a
clean build:

| unit | non-matching before -> after |
|---|---|
| `zNPCTypeBossPlankton` | 105 -> 58 |
| `zNPCTypeKingJelly` | 93 -> 56 |
| `zNPCFXCinematic` | 72 -> 51 |
| `xShadow` | 28 -> 18 |
| `zNPCTypeBossSB2` | 101 -> 94 |
| `zEntPlayerBungeeState` | 70 -> 69 |

Two of the six ran out of session mid-edit and left work that did not
compile; both were salvageable after a few minutes of repair, so a dead
agent is worth triaging rather than reverting. **Do not trust an agent's
own claim that a unit is clean** - re-run `tools/solo.py` and `clang-format -n
--Werror` on its files yourself before committing. One agent reported no
new formatting violations when it had introduced one.

Things committed from that run that are guesses, not evidence, and should
be revisited if they ever block something:

- `enum en_npcburst` in `zNPCFXCinematic` is **fabricated**. The mangled
  name of the function forces *an* enum of that name; the enumerators are
  invented. No such enum exists anywhere in the repo.
- `sphere_hits_sphere_xz` returns bare `4`/`2`/`1`; there was surely an
  enum.
- `SysEvent` uses `switch ((S32)toEvent)` purely as a shape hack.
- `ShadowLight` / `ShadowCamera` / `ShadowCameraRaster` are declared
  `volatile` as a matching device, not because they are.
- `zAnimListInit` reads `nals` through `*(volatile S32*)&nals` as a matching
  device. `nals` is not volatile in retail — the target simply reloads it after
  the store (the 2b defect). The cast buys fuzzy 98.581 -> 99.488 and **zero**
  matched functions, because volatile then forbids CSE-ing the reload into the
  following `slwi`, so the two residuals are mutually exclusive from source.
  Kept only because removing it costs fuzzy for nothing; it is not evidence.
- The bone index `21` in Plankton's `aim_gun` is a literal.

Note `tools/solo.py` parses `build.ninja`, which has two rule layouts: the source
file is on the same line as the `build` statement when it fits, on the next
line when it does not. The parser handles both; if you extend it, keep that.

## dwarf/ — the resource this project has been under-using

`dwarf/` is **already tracked in the repo**: 231 files, 32 MB, essentially 1:1
with the source tree (110 game `.cpp` against 110 in `src/SB/Game`, 89 against
88 in `src/SB/Core/x`). It is DWARF-derived source from the **PS2** build, and
until now it was referenced in this document exactly once, in passing, about
rwsdk headers. It should be the second thing you open after the asm diff.

For every function it gives the full signature, every local by name, and the
register or stack slot each one occupied:

```
void CollideReview(class zNPCRobot * this /* r21 */) {
        class zNPCGoalCommon * goal;   // r2
        signed int goaldidit;          // r16
        class xEntCollis * npccol;     // r20
        class xVec3 vec_depen;         // r29+0x90
        float goodep;                  // r29+0x9C
```

Three things transfer usefully to the GameCube build:

- **Local declaration order**, which controls stack layout under CW as much as
  under MW MIPS. Reordering three locals in `NPCMessage` from the DWARF order
  took it 99.016% -> **100%**, and the same trick fixed `CornerOfArena` and
  `zNPCSlick::AnimPick`.
- **Signatures for functions declared nowhere in our tree.** This is the big
  one: the zNPCTypeRobot pass stalled on 16 member functions with no
  declaration anywhere, and DWARF has most of them (`VFXStarTrek`,
  `NightLightUVStep`, `SnoreNZeez` matched the asm-derived signatures exactly).
  Recovering a header declaration from DWARF beats guessing it.
- **Local *names***, which make a reconstructed body readable and reviewable
  instead of `iVar1`/`uVar5`.

**A DWARF definition omits parameters that were unnamed or unused, and this
will silently corrupt a rename pass.** The same file carries both forms — the
full declaration near the top, and the definition further down with only the
named parameters:

```
line   12  signed int zGustEventCB(class xBase *, class xBase *, unsigned int, float *, class xBase *);
line 1418  signed int zGustEventCB(class xBase * to /* r2 */, unsigned int toEvent /* r2 */) {
```

Our signature is `(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
xBase* b)` and the **declaration agrees with it exactly**. Match names against
the *definition* positionally and you rename our `from` to `to` and our `to` to
`toEvent`. Nothing downstream catches it: renames are byte-neutral, so
`solo.py` stays identical and the DOL still hashes. **Always resolve a
short parameter list against the declaration, never against the definition.**
`tools/dwarfaudit.py` now refuses to compare across an arity mismatch and
reports those functions under `ARITY` instead.

The corollary is that most apparent "signature divergences" are not divergences
at all — they are unused parameters being dropped. `RepelBowlBall` and
`ConeOfRange` showing no parameters is this effect, not a PS2/GC difference.

**Register numbers are MIPS and do not transfer at all.** Genuine PS2/GC
divergence does exist, so the asm still overrides DWARF — but check the
declaration before concluding you have found one.

### Reordering locals to dwarf order: tried, mostly does not work

Worth recording as a negative result so nobody re-runs it. `tools/dwarforder.py`
ranks candidates; two agent waves worked **39 functions** between them.

Yield: **three real improvements** — `HAZ_Iterate` 98.636% -> 99.848%,
`zThrownCollide_ThrowFruit` 97.342% -> 97.650%, `TurnThemHeads` 96.584% ->
96.619% — plus about ten reorders kept at zero cost because they now match the
original order without changing codegen. Everything else reverted or was
unachievable, and several regressed hard: `ZNPC_AnimTable_Tubelet` 99.583 ->
86.042, `render_closeup` 99.868 -> 90.198, `NCIN_OilHazard` 96.923 -> 89.650.

Why it underperforms, in order of importance:

1. **Scope, not order, is the real blocker.** dwarf flattens block-scoped
   locals into one list per block, so reaching its order usually means hoisting
   variables out of the `if`/`for` blocks they live in. That is a code change,
   not a reorder, and it was the reason for most skips.
2. **A near-100% function is usually wrong for some other reason**, and
   disturbing a working stack layout makes it worse. Reordering is only
   plausible when stack layout is the *last* remaining defect.
3. **Statics are not declaration-ordered.** See below.

The premise is still sound, which is why the failures are worth understanding
rather than dismissing: of functions we already match byte-for-byte, **85%
already agree with dwarf's order** (367 of 433). dwarf order really is the
original source order. It just is not usually the thing standing between a
function and 100%.

### Function-scope statics are listed in DESCENDING address order

Not declaration order. `zEntCruiseBubble::init_states` is twelve statics
annotated `// @ 0x005CB880` and interleaved with compiler `@8149` init-guard
flags; sorted ascending by address they come out in exactly the order our
source already had. Reordering to dwarf's listing cost 99.161% -> 94.699%.

`tools/dwarforder.py` therefore matches only entries annotated with a register
or stack slot (`// r18`, `// r29+0x90`). That alone removed a third of the
worklist, 50 candidates down to 32 at >=95%.

Checking our static order against ascending-address order is *also* not a
lever: 70% already agree, and the disagreements are mostly an artifact of
comparing across sections (`.bss` at `0x50FExxx` versus `.data` at
`0x5CBxxxx`), where relative address says nothing about declaration order.

One case is worth knowing about because it looks like a missed win and is not.
Reordering `DoWallJumpCheck`'s three statics to dwarf's *descending* listing
flipped a neighbour, `PlayerCollCheckEnv`, to 100%. But ascending-address order
for those three is `sAtdist, sSweptrad, sVerticalCos`, which is already exactly
what our source has — so the reorder moved us *away* from the original source
order and the flip was coincidental. Almost certainly it compensated for a
`.bss` layout error elsewhere in the unit rather than fixing anything. Treat a
percentage win from a change you know to be less faithful as a symptom, not a
fix; the real bug is whatever made the compensation work.

**Best combination found so far**, and worth assembling up front for any future
bulk pass: Ghidra output for control flow and call graph, plus a dump of the
target's `.sdata2`/`.rodata` decoded as floats/RGBA to resolve `@NNNN` literals,
plus `dwarf/` for local names and declaration order. Ghidra alone gets to
roughly 85-90%; the last ten points came almost entirely from the other two.

## Bulk Ghidra: what it can and cannot do

Measured, not estimated. `tools/ghidra/DumpFuncs.java` plus the local
`analyzeHeadless` wrapper dumped **every game function currently at 0.000%
fuzzy — 597 unique names across 38 units, 278 KB of code — in 73 seconds, with
0 decompile failures and 0 not-found.** Extraction is emphatically not the
bottleneck. Per-unit corpora land in `scratchpad/ghidra/<unit>.c`.

**It is safe to attempt.** A unit containing a 0%-fuzzy function cannot be
byte-identical, so none of those 38 units are `Matching` — verified, 0 of 38.
Bulk-filling them **cannot break the DOL sha1**. The only exposure is pool
shifts knocking neighbours off 100% inside those same units, which `solo.py`
before/after catches.

**What the output is actually like**, over the 100-block zNPCTypeRobot sample:

| trait | share | meaning |
|---|---|---|
| decompile failures | 0% | control flow always recovered |
| `halt_baddata`, unrecovered jumptables | 0% | no dead ends |
| correct mangled callee names | ~all | the symbol-bearing ELF earning its keep |
| `undefined*` types present | 52% | needs retyping against our headers |
| raw offset derefs `*(int *)(p + 0x228)` | 30% | needs mapping to struct members |
| `goto`/`LAB_` | 5% | |
| bogus `undefined8 param_1..9` signature | 5% | C++ `this` misplaced by ABI misread |
| `extraout_*` artifacts | 1% | garbage, e.g. `param_1 = extraout_f1;` |

So: **the algorithm and call graph come out right; compilable C++ does not.**
Even the cleanest cases need work — `xMat3x3RMulVec` decompiles perfectly but as
`(float *param_1, float *param_2, float *param_3)` with `param_2[5]` indexing,
where we need `xMat3x3*` and named members. Effectively **0% compile as-is.**

The correct use is therefore **Ghidra output as agent input, not as committed
code.** Agents previously started from raw PPC asm; starting from recovered
control flow with real callee names is a large accelerant. It is not auto-fill,
and anything from it needs the same measurement discipline as hand-written code.

Two traps worth keeping:

**Never pass function names as command-line arguments on Windows.** Mangled C++
names can contain `<` and `>` — `xUtil_choose<i>__FPCiiPCf` is real and lives in
this project. cmd.exe reads them as redirection and the entire `analyzeHeadless`
invocation dies **silently, in 0.2 s, with no error output**. `DumpFuncs.java`
takes a list file for exactly this reason.

**Some symbols have several copies in the ELF.** 597 requested names produced
650 blocks. Duplicated symbols are annotated in the per-unit corpora; check the
address matches the unit before trusting a copy.

## Shared-header changes: the xSndPlay3D case

Settled, and the method generalises. The 9-arg `xSndPlay3D(const xVec3*, ...)`
is now `inline` in `zEnt.h`, with two TUs opting out via
`#define XSNDPLAY3D_OUT_OF_LINE` before the include. Result: **+12 exact
(zFX +10, zEntTeleportBox +2), nothing lost, `complete_units` unchanged.**

Getting there killed three assumptions worth writing down.

**1. objdiff being blind to definition order can hide a broken `Matching`
unit.** The naive change — mark it `inline`, delete the body from `zEnt.cpp` —
built a `main.dol` that failed its sha1. Not because of the unit I expected:
`zEnt.cpp` is `Matching` too, and deleting the out-of-line body *relocates the
symbol inside `zEnt.o`*. objdiff pairs symbols by name, so it still cheerfully
reported zEnt at 38/38 while the object was no longer byte-identical. **A unit
reading 100% in objdiff is not proof its object is byte-exact.** For `Matching`
units, only the DOL sha1 is proof.

**2. A `solo.py` gain can be worth exactly zero on the project metric.** That
same change measured net +7 exact and showed **+0 matched functions, −1
complete unit** in `report.json` — every one of the +7 was a 99.x% -> 100%
crossing that report.json already counted. Before trading a complete unit for
exactness, check whether report.json can even see the gain.

**3. `WEAK` is not a substitute for `inline` here.** Defining the body `WEAK`
in the header (the convention this codebase uses elsewhere in `zEnt.h`) emits a
weak copy into every TU that includes it: **net −217 exact, 340 functions out
of exact across 24 units.** Catastrophic. Do not reach for it.

**The pattern worth reusing:** retail inlined a given helper into some callers
and not others, so a single global choice is wrong either way. Define the
inline in the header by default and let the TUs that must not expand it opt out
with a macro. Finding which TUs those are is mechanical — snapshot every caller
with `snapshot.py`, apply the change, snapshot again, and read the drop list.

`XSNDPLAY3D_OUT_OF_LINE` is currently set by four TUs: `zEnt.cpp` (layout of a
`Matching` unit), `zEntDestructObj.cpp` (pool shift in a `Matching` unit),
`zLasso.cpp` and `zPlatform.cpp` (pool shift, both `NonMatching`).

**4. Comparing exact-match sets is not enough, and this cost real accuracy.**
The first version of `snapshot.py --cmp` only diffed which functions were at
100%. It reported the change as clean. It was not: the inline interned a `0.25f`
literal in `zLasso` and `zPlatform`, dropping their `.sdata2` match from
65.217% to 63.830% and 92.063% to 90.625%. No function crossed the 100%
boundary, so nothing showed up. A later agent working `zLasso` found it
independently and applied the opt-out; `zPlatform` was only caught by re-running
the comparison over the **full percentage distribution**. `snapshot.py --cmp`
now always prints an `ANY DROP` section covering sub-100% functions and data
symbols. Read it.

`snapshot.py <out.json> <src>...` / `snapshot.py --cmp <before> <after>`
(scratch) does that sweep: compiles each unit privately, records every symbol's
percentage, and diffs two snapshots into per-unit GAIN/LOST lists with a net.
It replaces the old `shadowhdr.py`, which could not measure any TU including
`<new.h>` because `-cwd explicit` breaks that include chain.

## Merging upstream

This branch tracks `bfbbdecomp/main` but never merges back. Upstream keeps
decompiling the same functions we do, so most conflicts are two independent
implementations of one function — not something a 3-way merge can judge.

**Resolve by measurement, not by reading.** Extract both sides and compare
exact-match counts:

```
git show :2:<path> > ours.cpp     # stage 2 = HEAD (ours)
git show :3:<path> > theirs.cpp   # stage 3 = upstream (theirs)
python candidate.py <unit-frag> <path> ours.cpp theirs.cpp
```

The "exact only in X" lists are the part that matters. A side can lose 10-for-1
overall and still hold the one function the other lacks — take the bulk winner,
then port the individual wins across.

From the 24d388c4 merge, all seven conflicts:

| file | resolution | evidence |
|---|---|---|
| `xShadow.cpp` | ours | ours 46 exact, theirs 28, **zero** exact-only-in-theirs |
| `zFX.cpp` | ours + 1 port | ours 63, theirs 53; theirs held `zFX_SpawnBubbleTrail` |
| `zEntTeleportBox.cpp` | ours | dead tie 32/32, nothing exact-only either way |
| `xHudFontMeter.cpp` | ours | their only hunk (a `const`) already in ours |
| `zNPCTypeRobot.cpp` | ours | their only hunk (a float literal) already in ours |
| `xShadow.h` | ours | cosmetic param rename, ours a superset |
| `zNPCHazard.h` | **both** | their `const` + our added declaration |

Two traps worth knowing:

**A one-hunk upstream diff can produce a huge conflict.** `zNPCTypeRobot.cpp`
showed ~20 conflicting lines for what was a single changed float literal. We
had relocated `zNPCSleepy_Timestep` for definition-order matching, and the
merge could not align the moved block. Always diff `<merge-base>..upstream` for
the file before judging the conflict — the real change is usually tiny, and
often something we already have.

**Upstream's side can carry code that is dead in ours.** Their
`zNPCTypeRobot.cpp` hunk included `extern char stringBase[];`, live upstream
but dead here because we replaced every `stringBase + 0xNN` with real string
literals. Check whether a symbol is actually referenced before preserving it.

The one genuine win in that merge was a bug: our 2-arg
`zFX_SpawnBubbleTrail` passed `&bubblehit_pos_rnd` / `&bubblehit_vel_rnd`,
copy-pasted from `zFX_SpawnBubbleHit` directly above it. Upstream had the
correct `&bubbletrail_*`. Worth reading their version of anything we already
"finished" — they catch things.

## Priority: game code first

`src/SB/**` comes before library code (rwsdk, MSL, Dolphin SDK, bink,
MetroTRK), even though library code is in scope on this branch. Raw function
count makes rwsdk look like the biggest lever — it is not what makes the
decompilation worth anything. Library units are a fallback for when the game
units are saturated, or when one cheap enabling fix unblocks a whole bucket.

## rwsdk — 1039 functions, now reachable

Nothing here was imported from anywhere. `include/rwsdk/*.h` are upstream
files reconstructed from the BFBB PS2 DWARF data, and `configure.py` already
listed all 120 rwsdk units with `objdiff.json` targets. They were simply never
built: 118 of the 120 `.c` files did not exist, and with no source there is no
build rule, so `solo.py` could not touch them.

Two things were in the way, both now fixed:

- The headers were written for the C++ TUs that also include them — bare tag
  names used as types, `typedef struct X;` with no declarator, an empty
  struct, a member `operator=`, and quoted includes that only resolve with
  `-i include/rwsdk`. But the rwsdk objects compile `-lang=c`: their target
  symbols are unmangled. Every tag now has a self-typedef (structs/unions in a
  block near the top, enums immediately after their definition — C has no
  incomplete enum type), and the C++-only pieces are behind `__cplusplus`.
- Empty stub `.c` files exist for the other 118 units so `configure.py` emits
  their rules.

Verified with a compile of all 343 units the build knows about: 0 failures.

`ctbsp` is the worked example: 8 non-matching -> 4 in one pass, straight from
`gh.sh` output read against `rpcollbsptree.h`. The rwsdk headers are good
enough that struct offsets mostly just line up.

## Lead: the `$localstaticN$` counter says zEntPlayer's TU is missing an entity

The target names the Chuck offset vector
`offsetChuck$localstatic4$get_reticle_bound__FR5xVec3Rf`; ours is
`$localstatic3$`. CW's `$localstaticN$` counter runs over function-scope
statics in inline/instantiated functions in the TU, so **retail's translation
unit has one more such static before that point than ours** -- most likely
inside an inlined function pulled in from a header, since none of our own
file-local statics before that point use the `localstatic` mangling.

objdiff scores those rows as identical (it pairs relocations by offset), so
it costs nothing today. But it is a genuine missing entity in the TU, it is
the same family as the `.rodata`-ordering problem, and unlike that one it
names a specific counter you can check.

## Queued: dwarf identifier-name recovery for zThrown

Byte-neutral (locals do not affect codegen), recovered from
`dwarf/SB/Game/zThrown.cpp`, not yet applied. Worth a dedicated pass:

    zThrown_Update:  killIt->removethis, bound->oldbound, pos->oldpos,
                     delta->stackDelta, dir->velunit, oldGravity->oldgrav,
                     stackTgt-block d->posdot, sws t->lerp, lim->lerpdist,
                     reflection-loop d->dothdng,
                     hx/hy/hz->boxX/boxYupper/boxZ
    ThrowFruit:      idx->collfound, speed->velmag, pct->lerp

Caveat recorded with them: dwarf OMITS about ten float locals in
`zThrown_Update` (all of `px..tz`, `nx/nz`, `r`, `center`) that the asm proves
must exist. So for this unit dwarf is name evidence only, and is NOT a
completeness oracle -- consistent with it having pointed the wrong way five
times this week.

## Queued shared-header changes

Agents may not edit shared headers, so they report them instead. Outstanding:

- ~~**`zEnt.h` — the 9-arg `xSndPlay3D`.**~~ **DONE**, all 27 callers measured.
  Landed as an `inline` in the header with `XSNDPLAY3D_OUT_OF_LINE` opt-outs in
  `zEnt.cpp` and `zEntDestructObj.cpp`: +12 exact, nothing lost, no unit
  flipped. See "Shared-header changes: the xSndPlay3D case" above — the
  original -1-complete-unit framing was wrong in both directions, and the three
  assumptions it broke are the reusable part.
- **`xDebug.h` — no `xVec3*` overload of `xDebugAddTweak`.** There are `F32*`,
  `S16*`, `U8*` and `const char*` ones. `zNPCTypePrawn.cpp` currently carries
  a file-scope declaration instead, which is the same workaround Dutchman
  already uses. The header version is **unvalidated** — nobody has measured
  the collateral on everything that includes `xDebug.h`.

- ~~**`containers.h` — three container fixes.**~~ **ALL DONE.** The first two
  landed in `65afa1ba` on 2026-08-04 and sat in this queue for a week after
  they had shipped; `operator-=` and the wrap-`size()` both measure 100% today.
  Note the size() one was **`fixed_queue`, not `static_queue`** —
  `static_queue::size()` is an 8-byte `lwz`/`blr` and was never the subject.

  **The third was landed on a misreading, and the misreading is the lesson.**
  The evidence recorded here said the target's `add r4, r6, r4` "reuses the
  register already holding `it._it`". It does not: for
  `erase(const iterator&, const iterator&)` the argument mapping is
  `r3=this, r4=&it, r5=&other`, so `lwz r6, 0x0(r3)` loads `this->_first`
  (offset 0), and the `add` sums **`_first`**, not `it._it`. `65afa1ba`
  rewrote the source to `it._it` on that basis and recorded "no measured
  gain" — the two spellings are value-equivalent inside the
  `it._it == _first` branch, so nothing caught it. Corrected to `_first` and
  written with **one** temp rather than two (the target keeps exactly one
  value live across the `stw`), `erase` is now **100%** in all three
  instantiations, 97.759% → 100%.

  Two habits follow. **Re-verify a queued item against the tree before
  working it** — half this queue had already shipped. And **"no measured
  gain" on a change made for a stated reason means the reason is probably
  wrong**, not that the change is free.
- **`xSnd.h` — declare `xSndPlay3DFade`.** Two units declare it locally. The
  signature is forced by the mangled name
  `xSndPlay3DFade__FUiffUiUiPC5xVec3ff14sound_categoryff`, though the meaning
  of the final two `F32` parameters is still a guess.
- **`zMovePoint.h` — add inline `RadiusArena()`, `NodeByIndex(S32)` and define
  `NumNodes()`.** The target emits all three out of line into
  `zNPCTypeRobot.o`. There is a stopgap `inline zMovePoint::NumNodes` sitting
  in `zNPCTypeRobot.h` that belongs here.
- **`zNPCHazard.h` — `UVAModelInfo::Valid` should be `const` and defined
  inline** (`return model && uv;`). The target symbol is
  `Valid__12UVAModelInfoCFv`, emitted per-TU; it is currently declared and
  defined nowhere, which is a live unresolved external.
- **`xShadow.h` — declare `gShadowObjectRadius`,
  `xShadowVertical_FillCache`, `xShadowVertical_DrawCache`,
  `xShadowReceiveShadowSetup` and `xShadowReceiveShadow`.** All are defined in
  `xShadow.cpp` and declared nowhere; `zNPCSupplement.cpp` carries a local
  prototype block as a workaround. Note `xShadowReceiveShadowSetup` must be
  declared returning **`U32`** — `xShadow.cpp` defines it `S32`, but the caller
  emits `cmplwi`, and the unsigned declaration is what took
  `NPCC_RenderProjTexture` to 100%.
- **`zNPCGoalStd.h` — `zNPCGoalAttackMonsoon::SpitCloud` takes `F32 dt`**
  (`SpitCloud__21zNPCGoalAttackMonsoonFf`), and
  `zNPCGoalAttackHammer` is missing `ShockwaveTests(xVec3*, F32)` and
  `FXStreakUpdate(xVec3*)` entirely. Four fully decoded functions are waiting
  on these three declarations.

### Rejected

- ~~**`containers.h` — `tier_queue<T>::wrap_block` returning `u32`.**~~
  **LANDED** once `xFX` was rewritten. The original evidence was right (a
  `U8`-returning member forces `clrlwi r3,r3,24` at every call site, and the
  target's own `wrap_block` is `clrlwi r3,r4,24; blr`, i.e. a `u32` return
  with a `(U8)` truncation in the body), but when first measured it cost a
  different `xFX` function for net -1, so it was held. After the 17 absent
  bodies landed, that conflicting function no longer exists: re-swept over
  the 75 TUs, it is 1 improved / 0 regressed, taking
  `tier_queue<joint_data>::clear` 97.273% → 100%.

  The rule it was filed under still stands, and so does its converse:
  **measure every requested header change on its own before believing it —
  and re-measure a rejected one after the unit around it changes.** A
  rejection is a measurement of a tree, not a fact about the source.

## Open leads

- **What actually creates the eleven ghost `.rodata` templates.** Four 12-byte
  and seven 40-byte all-zero anonymous objects open the `.rodata` of at least
  `zVar` and `xParEmitterType`, referenced by nothing in either object, and
  they carry the *same* anonymous index range (`_617`–`_623`) in both. That
  shared range is the clue: a single header almost certainly emits them into
  every TU that includes it, which is exactly the parse-time aggregate-
  initialiser mechanism written up under "Settled". Upstream never found the
  cause and worked around it with `__deadstripped_*` in 15 files; the
  workaround is now sanctioned (see the skill), but finding the header would
  let all fifteen be deleted and would probably unblock units nobody has
  connected to this yet. Look for a header with eleven aggregate initialisers
  in `inline` bodies, sized 12 and 40 bytes.

- **The reload-after-aliasing-store defect now has seven witnesses in one
  unit.** Retail's `mwcceppc` **reloads** a value after an intervening store
  that could alias it, where this branch's compiler keeps the cached copy. In
  `xFX` alone this is the sole blocker for six of the nine remaining sub-90%
  functions, and it is the same family as the alias oracle at `0x511fc0` (see
  `project_float_meme_root_cause`), not the scheduler patch:

  - `xFXRingCreate` (89.983%) — retail reloads `1.0f` (`@958`) once per
    `*= 1.0f / lifetime`, because each `stfs` into the ring invalidates the
    `.sdata2` load. The pre-existing `// non-matching: 1.0f is only loaded
    once` comment turns out to be exactly this.
  - `activate_ribbon` (45.000%) — retail reloads `active_ribbons_size` after
    `active_ribbons[i] = ribbon`; we keep it in a register and sink the store.
  - `xFXAuraUpdate` (85.147%) — retail reloads `gFrameCount` in each of four
    unrolled iterations; we hoist it out.
  - `LightResetFrame`, `DrawRing`, `xFXShineRender`, `xFXStreakRender` — same
    shape, plus one base-address materialisation retail repeats and we CSE.

  This is a better-evidenced patch target than anything currently on the list:
  it is one rule, it is measurable in a single unit, and no source form reaches
  it short of `volatile`, which would be wrong (and was already rejected on
  `zNPCHazard::Discard` for emitting a load the target does not have). The
  natural companion case is `zNPCHazard::Discard`'s residual, which survives a
  control experiment.

- **Epilogue `lwz` swap** (`lwz r31` before `lwz r0`) — checked, it is the sole
  blocker for only 2 functions project-wide, so it is *not* worth a compiler
  patch.
- **Ghidra for the MISSING bucket.** 1483 functions have no implementation at
  all; `gh.sh` gives a usable starting point for each.
- **Five units at 100% still cannot be marked Matching.** Run
  `tools/symorder.py` on each for the specific reason.
  - `xDebug.cpp` — the blocker is `__as__10iColor_tag`, which xDebug **owns**,
    sitting one position too early in `.text`. Defining `__deadstripped_xDebug`
    is NOT a problem; it is what causes the ten weak inlines xDebug owns to be
    emitted at all. The trailing weak group comes out in reverse order of use
    inside that stub, so this is a statement-order question within it.
  - ~~`xParSys.cpp` emits four `operator=` instantiations the retail link
    dropped~~ — **fixed and linked, `98560c47`.** Those four were owned by
    other units and never mattered; the real blocker was `using_ptank_render`,
    which xParSys owns, emitted at end-of-TU because it was a header `inline`.
    See the rewritten 2c above for the general rule and `tools/symowner.py`.
  - `global_destructor_chain.c` and `__init_cpp_exceptions.cpp` put their
    `_reference` objects in `.sdata2`, where the target has them in `.ctors`
    and `.dtors`. The sources already carry
    `__declspec(section ".dtors")` and it is simply not being honoured.
    `#pragma section`, dropping `const`, and `-sdata 0 -sdata2 0` were all
    tried: the thresholds move the objects to `.rodata` instead of `.sdata2`
    but never to `.dtors`, and they also push `fragmentID` out of `.sdata`,
    so they are the wrong answer. `__init_cpp_exceptions` is 20 bytes of
    `.text` off besides.
  - `ptankgcntransforms.c` is missing `_rwConst`/`_rwConstants`/`_rwFifo`.

  Note that defining a function the retail link deadstripped is *not*
  automatically fatal -- `mem_funcs.c`, `FILE_POS.C`, `nubevent.c` and
  `float.c` all do and all link fine, so check the order before blaming the
  extra symbol.
- **`report.json` marks units complete that are not.** It scored `zSurface`
  28/28 while `solo.py` had `zSurfaceUpdate` at 99.733% and our object
  additionally emitted an out-of-line `xVec3::operator=` that the retail
  link deduplicated away. The link test caught it. This is the same class of
  artifact that blocks `xModel`: our object carries weak copies of
  `xAnimFileRawTime` and the `xMat3x3`/`xMat4x3` assignment operators which
  survive only in `xAnim.o` and `xCamera.o` in the retail link, so dtk's
  extracted object lacks both them and the `0.5f` literal one of them
  creates.
- **`xSFXUpdateEnvironmentalStreamSounds` — the source corrections are known
  and still do not help.** Five errors were proved against the target and the
  control flow was brought to an exact match, yet the best variant scored
  69.8% against a 72.883% baseline, so the file was left alone. The
  corrections, for whoever tries again: `break` -> `continue` on the
  `dist > cachedOuterDistSquared` test; `s_managedEnvSFX[0] = NULL` rather
  than assigning through `->id` (the target stores through the array's own
  address); `bestDist2[k] > dist` rather than `dist > *bestDist2`;
  `bestSFX[k] == NULL` rather than comparing the array address; and the tail
  calls `xSFXPlay(best)` on both paths. The blocker is that the target
  indexes all three arrays with a *variable* (`li`/`slwi`/`stwx`) while every
  source shape tried -- S32/U32/S8/register/const index, declared early or
  late, literal-load plus variable-store, volatile -- makes mwcc fold the
  index and hoist `&arr[k]` into callee-saved registers, costing ~10
  instructions and adding an `stmw` prologue. Eight variants measured, all
  65-70%.
- **An unreferenced 4-byte zero object in `.sbss2` blocks
  `ZDSP_elcb_event` at 99.984%.** One relocation index is off by one because
  our object carries an extra anonymous `@148` ahead of the `iColor_tag
  clear` constant. Its id is low, so it is created during header parsing, and
  it survives commenting out every removable `#include` -- it comes from the
  mandatory `zDispatcher.h`/`zGlobals.h` chain and cannot be reached from the
  .cpp. This is the same unsolved problem as the POOL bucket: what creates a
  literal before its first `.text` use.

## The playtest build is playable (2026-08-25)

It boots, plays the Bink logos, reaches the title screen, loads a level, and
plays. Music, voice lines, HUD counters and collectibles all work. Eleven
behavioural bugs were fixed on top of the link work, none of which moved the
matching DOL (still sha1 306526d9); Game Code went 79.2852% -> 79.3402% exact,
7195 -> 7200 functions.

Boot-blockers, in the order they were hit:

- `xAnim::_xAnimTableAddTransition` re-tokenized its source string forever
  (`xStrTokBuffer` restarts unless `string` is NULL) and used the animation
  table as the tokenizer scratch buffer. The first ATBL asset never returned,
  so the packer stopped issuing DVD reads 140 reads into boot.HIP.
- The same function never linked its transition lists into the states: three
  loops wrote through one `xAnimTransitionList*` without advancing it or
  storing it back, plus one deref too many.
- `isavegame`'s `cardwork` lost `ATTRIBUTE_ALIGN(32)`, so CARDMount DMA'd the
  directory and FAT into a buffer 8 bytes off and every card read as BROKEN.
- `oob_state::init` never set `shared.state`, so the first player update called
  through a null vtable.
- `xHud::for_each` passed the `xBase` header instead of the widget that follows
  it, so `zGameSetup` called vtable slot 6 of a header. This was the "freezes
  when loading gameplay" crash.
- `xQuatSlerp` blended through two null pointers.

Gameplay bugs after it booted:

- `XSER_get_client` corrupted the serializer's client registry three ways, so
  `zSceneLoad` read garbage for `sceneExist` and ran its *load* path over an
  uninitialised buffer on first entry to a level — `xBaseLoad` then disabled
  every entity. That is why no shiny object existed anywhere.
- `zMusicUpdate` only counted its delay down when it was already negative, so
  `zMusicDo` was never reached at all. Same condition had a `&`/`==`
  precedence bug.
- `meter_widget::set_value` stored the direction (+/-1) into `end_value`
  instead of the target, and `updater` dropped the `value_vel * dt` term — the
  two together are why every counter sat at 0, then at 1.
- `iAnimEvalSKB` lerped bone Y and Z from the *end* key (`b + t*(b-a)`), which
  is the vertical sway; its single-keyframe path also indexed `keys[i]` where
  the target strides two keys per bone.
- `zTalkBox::trigger_sound` returned out of the `ACTION_SET` case instead of
  falling through, so no character ever spoke.

The method that found all of these is written up in the
`feedback_behavioural_bug_hunting` memory: trace our own code with `OSReport`,
read the Dolphin log, and confirm against the target's disassembly. The fuzzy
percentage is not the detector — most of these live in functions scoring 87-99%,
while several sub-60% functions are semantically perfect.

### Open playtest issues (2026-08-25)

Reported while playing, not yet diagnosed. Method: see the
`feedback_behavioural_bug_hunting` memory -- trace with OSReport, confirm
against the target's disassembly, do not trust the fuzzy %.

1. **Patrick's throw-target reticle appears while playing as SpongeBob.**
   The user clarified 2026-08-25: the icon is the hand that shows where
   Patrick can throw a carried object, and it is showing up for SpongeBob
   when it should not. The asset is `target_reticle_hand`, loaded in
   `zEntPlayer.cpp` and drawn by `zEntPlayer_ReticleRender` off the global
   `gReticleTarget`.
   **Ruled out so far:** `zEntPlayer_ReticleRender` and
   `zEntPlayer_MinimalRender` are both 100%; `zEntRecurseModelInfo` (which
   builds `model_patrick`) is 100%; and `zEntPlayer_Update` has exactly the
   same 43 `gReticleTarget` and 9 `sTypeOfTarget` accesses as retail, with
   every difference in that region being register allocation. The target is
   only ever set inside `if (ent->model == globals.player.model_patrick)`
   (Patrick, types 2 and 3) or `== model_sandy` (lasso, type 1), so on a
   static reading SpongeBob cannot set it. `zThrown.cpp` is the only other
   TU that touches `gReticleTarget`, and it only clears it.
   **Next step is runtime, not static:** print `gCurrentPlayer`,
   `sTypeOfTarget`, and which of the three model pointers `ent->model`
   equals, from inside `zEntPlayer_ReticleRender` when it actually draws.
   A stale/dangling `gReticleTarget` surviving a scene change is the
   leading untested theory.

1b. **(superseded)** Original framing was "icon over breakable objects".
   **Ruled out: it is not an NPC glyph.** `zNPCGlyph`'s two sub-100 functions
   (`ScenePrepare` 96.87%, `Glyphs_RenderAll` 97.70%) are both register
   allocation only, every `NPCGlyph` method is already 100%, and retail
   references `GLYF_Acquire` from exactly the two objects we do --
   `zNPCGoalRobo` (DAZED) and `zNPCGoalVillager` (TALK/TALKOTHER). The
   NPC_GLYPH_SHINY* types are dead in retail too. Needs a screenshot to
   identify what it actually is before going further.
2. **Out-of-bounds detection is too lenient** (and the OOB hand with it).
   `oob_state` is the least-matching visual set on the player:
   `move_hand` 73.89%, `set_camera` 82.69%, `render_fade` 85.78%,
   `grab_state::start` 91.36%, `render_hand` 94.93%. Note the thresholds come
   from SB.INI (`player.state.out_of_bounds.*`, parsed in
   `oob_state::load_settings`), so check the parsed `fixed` values first --
   that is a data path, not code.
3. **Bubble Bowl cancels immediately, but only after taking damage.**
   Ruled out: every `Bbowl*` callback is 100% (`BbowlCheck`, `BbowlCB`,
   `BbowlWindupEndCheck`, `BbowlTossEndCB`, all the recover checks), and so is
   every damage entry point (`zEntPlayer_Damage` x2, `DamageNPCKnockBack`,
   `DamageKnockIntoAir`, `KnockToSafety`). `zEntPlayer_Damage` clears
   `IsBubbleBowling` but leaves `sShouldBubbleBowl` / `sBubbleBowlTimer` /
   `sBubbleBowlLastWindupTime`, which is also what retail does.
   **Prime suspect: `_xAnimTableAddTransition` (81.89%, 279 retail insns vs
   270 ours -- 9 genuinely missing).** It builds every animation transition
   list; the two bugs already fixed in it (commits 45ec670d, bad45518) were
   found this way. Since damage moves the player through a Hurt state, a wrong
   transition set on the state it returns to would show up exactly like this.
   Next step: log the anim state name each frame while attempting a bowl,
   before and after damage, and see which state it falls into and why.
4. **Music does not stop when it should -- tracks overlap.** Switching between
   the sliding-track music and the main track leaves both playing. `zMusicDo`
   does stop the previous voice on its own track
   (`if (sMusicTrack[track].snd_id != 0) xSndStop(...)`), so suspect either the
   *other* track (TRACK_COUNT is 2 and each is stopped independently) or the
   notify/queue layer choosing a different track for the new music.
   Sub-100 functions left in `zMusic` after this session's fixes:
   `zMusicNotifyEvent` 81.21%, `zMusicDo` ~86%, `zMusicSetVolume` 70.75%,
   `zMusicNotify` 98.06%. Note `zMusicUpdate`'s per-track gate --
   `(gGameMode == eGameMode_Game) == sMusicQueueData[i]->game_state` -- was
   only just corrected (55cd4099), so re-read the queue/track selection with
   that in mind. `zMusicKill` stops both tracks; check who calls it on scene
   and situation changes.
5. **Goo (water) does not appear in levels.** Start at `zFX`'s goo functions
   (`zFXGooEventMelt` 93.23%) and whatever renders the goo surface; also check
   the JSP/env path, since goo is level geometry rather than an entity.


6. **RoboSandy phase 3: hitting the damage area only flashes red.** Reported
   2026-08-25. The generic NPC damage tint fires but the boss-specific reaction
   that should accompany it does not. Note `zNPCTypeBossSandy` is already almost
   entirely 100% (only `Process` 99.36% and `Reset` 99.67% are below), so expect
   the defect in shared boss/damage/FX code or in something that file calls,
   not in `zNPCTypeBossSandy.cpp` itself. Likely shapes: an off-by-one or
   `>=`/`>` on the phase index, or a switch missing its third case falling
   through to just the tint.


### 2026-08-25: eight playtest bugs fixed, and a whole bug class we had been blind to

Game Code 79.3402% -> **79.4467%** exact, 99.01354% -> **99.04296%** fuzzy,
7200 -> **7204** functions. Matching DOL sha1 306526d9 unchanged.

Fixed this round (one commit each):

| bug | cause |
|---|---|
| goo invisible in every level | `zFXGooRenderAtomic` had the default render nested inside the one-time texture lookup, and that callback *replaces* the entity's own |
| music tracks overlapped | `zMusicDo` clobbered the voice handle with the asset hash, so every `xSndStop` in the game was a silent no-op |
| startup crash, invalid read 0x90010018 | `unit_meter_widget::model` declared `[2][6]`, indexed `[unit][which]`; retail's store stride proves `[6][2]`. Constructor also discarded every `load_model` result |
| out-of-bounds far too lenient | `out_state_type::update` tested `reset_time`, assigned from a constant on the line above, so `STATE_GRAB` was unreachable |
| RoboSandy phase 3 only flashed red | animation index array off by one, no `0` terminator, and `NoHeadHit01` missing -- the only anim with no transition into it |
| menu warp did nothing | `zUI_ScenePortalInit` inner loop wrote `i++` instead of `j++`, leaving every `task[1..7].portal.passet` NULL |
| (latent) camera tweak removal | `for (j = i; j < count; j--)` -- an unbounded downward memmove through MEM1. Fires on any `Camera_Tweak` Stop event |
| SpongeBob's bowl streak never fired | `zEntPlayer_StreakFX` hardcoded player index 1 and had the Bbowl/Stun branches swapped, plus an `&&`/`||` precedence bug |

**The lesson that outlives these fixes: `fuzzy_match_percent` only measures
`.text`.** `zNPCTypeBossSandy` reported 99.92% with the relevant function at a
clean 100% while the bug sat entirely in `.rodata`. Sweep
`report.json` `units[].sections[]` for non-`.text` sections under 100% -- there
were 45 project-wide. Diff the bytes *and* the relocations
(`llvm-readobj -r`, `.rela.<sec>`): **a missing relocation is a pointer that is
NULL for us and non-NULL in retail**, i.e. a callback never invoked. That is how
`static const tweak_callback cb = {};` was caught.

Two tooling traps worth remembering. `llvm-objdump` does not know the PowerPC
small-data relocation types and prints them as `Unknown`, so a differ matching
only `R_PPC\S+` silently drops every `r13`-relative global and shows it as an
anonymous `0(0)` -- precisely where wrong-global bugs live. And
`llvm-objdump -s` prints 4-digit addresses, not 8.

Ruled out, do not re-tread:
- **`GetCurrentH()` using `dMultiplier`/`dOffset` for height is retail's own
  bug**, faithfully reproduced (retail loads `dMultiplier__21@unnamed@zCamera_cpp@`).
  `hMultiplier`/`hOffset` are written and never read. Do not "fix" it.
- `move_hand` (73.89%), `xVec3::cross` (59%), `NPCS_SndTimersReset` (57%) and
  `bind_nodes` (36 insns vs retail's 76) are all semantically exact -- float
  scheduling and retail-side loop unrolling.
- xAnim is **not** the Bubble Bowl cause: the substitution path it fixes needs a
  `dest` containing `@` or `~`, and no caller in the tree passes one.

Still open:
1. **Patrick's throw reticle appears while playing as SpongeBob.** Every clear
   and set of `gReticleTarget` matches retail (same 43 accesses, same 9
   `sTypeOfTarget`), both render functions are 100%, and `zEntPlayerReset`
   clears it on scene reset. Static analysis is exhausted -- needs the runtime
   probe in `zEntPlayer_ReticleRender`.
2. **Bubble Bowl cancels immediately, only after taking damage.** All `Bbowl*`
   callbacks, `zEntPlayer_Damage` and the Hit checks are 100%; `HIT_STATES` is
   byte-identical to retail (63 entries) and includes all five Bbowl states;
   the charge loop in `zEntPlayer_Update` diffs clean. The live lead is
   `xAnimPlayChooseTransition`: `if (curr && curr->T->Conditional)` discards a
   state's **entire** transition list when the head node has no Conditional, so
   the list order built by `_xAnimTableAddTransition` decides whether the Hit
   transitions -- the only thing that clears `player_hit` -- are reachable at
   all. If `player_hit` sticks, every later bowl attempt gets yanked.

### Resuming after a compact

Branch state at 2026-08-25: working tree clean, matching DOL sha1 **306526d9**
unchanged, Game Code **79.3402% exact / 99.01354% fuzzy / 7200 functions**.
The playtest build is playable.

To build and run:

    python configure.py --non-matching && ninja
    python tools/playtest_iso.py "<retail iso>" "E:/ROMS/GC-WIi/Roms/BFBB-decomp-playtest.iso"

The retail disc is `SpongeBob SquarePants - Battle for Bikini Bottom.iso` under
`E:/ROMS/GC-WIi/Roms`. Dolphin is the one in `Downloads/Dolphin-x64`; launch it
with `--batch --exec=<playtest iso> --config Dolphin.Interface.UsePanicHandlers=False`
and read `%APPDATA%/Dolphin Emulator/Logs/dolphin.log`. Delete that log before
each run, and Stop-Process the old Dolphin *first* -- otherwise the delete races
the exit and two runs concatenate into one file, which has burned a cycle twice.

`python configure.py` (no flag) restores the matching build; verify sha1
306526d9 and the Game Code numbers before every commit.

Scratch tools worth rebuilding if lost (they lived in the session scratchpad):
a per-symbol disassembly differ with branch targets normalised, which is how
most of these bugs were localised, and the same thing for relocations, for when
instructions match but the score does not. Resolve float constants with
`llvm-objdump -s -j .sdata2 <retail .o>` plus the `@NNN` offsets from
`llvm-nm -S` -- guessing them sends you the wrong way (@688 was 0.25f, not 25.0f).

## Playtest round 2 (2026-08-25): ledge grab, and a filter for the rest

### The ledge-grab bug, and why nothing on the ledge path looked wrong

Reported symptom: ledge grabbing does not work at all. Every function on the
obvious path was already exact -- `LedgeGrabCheck`, `LedgeGrabCB`,
`LedgeFinishCB`, `PlayerLedgeInit`, `zLedgeAdjust`, `xSceneNearestFloorPoly`,
`gridNearestFloorCB`, `boxNearestFloorCB`, `sectorNearestFloorCB`,
`PlayerCollsDetect`, `xRayHitsSceneFlags`, and every anim-table entry that
wires the transition. `PlayerLedgeUpdate` sat at 96% but every one of its
diffs was register allocation.

The bug was one level below all of that, in `nearestFloorCB` (xScene.cpp) --
the triangle test every floor-poly callback funnels into, and a function
reachable *only* from ledge grab. It closes the triangle ring with

    xformVert[3] = xformVert[0];

so the edge loop `xformVert[i + 1] - xformVert[i]` for i = 0,1,2 walks
(0->1), (1->2), (2->0). That fourth slot is why the array is declared
`xVec3 xformVert[4]`. We wrote slot 1 instead, which destroys vertex 1, makes
the i=0 edge vector zero, trips `if (denom < 0.000001f) return collTriangle;`
on the first iteration, and bails on every triangle before touching
`nfpoly->neardist`. neardist stays FLOAT_MAX, so `xSceneNearestFloorPoly`
returns 0 and detection dies on its first pass every frame.

The lesson to carry: **a dead feature's bug is often not in the code named
after the feature.** Walk the call graph down to the leaves and check what is
reachable only from the broken path -- `xSceneNearestFloorPoly` had exactly
one caller, and its callee had exactly one purpose.

The stack offsets settled it without any guessing: `xformVert[0]` at 0x44 and
12-byte elements make slot 3 = 0x68, and the target's copy reads
`addi r3, r1, 0x68` against our `mr r3, r30` where r30 is `r1 + 0x50`.

### tools/semdiff.py -- separating real bugs from compiler noise

Most non-matching functions differ only in register allocation and
scheduling. Those are compiler-track: the source is right and the game plays
correctly. `semdiff.py` filters them out by comparing *multisets of
normalised instructions* -- registers, branch destinations and anonymous
`@NNN` pool ordinals erased, mnemonic plus every literal constant and memory
offset kept. Reordering cannot change a multiset; renaming cannot change a
normalised one. What survives is evidence about behaviour.

Over the 224 game-code units it flags 249 functions. Sort by term count and
work upward: the one-and-two-term cases are where the behavioural bugs are.
It reads objects `ninja` already built, so it is safe to run alongside other
agents.

Validated by reintroducing the ledge bug and confirming it reports exactly
the two terms, then confirming it goes silent with the fix in.

Known blind spot: two different float constants both normalise to
`lfs R, @P@sda21`, so a wrong .sdata2 value cancels out. The 0.7010677 /
0.70710677 bug in iCollide would not have been caught. That class needs a
section comparison.

### False-positive shapes that cost time, so recognise them on sight

Four idioms account for nearly every small-term hit. None is a bug.

1. **The MAX/MIN idiom.** Target `fcmpo; ble L; b M; L: fmr` against our
   `fcmpo; bge M; fmr`. `if (x <= c) x = c;` and `if (x < c) x = c;` differ
   only at equality, where the assignment is a no-op. `CalcCombinedDepen`
   looked like an inverted collision test and was not.

2. **Retail reloads, we cache.** This is the big one -- it explains around
   twenty functions whose only difference is `target only: lfs R,
   someStatic@sda21`. CodeWarrior at -O4 re-reads a global (or an array
   element) in each statement where retail's source names it, while our
   source hoists it into a local. Seen in `zEntPickup_Update`
   (sSpatulaGrabbedLife, sSpatulaGrabbedSpinMult), `zThrown_AddFruit`,
   `find_weight`, `zThrown_Update`, `zGameUpdateMode`, `zSceneSetup`,
   `zLOD_Update`, `zUIRenderAll`, `xCMupdate`, `zCameraTweakGlobal_Update`,
   `zParPTankSteamUpdate`, `zParPTankInit`. The matching fix is mechanical:
   write the global, then read it back rather than reusing the local.

3. **The store that gets overwritten.** Target stores an intermediate to the
   same stack slot a later store overwrites -- `life = a / C1; life *= C2;`
   against our `life = a / C1 * C2;`. `UpdateGustFX` is this.

4. **Addressing that reaches the same byte.** `addi 0x4b4` + `lwz 0x4(R)`
   against `lwz 0x4b8(R)`; `jsp_shadow_hack_textures + 0x14` against
   `animTable + 0` when the two are adjacent. Always check the addend before
   believing a relocation points somewhere else.

Also benign: `lmw/stmw` at a different offset (register save area size),
`clrlwi. R, R, 24` against `cmplwi R, 0x0` (U8 versus int for a flag),
`srwi R, R, 5` against `extrwi R, R, 8, 19` (bool versus U8), and any
`$NNNN` suffix difference on a static's name (DWARF numbering).

### The data sections are a separate, richer seam

`semdiff` only reads code. Comparing `.rela.data` / `.rela.rodata` /
`.rela.sdata` between the two objects as multisets of (symbol, addend) pairs
is what finds NULL dispatch tables and wrong table entries -- the class that
produced most of the playtest fixes. Three found this round:

- **zNPCTypeBossPatrick**: `newsfish_cb` and `recenter_cb` were `= {}`, so
  ten NULL function pointers each and two registered tweaks with nothing to
  call. Retail relocates `on_change` at +0x5c and +0x84, one tweak_callback
  (0x28) apart. Same shape as the zNPCTypeBossSandy fix. Data 44.87% -> 100%.
- **zNPCTypeKingJelly**: `sound_name[11][3]` holds up to three interchangeable
  takes per sound, and `play_sound_immediate` picks between them at random
  when `amount > 1`. Retail's twelfth relocation sits at 0x40 -- four bytes
  into the row at 0x3c, i.e. its second element -- so row 5 is
  `{ "KJ_Land1", "KJ_Land2", NULL }` and row 4 repeats `"KJ_grunt"`. We had
  eleven single-take rows, so the landing sound never varied and sound 4
  played the wrong clip. Data 47.65% -> 57.39%.
- **zEntPlayerOOBState**: `tutorial_callback` declared an empty
  `virtual void on_signal(U32) {}`, which emits a distinct function and puts
  it in the derived vtable where retail's holds the base version. Behaviour
  identical, data 19.57% -> 31.68%.

After those three the (symbol, addend) scan is clean across game code apart
from jump tables (whose entries shift with our code size), zGame's string
pool living in .sdata, and the two `.rela.debug` sections our build emits and
retail's does not -- exclude all of those or the signal drowns.

Sweeping for the all-NULL shape directly is worth doing after any new unit
lands: `grep -rn "= {};" src --include=*.cpp` on struct-of-function-pointer
types. It is currently empty.

### Still open on the ledge path

`nearestFloorCB` 95.581%, `PlayerLedgeUpdate` 96.175% and
`xClumpColl_ForAllIntersections` 96.236% all pass the multiset test, so they
are register allocation and scheduling only. Ledge grab is correct; those
three are compiler-track work.

## Playtest round 3 (2026-08-25): the opcode-family filter

`semdiff` finds *that* a function differs. Sorting its output by term count
was the wrong first move -- the small-term list is dominated by the four
benign shapes recorded above. Sorting by **which opcodes differ** is far
sharper, because a handful of opcode pairs cannot be produced by scheduling
or register allocation and always mean the source says something different.

Pair the target-only and ours-only terms that have *identical operands* and
differing mnemonics, then keep only pairs drawn from one of these families:

    arith      fmadds/fmsubs/fnmadds/fnmsubs, fadds/fsubs, fmuls/fdivs,
               add/subf
    precision  fadd/fadds, fsub/fsubs, fmul/fmuls, fdiv/fdivs
    signedness cmpw/cmplw, cmpwi/cmplwi, srawi/srwi
    width      lbz/lhz/lwz/lha, stb/sth/stw, lfs/lfd, stfs/stfd

Across the 224 game units this produced **two** arith hits, and both were
real bugs (xDecal's UV corner, xTRC's centring sign). Precision produced
xCM's double-vs-single arithmetic. Signedness and width produced xStricmp,
xCMcolor_scale and _xAnimTableAddTransition -- type-declaration errors rather
than behaviour, but all real. Nothing in the family scan was a false positive
in the sense of "the source is already right"; the only judgement needed was
whether a given type error could change an answer.

Run it first. It is a dozen candidates instead of two hundred.

### semdiff had a flaw that buried a real bug

It normalised anonymous `@NNN` pool ordinals but not the `$NNNN` suffix
CodeWarrior appends to function-local statics (`tb$731` against `tb$165`).
Any function touching a static drowned: xTRC's `RenderText` reported 30 terms
and `render_message` 50, both far down a list being read from the top. The
real content of RenderText's diff was two instructions. Stripping `$NNNN` in
*both* the resolved relocation name and the instruction text dropped it to 2
terms and cleared render_message entirely. Fixed; re-run anything triaged
before that.

### `lfd` from .sdata2 is usually not a double literal

This cost a wrong turn. `iCameraSetFogParams` showed target `lfd R, @P@sda21`
against our `lfs R, @P@sda21`, which reads as "retail's constant is a double"
-- so I wrote `time * 1000.0`. Wrong. The accompanying `lis R, 0x4330` is the
other half of the integer-to-float magic-number conversion, and the `lfd`
loads `0x4330000000000000`, not a program constant. The giveaways:

- `lis 0x4330` nearby -- always the conversion.
- `fsubs` (not `fsub`) after it -- converting *to float*, so the destination
  variable is F32.
- `lis 0x8000` / `xoris` -- the signed variant.

Reading it correctly is what found the actual expression, because the same
sequence also carried `lwz r, 0xf8(r)` and `srwi r, r, 2`, which is
`GET_BUS_FREQUENCY() / 4` spelled out.

### iTime is timebase ticks, and one caller forgot

`typedef S64 iTime`, `iTimeGet()` returns OSGetTime deltas, and
`iTimeDiffSec(t) = (F32)t / (GET_BUS_FREQUENCY() / 4)` -- so a second is
about 40.5 million ticks. `iCameraSetFogParams` built its blend window with
`(iTime)(time * 1000.0f)`, making a one-second fog transition 1000 ticks,
roughly 25 microseconds. `iCameraUpdateFog` divides by that window, so `dt`
saturated at 1.0 on the first frame and every fog transition snapped instead
of easing. Correct form, confirmed by the function reaching 100% exact:

    xglobals->fog_t1 = xglobals->fog_t0 + (iTime)(time * (GET_BUS_FREQUENCY() / 4));

Swept the rest: zGame, xFont and zSaveLoad all convert through
`GET_BUS_FREQUENCY() / 4` already. This was the only one.

### `beq / bge / b` to the same place means a switch, not an if

`stop_audio_effect` in zTalkBox had

    if ((shared.active) && (shared.active->asset->audio_effect != 1))

against retail's

    lbz   r0, 0x23(r3)
    cmpwi r0, 0x1
    beq   <call>        ; case 1
    bge   <exit>        ; default
    b     <exit>        ; case 0, empty

An `if (x == 1)` compiles to a single `bne`. Three branches off one compare
means a switch whose empty case and default land together, which is exactly
what `start_audio_effect` twenty lines up already looked like. Writing it as
that switch took the function to 100% and fixed the inversion: the music was
being un-ducked for the boxes that never ducked it, and left ducked forever
by the ones that did.

The switch also explains the signedness: a `bool : 8` bitfield promotes to
int in a switch (`cmpwi`), where `!= 1` compared it unsigned (`cmplwi`).

### Two more shapes for the false-positive list

5. **The magic-constant `lfd`** -- see above. Check for `lis 0x4330` before
   believing a `.sdata2` double.
6. **`frsp` on an already-single value.** Retail inserts `frsp` to model the
   rounding of storing through an F32 variable, where our source passes the
   expression directly. `update_turn` in Dutchman, Plankton and BossSB2 all
   show it against an `xatan2` result, which is already F32, so it is
   numerically a no-op.

Also confirmed benign this round: `bltlr`/`bgelr` and `bne`/`beq` pairs with
swapped targets (block layout -- `sound_queue::size`, `zEntHangable_UpdateFX`),
redundant rematerialisation of a constant inside a loop
(`iSndWaitForDeadSounds`), and `addi rX, base, 0x786c` + `lwz 0x18(rX)`
against `lwz 0x7884(base)` (`xtextbox::layout::yextent`).

### No calls are missing

Comparing `bl <symbol>` terms across all 224 game units: every mismatch is
*ours-only*, i.e. retail inlined a helper we call out to (`SQ`,
`ztextbox::deactivate`, `trigger_pads`, `pad_pressed`,
`unit_meter_asset::operator=`). There is no function retail calls that we do
not, so no behaviour is missing by that route. `xLine3VecDist2` was the one
place our `SQ` call had no counterpart at all; squaring in place took it to
100%.

### Two more filters that looked promising and are not

**Off-by-one branch pairs** (`blt`/`ble`, `bgt`/`bge` on the same operands,
excluding the inversions that block layout produces). One hit across the whole
build, `zShrapnel_BB03FloorInit`: target `cmpwi 0x3; blt` against our
`cmpwi 0x2; ble`. For integers `x < 3` and `x <= 2` are the same test, so this
is a spelling difference. The filter is cheap to run and worth keeping for the
float cases, but on integers it will mostly report this shape.

**`cror` presence.** `fcmpo` + `cror eq, gt, eq` + `beq` is a precise float
`>=`; a bare `bge` after `fcmpo` is the same test but also taken when
unordered. So ours-only `cror` means retail spelled a range check as the
positive strict form and we spelled it as negated exits:

    retail : if (x < hi && x > lo)  { ... }        -> bge exit; ble exit
    ours   : if (x >= hi) exit; if (x <= lo) exit; -> cror/beq; cror/beq

Identical for every non-NaN input. Seen in `zFrag_ProjectileSetupPath` (six),
`zShrapnel_DestructObjInit` (four), `zEntPlayer_Update` (three) and
`refresh_prompts`. Worth rewriting for match, but it is not a bug.

With those two ruled out and the opcode families clean, the automated seams on
*code* are close to exhausted for behavioural bugs. What is left is the
compiler track -- register allocation, block layout, the reload-versus-cache
pattern -- plus the data sections, which are a different tool.

## The data sections (2026-08-25)

`tools/datadiff.py` is the data-side counterpart to semdiff. It compares
.data/.rodata/.sdata/.sdata2 byte by byte and relocation by relocation, and
splits the relocation comparison three ways because each failure reads
differently: **count** (fewer entries than retail -> NULLs where pointers
belong), **positional** (same offset, different target -> wrong function or
string), **set** (same targets, different offsets -> declaration order).
`--consts` compares .sdata2 as a multiset of 4-byte words, which is semdiff's
blind spot: two different floats both normalise to `lfs R, @P@sda21`.

Run with no arguments it ranks every game unit by bytes missing.

### The metric is all-or-nothing per symbol

report.json scores a data symbol as matched only at exactly 100%. A 36KB
`[.data-0]` blob with one wrong pointer scores **zero**. That cuts both ways:
it makes the percentages alarming, and it makes small fixes enormous.
zCutsceneMgr went 62.29% -> 100.00% on seventeen bytes; xpkrsvc went
96.06% -> 100.00% on ninety-two.

So read the byte counts, not the percentages, and always check whether a unit
is *nearly* right before assuming it needs real work.

### objdiff tolerates trailing section padding

Retail's sections are padded to 8-byte boundaries; ours often are not. Every
one of the "ours is a strict prefix, missing 4 zero bytes" cases is this, and
none of them costs anything. zCutsceneMgr reached 100% while still one byte
shorter than retail. Do not chase them.

### What the remaining gap actually is

After this round: 56,024 bytes over 25 units. Essentially none of it is
*wrong* data. Comparing relocation (symbol, addend) multisets across all 224
game units, every remaining mismatch is either a jump table whose entries
point into a function at offsets that shift with our code size -- these fix
themselves when the function reaches 100% -- or DWARF `$NNNN` numbering. No
dispatch table is NULL, no pointer aims at the wrong thing.

What is left is **ordering**, in three flavours:

1. **vtable emission order** -- zNPCTypeRobot, zNPCGoalRobo, zNPCTypeKingJelly,
   zEntCruiseBubble, zEntPlayerOOBState, about 19.7KB between them. Retail's
   zNPCTypeRobot emits Slick, TubeSlave, TubeNotice, Tubelet, Chuck, ArfDog,
   ArfArf, Sleepy, Monsoon, Glove, TarTar, Hammer, Critter, Chomper, FodBzzt,
   FodBomb, Fodder, Robot, xPSYNote; ours emits xPSYNote first and then a
   different order entirely. It matches neither our source's order of first
   member definition nor anything else obvious -- worth an experiment, but the
   rule is not yet known.
2. **string pool order** -- .rodata byte differences where the same strings
   appear in a different sequence.
3. **constant pool order** -- .sdata2, same story with floats. zEntPlayer's
   568 differing bytes are this, which is why its 8232 bytes are the largest
   single gap and also one of the least tractable.

### Dead-stripped strings, and where to put them

Three units this round were missing strings outright, all residue of debug
functions the linker dropped. The tree's idiom is
`void __deadstripped_<unit>() { printf("..."); }`.

**Placement is the whole trick.** Strings intern in order of first appearance,
so the function has to sit where retail's dropped code sat:

- zCutsceneMgr's "FINISH EXIT...\n" is the *last* string, so the function goes
  at the end of the file.
- zLightning's six ("X to test lightning\n", fifteen spaces, "1", "0", "-",
  "\n") are also last.
- xpkrsvc's twelve (the en_LAYER_TYPE names plus "<unknown>") are *first*, so
  the function goes immediately after the includes. Appending them would have
  put them behind "%s %s %s %s" and shifted nothing into place.

Read run lengths off the hex dump rather than guessing: zLightning's space
string is fifteen, and sixteen put the terminator one byte late and pushed
every string after it.

### The nm placement scan

Comparing each symbol's nm letter between the two objects finds a class no
byte comparison does: `R`/`r` in the target against `D`/`d` in ours means
retail declared it const and we did not. `xTRC`'s `yellow` was this -- moving
it to .sdata2 put the whole pool into retail's order behind it and took the
unit to 100%.

Still outstanding in that shape: xColor's colour globals, xFont's
`text_delims`, zEntPlayer's `SBBBashBones`/`SBBBounceBones` (already written
`static const` yet still landing in .data -- placement is being driven by
something other than the declaration, and a fully-written-out initializer does
not change it), and anonymous entries in zFX, zScene, zPlatform and
zEntCruiseBubble.

Also found by that scan: `iPad`'s `sPadData`, a file-scope array that `-common
on` turned into a COMMON symbol so our object had no .data section at all.
`static` plus an explicit initializer put it where retail has it.

### Two genuinely wrong constants

`--consts` found both. zNPCSupplement called `xShadowVertical_FillCache` with
`0.087156497f` where the other three call sites in the tree -- and retail --
write `0.0871557f`, which is sin(5 degrees), the slope below which a surface
takes a vertical shadow. xCutscene's `0.03333333f` is one ULP below 1/30;
retail's word is exactly `1.0f / 30.0f`.

Still flagged and not guessed at: zNPCSupplement has one ULP on a 0.025f
streak frequency (retail 0x3cccccce against our 0x3ccccccd) that no natural
literal reproduces.

## The source-complete metric (2026-08-25)

`tools/srcprogress.py`. The exact metric conflates two states that call for
completely different work:

- our C says something different from retail's C -- **something to write**
- our C is right and only the scheduler or register allocator disagrees --
  **nothing to write**

Splitting them with semdiff's multiset test gives a second number:

    exact             7211 fns   1307160 bytes   79.59%
    codegen-only       250 fns    160460 bytes    9.77%
    ---------------------------------------------------
    SOURCE-COMPLETE   7461 fns   1467620 bytes   89.36%
    needs source       212 fns    174780 bytes   10.64%

    132 of 221 units are source-complete.

This reframes the project. The remaining *writing* is a tenth of the game,
not a fifth, and 60% of units have nothing left to write at all. Everything
else is the compiler track -- the alias patch, the scheduler, the register
allocator -- which is one research problem rather than four hundred small ones.

**It is an upper bound, not a proof.** The multiset test cannot see a wrong
float constant, because two different values both normalise to
`lfs R, @P@sda21` -- precisely where the iCollide 0.7010677 bug lived.
srcprogress cross-checks each unit against `datadiff --consts` and marks the
ones carrying a value difference with (!), where a codegen-only verdict is
weaker. Five today: zEntPlayer, zGame, xCM, zMain, xHudUnitMeter. The DOL
sha1 is still the only proof of anything.

### Where the remaining source work is

    48556  17 fns  zEntPlayer          <- 28% of everything left
     6280   2 fns  zNPCTypeBossSandy
     4732   4 fns  zUI
     4620   3 fns  zNPCHazard
     4216   2 fns  zThrown
     4048   1 fn   zLasso
     3892   1 fn   zCamera

zEntPlayer is seven times the next unit and three functions carry most of it:
`zEntPlayer_Update` (18188 b, 96.04%, 80 terms), `zEntPlayer_SNDInit`
(10160 b, 91.97%, **215 terms**) and `PlayerAbsControl` (5460 b, 97.56%,
17 terms). SNDInit's 215 terms in a 10KB function is the shape of a big
registration table with many wrong entries -- likely tractable and
gameplay-visible, since wrong entries there mean wrong sounds.

Also worth knowing: several multi-KB functions sit one or two terms from
clean -- `zThrown_Update` (3784 b, 99.79%, 1 term), `zSceneSetup` (3196 b,
99.62%, 1 term), `Process__10zNPCBSandy` (3852 b, 99.36%, 2 terms). All three
are the reload-versus-cache pattern, so they are cheap bytes if that pattern
turns out to be mechanically fixable.


### The constant behind the relocation

A float literal never appears in a PowerPC instruction. `x <= 0.001f` becomes an
`lfs` naming an anonymous pool symbol, and that ordinal is assigned in first-use
order per TU, so every differ normalises it away -- and never compares the four
bytes the instruction actually names. A function can be byte-identical in
`.text`, score 100.0%, and load a different number.

`datadiff --consts` does not cover it: it compares `.sdata2` as a multiset, so a
value present in *both* pools but attached to the wrong *site* cancels out. All
fifteen findings on the first sweep were exactly that.

`tools/pooldiff.py` resolves each pool reference and walks the streams in
lockstep. It found, among others, a swept-sphere epsilon whose sign was never
flipped for the second winding (`<= -1e-5f` for `<= 1e-5f`), so contacts on the
edges of clockwise triangles were rejected game-wide; a bungee `horizontal.sway`
default of 0.0f where retail passes 3.0f, with nothing in the shipped INI to
override it; and Patrick's spawn goal turning at PI/4 where every sibling call
passes PI/2.

Two traps. Only `lfs`/`lfd` count -- integer small-data refs name real globals,
and the `lis`/`addi` pairs that build a `.rodata` address name `@stringBase0`,
whose contents legitimately move when a string moves. And the section must come
from `objdump -t`, never `nm`: a pool ordinal is unique only *within* a section,
and nm's one-letter class cannot tell `.sdata2` from `.rodata`. Resolving
against the wrong one turned 2 findings into 1157.

Residual hole: 1-ULP values with no natural decimal spelling (zEntPlayer's
`0x3eaaaaa0` against our `0.333333f`, zNPCSupplement's `0x3cccccce` against
`0.025f`). mwcc's literal conversion matches Python's exactly, so these are not
misspellings, and at 3e-8 they are gameplay-irrelevant.

### zEntPlayer_Update is mostly compiler-track, not source-track

It is the single biggest item on `srcprogress --list` at 18188 bytes, and
zEntPlayer as a unit holds a third of all remaining source work in the game. But
the gap inside this function is not mostly ours.

Of 76 semantic terms, **22 are store-then-reload reloads** -- retail stores a
static and immediately loads it back, our mwcc forwards the stored value. There
are 18 such pairs (sReticleRot, sReticleAlpha, sBubbleBowlTimer, sTimeToRetarget,
sPlayerCollAdjust, sHackStuckTimer, stuck_timer, not_stuck_timer, inact_tmr,
bbash_end_tmr, sGooKnockedTimer, sCatchCapsuleTimer, sLastBubbleEmit,
sLastInvulnEmit, sPlayerSndSneakDelay, sHitchAngle) and the net instruction
deficit for the whole function is exactly 16. The same defect appears once more
on a stack slot: retail spills an `xsqrt` result to `456(1)` and reloads it, we
keep it in `f28`. That is the `lfs R, 0x1c8(R)` term.

Two things that look like findings and are not:

- `lis/addi ...bss.0` against retail's `sHackStuckDir` is a naming artifact.
  `...bss.0` is the section anchor and `sHackStuckDir` is the first object in
  `.bss`; both are `.bss+0` and link identically. `.bss` is the same size on both
  sides, and all 104 `.sbss` objects sit at identical offsets -- the 4-byte
  `.sbss` section delta is trailing alignment.
- the `zEntPlayer_Init` gust test reads zScene+244 on *both* sides. Counting
  accesses at 244 and 708 across every object in the game gives identical totals,
  which is what showed that the two sites had simply been exchanged.

Genuinely still source-shaped, and unsolved:

1. `for (U32 i = 0; i < sc->num_npcs; i++) {}` -- an empty placeholder loop.
   Retail emits an empty loop too, but keeps `lhz 0,14(r27)` *inside* the
   condition and carries a second dead induction variable stepping by 4, so it
   never unrolls. We hoist the trip count and mwcc unrolls by 8, costing ~12
   terms. **This is not the compiler patch**: stock 2.0p1 and patched 2.0p1a both
   produce the hoisted, unrolled shape (3 `bdnz` against retail's 1), so it is a
   source difference. Adding `xEnt* npc = sc->npcs[i];` as a dead body does not
   reproduce it -- the load is eliminated before strength reduction leaves a
   residue. The 4-stride induction variable is the clue worth chasing.
2. `if (sCatchCapsuleTimer == 0.0f)` -- retail materialises the boolean through
   `mfcr`/`xori 1`/`cntlzw`/`>>5` and then tests it, i.e. it computes the
   condition as a *value*. We compile a direct float branch. Writing it as
   `!(x != 0.0f)` does not help; mwcc folds it straight back.


### What srcprogress was counting, and three ways it lied

The tool now judges the reload class per TERM and ranks by unexplained terms
rather than bytes. Both changes were needed and neither was sufficient, because
the term count itself had two more distortions in it.

**Bytes.** A function's size says nothing about how much of it is wrong. Judging
the reload class per function meant one unexplained term threw every byte of an
18KB function into "needs source"; zEntPlayer_Update sat at the top of the list
on the strength of two source-shaped clusters while 25 of its 76 terms were the
compiler defect. Per-term, ranked by terms, it is sixth.

**Our own naming.** We spell a recovered local static `sStripVert_2188` where CW
emits `sStripVert$2188`. LOCALNUM erased the compiler's numbering but not ours,
and a symbol name rides on every instruction that references the variable, so
the mismatch counted once per reference. xFXShineRender reported 172 terms and
has 8. That was 245 phantom terms and the whole of xFX's first place.

**Loop unrolling.** A multiset cannot tell a duplicated loop body from new work.
Tridiag_Solve reports 110 terms and is correct: mwcc unrolls our loop by two and
did not unroll retail's, and retail keeps `mulli r,r,12` indexing where we
strength-reduce to pointer walking. Neither stock 2.0p1 nor patched 2.0p1a
reproduces retail's shape, so it is not the compiler patch -- but nothing is
wrong with the source either.

`semdiff --kinds` is the triage for the third: it prints the mnemonics one side
executes and the other never does, with address arithmetic, branches and
register moves filtered out, since those are exactly what unrolling and strength
reduction rewrite. Tridiag_Solve comes back with nothing but address arithmetic.
xScrFXGlareRender comes back with retail using `psq_l`/`psq_st`/`lmw`/`stfd`
against our `fctiwz`/`fmuls`, which is a different computation and is now the
best-evidenced target on the list at 40.04%.

Ruled out, do not re-tread:

- **xFXShineRender / xFXStreakRender.** Declaring `blah` as a function-local
  static does produce `blah$1557` and does collapse the terms, but CW then emits
  it at the end of `.bss` instead of between sFirework and active_ribbons,
  moving three other objects and taking the unit's `.bss` from 100% to 94.63%
  for no `.text` gain at all. Retail places both statics early, so its source
  must order these functions differently -- our `.text` already runs ~2KB long
  by xFXRibbonRender and carries template bodies retail does not. That file's
  structure is a separate job; the metric was what was wrong.
- **Tridiag_Solve** is semantically exact. See above.
- **xsqrt** still differs for a reason recorded in the source: retail applies
  `fmuls` straight to the `frsqrte` result while we round it first, because
  `__frsqrte` is declared to return double and `F32 guess` narrows it. Do not
  redeclare the intrinsic -- xCollide's `std::sqrtf` and xSpline's `sqrt` both
  match at 100% precisely because they take the double.
- **ArcLength3** differs by exactly two contractions: retail computes two of its
  five product-sums as `fmul` + `fadd` where we emit `fmadd`. Which two, and
  why, is not apparent from the codegen. 6 terms at 88.02%.


### The held-across-a-call pattern, and the limits of sweeping for it

Two fixes in one session came from the same shape. A member load cannot survive
a call in a register -- the callee might write the object -- so the compiler must
reload it afterwards. If the target instead keeps the value in a *callee-saved*
register across the `bl`, the original source had bound it to a LOCAL.

  zEntPickup_SceneUpdate  87.67% -> 95.31%  (currRequest across rewardRequest)
  xHudMeter::set_value    89.45% -> 100.0%  (v - value across xsqrt)

Sweeping for it mechanically is harder than it looks, and three formulations
failed before one worked:

- **"we load an address more often than the target"** -- 548 functions. That is
  ordinary register allocation, not a pattern.
- **"split the function at its first call, require loads on both sides"** -- 0
  functions, including on a known positive. zEntPickup_SceneUpdate calls
  zGameIsPaused() on line one, so everything counts as after a call.
- **"a call sits between two loads of the same address"** -- 0 on the known
  positive as well. The target has that too: `currRequest++` genuinely re-reads
  memory after the call. The distinguishing feature was never the call placement
  but the raw count (target 2 loads, ours 5).

What works is already in semdiff: **ours-only load terms with a non-zero struct
offset**. Across the whole game that is 8 functions, and the top one
(Tridiag_Solve, 43) is known noise from loop unrolling. So the pattern is real
but there is no seam left to mine.

A trap worth naming: a sweep script in the scratchpad must not copy
`ROOT = dirname(dirname(__file__))` from tools/*.py. It resolves to the temp
directory, os.walk finds nothing, and the sweep reports a clean result over zero
objects -- the failure mode that looks exactly like success. Validate every
detector against a known positive before believing a null result.

### xMat3x3Mul is register allocation, not source

73.39%, and every source shape tried made it worse. Recorded so it is not
re-tried:

- `usetemp` **is** a U8 or bool in retail -- it emits `clrlwi. r7,r0,24`, a
  truncate-and-test that a U32 cannot produce. Changing our U32 to bool does
  reproduce that instruction, and costs 3 points (73.39 -> 70.38) because the
  surrounding allocation shifts. Instruction count is unchanged at 108.
  Retail truncates once into r7 and re-tests with `cmplwi r7,0`; we truncate
  twice either way.
- `b->at.y` is loaded once by retail (into callee-saved f29) and three times by
  us, once per use -- textbook held-across-a-call. Restoring the
  commented-out `F32 bay = b->at.y;` local *does* fix the load count to 1, and
  costs 6 instructions in spills (108 -> 114, retail 106) because the extra live
  value does not fit. 73.39 -> 67.63. Both changes together: 60.25.

The function shares 51 of 106 instructions with the target. That is an
allocation problem, and the source shapes that would fix the load counts cost
more than they save.

## The 2026-08-31 audit: what a whole-tree sweep of everything-but-functions found

Run after a clean build at `cb195646` (DOL sha1 correct). Game code at that
point: exact **80.706%**, fuzzy **99.223%**, 7251/7673 functions, 94/224 units,
413 non-matching game functions in 315,636 bytes.

Every non-function detector was run over the whole tree. These are now clean and
should not be re-run without a reason:

| sweep | result |
|---|---|
| `strdiff.py` | 0 functions reference different strings |
| `argswap.py` | 0 transposed-argument sites over 224 units |
| `cmpswap.py` | 0 swapped-operand sites over 224 units |
| `stubs.py` | 0 stub functions |
| `poolmulti.py` | 0 functions load a constant the other side never loads |
| `calldiff.py` | 4 functions, all in `zEntPlayerBungeeState`, all the anonymous-class per-TU counter (`class$912` vs `class$145`). Not a defect. |

### `datamulti.py` was reporting 40 units that are byte-identical

`objdump -s` prints a section's last group short when the section size is not a
multiple of four. `zSurface`'s `.rodata` ends `...4e450000` on the target and
`...4e4500` in ours -- the same bytes, one side padded to alignment. The old
parser required exactly 8 hex digits, so it dropped our short group and kept the
target's padded word, then reported `4e450000` as target-only. Forty units
showed exactly one such word each, which is what a systematic artifact looks
like.

Fixed by concatenating a section's groups and zero-padding to a word boundary
before counting. **65 units -> 16.** Anyone reading an old datamulti ranking
should discard every single-word `.rodata` row in it.

### What the 16 surviving units actually are

Explained and deliberate:

- Both Prawn and Dutchman retail animation templates contain explicit zero
  terminators. Prawn's ten-entry list was already correct and source-linked;
  Dutchman's source-only `00000007` was an extra Taunt01 entry, not a
  terminator workaround. Its thirteen-word retail list has twelve animation
  indices followed by zero. See the 2026-10-02 data audits below; the unused
  `NPCC_ANIM_LIST_END` macro is removed.
- `3acccccd`/`3b088889` (1/640, 1/480) ours-only in `zGame`, `zMain`, `xTRC`,
  `xCM`, `zUIFont`: our object emits weak out-of-line copies of `NSCREENX__Ff`
  and `NSCREENY__Ff` along with their pool constants. Retail's objects did too --
  the linker kept only xDebug's copy, so dtk's extraction shows the other five
  as `*UND*`. Verified inside `zUIFont_Render`: both sides emit
  `bl NSCREENX__Ff`. Expected extraction artifact, not a source difference.

Real, and one is fixed:

- `zNPCTypeDutchman` `.data` `be32b8c2` vs ours `be32b8c3` -- **fixed**, see the
  DEG2RAD note below.
- `zNPCFXCinematic`: the target carries `"fx_pickup_emphasis"` and an anonymous
  12-byte `.rodata` object `@1756` = `{0.25f, 0.0f, 0.0f}` sitting between
  `tym_anger$1601` and `vec_offset$1853`. Nothing references either. This is the
  `__deadstripped_<unit>` case and the file already has one such function, but
  the string pools *last* while the vector pools *mid-file*, so a single
  never-called function cannot produce both orders. Still open.
- `zEntPlayerOOBState` 4x, `zNPCSupport` 2x and `zNPCTypeRobot` 1x missing
  `1.0f` in `.rodata`; `zNPCTypeAmbient` extra `2.0f`; `xModel` and `zUIFont`
  extra `0.5f`; `zUIFont` extra `176.0f`; `zHud` target-only `64006875`.
- Two one-ULP constants: `zNPCSupplement` retail `3cccccce` where `0.025f` is
  `3ccccccd`, and `zEntPlayer` retail `3eaaaaa0` where `0.333333f` is
  `3eaaaa9f`. Retail is one ULP *above* the correctly-rounded literal in both
  cases, so the source decimal differs; no expression tried reproduces either.

### DEG2RAD: the macro is multiply-first, two constants are not

`DEG2RAD(x)` is `((PI) * (x) / (ONEEIGHTY))` with `PI = 3.1415927f`. Of every
angle the tree passes it, only **10, 20 and 63** give a different float
depending on whether you multiply or divide first.

The macro itself is right. Retail's runtime call sites (`zPlatform`, `zSurface`,
`zMain`, `zNPCTypeCommon`) pool `PI` (`40490fdb`) and `180.0f` (`43340000`)
separately and never pool `PI/180` (`3c8efa35`), which is what a divide-first
macro would have emitted. So the fix is site-local, like the `DEG2RAD(63.0)`
double literal already in `zNPCGoalRobo` and the `0.052359881f` literal in
`zNPCGoalVillager`. `zNPCTypeDutchman`'s `boss_cam` now writes `-0.17453292f`.

### `tools/stridediff.py` -- read the mulli half, ignore the addi half

New tool for the "raw byte offset on a typed pointer" class. It compares the
per-function multiset of `mulli rD,rA,IMM` and self-incrementing
`addi rX,rX,IMM`. The `addi` half produces 772 hits across the tree, dominated
by frame adjustments and pool bases -- unusable as written. The `mulli` half is
short enough to read by hand. Most of its rows are the documented
mis-attribution: `LastTarget__11XSGAutoData` and `zUpdateThumbIcon` show the
same `mulli 108` + `mulli 12` on opposite sides, which is one body under two
names. The one row that was real is below.

### `iBoxIsectSphere` tested the quotient where retail tests the remainder

`xcode`, `ycode` and `zcode` are 0..5, so `code / 3 == 2` is never true and the
early-out for a sphere lying wholly outside one slab never fired. The target
computes `mulhwu` / `srwi` / `mulli 3` / `subf`, which is a remainder. Fixed to
`% 3`, plus hoisting `center +- r` into locals where retail computes them:
89.790 -> **98.473**. The residual is FP colouring -- retail overwrites the
register holding `center` with `hi`, ours takes a fifth register -- and it did
not move under four declaration-order permutations or three rewrite shapes.

`iCameraSetFOV` was the other win. Reading `vw.x` back made CW round the `itan`
result to single a second time; `vw.y = 0.75f * (vw.x = itan(...))` reproduces
retail exactly. **95.208 -> 100.000, and `iCamera` is a complete unit.**

### Two more residuals proven not source-reachable

- `xTRCDisk` (75.909, 44 bytes): retail stores the parameter first, which frees
  r3 for the address of `gTrcDisk[1]`; ours hoists the address materialisation
  above the store and must use r4. Four source shapes tried, best 80.909.
- `xSndAddDelayed` (92.000): retail keeps the `lfs 0.0f` inside the search loop,
  ours hoists it out. Four shapes tried, none moved it. LICM, not source.
- `zNPCSpawner::Owned` (87.145): retail's unroller bumps the base pointer by
  0xc between the eight unrolled bodies; ours uses eight immediate
  displacements and one `addi 0x60`. Three pointer-walk shapes tried, all worse.

## The remaining SCHED list has no eighth clause in it (2026-08-31)

All 72 SCHED-class game functions were diffed, their movers extracted
mechanically, and the whole list re-compiled under stock `GC/2.0p1` as a
control. Every one is a pure reorder -- the classification is clean. They do
**not** share a defect:

- **~22 functions: entry-4 subrange x subrange.** A frame store whose value came
  from a latency-bearing load is sunk below the neighbouring store to a sibling
  subrange of the same frame aggregate; retail leaves the stall. Canonical:
  `xEntDriveMount`'s `xVec3NormalizeMacro` zero branch. Clause B's *tests* do
  not reject these pairs -- the dispatch does, because B is installed on entries
  0/1/3 and entry 4 is the one never redirected. Every redirection of entry 4 is
  already measured: clause-B form **-199**, static-gated **-21**, unconditional
  with VN e1 **+16/-560**. This is the dead end already named above as the
  single largest confirmed one in the project; the new fact is that it is now
  the *plurality* of what is left.
- **~30 functions: alias-invisible pick order.** The movers are ALU, branch or
  load-only -- address materialisation against a store, FP ALU against a store,
  or the epilogue LR-slot load hoisted above the callee-saved restores. One side
  has no memref, or both are loads, so no alias predicate is ever consulted.
  This is the 2026-08-12 NO-GO's "78 crossing no memory op" population.
- **~4 functions: retail is the aggressive one.** `set_object_state` (70.175,
  the worst on the list, and byte-identical under stock), `turn_to_face`,
  Prawn's `create`, `NCIN_SleepyDRay_AR`. Retail hoists an indexed load above a
  store through a heap pointer. Reproducing it needs a **no-alias** answer where
  stock says may-alias, justified by escape analysis the byte clause does not
  have. That is a miscompile, not a mismatch. Do not.
- **2 functions: entry-0 whole x whole.** `eval_joint` and
  `iModelMaterialMulCB`, both patch-insensitive: an 8-byte `.sdata2` `lfd`
  hoisted above a `stw` to a whole 4-byte frame local. The only legitimate
  unmeasured candidate left is **E3n's predicate verbatim on scheduler entry 0**
  (the entry-0 chain at cave `0x57ead8`, tested before falling into clause C+).
  Ceiling on this list is ~2 functions / ~660 bytes. Price it with the frida A/B
  method before writing bytes -- clause D's recorded 17x collateral and C+'s
  entry-0 over-fire on `xFXRenderProximityFade` are what that ceiling competes
  against.

Control totals: 46 of 72 are byte-identical under stock `GC/2.0p1`; 22 are
*better* under the patched compiler, where the clauses close most of the gap and
leave only the entry-4 swap (`BoulderRollCB` 94.437 -> 99.946).

**Two entries are misclassified and should be tagged E3N-COST, not SCHED.**
`MoveNormal__14zNPCGoalPatrolFf` and `BasisBspline__FPA4_fPf` compile to exactly
**100.000%** under stock. They are E3n over-fire cost, paid for by E3n's +82,
and the narrowing that would rescue them is the conclusively failed 204-probe
attempt. Nobody should hunt a scheduler defect in them.

### `xVec3::cross`'s brace init: +1 in xCollide, -1 in zEntPlayerBungeeState. REJECTED.

Measured 2026-08-31. The change looks compelling and nets zero.

```diff
     xVec3 cross(const xVec3& c) const
     {
-        xVec3 v = xVec3::m_Null;
+        xVec3 v = { 0.0f, 0.0f, 0.0f };
```

The evidence for it is real: the target's `cross` in `xCollide.o` references an
anonymous all-zero 12-byte `.rodata` object (`@410`), `m_Null__5xVec3` does not
appear in that object at all, and `cross__5xVec3CFRC5xVec3` goes **59.355 ->
100.000**. All eight TUs that call `.cross(` are `NonMatching`, so the DOL cannot
move.

It still loses. `start__…hanging_state_type` in `zEntPlayerBungeeState` goes
**100.000 -> 99.950**, and `rodatalayout.py` says why: that unit's
`__deadstripped_zEntPlayerBungeeState` already reproduces `@410`, so the brace
init interns a *second* 12-byte zero template. It lands fifth, not third, and
shifts every later `.rodata` object by 12 — visible as `addi r5,r30,0x1ec` where
retail has `0x1e0`. `datamulti.py` cannot see it, because a duplicated all-zero
object is filtered as padding; only `rodatalayout.py` shows it.

So the two units disagree about whether `cross` interns a template, and one
header cannot satisfy both. Either the real shape is something else that emits
the template only where the target has it, or `zEntPlayerBungeeState`'s
deadstripped block is wrong about `_410` and the ordering has to be re-derived
there first. Do not re-apply the diff on the xCollide measurement alone.

### `get_grid_index`: 60.558 -> 98.372, banks nothing, held

Also measured, also in a shared header (`xGrid.h`), also not applied — 98.372 is
worth zero and it is a 169-TU header:

```diff
 inline grid_index get_grid_index(const xGrid& grid, F32 x, F32 z)
 {
-    grid_index index;
-    index.x = range_limit<U16>((U16)((x - grid.minx) * grid.inv_csizex), 0, grid.nx - 1);
-    index.z = range_limit<U16>((U16)((z - grid.minz) * grid.inv_csizez), 0, grid.nz - 1);
+    F32 gx = (x - grid.minx) * grid.inv_csizex;
+    F32 gz = (z - grid.minz) * grid.inv_csizez;
+    grid_index index = { range_limit<U16>((U16)gx, 0, grid.nx - 1),
+                         range_limit<U16>((U16)gz, 0, grid.nz - 1) };
     return index;
 }
```

The target's `lwz @587 / stw 0x8(r1)` ahead of the field stores is a brace-init
template, and the `F32` temps are what hold `gz` in `f31` across the first
`range_limit` call. The other two `xScene` functions are unchanged by it. Land it
for fidelity if something else in that unit ever crosses; on its own it is not
worth the blast radius.

### Levers that paid on 2026-08-31, worth trying first on a new function

- **A missing local shows up as a frame-size difference.** `Process__zNPCGoalWander`
  had `stwu r1,-0x60` against retail's `-0x70`; the gap was a third `xVec3` that
  retail passes to `XYZDstSqToPos`. Adding it fixed every offset in the function.
  Compare the two prologues before anything else.
- **mwcc unswitches a small loop.** Retail's two adjacent tail loops in
  `_xAnimTableAddTransition` come from ONE source loop carrying a ternary; written
  as two they do not share the hoisted preheader or the induction registers. The
  larger loop earlier in the same function is *not* unswitched, so this is a
  size heuristic, not a rule about the file.
- **Operand order inside a folded constant.** `arg0[i] / 1024.0f` and
  `0.0009765625f * arg0[i]` fold to the same word but emit `fmuls` with the
  operands swapped. That alone took `CalcRecipBlendMax` 98.293 -> 100.000.
- **Reading a struct field back forces a second rounding.**
  `vw.y = 0.75f * (vw.x = itan(...))` matched where
  `vw.x = itan(...); vw.y = 0.75f * vw.x;` emitted an extra `frsp`.
- **A hand-written word-by-word struct copy is an inlined assignment.** Seventeen
  `*(U32*)&a.x = *(U32*)&b.x;` lines in `zSaveLoad_Tick` were one
  `xMat4x3 m = *xEntGetFrame(...)`, and the hand version did not even follow the
  struct's field order.
- **Post-increment subscript recovers an indexed store.** `list[count++] = x`
  emits `stwx`; `list[count] = x; count++;` does not.
- **Binding a literal to a named local can stop a hoist.** `const F32 zero = 0.0f;`
  above the cross products in `nearestFloorCB` put the pool load ahead of the
  pdx/pdz stores, where retail has it: 95.581 -> 100.000. Same device as
  `nearestTrackCB`.

## The complete patch-cost list is seven functions (2026-08-31)

`tools/patchcost.py` compiles every game unit twice — once with `GC/2.0p1a`,
once with stock `GC/2.0p1` — and reports the functions that are exact under
stock and not under ours. Tree-wide, that is **seven**:

| unit | function | ours | bytes |
|---|---|---|---|
| `zNPCGoalStd` | `MoveNormal__14zNPCGoalPatrolFf` | 97.430 | 716 |
| `zEntPlayer` | `zEntPlayer_AnimTable__Fv` | 97.249 | 23,820 |
| `xSpline` | `BasisBspline__FPA4_fPf` | 96.264 | 576 |
| `zNPCTypeRobot` | `Setup__11zNPCFodBzztFv` | 95.347 | 288 |
| `zNPCGoalAmbient` | `Process__18zNPCGoalJellyBirth…` | 92.954 | 348 |
| `zNPCFXCinematic` | `NCIN_SleepyLamp_AR__…` | 87.567 | 628 |
| `xMath3` | `xBoxFromCircle__FR4xBoxRC5xVec3RC5xVec3f` | 77.875 | 256 |

The other direction, `patchcost.py --gains`, is **427 functions and 209,216
bytes**: exact under `GC/2.0p1a` and not under stock. So the scoreboard for the
whole patch is **+427 / -7**, and it is concentrated where the work has been --
zEntPlayer 54, zNPCGoalRobo 22, zEntCruiseBubble 19, zNPCTypeKingJelly 12,
xFX 12, zNPCTypeBossSandy 11, zNPCHazard 11.

Against that the cost list is a rounding error, and it settles two things.

**Every one of these seven is source-correct by construction.** Reaching
100.000% under stock means the source already says what retail's said, so
there is nothing left to recover from source in any of them. Anybody who opens
one looking for a wrong expression is wasting the session. Three of them were
already recorded individually (`zEntPlayer_AnimTable`, `MoveNormal`,
`BasisBspline`); the other four were not.

**They are also the exact target set for narrowing a clause.** Any future
narrowing should be priced against this table and nothing else — if a
narrowing does not recover one of these seven it is buying nothing, and the
frida A/B method is how to check before writing bytes.

Note `xFXRenderProximityFade` and `refresh_bound` are *not* on this list.
They are worse under the patch than under stock but not exact under either, so
they are partial patch cost, not recoverable by reverting a clause.

## `xVec3::create` is not the source of the `@405` template

`create__5xVec3Ffff` and `create__5xVec3Ff` sit at **50.000%** (80 bytes each)
in every unit that emits them, and the target's body opens by copying an
anonymous 12-byte all-zero `.rodata` template into the local before assigning
x, y and z. That looks exactly like `xVec3 v = { 0.0f, 0.0f, 0.0f };` and it is
not. Four shapes measured through `solo.py --shadow` (which puts the header
change in a private include directory so no other agent ever sees it):

| shape | % |
|---|---|
| `xVec3 v;` (current) | 50.000 |
| `xVec3 v = { 0.0f, 0.0f, 0.0f };` | 50.000 |
| `xVec3 v = m_Null;` | 50.000 |
| `const xVec3 zero = {…}; xVec3 v = zero;` | 50.000 |
| `static const xVec3 zero = {…}; xVec3 v = zero;` | 50.000 |

CodeWarrior eliminates the initialisation in every one of them, because all
three members are overwritten immediately after. It also eliminates it under
**stock `GC/2.0p1`**, so this is not patch collateral. Whatever keeps retail's
copy alive is not an initialiser CW can see through, and guessing at brace
inits is settled — stop.

Do not read the anonymous head templates (`@405`, `@406`, `@410`, `@441`) as
"these belong to xVec3.h inlines and would appear on their own if the source
were right". They are reproduced deliberately by `__deadstripped_<unit>` in the
units that need them, and that is currently the only thing that produces them.

## `xsqrt` and two more residuals that are constant-load placement

`xsqrt__Ff` in `xBound` (172 b, **67.512%**) is the clearest specimen of the
rematerialise-vs-hold class. Retail's frame is 0x40 and saves f29/f30/f31: `x`
in f29, and `half` and `three` **loaded before the `__fpclassifyf` call and
held across it**. Ours is 0x20, saves only f31, and rematerialises both
constants after the call. Measured and inert: `static const`, non-const, a
named copy of `x`, and `double guess` in three spellings (all 58.814 — worse,
because the `frsp` the source comment blames is not the residual).

Two siblings found the same day, both compiler-track:

- `xSndAddDelayed` (92.000, one-away unit `xSnd`): retail keeps the `lfs 0.0f`
  **inside** the delayed-slot search loop; ours hoists it out. Four shapes
  measured, none moved it. LICM, not source.
- `zNPCSpawner::Owned` (87.145, one-away unit): retail's unroller bumps the
  base pointer by 0xC between the eight unrolled bodies and reads a constant
  displacement; ours uses eight immediate displacements and one `addi 0x60`.
  Three pointer-walk shapes measured, all worse.

And one that is neither: `xTRCDisk` (44 b, 75.909, one-away unit `xTRC`).
Retail stores the parameter first, which frees r3 for the address of
`gTrcDisk[1]`; ours hoists the address materialisation above the store and has
to use r4. Four shapes measured, best 80.909.

## Six units are at 100% and still cannot be marked Matching

`tools/promotable.py` lists them from `report.json` and `configure.py`:
`iCamera`, `xDebug`, `xHudMeter`, `xParEmitterType`, `zSurface`, plus
`zAnimList` which is deliberately `Equivalent`. `iCamera` is new as of today
(`iCameraSetFOV` reached 100.000).

`symorder.py` gives the reason for each and it is the same reason every time:
our object emits a weak out-of-line inline copy that the retail link
deduplicated into another object, so the `.text` symbol order differs even
though every function matches.

- `iCamera` — `__as__6RwRGBAFRC6RwRGBA`, emitted between `iCameraUpdateFog` and
  `iCameraSetFogRenderStates`.
- `zSurface` — `__as__5xVec3FRC5xVec3` after `zSurfaceInit`, plus a
  `.sdata2` SAME SET, WRONG ORDER on the last two slots.
- `xHudMeter` — four `xhud::widget` virtuals plus a reordered
  `sound_queue<4>` group.
- `xParEmitterType` — `xVec3SMulBy`, `__as__5xVec3`, the `xVec2` operators and
  `__deadstripped_xParEmitterType` itself.

Defining a function the retail link deadstripped is not automatically fatal —
`mem_funcs.c`, `FILE_POS.C`, `nubevent.c` and `float.c` all do and all link —
so the order is what to check, not the extra symbol. The DOL sha1 is the only
test that settles a promotion.

## The MISSING bucket in game code is eight functions, and four of those are noise

Swept with `solo.py --missing` over all 224 game units on 2026-08-31. The whole
result:

| unit | function | bytes |
|---|---|---|
| `zEntPlayerBungeeState` | four `__as__…hook_asset…class$91N` | 12 / 28 / 52 / 84 |
| `xCollide` | `__as__6RwBBoxFRC6RwBBox` | 52 |
| `xCollide` | `__as__7xCollisFRC7xCollis` | 164 |
| `xCamera` | `__as__6xBoundFRC6xBound` | 224 |
| `zNPCGoalStd` | `Remove__17xListItem<5xGoal>Fv` | 56 |

**The four `zEntPlayerBungeeState` rows are not missing.** They are the
anonymous-class per-TU counter: retail's are `class$910`…`class$913`, ours are
`class$143`…`class$146`, so `--missing` pairs nothing and `calldiff.py` reports
the same four as a differing callee set. Same code, different ordinal.

The other four are genuinely absent weak symbols the retail link kept from this
object. Each needs both a definition and a real call site in the TU, and
`__as__7xCollisFRC7xCollis` carries a question with it: it copies offset 0x10
with `lfs`/`stfs` and everything else with `lwz`/`stw`, which is a **memberwise**
`operator=`. That means retail's `xCollis` was not trivially copyable, which
sits in tension with the recorded `xCollis::tri_data` fix. Somebody has to
decide whether a member gets a user-declared `operator=` back before this one
is worth writing.

So "1483 functions have no implementation at all", recorded above under the open
leads, is a whole-project figure dominated by Renderware and Bink. For `src/SB`
the bucket is closed: the remaining work is all wrong bodies, not absent ones.

## Where the remaining 404 game functions actually sit (2026-08-31, after the day's work)

`classify.py`'s mechanical buckets, restricted to the functions `report.json`
scores as non-matching:

| class | count | share |
|---|---|---|
| OTHER | 145 | 35.9% |
| SIZE | 120 | 29.7% |
| SCHED | 72 | 17.8% |
| REGS | 67 | 16.6% |

So 34.4% is compiler-track by the mechanical reading — **and that is an
undercount.** A whole day of hand-sampling the SIZE and OTHER rows kept landing
on register numbering and constant-load placement: `NightLightUVStep` (60.700,
SIZE), `DiscoRender` (81.053, SIZE), `zNPCCommon::GetParm` (94.490, OTHER),
`zNPCCommon::Reset` (93.297, OTHER), `xBinaryCamera::update` (91.392, OTHER) and
`CalcJumpImpulse_Smooth` (88.147, OTHER) are every one of them a permutation or
a hoist, not a wrong expression. `classify` calls them SIZE or OTHER because one
extra `mr`, `clrlwi` or rematerialised `li` changes the instruction count, which
is exactly what a register-allocation difference does.

Treat the SIZE/OTHER buckets as "not yet triaged", not as "source is wrong".

### A patch-neutral witness worth keeping: `NightLightUVStep`

`zNPCSleepy::NightLightUVStep` (`zNPCTypeRobot`, 200 bytes) sits at **60.700%
under BOTH `GC/2.0p1a` and stock `GC/2.0p1`** — the clauses do not reach it at
all. Its source is four obviously-correct `+=` pairs over two 2-element static
float arrays followed by four `RANGEWRAP` calls, and the entire residue is our
scheduler hoisting a `.sdata2` literal load above a `stfs` to a small static,
which is the shape clause C and E3n exist to cover.

Small, self-contained, correct source, patch-insensitive. That makes it the best
first specimen for anyone reworking the clause set.

### Leads recorded, not yet worked

- **`PlayerAbsControl` (zEntPlayer, 5460 b, 97.601) is one callee-saved FP
  register short.** Target frame 0xd0 saving f26-f31; ours 0xc0 saving f27-f31.
  By the frame-size rule that is a missing named local, and the source names the
  candidates for us — it carries six commented-out declarations
  (`scalemag`, `dir_dp`, `turnfactor`, `diffAngle`, `autodist2d`, `camAngle`).
  The target also does `fmr f30, f29` right after `stfs f29, maxVelmag`, keeping
  a second copy of the 0.0f, which is the `fmr` copy-trio shape.
- **`zEntPlayer_Update` (18,188 b, 96.700)** is a whole-function callee-saved
  GPR rotation: ours uses r15/r16/r23/r24 where retail uses r26/r27/r17/r18.
  Everything else lines up. One decision somewhere near the top of the function
  shifts the entire allocation.

### Measured NO-GOs from the same pass

- **`iCamera` cannot be promoted by suppressing `__as__6RwRGBA`.** The unit is
  15/15 functions and 64/64 data, and `symorder.py` says the only blocker is a
  weak `__as__6RwRGBAFRC6RwRGBA` our object emits between `iCameraUpdateFog` and
  `iCameraSetFogRenderStates` that the target does not have. Copy-initialising
  `a` and `b` at their point of use instead of declaring them at the top and
  assigning — the documented way to get a flat word copy instead of an
  `operator=` — **breaks `iCameraUpdateFog` outright, 100.000 -> 76.024**. So
  retail assigned, and the weak copy comes with it. Whatever suppresses it is
  not the assignment shape.
- **`xAccelMove__FRfRfffff` (99.670) is not a bug.** `bugrank.py` flags it at
  two terms, and the difference is `mr r4, r3` against our `li r4, 0x1` at
  `var_r4 = var_r3;` where `var_r3` is provably 1. Rematerialise-vs-copy, the
  documented class. The source is already correct; the "Possible missing debug
  subroutine" comment above it is not evidence of anything.

### `xSndIsPlayingByHandle`'s stray `clrlwi`: +1/-1 whichever end you change. NO-GO.

`xSndIsPlayingByHandle__FUi` (32 bytes, `zEntPlayer`, **87.500%**) emits one
instruction the target does not: `clrlwi r3, r3, 24`, the `bool` -> `U8`
conversion on `return iSndIsPlayingByHandle(sndID);`. `xSnd.h` declares the
wrapper `U8`; `iSnd.h` declares the callee `bool`. Both ends were tried and
both are +1/-1.

**Making the wrapper `bool`** takes `zEntPlayer` 25 -> 24 and costs
`zNPCNewsFish::IsTalking` in `zNPCTypeVillager`, 100.000 -> 56.786. The
instructions are identical either way; only the position of the `clrlwi` moves,
from the call arm to the merged path. And that position is the proof that
**`U8` is right for the wrapper**: `IsTalking` is
`return (soundHandle) ? xSndIsPlayingByHandle(soundHandle) : false;`, so with a
`U8` return the two ternary arms have different types, the `U8` arm gets its
own zero-extension and `false` is a bare `li 0` — which is exactly what the
target emits. A `bool` return makes both arms the same type and sinks the
conversion below the merge.

**Making `iSndIsPlayingByHandle` `U8`** takes `zEntPlayer` 25 -> 24 and costs
that function itself, 100.000 -> 85.588: its two compound-boolean returns then
need the conversion instead, and CW puts the value in r4 and truncates into r3
at the end where the target has a bare `li r3, 0x1`. Two rewrites of the body
were measured — integer literals for the `false` returns (85.588, no change)
and fully decomposed early returns (**80.882**, worse). The target's shape
(`li r3,0` up front, `beqlr`, `bnelr`, `li r3,1`) is what our current `bool`
source already produces, so the body is not the lever.

So both declarations are individually correct against their own call sites, and
retail's compiler emitted the conversion in neither place. Whatever it did is
not reachable by changing either return type. Do not re-open this on the
`zEntPlayer` measurement alone — it looks like a clean +1 and it is not.

## The Ghidra-transcription seam: what is left of it

The premise -- a body carrying `iVar1`, `dVar2`, `param_N`, `local_xx` or
`uVar` identifiers is a literal transcription of decompiler output rather than a
reconstruction, and so is likelier to differ structurally -- **paid repeatedly
on 2026-08-31**. `Process__zNPCGoalWander` was missing a whole `xVec3` local
that `dwarf/` named; `CalcNewDir`'s `player_pos`/`npc_pos` were artifacts dwarf
does not list; `zSaveLoad_Tick` had seventeen hand-written `*(U32*)&` word
copies standing in for one struct assignment, in an order that did not even
follow the struct; `xSerial::prepare` assigned to a shadowing parameter and
never set the member at all.

Cross-referencing the artifact count against the remaining non-matching count
gives the to-do list. Units where both are high are the seam; units with many
artifacts and no non-matching functions are a pure fidelity job (the names are
wrong, the code is right) and bank nothing.

| artifacts | non-matching | unit |
|---|---|---|
| 74 | 2 | `Core/x/xScene` |
| 62 | 1 | `Game/zEntPlayerBungeeState` |
| 48 | 25 | `Game/zEntPlayer` |
| 48 | 2 | `Core/gc/iSystem` |
| 47 | 3 | `Game/zNPCSupplement` |
| 38 | 6 | `Game/zNPCGoalRobo` |
| 36 | 1 | `Game/zNPCTypeBossPatrick` |
| 26 | 1 | `Core/x/xEntBoulder` |
| 24 | 2 | `Game/zAssetTypes` |
| 18 | 7 | `Game/zNPCSupport` |
| 17 | 3 | `Core/x/xHud` |
| 15 | 2 | `Game/zNPCGlyph` |
| 15 | 1 | `Core/gc/iTRC` |
| 14 | 12 | `Game/zNPCTypeRobot` |
| 14 | 1 | `Game/zNPCGoalStd` |
| 13 | 3 | `Game/zDiscoFloor` |
| 12 | 4 | `Core/x/xCollide` |
| 12 | 1 | `Game/zSaveLoad` |
| 8 | 5 | `Core/gc/iModel` |
| 7 | 7 | `Core/x/xString` |
| 4 | 5 | `Game/zGame` |

Reproduce with a grep for
`\b[a-z]{1,2}Var[0-9]+\b|param_[0-9]|local_[0-9a-f]{2}|\buVar|\bauStack`
against each unit's source, joined to `report.json`'s non-matching count.

Note that `xEnt`, `xCollideFast`, `xordarray` and `xHudText` all still carry
artifacts and have **zero** non-matching functions, which is the useful
counter-example: a Ghidra name is a hint about how the body was written, not
evidence that it is wrong.

### Two `__deadstripped_*` functions place objects at two parse positions

Recorded as an open limitation above, under `zNPCFXCinematic`: a single
never-called function interns all its locals at one point in the source, so it
cannot reproduce a target whose unreferenced templates sit in two different
places. **Split it.** `zNPCFXCinematic` now carries three
(`__deadstripped_`, `__deadstripped2_`, `__deadstripped3_`) and `zFX` two, and
both units' `.rodata` is byte-identical to the target.

`zFX` is the instructive one, because the fix **removed** a fabricated object
rather than adding one. `get_triangle_area`'s entire residual was `.rodata`
offsets shifted by 0x28: we emitted eight 0x28 templates where the target has
seven, and the target's seventh, `@558`, is not deadstripped at all — it is
`tweak_callback::create_change`'s `{ NULL x10 }` initialiser. Retail defines
that function mid-file, among the functions the link later removed, so its
template interns at 0xf8. Ours defined it after the named globals, so the
template landed at 0x380 and pushed everything between them along.

Moving the real definition between the two halves of the deadstripped block
puts it back, and the hand-written `_558` is deleted. 99.852 -> 100.000.

The general lesson: before fabricating a template, check whether a function you
already have owns it and is merely defined in the wrong place. `rodatalayout.py`
shows the count mismatch; the object being one of *ours* rather than the
target's is what tells you to move code instead of adding it.

## Session 2026-09-01: six crossings, and the classes they did not come from

Full `ninja`, DOL `306526d9...` intact: **matched_functions 8455 -> 8461,
complete_units 236 -> 238** (zNPCSndTable and xDecal now link from our
objects; xDecal's extra weak inline copies did not move the link). GAME exact
81.513 -> 81.600, fuzzy 99.306 -> 99.321. Crossings to 100.0: `xDecal` `update_frac`, `get_render_data`,
`select_texture_unit` (unit now 46/46), `zNPCSndTable` `NPCS_SndPickSimilar`
(unit 11/11), `zVolume` `zVolumeEventCB`, `xParEmitter`
`xParInterp::operator=`. Moved without crossing: `UpdateGustFX` 97.16 -> 99.15,
`GIDInStack` 86.1 -> 98.9, `IndexInStack` 85.4 -> 98.8.

Two of the crossings were semantic bugs, both invisible to the percentage:

- `select_texture_unit` returned `prev++ % units`; retail returns
  `++prev % units` (the `divwu` reads the incremented register). Every cycling
  decal texture was one frame behind.
- `zGustUpdateEnt`'s release loop tested `data->g[0]` and `data->lerp[0]` for
  every `j`; retail walks `g[j]`/`lerp[j]` (the target advances a pointer by 4
  through the loop). Fixed, banks nothing -- the residue is the literal-reload
  class below.

### Levers that paid, for the pattern file

- **Loop on the member, not a copy of it.** `update_frac` reloads
  `this->curve_index` at the loop test and after `unit.curve_index = ...`,
  because the `stb` through `unit` may alias `this`. Writing the loop over
  `this->curve_index` directly, plus `curve_node&` references for the two
  tail reads, went 70.9 -> 100. The same reference trick alone finished
  `get_render_data` (89.2 -> 100): retail computes `(ci+1)*12` with a second
  `mulli`; a single `this->curve[ci+1].color` expression lets CW fold it into
  `curve[ci] + 0x10`.
- **Initialise at the declaration.** `U32 aid_choice = 0;` instead of a later
  `aid_choice = 0;` moved `NPCS_SndPickSimilar` 90.3 -> 98.0. Retail keeps the
  zero in `r0` across the two template-copy loops; the late assignment let our
  colourer hand `r0` to the copy counters first. Then `list[cnt++]` (84.8 ->
  90.3 earlier) and an explicit `if/else` for the `trax` pick (98.0 -> 100).
- **A global incremented in place is reloaded after the array store.**
  `gOccludeCount++` after `gOccludeList[count] = vol` reproduces retail's
  `lwz`/`addi`/`stw`; `gOccludeCount = count + 1` forwards the local and
  cannot. 89.2 -> 100.
- **The generated `operator=` copies an array member as words.** Retail's
  `__as__10xParInterp` does `lwz/stw` for `val[2]` and `interp`, `lfs/stfs`
  for `freq`/`oofreq` -- the memberwise copy CW generates, not the
  hand-written one in `xParEmitter.cpp`. Deleting the declaration is a header
  change; `sweep.py` over the 132 including TUs (with reloc rows counted) was
  +1 / -0.
- **Two statements where one expression would fold.** `info.life.val[0] = x / 5.0f;
  info.life.val[0] *= 2.0f;` reproduces the target's two `stfs` to the same
  slot in `UpdateGustFX`.
- **A member bound to a local survives the call.** `S32 top = this->staktop;`
  before the `GetID()` loop in `GIDInStack`/`IndexInStack` (the
  held-across-a-call rule). The last ~1% in both is an `r30`/`r31` tie.

### Measured NO-GOs, so nobody re-treads them

- **`xatan2` returning `F64` is wrong tree-wide.** The "retail's `xatan2` may
  return `double`" lead (`update_turn` `frsp`) was tested by changing the
  declaration in `xMathInlines.h` and sweeping the 69 including TUs:
  51 functions down (xCamera, zNPCTypeBoss*, zEntCruiseBubble, xPad, zEntPlayer
  ...), 4 up. The three `update_turn` siblings' `frsp f0, f31` before
  `yaw + diff` is not this.
- **`xFuncPiece_Eval` (81.6) and `xFuncPiece_ShiftPiece` (95.5) are LICM with
  no store in the loop.** Retail keeps `lfs 1e-5f`/`fsubs` (Eval) and
  `lfs 0.0f` (ShiftPiece) inside loops that contain no store at all, so
  clause H's "loop storing to a static array" does not describe them. Fifteen
  source shapes on Eval (while/for/do/goto/const local/named local/...) all
  hoist; `-O2` un-hoists but wrecks the other 14 functions; `#pragma
  opt_loop_invariants off` is the IR optimiser's flag and changes nothing in
  the backend. Same compiler under `--mw GC/2.0`, `1.3.2`, `2.5`, `2.6`.
  Probe result worth keeping: a store *through a struct pointer* inside the
  loop (`func->order = 0`, or `*it = func`) makes OUR compiler emit retail's
  exact loop (`lfs const; lfs end; fsubs; fcmpo; blt`), and a global store does
  not. So retail's LICM answers "may alias a loop def" for a literal load in a
  loop with no def, which points at the `isloopinvariant` memory-operand path
  in `CodeMotion.c`, not at `maymove`. Same class as `xSndAddDelayed` and
  `zEntPlayer_SNDPlayStreamRandom` already listed.
- **The offset-induction-variable shape is compiler-track, three witnesses.**
  `find_entry` (86.9), `xShadowSimple_CacheInit` (97.0) and
  `unit_meter_widget::unit_meter_widget` (90.2) all have retail keeping
  `i*stride` in a register and doing `add base, iv` per iteration where we
  strength-reduce to a walking pointer. Element references, inline indexing,
  and pointer locals measured on all three: none moved, two got worse. It is
  the "second `i*4` induction variable surviving" note under `zEntPlayer`.
- **`xCMcolor_scale` ceiling is 74.3.** Retail homes the by-value `iColor_tag`
  parameter to the frame and byte-reads the copy; ours reads through the
  incoming pointer. `iColor_tag ret = color;` gets 65.7 -> 74.3 (retail does
  pre-fill `ret` with `color`), and nine further shapes (assignment, second
  copy, byte pointer, const param, address-of, inline args) do not home it.
- **`xParCmdAnimalMagentism_Update` (81.0) is one callee-saved FPR short.**
  Retail keeps `pos.x` in `f31` across the loop; ours reloads it from the
  frame each iteration. Declaration order, assignment form, `xVec3Sub` and
  splitting `pos.y += 1.0f` all measured <= baseline. dwarf lists exactly our
  locals.
- **`zMusicNotifyEvent`**: the `sMusicTimer[track]` / `music_enum = toParam[0]`
  rewrite matches retail's second `fctiwz` but drops 86.1 -> 74.3 because our
  compiler CSEs the body's `s->track` with the `track` local across the
  condition blocks and retail reloads it (cross-block CSE class). Semantically
  identical either way; kept the higher-scoring original.
- Also compiler-track after measurement: `CoefToUnity3` (identical multiset,
  a `factor2` local does nothing), `async_cb` (retail keeps both arms of the
  `length` if/else, ours if-converts; ternary and plain `0x8000` measured
  equal), `sqrt`'s missing second `fcmpu` (cross-block CSE), `DoAliveStuff`'s
  missing `cmplwi 1/2/4` chain (retail keeps the switch skeleton after DCE;
  an empty switch in our source is deleted outright), `Decompress_frame`'s
  `lis 0x8000`+`and` (four mask spellings, none produce it).

## The fourth hook: LICM never hoists a whole static read (2026-09-01)

Shipped. New `GC/2.0p1a` sha1 `a2feefd63e81ba708cc2e5cdfe5bd34066e72342`.
Full `ninja`: **matched_functions 8461 -> 8474 (+13 / -0)**, DOL `306526d9...`
intact, `objsnap.py cmp` shows exactly the 13 objects whose functions moved
and nothing else. Fable agent's full record, scan and frida harness:
session scratchpad `patch/patch_REPORT.md`, `patch_litscan.py`,
`patch_frida.py`.

**The defect, from the decomp and the 2.0 binary.** `CodeMotion.c:856`
`isloopinvariant` decides a read by walking the defs of the read's OWN object
and asking `may_alias` (0x511fc0) about each one. A loop with no store and no
call has no def of a `.sdata2` literal, so the def loop runs zero times and
NO alias table is consulted; the load goes to the preheader. Retail never does
this: a scan of every retail object finds 2,442 literal loads inside loops and
none hoisted by the compiler. The literal load reaches LICM as `flags 0x2`
(`fIsRead` only -- `fIsConst` is NOT set in 2.0, so a predicate keyed on it
never fires), alias kind 0 (whole object), object head 5 (static).

**Where it hooks.** Not an alias predicate: the `call isloopinvariant` at
`0x56f472` inside `moveinvariantsfromloop` (a REL32 with no `.reloc` entry) is
retargeted to a stub that calls `sb_licm_invariant(pcode)` and returns 0 to
the caller on a hit. `simpleunswitchloop` and `srawi_addze_isloopinvariant`
keep the stock routine. Predicate: read, alias present, kind byte
(`Alias+0x2c`) == 0, object head == static. Gains: `xFuncPiece_Eval`,
`xFuncPiece_ShiftPiece`, `xSndAddDelayed`, `zEntPlayer_SNDPlayStreamRandom`,
`xCutsceneConvertBreak`, `xFXRingCreate`, the three `xParCmd_*_Update`,
`zNPCTiki::SetCarryState`, `slugs_ready`; up without crossing:
`EffectSingleLoop` 99.78, `CalcJumpImpulse_Smooth` 90.1, `zGustUpdateEnt`
93.6, `NPCLaser::Render` 94.7, `zNPCBSandy::Process` 99.7.

**Shipped with it: clause E3n on scheduler entry 0** (one line in
`AliasPatch.c`, ahead of C+). +2 / -0: `eval_joint` and `iModelMaterialMulCB`,
the "entry-0 whole x whole" pair recorded above. Priced in isolation as
variant `e3n0` before combining.

**Corrections to earlier entries.**
- `0x511cb0` (`may_alias_object`, clause H's hook) is called from
  `computeusedeflists` and `precomputeusedefcounts` in UseDefChains, NOT from
  CodeMotion. The "CodeMotion.c ONLY" attribution in the clause H section is a
  module-map artefact (`computeusedeflists` sits at 0x56e8d0 directly before
  CodeMotion). Clause H works by making a static-array store a def of every
  small static in the usedef chains; C+ on entry 0 then answers.
- The "thousands of matched functions with hoisted literals" premise used
  to argue against a LICM change was false. Matched functions with a literal in
  a store-free loop (`shadowCacheLeafCB`, `zFXGooFreezeTimeLeft`, `iSndGetVol`)
  are blocked by `isuniquedefinition` or sit in an exit block.
- The patch-cost list is six, not seven: `NCIN_SleepyLamp_AR` is 99.73 under
  stock today, so it is a loss but not a "cost" by patchcost's definition.

**Measured NO-GOs.**
- Widening the predicate to subrange static reads (kind 0 OR 1): same +11,
  but **-5 from 100** (`xSER_xsgclt_svinfo_fill`, `get_next_quadrant`,
  `zNPCSpawner_GetInstance`, `NPAR_Upd_GloveDust`, `NPAR_Upd_MonsoonRain`).
  Retail does hoist subrange static reads; the whole-object test is
  load-bearing.
- Narrowing to `lfs`/`lfd` is indistinguishable from the general rule: the
  only whole-static reads LICM ever hoisted tree-wide were the 22 literal
  loads.
- Neither change moves the six-function cost list or `NightLightUVStep`
  (60.7 under stock, old patch and new: a scheduler reorder against a
  subrange static store).

**Tooling hazard.** `variant.py` unlinks `GC/2.0p1a/mwcceppc.exe` while
re-deriving. A frida sweep running at the same time died with
`ExecutableNotFoundError`, and units compiled inside the window could have used
the variant's bytes. Never run `variant.py` while any `solo`, `patchcost` or
frida job is using `GC/2.0p1a`.

One sub-100 move the frida sweep did not predict: `zEntPlayer_Update` 96.776
-> 96.677 on the real build. The frida test hooked `isloopinvariant` for
every caller; the shipped stub retargets only `moveinvariantsfromloop`'s
call, so `simpleunswitchloop` still sees the stock answer. No crossing lost.

## Source sweep of never-tried functions (2026-09-01, evening)

Targets were the game functions the doc had never mentioned, worked one unit
per agent under the worker brief. Every entry below is measured; shapes that
did not move are listed so nobody re-runs them.

**Banked without a function moving.** `gDebugPad` was defined in
`xEntMotion.cpp`; retail's `xPad.o` `.sbss` is `gDebugPad, gPlayerPad` and
`xEntMotion.o` has no `gDebugPad`. Moved the definition to `xPad.cpp`
(declaration in `xPad.h`). Both units' `.sbss` symbol sets now match.
`xSnd` passed `fliptest` and is Matching (98/224 game units).

**Data-section misses, triaged.** `xScene .sbss2` (0%, 8 b) is `get_grid_index`'s
4-byte zero template `@587`, the held `xGrid.h` change above. `xCollide .rodata`'s
missing `@12` is `xVec3::cross`'s zero template, the recorded DO-NOT-APPLY.
`zAssetTypes` emits an extra `.rodata @12` and `.sbss2 @8` that no `.text`
reloc on either side references. `xParEmitter .data` is three anonymous
zero templates of identical size and content on both sides, no relocs, and
objdiff still scores 78.75; cause unknown.

**`zNPCSpawner::Owned` (87.145) — NO-GO, ten shapes.** The 7-instruction SIZE
delta is our unroller folding eight `addi r3, r3, 0xc` base bumps into
immediate displacements `0xe0..0x134` plus one `addi 0x60` at the tail; retail
walks the base. Bit-identical at 87.145: `(npcpool + i)->npc`, `U32 i`,
`i != 16`, `this->npcpool[i].npc`, `!(… != npc)`. Worse: a separate
`const SMNPCStatus*` pointer local 83.455, `U8 found` + `break` 74.691. The
dead `addi r5, r5, 0x7` IV is on both sides.

**`_iGCUVRenderCallback` (iFX, 92.895) — NO-GO, two regions, 0 semantic
diffs.** Region 1: `(header + n*8) + 0x14` vs ours `header + (n*8 + 0x14)`.
Writing retail's association literally, in two spellings, emits ours
unchanged: the backend canonicalises it. Eleven index spellings measured,
best 92.895. Region 2: retail rematerialises four `lfs` at their `stfs` in 5
FPRs; we hoist all eight into the `Mtx = {0}` copy block in f0-f7. Named
locals, `= {}`, row-major ordering: 92.895 / 92.895 / 92.860. The
rematerialise-vs-hold class, inverted.

**`zEGenerator_TurnOn` (97.900) — NO-GO, 8 rows.** Group A: the `add.flags`
`stw` sits last before `bl zLightningAdd` in retail and nine slots earlier in
ours; moving the assignment above `add.rand_radius` in the source gives a
byte-identical object, so the position is scheduler-fixed. Group B:
`&egen->src_pos` in r5 (retail) vs r6, which flips the last two
`xColorFromRGBA` argument fills. `color` before `thickness` 95.896, `end`
before `start` 97.881. The file's nonmatch comment was rewritten to say this.

**`xShadow`, three moves, no crossing.** `const F32 sf[3][2]` (read-only
aggregate lever) takes `xShadowRenderWorld` 87.531 -> 98.315; `(U8)` instead
of `(S32)` on `param.shadowValue` puts the `fctiwz` temp in r8 and takes
`ShadowRender`/`xShadowVertical_DrawCache` to 99.943/99.952. All six residues
now have identical multisets. `InvertRaster` (89.495) is scheduler entry 4
(subrange stores of `vx[]` hoisted into `lfs` shadows), not opened.
`Im2DRenderQuad`/`PickByRayCast` are one hoisted store each. The last two rows
of `ShadowRender`/`DrawCache` are the `gRenderBuffer` address temp in r5 vs
r4; seven block permutations measured, all <= baseline. `min_t`/`max_t` swap
in `RenderWorld`: 97.842, reverted.

**`TimerUpdate__7xPsycheFf` (xBehaveMgr, 52.941) — NO-GO, 22 shapes.** The
target is `lwz/cmpwi/bltlr`, then two UNCONDITIONAL `b` to two byte-identical
arms: an `if (A || B) { X } else { X }` whose condition folded while both arms
survived. The `||` layout is reachable at exactly 68 bytes, but our compiler
either reuses cr0 with a conditional branch or deletes the dead arm; it never
emits `b`/`b`. Literal constants fold to one arm, re-tests of `staktop` give
`bge`/`bge`, `this != 0` and friends emit a real compare. Same family as
`DoAliveStuff`'s kept switch skeleton. The `staktop >= 0 || staktop < 0`
spelling would score ~88 and was not written.

**`zGame` / `zCutsceneMgr`, two moves, no crossing.**
- LEVER: an anonymous CSE temp takes GPR colour index 0 unless a declared
  local claims the value. `zGameUpdateMode` 99.088 -> 99.355: `zScene* scene
  = globals.sceneCur` declared after the four byte temps (order pinned to
  `b, d, a, c`; `a,b,c,d` and `d,c,b,a` 99.167, `b,a,d,c` 99.261) puts six
  values on retail's registers. Residual: the `subfc/stw/subfe` 64-bit store
  pair order, same at the identical site in `zGameLoop`.
- `zCutsceneMgrPlayStart` 97.914 -> 98.777 from `if (x.radius)` instead of
  `!= 0.0f` (operand order of `fcmpu`). Residual is retail reloading
  `alphaBits` after `stw s_atomicNumber`.
- NO-GO `zCutsceneMgrFinishLoad` (82.878) / `FinishExit` (85.151): retail
  keeps `to` and `t = (zCutsceneMgr*)to` as two locations (two callee-saved
  regs; a stack home in FinishExit; PS2 dwarf has `to` in memory in both).
  Split decl/assign, reference binding, `t` inside the loop, either alias at
  the final call, `for(;;)`: all bit-identical. `break` instead of `return`:
  68.976.
- `zGameUpdateTransitionBubbles` (80.000) and `zGameLoop` (93.949) are
  entirely the 64-bit static reload-after-store class (semdiff 4 and 12
  terms). `zGameScreenTransitionUpdate` / `zGame_HackDrawCard`: identical
  multisets, Im2D vertex stores interleaved by our scheduler.
- Recurring: retail rematerialises (`&globals.player.ent`, `li r0,0`,
  `alphaBits`) where we hold. Four sites this session.
- `solo.py zCutsceneMgr` once reported a phantom `check_hide_entities` row at
  91.047 on unchanged source; the next three runs did not. Same flake as the
  one recorded in `measuring.md` for the built object.

**`zNPCTypePrawn`: three crossings, one lever.** `update_sweep`,
`init_look_dir` (99.66 / 99.40 -> 100.0): `RwMatrix* mat;` declared bare
BEFORE `prawn`, assigned AFTER it. A bare declaration sets the callee-saved
colour (earlier declaration = higher register); the assignment position sets
emission order. This settles the OPEN CONFLICT entry: lexical declaration
point is the GPR colour key, definition point drives emission.
`load_patterns` (98.33 -> 100.0): retail's prologue `mr r28, r7 / mr r30, r6`
copies the 4th parameter into a HIGHER callee-saved register than parameter
order predicts, which means retail walked a local copy: `range_type* p =
pattern;` declared before `i`, and the loop walks `p`. Rule of thumb recorded.
- NO-GO `turning__9zNPCPrawnCFv` (87.870): identical multiset; retail hoists
  `lfs turn.vel` and `-1e-5f` above the `xVec2 facing` template stores.
  `result` declared last: bit-identical. Brace-init `facing = { mat->at.x,
  mat->at.z }`: 87.500.
- NO-GO `update_turn__9zNPCPrawnFf` (99.566): retail reuses the `1` from
  `decel = true` (`mr r4, r3`), we rematerialise. Swapped `==` operands
  98.648; `bool decel` hoisted beside `time_to_target` 97.005.
- `zFX`: `SkinXformVertAndNormal` 85.391 -> 85.504 from `mask` before
  `shift` (mask on r27); the rest is a 3-cycle on r26/r27/r28 plus prologue
  scheduling, `done,scratch,mask,shift` 85.353. `validate_popper` (91.651):
  the 12-byte `scale` copy uses two scratch regs where retail uses three; bare
  `xModelInstance* m` and hoisting `m = ent.model->Next` both bit-identical.
  `set_popper_alpha` (92.0): epilogue `lwz r0, 0x24(r1)` placement only.
  `zFX_SpawnBubbleWall` (96.774): `xVec3 offset;` at loop-body top is
  bit-identical.

### Worked directly, 2026-09-02 (fable, no agents)

- **`xCMrender` 92.719 -> 99.921.** Real source differences, all faithful to
  the target: `xfont::create`'s spacing argument is `0.0f` in all three calls
  (ours read `char_spacing.x`); the second text box is created with
  `box0->font`; `x0` is ONE outer local (declared before `a`), computed
  before the colour call in case 0, accumulated with `+=` for the second box
  and reused for the texture quad; `x0 = 0.5f * (1.0f - box0.x - box1.x -
  innerspace)` in that order; the return is the bool expression (`clrlwi`).
  Case 4's corners: bare `F32 y1; F32 x1; F32 y0;` before the assignments
  gives retail's f27/f29/f31. Last two rows: `tex->x`/`tex->y` CSE temps
  f5/f4 vs f4/f6; named temps, both orders, and six computation orders
  measured, none better.
- **`ArcLength3` (xSpline) 88.023 -> 100.0.** Three levers: (1) `E`'s three
  squares as named `F64` locals (retail: fmul, fmul, fadd, where an inline
  expression fuses) 88.0 -> 94.4; (2) the nine coefficient loads as named
  `F64` temps and the eight loop doubles DEFINED in the order E, D, C, B, A,
  h, sum, u, which is retail's callee-saved FPR order f31..f24 -> 99.06;
  (3) temp declaration order `y0, x0, z0, x1, y1, z1, y2, x2, z2` for the
  volatile colours -> 100.0. Retail's source never loaded the coefficients
  into the loop variables; ours did (A = x.a[0] ... reassigned later).
- **`xQuatMul` (xMath3) 84.243 -> 86.297.** Retail seeds each fmadd chain
  with the RIGHT product of the first `+`: `a.s*b.x + a.x*b.s + a.y*b.z -
  a.z*b.y` per component, statements x, y, z, s. Rest is FPR colouring;
  z,y,x order with the new association 73.757.
- **`iSG_mcidx2slot` (isavegame) 90.391 -> 95.672**: `*out_slot = -1` as the
  first statement. Residue: retail reads the parameter from r25 (its
  callee-saved copy) for that store and uses r4/r3 as template temps; ours
  reads r4 and takes r8/r7. Nine declaration/statement orders measured
  (90.156 .. 93.172), same under stock `GC/2.0p1`.
- NO-GO `impart_velocity` (Plankton, 91.127): the `diff` template load is
  hoisted above the `add` stores. `add.y = 0` after `diff` 87.746; split
  decl 84.789; `max_dist` before `diff` 35.282; `loc` reference 70.451.
- NO-GO `zCameraTweak` (all four): static loads hoisted over static stores;
  stock compiler 36-80, patched 88-95, the residue is the uncovered entries.
- NO-GO `LOD_r_PLAT` (xpkrsvc, 87.5): the four `char[32] = {}` copies are
  scheduled interleaved by retail, hoisted by ours; the `@1244` vs
  `.rodata+0` reloc names are the same address.
- NO-GO `TTGunSmoke_AR` (zNPCFXCinematic, 83.5): the `0.0f` of
  `MAX(0.0f, MIN(rat, 1.0f))` loaded into the `fdivs` latency; same as
  `ConeOfRange`.
- NO-GO `player_left_territory` (Plankton, 96.524): retail forms
  `&territory[i]` (addi) for `.platform` and folds `.crony_size`;
  `territory->platform` 93.310, mixed `territory[i].crony_size`/`t.platform`
  bit-identical.
- NO-GO `zNPCGlyph_ScenePrepare` (96.873): retail copies `i` into r29 for the
  inner loop (one more callee-saved). Named `en_npcglyph` local, inline and
  bare-declared: bit-identical.
- NO-GO `xSerial::Read` (94.167): per-arm pointer locals 93.333. Retail's
  `mr r30, r31` copy of a zero is rematerialise-vs-copy.
- NO-GO `iScrFxCameraCreated` (90.083): retail materialises the pointer NULL
  and the U16 zero separately; `(RwRaster*)NULL` bit-identical.
- `NPCC_BuildStandardAnimTran` (96.05): retail reloads `ourAnims[i]` for the
  `==` test after the `!= 0` test (cross-block CSE class).
- **REFUTED: `xatan2` returning `double`** (the lead above). With `F64
  xatan2(F32, F32)` in `xMathInlines.h`, `update_turn__12zNPCDutchmanFf`
  emits `frsp f31, f1` AT the `cur` assignment and `fsub`+`frsp` for `diff`;
  retail has `fmr f31, f1`, `fsubs`, and the lone `frsp f0, f31` only at
  `angle + diff`. 94.308 -> 91.000; header reverted before any sweep.
  `F64 cur` 89.292; `F64 cur` + `F64 diff` 86.215. The frsp is on a value
  the compiler otherwise treats as single (it `stfs` it unrounded), so it is
  not a declared-double local either. Still open, and it is the same row in
  `update_turn` of Plankton and SB2.
- **`zGustUpdateEnt` 93.554 -> 94.098, a SEMANTIC fix.** In the `dt >=
  lerpinc` branch retail tests `!(data->lerp[i] < 0.0f)` (pool @789 = 0),
  ours tested `< 1.0f`. Residue: GPR colour permutation (data r28/i r30/
  minidx r29 vs ours r30/r29/r28), the unrolled `j` loop's `li` before the
  `lwz`, and retail storing `gust_on = 0` from a held r31 where we
  rematerialise. `data` declared before the `gusts` test 92.739; after
  `coll` bit-identical.
- `xCutscene_Render` (96.666): `(numFrame * 2 + numRun * 2 + 5)` — retail
  keeps two `slwi` and adds; ours factors to `(a + b) << 1`. `2 * a + 2 * b`
  bit-identical; `a*2 + (b*2 + 5)`, `a*2 + 5 + b*2`, `5 + a*2 + b*2` all
  95.403. Also `lwzu` vs `addi`+`lwz` for the 16-byte NULL template of
  `v_array`. Not closed.
- `zParPTankBubbleUpdate` (95.779): ours hoists `plock.data`/`uvlock.data`
  reloads above `stfs xp->life` and holds one more callee-saved; the
  locals' addresses escaped to `RpPTankAtomicLock`, so this is the
  alias-hoist class.
- `Show_frame` (iFMV, 96.847): retail reloads the u32->double magic `lfd`
  for the second conversion in the same block; we CSE it. Literal-reload
  class.
- **`update_turn` x3 (Dutchman 94.308, Plankton 94.308, SB2 96.552): the
  `frsp` is on the yaw copy, and twelve shapes are bit-identical or worse.**
  All three retail bodies emit `frsp f0, f31` at the wrap-branch join, then
  `stfs f31, 0x8(r1)` for the by-reference argument, then `fadds f3, f0, f3`.
  Measured on Dutchman: `angle` removed and `cur` passed by reference 74.431
  (same on Plankton; SB2 83.598); the copy moved above the wrap branches
  88.615 (SB2 92.874); `F64 cur` 89.292; `F64 cur` + `F64 diff` 86.215;
  named `target = angle + diff`, `diff + angle`, `F32& ref = angle`, and bare
  `F32 angle; angle = cur;` all bit-identical at 94.308. In matched code the
  same `frsp` appears on a value that was stored to an address-taken slot and
  read back (`CheckObjectAgainstMeleeBound`: `stfs f1; frsp f0, f1; fcmpo`),
  and after `fabs`/`fneg` (`xGridInit`, `xQuickCullForSphere`); none of those
  constructs is present here. The build is otherwise DOL-clean at 7308/7673.

## Clause S: a static of at most 8 bytes is one alias unit (2026-09-22)

Shipped. `GC/2.0p1a` sha1 `d607436cef80246fa74bce4293eb2a997195292d`. Full
`ninja`: game **matched_functions 7308 -> 7313 (+5 / -0)**, matched_code
81.968 -> 82.157, fuzzy 99.353 -> 99.370, DOL `306526d9...` intact, no function
anywhere lower in `report.json` (rwsdk `_rpGeometryOpen` also crosses).
`patchcost.py --stock` against the same compiler with the clause compiled out:
+5 functions / 3,100 bytes, 0 lost.

**The defect.** `make_alias` (Alias.c) gives an access to part of an object a
subrange alias (kind 1). Two subranges of one object that do not overlap never
alias (`may_alias_alias` case 1x1), and a store to one subrange only kills that
subrange in value numbering (`update_alias_value` AliasType1). Retail treats a
static object of at most 8 bytes -- the `-sdata` threshold, i.e. anything in
small data -- as one unit:

- `sTimeCurrent = iTimeGet()` (an `S64`): retail reloads both words after
  storing them; we forwarded the registers. `zGameUpdateTransitionBubbles`,
  `zGameLoop`, `zSaveLoad_Tick`.
- `gTrcDisk[0] = state; gTrcDisk[1] = ...` keep their order (`xTRCDisk`).
- `sAuraPulseAng[0]`/`[1]` (`xFXAuraUpdate`), the two `F32[2]` UV statics in
  `NightLightUVStep`, `sCamTweakDistMult` in `zCameraTweak`.

**The clause** (`small_static_whole` in AliasPatch.c):

- scheduler entry 4 (subrange x subrange): both sides subranges of static
  objects of at most 8 bytes -> the same object always may-alias; two
  different objects may-alias under clause A's test (differing opcodes, plain
  accesses, both at most 4 bytes);
- VN entry 1: a store to such a subrange records a fresh value number instead
  of the stored register (clause F for subranges).

Each half is load-bearing (frida A/B on the shipped compiler): without the
same-object rule +1, without the different-object rule +4 and one partial
down, without the VN half +4 and one partial down. The size bound is
load-bearing: 12 bytes +5/-3, 16 bytes +5/-4, unbounded +5/-119. Killing the
sibling subranges in VN as well is inert. Excluding anonymous `@NNN` objects is
inert.

A whole-alias rewrite at `make_alias` (the direct model: return the whole
alias for a subrange of a small static) reaches a different set: +4/-1, five
partials down, because the whole alias then has size 8 and falls outside
clauses A/C/C+, whose size tests were fitted on access size. Excluding `@NNN`
templates takes it to +4/-0 with the partials still down. The predicate form
is strictly better.

**Injection layout changed.** The blob grew to 1,118 bytes and no longer fit
the grown page. It now starts at `0x57ea50` in the original cave and runs
into the grown page; the eight stubs follow it (1,367 of 1,460 bytes used).
Blob sections are packed at 4-byte alignment instead of 16. Refactor checked
by compiling the clause out: all 224 game objects byte-identical to the old
compiler's.

**Direct stores also run clause V's walk (2026-09-22, +1 / -0).** A store that
names part of a small static directly (`stfs f0, sCamTweakPitch@sda21`) kills
the cached small statics the way a whole-object store does:
`zCameraTweakGlobal_Reset` reloads `0.0f` after it. A store to the same half
through a pointer (`stfs f0, 0x4(r4)`) does not: `zCameraTweakGlobal_Remove`
holds `1.0f` in `f2` across two of them. Running the walk on every
small-static subrange store gains the same function and drops those two
partials; running it on every direct subrange store of any static costs 2
exact functions. Applying clause S's forwarding half only to direct stores is
indistinguishable from applying it to all of them on this corpus. New sha1
`c1241e54e45c258cca85d5860b6a911e2f82db2a`; game 7314, matched_code 82.162,
fuzzy 99.371. The injected region now has 29 bytes free.

**The patch now lives in its own section.** The .text tail had 29 bytes left
after the direct-store walk. `patch_compiler.py` now appends an executable
`.sbpatch` section (4 KiB at RVA `0x20e000`, the stock SizeOfImage; the file
ended exactly at `.reloc`'s raw data and the section table had a free slot
below SizeOfHeaders) and puts the blob and all eight stubs there; `.text` is
no longer grown and its cave is untouched. 542 objects byte-identical before
and after on a full build, sjiswrap units included. New sha1
`5c4e8e29f9d24079bb1f52d4d79bb3ec30bd4566`; about 2.7 KiB free.

**Two small statics are compared the way entry 0 compares whole objects
(2026-09-22, +2 / -0).** Clause S's different-object test was clause A alone,
so two same-opcode stores to different small statics could still pass each
other: `xFXanimUVSetAngle` stores `xFXanimUVRotMat0[1]` (through a pointer)
before `xFXanimUVRotMat1[0]`, and retail keeps that order. The test now
mirrors entry 0 under the same 4-byte gate: clause A for differing opcodes,
clause B for two stores of one opcode. `xFXanimUVSetAngle` and
`xFXanimUV2PSetAngle` cross, nothing moves down. Clause B's own tests are
satisfied by any subrange (it belongs to its whole object and contains
nothing), so no new predicate was needed. sha1
`e4b080e02f4437e788524b14e9736089c86a9d14`, game 7318, matched_code 82.307,
fuzzy 99.382.

Moved but not closed: `zGameLoop` 99.979, `xFXAuraUpdate` 99.838,
`zCameraTweakGlobal_Add` 96.331, `NightLightUVStep` 67.700.

## Clause A needs a static side (2026-09-22)

Shipped. `GC/2.0p1a` sha1 `61ab511748b3dc08df62f55a72e28218c72dac7a`. Full
`ninja`: game **7314 -> 7316 (+2 / -0)**, matched_code 82.162 -> 82.296,
fuzzy 99.371 -> 99.381, DOL intact. One partial down:
`SkinXformVertAndNormal` 85.504 -> 85.451.

An ablation of every scheduler clause against the clause-S compiler found one
that costs more than it pays. Clause A (entry 0: two whole accesses of at most
4 bytes, differing opcodes, plain) had no storage test, so it also serialised
two frame objects. Retail does not: `LOD_r_PLAT` interleaves its frame-slot
copies, and `xFXRenderProximityFade` (the recorded entry-0 over-fire witness)
hoists a frame reload over a frame store. Removing clause A outright is
+2 / -1 (`xShadowManager_Render` needs it for a frame load against a store to
`sEntSelf`). Requiring one side to be a static object keeps that and drops
the frame-frame pairs; "not both frame objects" measures the same.

Ablations of the other scheduler clauses on the same compiler, each removed
alone (exact functions lost): C on entry 1 -35, B on entry 1 -16, C on entry
3 -86, B on entry 3 -6, E3n on entry 0 -2, C+ -57, B on entry 0 -30. None
is free.

## Clause H is gone: the LICM call-site hook subsumes it (2026-09-22)

Removed. `GC/2.0p1a` sha1 `b4f01e81afc552380a76dd5b3f7ea790aeee42b0`. Full
`ninja`: **542 objects byte-identical** to the previous compiler, 7316 / 7673,
DOL intact.

Clause H made a store to a static array in a loop a def of every small static
in the use-def chains, so `isloopinvariant` kept a literal load in the loop.
The later `sb_licm_invariant` hook refuses every whole static read at
`moveinvariantsfromloop`'s call regardless of defs, which covers the same
loads. Returning 0 from `sb_licm_clause` changed no object anywhere, so the
predicate, its stub, the jump rewrite at `0x511ce5` and the retyped
relocation all went.

Ablations of the other non-scheduler pieces on the same compiler (exact
functions lost when removed alone): clause F -39, clause V's walk -38, the
LICM hook -11. E3n on entry 3 is -120 / +5, and those 5 are five of the six
functions on the patch-cost list (`xBoxFromCircle`,
`zNPCGoalJellyBirth::Process`, `BasisBspline`, `zEntPlayer_AnimTable`,
`zNPCGoalPatrol::MoveNormal`); the sixth, `zNPCFodBzzt::Setup`, is recovered
by no single ablation.

## Clause E3n counts indirect stores into frame arrays (2026-09-22)

Shipped. `GC/2.0p1a` sha1 `1d1bae88d9550883de90779e85cabcb9983e29ac`. Full
`ninja`: game **7318 -> 7322 (+4 / -0)**, matched_code 82.307 -> 82.726,
fuzzy 99.382 -> 99.386, DOL intact, nothing down.

This closes the "literal load hoisted over an INDEXED frame store" candidate
recorded under zNPCTypeBossPlankton. A store through a pointer into a local
array carries the indirect bit (0x20, set by `gather_alias_info` when the
alias is the whole object rather than the accessed word), so E3n's
`flags == 4` test declined it. Retail keeps the literal load after
`stwx` into `anim_list[]` (`ZNPC_AnimTable_BossPlankton`,
`ZNPC_AnimTable_BossSB2`), after `stfs` into `pos[100]`
(`zFX_SpawnBubbleWall`) and after `stfs` into `tranresult[]`
(`xcsCalcAnimMatrices`). The clause now tolerates the bit, as clause C+ did
for clause C, with two gates, each backed by one witness:

- the store writes a word or more: `zMainFirstScreen`'s `stb` into
  `text[617]` lets a template load pass (without this gate, -1);
- the array is larger than 16 bytes: `zNPCBSandy::Process`'s `stfsx` into a
  12-byte local lets `1.0f` pass (without this gate, one partial down).

Measured variants: indirect bit tolerated with no gate +4 / -1; float loads
only +4 / -1.

## Clause W: E3n's write-after-read half, for float literals (2026-09-22)

Shipped. `GC/2.0p1a` sha1 `a78a5fdb6c1d5677e987636b2e0743dbaefe9542`. Full
`ninja`: game **7322 -> 7357 (+35 / -0)**, matched_code 82.726 -> 84.288,
fuzzy 99.386 -> 99.421, DOL intact. Four partials down:
`add_trail_sample` 97.661 -> 91.457, `RendConeRange` 89.021 -> 85.967,
`DiscoRender` 81.053 -> 79.620, `NPCCone::RenderCone` 99.056 -> 98.925.

**The rule.** A plain store to a declared frame local (Object+0x18 non-zero,
E3n's frame gate) may not pass an earlier `lfs`/`lfd` from the literal pool
(an anonymous static of at most 8 bytes). E3n is the read-after-write
direction (store, then a later static load); this is the write-after-read
direction, and only for float literals. The alias edge carries the load's
latency, so retail leaves the store behind it. That is the "retail leaves the
stall" shape these notes recorded as the entry-4 plurality of the SCHED list:
`BoulderRollCB`, `BoulderRollDoneCB`, `xEntDriveMount`, `xEntDriveUpdate`,
`xSphereHitsOBB_nu`, `InvertRaster` and `NPCC_LineHitsBound` all cross. The
store's frame object and the literal are different objects, so the pair
reaches the clause on entries 0 and 1, not entry 4.

**How it was found.** `scratchpad`-style frida probe: every `may_alias` query
tagged with a feature signature (opcode class, indirect bit, direct operand,
storage class -- literal / named static / declared frame / temporary --
subrange or whole, size bucket), counted per function, then each signature
ranked by how many non-matching functions contain it against how many
matching ones do. Flipping the top signature alone was +13 / -5; the five
losses were all stores into `const` locals (flag 0x40), and widening from
`stw` to every plain store took it to +35 / -0.

**Measured variants (frida, against the E3n-indirect compiler):**

| variant | result |
|---|---|
| shipped: lfs/lfd literal, any plain store, declared frame | +35 / -0 |
| `stw` stores only | +13 / -0 |
| integer loads too (lwz of templates) | +39 / -43 |
| named statics too | +13 / -2 (lfs only) |
| compiler temporaries too | +31 / -176 |
| indirect stores, or static stores, too | identical (never reached) |
| store size up to 8 | identical |

## Measured NO-GOs and the remaining residue (2026-09-22)

Every entry below was measured tree-wide (224 game units, objdiff exact
counts) against the compiler of its day. Tool: `tools/frida/sigrank.py`.

**Value numbering / constant CSE**
- `li` CSE (`isCSEop`, ValueNumbering.c): the `mr`-for-`li` witnesses
  (`xAccelMove`, `zTalkBox wait_state::stop`, Prawn `update_turn`,
  `xSerial::Read/Write`, `iSndInit`) all need a user-variable `li` to be CSE'd.
  Allowing it: lower bound at the real registers -320, no bound -871, CSE only
  into temporaries from a user variable -202, cross-block only -81. Disabling
  `li` CSE -1091. The stock register-range test is right; the witnesses are
  source shape or something upstream.
- Constant propagation of `mr` (`propagateconstantstoblock`): disabling it is
  -1 / +0.
- Clause V's walk killing 8-byte literals: on every store -2, on static
  stores only 0 / 0 (`Show_frame` up, `iParMgrInit` down); on stores through a
  pointer to a large static 0 / 0; killing large static wholes -6.
- Clause V's walk skipping stores to compiler temporaries: 0 / 0, four
  partials up (`xFXStreakRender` 76.8 -> 92.2, `NCIN_SleepyLamp_AR` 94.0 ->
  99.7, `xFXShineRender` 94.1 -> 97.9, `xScrFXGlareRender` 62.4 -> 65.0), none
  down. Held because it crosses nothing; the residue is register numbering.
- Store-kill rules ranked from a VN probe (small-static store kills cached
  large-static loads, the `zThrown_AddFruit` / `zCutsceneMgrPlayStart` shape):
  0 / 0.

**Scheduler**
- Small-static rule on frame objects (<=8, 12, 16 bytes): -223 at best.
- Store order within one declared frame aggregate: +10 / -365.
- E3n on entry 1 -7, on entry 4 +1 / -192; E3n restricted to literal loads
  -17, to named statics +5 / -112 (the five are E3n's patch-cost list).
- C+ on entries 1 and 3 -5; C+ allowing 8-byte literals after an indirect
  static store +2 / -1 (`zMusicDo`).
- Clause B replaced by "both static" -1.
- Frame-to-frame write-after-read -1 to -8.
- The top 45 signatures of the post-W ranking, flipped one at a time: at most
  +2, nothing clean.

**Other passes**
- Alias propagation through pointer induction variables (stores through an
  IV pointer answered as worst case): +6 / -12 (`iParMgrInit` 70 -> 100 among
  the gains); loads too +4 / -74.
- `find_entry`: `#pragma opt_strength_reduction off` gives retail's loop
  shape exactly (an offset IV plus `add`, the address kept across the call);
  only register numbers differ. The other two offset-IV witnesses do not move
  with the pragma, and rejecting IRO's pointer-form `Reducable` costs 406.

**What is left.** 316 game functions. Mechanical buckets (`classify` on the
diff, registers abstracted):

| bucket | count | reading |
|---|---|---|
| reorder + register renumbering | 93 | mostly allocator colour order |
| pure reorder | 31 | 22 invisible to alias (ALU pick order), 9 memory pairs |
| register renumbering only | 68 | allocator |
| `li`/`mr`/`addi`/`fmr` deltas | 40 | rematerialise-vs-copy, see above |
| retail reloads a value we reuse | 22 | half source (templates), half VN |
| we reload what retail reuses | 6 | clause V/E3n over-fire |
| branch form | 7 | kept switch skeletons, inverted tests |
| other small deltas | 20 | `frsp` x3, `clrlwi`, IV shapes |
| large differences | 20 | source |
| frame layout | 1 | source |
| missing | 8 | not written |


## Plankton final two functions (2026-10-01)

`player_left_territory` improves from 96.524 to 97.357 with an explicit
cached `active_territory` index and the `platform` local declared before it.
This fixes the r6/r7 allocation throughout the function. The remaining
difference is retail's `addi r5,r3,1204` inside the loop followed by
`lwz r0,4(r5)`; GC/2.0p1a folds this to `lwz r0,1208(r3)`, making the
function four bytes short.

The same source produces an exact `player_left_territory` under stock
GC/2.5. This is diagnostic only: that compiler regresses other functions
in the unit. Stock GC/1.3.2r, GC/2.0 and GC/2.0p1 retain the folded
address. Equivalent pointer/member accesses, manual pointer induction,
loop control-flow rewrites and local optimizer pragmas did not recover
the missing instruction under the configured compiler.

`impart_velocity` remains at 91.127. Return-value qualifiers, aggregate
wrappers, local qualifiers, initializer forms and optimizer-pass switches
did not resolve the return-copy/template-load scheduling difference.
All unsuccessful experiments were reverted; no compiler changes or
assembly workarounds were retained.

The unit remains NonMatching at 178/180 functions, 30,756/31,208 exact
code bytes and 6,728/6,728 data bytes. The normal full build still
reproduces the retail DOL SHA-1, with no other SB matching regressions.
This does not validate source-linking Plankton: its unmatched object is
still supplied by the original binary.


## Save/load follow-up (2026-10-01)

`iSGReadLeader` improves from 99.000 to 99.875 with an explicit
`if (iSG_mc_fread(...) != 0) readret = 1; else readret = 0;`, matching
the success/failure style already used by `iSGSaveFile`. The compiler now
emits retail's direct `srwi r31,r0,31` instead of a temporary result plus
`mr r31,r0`; the function is the retail 480 bytes. The only remaining
difference is allocation size held in r22 where retail uses r29 for three
instructions before reusing r29 for the allocation pointer. Local order,
scopes, temporary pointers, scalar types and one-element arrays did not
close it; those experiments were reverted.

Other measured holdouts remain unchanged: `zSaveLoad_Tick` moves the
active-pad byte load across the two `sTimeLast` stores; volatile accesses
did not change this and memcpy introduced a call. `iSG_mc_format` and
`iSG_mc_fdel` retain their branch-direction differences under equivalent
error/default case groupings and localized optimizer settings. The
`iSG_mcidx2slot` initializer-order variants did not improve its current
95.672 score. No compiler changes were retained.

The full build reproduces the retail DOL SHA-1; no other SB function
matching scores regress. `isavegame` remains NonMatching with four
holdouts and 100% data.

## Robo source linking (2026-10-01)

zNPCGoalRobo now links from source with all 361 functions and all 5,584
data bytes matching. Robot goal declarations belong together in
zNPCGoalRobo.h; their inline constructors previously straddled the Std
and Robo headers. Shared inline definitions now live with their classes
or helper declarations, and ROBO_PrepRoboCop precedes its two callers.

Five explicitly named dead-stripped layout stubs reproduce inline emission
groups and the first-use positions of 2.5f and PI. These are reconstruction
scaffolding, not claimed recovered debug routines. The linker discards the
stubs themselves; no assembly or output-binary modification is involved.

The full source build preserves every other SB function matching score,
and the linked DOL reproduces retail SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6.

## Math and collision-geometry source linking (2026-10-01)

`zCollGeom` already has matching function order and data layout under the
current compiler. Promoting its four functions (868 code bytes, 32 data
bytes) to Matching reproduces the complete retail DOL without source edits.

`xMath` has 18 matching functions (3,252 code bytes, 120 data bytes), but
its shared literals were emitted in a different order. Two explicitly named
dead-stripped stubs reproduce the early scalar constants and PI/-1 first-use
positions. They are layout scaffolding, not recovered original routines.
The linker discards the stub code and the unused xsqrt literal; the resulting
DOL is byte-identical to retail. No assembly or binary patch is involved.

A separate `xEntDrive` promotion failed because its generated tri_data
assignment appears between normal functions. An explicit inline assignment
fixed the per-object ordering, but changed global helper ownership and
regressed zEntPlayer. That experiment was fully reverted; xEntDrive remains
Equivalent.

## Textbox, Villager, and Boss Patrick source linking (2026-10-01)

All three units now link from source and reproduce the retail DOL SHA-1:

- `zTextBox`: 26/26 functions, 2,916 code bytes, 240 data bytes. The current
  source already has the correct layout; only its Matching marker changes.
- `zNPCTypeVillager`: 122/122 functions, 19,316 code bytes, 5,120 data bytes.
  Its first link differed by 13 bytes because 0.5f and 60.25f were emitted
  in reverse order. One documented dead-stripped reference before Fish Reset
  puts the shared half constant in the original position.
- `zNPCTypeBossPatrick`: 71/71 functions, 35,152 code bytes, 3,584 data bytes.
  Its first link differed by 671 bytes. A dead-stripped reference before
  UpdatePatrickBossCam emits -1.0f and the signed-integer conversion bias
  before 2.0f, restoring the shared pool order.

The two added stubs are explicitly identified layout scaffolding; their
original stripped routines are unknown. No live function behavior, assembly,
compiler patch, split boundary, or output binary is changed.

## Surface and HUD meter source linking (2026-10-01)

`zSurface` now links from source. Its initial full-link test differed by 43
bytes because the signed-integer conversion bias appeared after the UV
animation constants. A documented dead-stripped conversion reference after
GetSlideStopAngle puts that shared literal in the retail position.

`xHudMeter` also links from source. Enabling `-sym on` moves the inline
helpers out of the middle of the main functions. Two dead-stripped references
then establish the sound_queue group before the math group, and the std::powf
group before meter_asset helpers. This reproduces all 15 function positions
without modifying a shared header or any live function body.

The added stubs are explicit layout scaffolding, not recovered original
stripped routines. Both promotions reproduce retail DOL SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6.

## HUD source linking (2026-10-01)

`xHud` now links from source. Main functions follow retail order: disable,
setup, update and render dispatch precede motive management, callback dispatch,
model loading and rendering. Existing template instantiations then fall into
the correct order without changing their implementations.

The empty debug renderer is inline in xHud.h. The asset type moves to
xHudAsset.h so its type-name helper has its own emission group. Placement-new
and xColorFromRGBA move from xHud.cpp to new.h and xColor.h, respectively,
restoring their weak inline linkage. A labelled dead-stripped reference keeps
the otherwise-unused color helper owned by xHud; the existing stripped %d
reference precedes the model-extension strings. No split changes are needed.

All 67 functions are in retail order, the full source tree compiles, and the
linked DOL reproduces SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6.

## Debug and shrapnel source linking (2026-10-01)

`xDebug` now links from source with 16/16 functions, 808 code bytes and
16 data bytes. The explicit iColor_tag copy-assignment definition moves to
xFont.h, retaining its declaration in iColor.h, so the emitted helper joins
the font group before NSCREENY/NSCREENX. Removing the explicit assignment
also fixes that order, but changes aggregate-copy compilation in xFont and
Robo; that trial was rejected. The retained change preserves the exact
assignment body and its user-defined-copy behavior.

`zShrapnel` now links from source with 37/37 functions, 11,424 code bytes and
21,208 data bytes. `-sym on` separates its weak helpers from the main code.
The remaining 200 linked bytes came from shared literal order. Two labelled
dead-stripped stubs establish the early float constants and the double 3.0
reference before sound-update constants. These are layout scaffolding, not
claimed recovered routines.

Both promotions reproduce the retail DOL SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6; no split or compiler-patch changes
are needed.

## Shadow source linking (2026-10-01)

`xShadow` now links all 41 functions (14,648 code bytes) and 74,656 data
bytes from source. Main definitions follow retail order, including setup and
light configuration, raster/camera helpers, manager removal and the two
shadow-picking helpers. Their existing function bodies are retained.

`-sym on` separates weak helpers from main code. The empty draw helpers move
to xDraw.h and SQ moves to xMathInlines.h as inline definitions. Explicitly
named dead-stripped references retain those otherwise-unused helpers and
restore the first-use order of fraction, epsilon and conversion constants.
The original stripped routines are unknown; these references are layout
scaffolding and contribute no linked code.

The full source build reproduces retail DOL SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. No split or compiler-patch changes
are needed.

## Entity-drive source linking (2026-10-01)

`xEntDrive` now links all 6 functions (3,380 code bytes) and 16 data bytes
from source. Its implicit xCollis::tri_data assignment was emitted between
Mount and Dismount, while retail places it after Update. A labelled
dead-stripped header wrapper and early reference instantiate the implicit
operator in the header's weak group, preserving all original function bodies.
These two stubs are layout scaffolding, not claimed recovered routines.

Making that assignment operator explicit also moves it, but changes nested
aggregate-copy semantics and regresses zEntPlayer's xEntCollis assignment.
The retained approach preserves the implicit operator and has no SB match
regressions. The full source build reproduces retail DOL SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. No split or compiler-patch changes
are needed; no independent original linker map was found to substantiate
the alternative bad-split hypothesis.

## Memory-manager match and source linking (2026-10-01)

`xMemGetBlockInfo` now matches all 304 bytes. The optional block-header
size uses the flag's conditional value (sizeof(xMemBlock_tag) or zero)
rather than a manually expanded mask. A one-element temporary preserves
the retail register allocation; the scalar forms coalesce it with different
locals. This is a documented compiler-layout workaround, with no assembly
or compiler changes.

`xMemMgr` now links all 21 functions (2,264 code bytes) and 8 data bytes
from source. The all-source build passes, no SB function match scores
regress, and the linked DOL reproduces retail SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6.

A separate register-allocation capture of Plankton's `impart_velocity`
confirmed that the unwanted template-load scheduling is already present
at graph-coloring entry. Its five copy registers are unnamed compiler
temporaries; local declaration reordering cannot repair that schedule.
No Plankton source change was retained.

## Bink Huff4 byte-bundle follow-up (2026-10-01)

`CheckReadHuff4Bundle` improves from 88.90909% to 99.56364%, retaining
the retail 660-byte size. Its byte-store helper is now a macro, advancing
the output pointer before updating the bitstream. The bit count is read
after the byte store instead of reusing a cached count. The one-bit mode
read uses the existing branch-local extraction macro; the unsigned
countdown and shared peek-width/repeat-symbol temporary also follow
retail's instruction shape. Remaining differences are register allocation
and two setup-load positions. No assembly or compiler changes are used.

`python tools/check_bink_huff4.py` extracts the production macro and checks
it against an independent bit-at-a-time reader: 67,584 cases across all
0..32 buffered-bit counts and 1..8 peek widths, plus 1,000 unsigned-countdown
cases. This is a focused host-side check, not an end-to-end movie test.
The full source build passes, with no other function-score regressions;
the normal link still reproduces the retail SHA-1. `expand` remains
NonMatching and is still supplied by the original object in that link.

## Bink paired Huff4 match (2026-10-01)

`CheckReadHuff4PairBundle` now matches all 604 bytes (87.072845% -> 100%).
The pair reader uses the ordinary Huff4 helper for each symbol, derives
each mask at its lookup, and starts its destination cursor after recording
the bundle bounds. Its unsigned countdown follows the already-matched
instruction shape used by the byte reader.

The shared helper now reads the bit buffer directly instead of forcing
both branches through one cached local. This gives each path its retail
register allocation. It also improves `CheckReadRLEHuff4Bundle` from
93.27119% to 94.48588% and `CheckReadHuff4SBundle` from 92.53081% to
93.791466%, without regressing any other function score. No compiler,
assembly, or artificial storage workaround is needed.

`expand` advances to 8/16 exact functions, with 1,852/13,964 exact code
bytes and all 1,272 data bytes matching. It remains NonMatching. The host
checker now compares two consecutive production-helper reads as well as
the byte-store macro against the independent reader in each of its 67,584
cases (135,168 helper reads), plus 1,000 countdown cases. The full source
build and normal retail DOL SHA-1 check pass.

## Bink Huff8 low-symbol cleanup (2026-10-01)

Both Huff8 bundle readers now use the ordinary Huff4 helper for their low
symbol instead of explicitly sharing a precomputed mask. This improves
`CheckReadHuff8Bundle` from 91.891624% to 93.44827% and
`NewCheckReadHuff8Bundle` from 94.25128% to 96.02564%. The newer reader
has the exact 780-byte size; the older one is still four bytes short.
The unused masked helper and its caller-side mask locals are removed.
No other function score regresses, including the paired Huff4 match.

The existing production-helper/byte-store host checks pass, as do the full
source build and normal retail DOL SHA-1 check. `expand` remains
NonMatching at 8/16 exact functions. Byte-reader direct/scoped bit-buffer
trials did not improve its retained 99.56364% and were reverted.

## Bink signed Huff4 conversion placement (2026-10-01)

`CheckReadHuff4SBundle` improves from 93.791466% to 98.78199%, now at
the exact 844-byte size. A signed-byte Huff4 helper performs the conversion
in both decoding paths, matching retail's two sign extensions instead of
converting the unsigned helper's result at the join. The unsigned countdown
and decoder setup order remove the remaining size difference. Register
allocation, a result-register copy and some load ordering still differ.

The separate signed helper leaves the unsigned callers unchanged. No other
function score regresses; the paired Huff4 decoder stays exact. The host
checker now also verifies 67,584 signed reads, alongside 135,168 unsigned
reads, the byte-store checks, and 1,000 countdown cases. The full source
build and normal retail DOL SHA-1 pass. `expand` remains NonMatching.

## Bink plane block dispatch (2026-10-01)

`ExpandPlane` improves from 36.542934% to 40.515213% in the deduplicated
report. The decoded block and subblock IDs now use their stored byte type;
the switch cases follow the retail body order (skip, run, intra, residue,
inter, fill, pattern, motion, raw, scaled). Case bodies are unchanged and
all cases terminate with a break. This is a source-layout improvement,
not a complete reconstruction: the function remains 3,916 bytes against
retail's 5,916, and the expand unit remains NonMatching.

Unrolling the three motion-copy loops reduced the match; forcing the
scale helper inline also reduced it. Those experiments were reverted.
All-source compilation, the Huff4 host checker, and the normal linked
retail DOL SHA-1 check pass. No other function score regresses. The retail
DOL check does not validate execution of this NonMatching decoder.

## Bink scaled dispatch and indexed runs (2026-10-01)

`ExpandPlane` improves from 40.515213% to 48.724136% in the deduplicated
report, with no other function score changes. Scaled blocks now dispatch
through a switch in retail body order, and the scale helper is inline.
Run decoding reads the repeat/literal bit before its branch-local run length
and indexes the scan table by pixels written, using a signed countdown for
the stored length-minus-one. These structures follow the retail disassembly.
The function is now 4,472 bytes versus retail's 5,916; still NonMatching.

`python tools/check_bink_runs.py` compiles the production run helper against
synthetic decoded bundles: 32,768 cases check both run modes, lengths 1-16,
permuted scan tables, four destination pitches, padding, and exact bundle
consumption. Bit extraction is mocked, so this is not a movie playback test.
The Huff4 checker, full source compilation, and normal DOL SHA-1 check pass.

Direct scaled-pattern output using mask3/mask4 is visible in retail, but the
measured replacement still scores below the retained temporary-block path;
that experiment was not retained. Advancing local skip-copy cursors also
regressed. Revisit the direct scaled pattern alongside the remaining layout
work rather than claiming the current decoder is fully reconstructed.

## Bink RLE Huff4 shared temporary (2026-10-01)

`CheckReadRLEHuff4Bundle` improves from 94.48588% to 96.60452% by sharing
`peek` between the mutually exclusive Huffman-width and repeat-fill paths.
The repeat fill still uses the same unsigned-byte cast of a four-bit read;
no decoded values or bundle consumption change. ProDG's allocation changes
when this shared local is renamed `value` (95.38983%), so the original name
is retained. Compiled size is 704 bytes; the retail function is 708 bytes.
No other deduplicated function score changes. All-source build, Huff4 and
run-block host checks, and normal retail DOL SHA-1 verification pass.
The unit remains NonMatching.

## Bink byte IDCT scratch rows (2026-10-01)

`fastidct8x8` improves from 56.963562% to 58.927124% by staging the
second-pass even/odd terms through the existing scratch row as they are
calculated. Scratch slots 3 and 4 also now follow the retail first-pass
layout. Arithmetic, dequantization, rounding, and output order are preserved.
The compiled function is 948 bytes versus retail's 988; still NonMatching.

`python tools/check_bink_idct.py` checks 32,768 blocks against a separate
scalar one-dimensional transform applied by columns and rows. It covers
both production inverse-quantization table families, all 16 levels, DC-only,
sparse, and dense inputs, and four padded pitches. The host build uses
32-bit wrapping arithmetic, matching the tested integer operations. This
is not a GameCube or movie playback test.

Full source compilation and normal retail DOL verification pass after
integrating staging's RenderWare 2.0p1c update. No rebuilt function score
regresses. Beware scratch restores: Copy-Item retains old timestamps and
can leave experiment objects current in Ninja. Touch restored sources;
this validation explicitly rebuilt the reverted Bink sources and compared
a rebuilt pre-change DCT baseline. The local trial harness now touches its
restored source automatically.

## Bink doubled IDCT scratch rows (2026-10-01)

`fastidct8x8d` improves from 63.101215% to 67.41296% with the same staged
scratch-row structure as the byte-output routine. First-pass slots 3/4 now
use the corresponding retail layout. Quantization, rounding, and pixel
packing remain unchanged. Compiled size is 1,000 bytes versus retail's
988, so this remains NonMatching.

The IDCT checker now verifies both byte and doubled output on each of its
32,768 cases. Doubled pixels are compared as big-endian packed words using
word-aligned pitches; output padding is checked too. Full source build and
normal retail DOL verification pass. The deduplicated report changes only
this function; no score regressions.

## Bink motion IDCT scratch rows (2026-10-01)

`FastmIDCT8x8WithMotion` improves from 50.873135% to 55.77239%. Its final
pass now computes even terms before odd terms and stages them through the
same scratch-row layout as the byte-output routine. The first-pass scratch
slots 3/4 follow that layout too. Rounding and prediction-byte addition are
unchanged. The compiled routine is 1,028 bytes versus retail's 1,072 and
remains NonMatching.

The scalar-reference checker adds 16,384 motion blocks with generated
prediction bytes, checking byte wrapping and row padding alongside the
32,768 byte/doubled-output cases. All checks pass, as do full source build
and normal retail DOL verification. The deduplicated report changes only
this routine; no score regressions. The preceding doubled-output commit's
CI run 36916110711 passed.

## Bink lossless reader buffered-bit branch order (2026-10-01)

`ReadBPLossless` improves from 51.703175% to 61.21906%. Eight bit-reading
branches now put the buffered-bit path before the refill path, following
retail's layout; their bodies and conditions are otherwise equivalent.
The routine remains 3,516 bytes versus retail's 3,652 and NonMatching.
Full source compilation and normal retail DOL verification pass; the
rebuilt deduplicated report changes only this function.

`python tools/check_bink_bitplane.py` compares the production reader with
the pre-change reader at c9b08d77e0bb1c0a9b0e08d772c503fa68f29fdc (that
commit must be available in local git history). It passes 16,384 cases,
checking output plus the complete bitstream state across 0-15 magnitude
levels and every initial word offset. Only the leading-zero intrinsic is
replaced for host execution; the writer, reader, length routine, scan
order and bitstream macros come from source.

Important unresolved finding: 14,955 of those writer/reader round trips
fail output or length checks in the existing implementation too. The first
failure is level 1, offset 0, trial 1: coefficients 48 and 56 are dropped,
although writer length, reader consumption and LenBPLossless all report
25 bits. It reproduces with the pre-change reader and with host strict
aliasing disabled. This is a host test finding, not yet a diagnosis of the
writer versus reader or GameCube runtime behavior. The test reports these
separately; its passing equivalence result does not claim correct decoding.
Investigate this before claiming the bitplane codec is functionally verified.

## Corrected lossless bitplane indexing and magnitude lifetime (2026-10-01)

This resolves the host round-trip failures documented immediately above.
Lossless group arrays omit the first four-coefficient group, so expansion
must read groups[n-1], groups[n], groups[n+1], groups[n+2]. The length routine
and writer instead read n through n+3. Retail explicitly subtracts one for
the first lookup. They now derive the group number from the coefficient
index and use the correct four array positions.

The reader also shifted highbit before decoding the current plane. Retail
keeps the current leading bit and the shifted next value in separate
registers (r27/r30 at loop entry). A separate next_highbit now preserves the
current magnitude until the plane finishes. Previously even a single -3
coefficient decoded as -1. These are source-recovery corrections, not new
codec behavior or assembly workarounds.

The checker now requires full writer/reader round trips, guards, DC
preservation and exact encoded/consumed/calculated lengths. All 16,384 cases
pass, versus 14,955 failures before these fixes. It no longer depends on a
historical reader or git history. This remains host validation, not movie
playback or execution of a source-linked GameCube bitplane unit.

Deduplicated scores: WriteBPLossless 77.81737 -> 78.211075;
ReadBPLossless 61.21906 -> 61.396496; LenBPLossless 65.018 -> 64.1054.
The last layout/regalloc tradeoff is retained because its previous indexing
was demonstrably incorrect and disagreed with retail. No other scores
change. Full source build and normal retail DOL verification pass; the
bitplane unit remains NonMatching. Prior commit CI 36918100491 passed.

## Bink lossy reader buffered-bit branch order (2026-10-01)

`readlossy` improves from 54.733925% to 65.53215% by placing the buffered-bit
path before refill in ten branches, as in the lossless reader. Conditions
are inverted and branch bodies exchanged without changing their operations.
The rebuilt deduplicated report changes only this routine. Full source
build and normal retail DOL verification pass; the unit is NonMatching.

The bitplane checker now also runs 7,168 lossy trials over magnitude levels
1-7 and all initial word offsets. The 6,911 nonempty blocks round-trip in
scan order with exact bit consumption and output guards; the 257 empty
blocks must return without changing the stream. All 16,384 lossless cases
continue to pass. Lossy tests decode every plane and do not yet exercise
early mask cutoffs or GameCube runtime playback. Previous fix CI 36918932454
passed.


## Bink lossy child-index reuse (2026-10-01)

`readlossy` now uses the already-decoded byte child index for three coefficient
stores, improving its deduplicated match from 65.53215% to 65.71619%.
A controlled rebuild of HEAD and the candidate changes only that function's
score. The older report's `check_hide_entities` 100% versus 91.04651% discrepancy
also appears with unchanged HEAD source and is not caused by this edit.

The host bitplane checker passes 16,384 lossless round trips and 6,911 nonempty
lossy round trips. Added 27,644 early-cutoff checks cover the first, middle,
last, and beyond-last update: magnitude-bit subsets, signs, update counts,
stream position, buffered bits, and output guards. These are invariants, not
an independent retail oracle for partial coefficient ordering or movie playback.
Full all-source compilation passes; the normal linked DOL retains retail SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. Bitplane remains NonMatching, so that
DOL check does not establish source-linked Bink decoding.


## Bink lossy node control flow (2026-10-01)

`readlossy` improves from 65.71619% to 67.3592% by putting the remaining
node-presence bit read in buffered-first order and moving empty-node
advancement to the loop tail, consistent with retail branches. All decoding
operations and mask cutoff behavior are preserved. The checker passes 16,384
lossless round trips, 6,911 nonempty lossy round trips, and 27,644 early-cutoff
invariant checks. Full all-source compilation and the normal retail DOL SHA-1
check pass. No report scores regress; the unrelated unstable
`check_hide_entities` row returns from 91.04651% to 100% on this rebuild.
Bitplane remains NonMatching and is not source-linked into the normal DOL.

Rejected experiments: spelling refinement as explicit sample-minus-mask or
sample-plus-mask lowered readlossy to 65.57428%; moving the high-node case
before the group-node case left it unchanged at 65.71619%. Neither is retained.


## Bink deferred-coefficient layout (2026-10-01)

Moving readlossy's deferred-coefficient path after the child paths, with the
switch dispatching to its label, follows retail's block order and improves
its deduplicated match from 67.3592% to 73.266075%. No decoding operations
change. The full report comparison changes only this function.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and 27,644
early-cutoff invariant checks pass. Full all-source compilation and the normal
retail DOL SHA-1 check pass, including after integrating staging's 92251cd16
RenderWare compiler/linking update. Bink bitplane remains NonMatching; these
checks do not establish source-linked Bink playback.


## Bink child-index progression and sign branches (2026-10-01)

`readlossy` now decodes the child scan index into `node` and increments it
for successive children, matching retail's byte-index progression. That alone
raises the match from 73.266075% to 73.36364%. For the first three children,
branching on the sign inside the buffered/refill paths, instead of merging
before testing it, brings the deduplicated match to 80.00887%. The explicit
labels reproduce the retail control flow using ordinary C; no assembly or
compiler changes are involved.

The checker passes 16,384 lossless round trips, 6,911 nonempty lossy round
trips, and 27,644 early-cutoff invariant checks. Full all-source compilation
and normal retail DOL SHA-1 validation pass. No other game or Bink function
scores change; RenderWare differences from the earlier report belong to the
already-integrated 92251cd16 compiler update. Bitplane remains NonMatching,
so the retail DOL check does not establish source-linked Bink playback.


## Bink remaining sign branches (2026-10-01)

Extending the buffered/refill sign branching to the last child and deferred
coefficient paths raises readlossy from 80.00887% to 82.241684%. The individual
trials score 80.97339% (last child) and 80.35477% (deferred coefficient).
The deduplicated full-report comparison changes only readlossy.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and 27,644
early-cutoff invariant checks pass. Full all-source compilation and the normal
retail DOL SHA-1 check pass, also after integrating staging c2cf5cb6c's two
RenderWare source-link promotions. Bitplane remains NonMatching; source-linked
Bink playback is still unverified.


## Bink final presence and refinement branches (2026-10-01)

Matching the buffered/refill branching for the final child-presence bit raises
readlossy from 82.241684% to 84.89357%. Applying the same retail control-flow
shape to active-coefficient refinement bits brings it to 85.40576%. The full
deduplicated report changes only readlossy. Direct sample-minus-mask versus
sample-plus-mask arithmetic scored 85.330376% and was reverted.

The host checker passes all 16,384 lossless round trips, 6,911 nonempty lossy
round trips, and 27,644 early-cutoff invariant checks. Full all-source
compilation and the normal retail DOL SHA-1 check pass. Bitplane remains
NonMatching; source-linked Bink playback remains unverified.


## Bink node dispatch index lifetime (2026-10-01)

In readlossy, decoding the high-node index within its case and branching to
the common child path follows retail's block order and raises the score from
85.40576% to 88.117516%. Reusing `node` for the decoded group index instead of
a separate `node_kind` temporary brings the deduplicated match to 89.59424%.
The full report comparison changes only readlossy.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and 27,644
early-cutoff invariant checks pass, as do full all-source compilation and
the normal retail DOL SHA-1 check. Bitplane remains NonMatching; source-linked
Bink playback is not established by these checks.


## Bink deferred index reuse (2026-10-01)

Decoding the deferred coefficient index once into `node`, then reusing it for
the active list and destination store, improves readlossy from 89.59424% to
90.50555%. The full deduplicated report changes only readlossy. Separate
header-index lifetime (89.28381%) and byte loop-counter (89.49445%) trials
were reverted.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and 27,644
early-cutoff invariant checks pass. Full all-source compilation and normal
retail DOL SHA-1 validation pass. Bitplane remains NonMatching, so Bink
source linking and playback remain outstanding.


## Bink saved bit-buffer word (2026-10-01)

Using the saved original buffer word for the buffered-path bit tests raises
readlossy from 90.50555% to 90.61641%. Applying it consistently to the node
presence test and removing the now-unused `bit` and `code` temporaries gives
95.08647%. With these lifetimes corrected, direct sample-minus-mask or
sample-plus-mask refinement now improves the score to 95.19734% and is
retained (it had regressed earlier layouts). The full deduplicated report
changes only readlossy.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and 27,644
early-cutoff invariant checks pass. Full all-source compilation and normal
retail DOL SHA-1 validation pass. Bitplane remains NonMatching; source-linked
Bink playback remains unverified.


## Bink next-plane cursor lifetime (2026-10-01)

Keeping the next-plane cursor across iterations and deriving the traversal
cursor at the start of node processing follows retail's pointer lifetime and
improves readlossy from 95.19734% to 95.50776%. The full deduplicated report
changes only readlossy. Moving saved-word loads before bit-count decrements
had no effect; reusing node for the initial level scored 95.00887%; neither
experiment is retained.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and 27,644
early-cutoff invariant checks pass. Full all-source compilation and normal
retail DOL SHA-1 validation pass. Bitplane remains NonMatching, with Bink
source linking and playback still outstanding.


## Bink negative-mask lifetime (2026-10-01)

Giving the negative mask its own variable instead of reusing the coefficient
scan counter improves readlossy from 95.50776% to 95.585365%. The full
deduplicated report changes only this function. Rejected trials: explicit
outer-loop gotos 91.649666%; shared refinement-result temporary 95.34146%;
signed level count and widened mask both unchanged at 95.50776%.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and 27,644
early-cutoff invariant checks pass. Full all-source compilation and normal
retail DOL SHA-1 validation pass. Bitplane remains NonMatching; these checks
do not establish source-linked Bink playback.


## Bink lossless deferred-coefficient layout (2026-10-01)

Placing ReadBPLossless's non-final-plane deferred-coefficient decoding after
the child paths, and routing node advancement through the loop tail, improves
its deduplicated match from 61.396496% to 64.83242%. The full report changes
only ReadBPLossless. Replacing masked buffered bits with saved full words
scored 57.922234% and was reverted; the lossy reader's improvement does not
transfer directly to this function.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and 27,644
early-cutoff invariant checks pass. Full all-source compilation and normal
retail DOL SHA-1 validation pass. Bitplane remains NonMatching, so source-linked
Bink playback remains unverified.


## Bink lossless magnitude-bit branch order (2026-10-01)

Putting the buffered magnitude read before the refill path, and applying its
mask within each path, follows retail's instruction order and improves
ReadBPLossless from 64.83242% to 70.72618%. This removes the unnecessary
intermediate narrow value in the refill path. Both deferred coefficients and
the shared child macro use this form. The full deduplicated report changes
only ReadBPLossless.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and 27,644
early-cutoff invariant checks pass. Full all-source compilation and normal
retail DOL SHA-1 validation pass. Bitplane remains NonMatching; source-linked
Bink playback remains unverified.


## Bink lossless shared child-push path (2026-10-01)

Sharing the child-node push path between buffered and refill presence reads
in READ_LOSSLESS_CHILD follows retail's branch layout and raises
ReadBPLossless from 70.72618% to 74.16758%. The macro uses a label derived
from its existing per-child label argument. The full deduplicated report
changes only ReadBPLossless.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and 27,644
early-cutoff invariant checks pass. Full all-source compilation and normal
retail DOL SHA-1 validation pass. Bitplane remains NonMatching; source-linked
Bink playback remains unverified.


## Bink lossless child sign result (2026-10-01)

Matching READ_LOSSLESS_CHILD's buffered/refill sign branches gives 74.17087%
from 74.16758%. Keeping the signed result in the existing wide temporary
until the final halfword store then raises ReadBPLossless to 75.0011%,
avoiding an intermediate narrow signed assignment. The full deduplicated
report changes only ReadBPLossless.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and 27,644
early-cutoff invariant checks pass. Full all-source compilation and normal
retail DOL SHA-1 validation pass. Bitplane remains NonMatching; source-linked
Bink playback remains unverified.


## Bink lossless decoded-index progression (2026-10-01)

Reusing the group-node index and decoding the high-node index before entering
the common child path raises ReadBPLossless from 75.0011% to 76.21249%.
Advancing the byte child index sequentially brings it to 77.28806%, following
retail's index progression. No other game or Bink function scores change;
RenderWare differences from the previous report belong to the already
integrated 976c9c28f update.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and 27,644
early-cutoff invariant checks pass. Full all-source compilation and normal
retail DOL SHA-1 validation pass. Bitplane remains NonMatching; source-linked
Bink playback remains unverified.


## Bink lossless final-plane block layout (2026-10-01)

Sharing the final-plane child push path raises ReadBPLossless from 77.28806%
to 78.6046%. Moving final-plane deferred-coefficient decoding after the child
paths, with node advancement at the loop tail, brings it to 80.50164%.
The full deduplicated report changes only ReadBPLossless.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and 27,644
early-cutoff invariant checks pass. Full all-source compilation and normal
retail DOL SHA-1 validation pass, including after integrating staging's
d10c877fa bavector source-link promotion. Bitplane remains NonMatching;
source-linked Bink playback remains unverified.


## Bink lossless final-plane index progression (2026-10-01)

Reusing the final-plane group index, decoding the high-node index before
entering the child path, and advancing child indices sequentially improves
ReadBPLossless from 80.50164% to 82.90581%. The now-unused `kind` temporary is
removed. The full deduplicated report changes only ReadBPLossless.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and 27,644
early-cutoff invariant checks pass. Full all-source compilation and normal
retail DOL SHA-1 validation pass. Bitplane remains NonMatching; source-linked
Bink playback remains unverified.


## Bink lossless final-plane child sign branches (2026-10-01)

Matching the final-plane child sign branches and assigning the sign value
before the common store raises ReadBPLossless from 82.90581% to 86.59036%.
The full deduplicated comparison before the concurrent compiler update
changes only ReadBPLossless.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and 27,644
early-cutoff invariant checks pass. Full all-source compilation and normal
retail DOL SHA-1 validation pass, including after integrating a12d41401's
game-code switch to GC/2.0p1d. Bink continues to use ProDG. An initial ProDG
temporary assembly-file open failure cleared when using a worktree-local
TEMP/TMP directory; validation used that directory too. Bitplane remains
NonMatching; source-linked Bink playback remains unverified.


## Bink lossless deferred sign branches (2026-10-01)

Matching buffered/refill sign branches for both deferred-coefficient paths,
with sign selection before the common store, improves ReadBPLossless from
86.59036% to 87.150055%. No other Bink scores change. Game-code differences
from the earlier report belong to the integrated a12d41401 compiler update.
Retesting saved full-word bit reads scored 86.27382% and was reverted.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and 27,644
early-cutoff invariant checks pass. Full all-source compilation and normal
retail DOL SHA-1 validation pass. Bitplane remains NonMatching; source-linked
Bink playback remains unverified.


## Bink lossless node-presence order and child destination (2026-10-01)

Matching buffered node-presence branch order in both lossless loops raises
ReadBPLossless from 87.150055% to 87.36911%. Computing each non-final child
destination before its sign read brings it to 87.50055%. The destination
pointer uses u16, matching the coefficient array. The full report comparison
changes only ReadBPLossless; correcting the pointer signedness preserves the
score and removes host compiler warnings.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and 27,644
early-cutoff invariant checks pass. Full all-source compilation passed, and
the final pointer correction was rebuilt with normal retail DOL SHA-1
validation passing again. Bitplane remains NonMatching; source-linked Bink
playback remains unverified.


## Bink lossless level guard and cursor lifetime (2026-10-01)

Preserving the original level value for the final-plane guard instead of
materialising a boolean raises ReadBPLossless from 87.50055% to 87.73932%.
Keeping the next-plane cursor across iterations and deriving the traversal
cursor from it brings the score to 87.9529%. The final-plane entry condition
uses that next-plane cursor too. The full report changes only ReadBPLossless.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and 27,644
early-cutoff invariant checks pass. Full all-source compilation and normal
retail DOL SHA-1 validation pass. Bitplane remains NonMatching; source-linked
Bink playback remains unverified.


## Bink lossless combined stack workspace (2026-10-01)

Retail computes coefficient addresses from the tree workspace base with a
0x88 offset. Grouping the read tree and coefficient buffer into a single
local workspace struct follows that addressing model and improves
ReadBPLossless from 87.9529% to 88.45783%. This supports the layout hypothesis
but does not prove the original source declaration. The full deduplicated
report changes only ReadBPLossless.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and 27,644
early-cutoff invariant checks pass. Full all-source compilation and normal
retail DOL SHA-1 validation pass. Bitplane remains NonMatching; source-linked
Bink playback remains unverified.


## Bink lossless deferred coefficient destination lifetime (2026-10-01)

Decoding the non-final deferred coefficient's node index before magnitude
reading and computing its destination before sign reading follows retail's
address lifetime and improves ReadBPLossless from 88.45783% to 89.03834%.
Applying the same change to final-plane deferred coefficients scored
88.95509% and was reverted. The full report changes only ReadBPLossless.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and 27,644
early-cutoff invariant checks pass. Full all-source compilation and normal
retail DOL SHA-1 validation pass. Bitplane remains NonMatching; source-linked
Bink playback remains unverified.


## Bink lossless magnitude refill update order (2026-10-01)

Separating the refill word load from pointer advancement and advancing the
pointer after reservoir/count updates improves ReadBPLossless from 89.03834%
to 89.31216%. Updating the count earlier and deriving the reservoir shift
from it scored 89.20263% and was not retained. The full deduplicated report
changes only ReadBPLossless.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and 27,644
early-cutoff invariant checks pass. Full all-source compilation and normal
retail DOL SHA-1 validation pass. Bitplane remains NonMatching; source-linked
Bink playback remains unverified.


## Supplement streak interval data match (2026-10-01)

`zNPCSupplement` now matches all 95 functions (21,784 code bytes) and all
200,680 data bytes, up from 99.84852% data. The two tossed-jelly streak
intervals used the decimal literal `0.025f` (0x3ccccccd), while retail uses
0x3cccccce. Expressing the interval as `1.5f * (1.0f / 60.0f)` reproduces
that rounding; the same routine already uses a 60 Hz interval for another
streak. This is a plausible frame-based expression, not proven original
source. A direct `1.0f / 40.0f` still produces the wrong bit pattern.

The unit remains NonMatching for linking. A trial source promotion failed
at 0x801815F4: retail expects NPCC_StreakCreate, but source places
StreakInfo::Defaults there. The object has additional definition-order
differences to resolve before promotion. No split or compiler edits were
retained. The deduplicated report is build/supplement-frequency-report.json.

Validation: full all_source build passed and the normal link reproduces retail
DOL SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6. The deduplicated
report has no function-score changes across the project.


## Bink lossless single-bit refill temporaries (2026-10-01)

`ReadBPLossless` improves from 89.31216% to 89.64075% by keeping each
single-bit refill word in a block-local temporary instead of sharing the
function-wide sign/result variable. Magnitude refills retain their prior
form; localizing those too only reaches 89.38554%. Full-width saved-level
and boolean-condition trials did not improve the baseline and were restored.

Production-source checks pass all 16,384 lossless round trips, 6,911
nonempty lossy round trips, and 27,644 early-cutoff cases, including the
existing guard and bit-consumption checks. The full source build and normal
retail DOL hash pass. The deduplicated report changes only ReadBPLossless;
bitplane remains NonMatching, so the normal hash is not source-linked Bink
playback validation. Report: build/bitplane-local-bitword-report.json.


## Bink lossless writer depth temporaries (2026-10-01)

`WriteBPLossless` improves from 78.211075% to 78.95659% by using separate
local bit-depth temporaries for coefficient, group, and high-group scans.
These calculations previously reused the later bitstream/plane variable.
The coefficient-only form reached 78.23952%, adding the group scan reached
78.24701%, and separating the high-group calculation reached 78.95659%.
Separating root initialization regressed to 78.007484% and was rejected;
changing header pointer/count update order also regressed and was restored.

Production-source checks pass all 16,384 lossless round trips, 6,911
nonempty lossy round trips, and 27,644 early-cutoff checks, with the existing
bit-length, guard, and DC-preservation checks. This remains a NonMatching
Bink unit; normal executable identity does not establish source-linked
movie playback. Report: build/bitplane-write-depth-report.json.

Full all_source build passed; normal DOL SHA-1 remains
306526d90b48e99894c3138f5fc8f2716d9fecf6. The project-wide deduplicated
report changes only WriteBPLossless, with no function-score regressions.


## Bink lossless length-estimator child traversal (2026-10-01)

`LenBPLossless` improves from 64.1054% to 71.37789%. A local coefficient
bit-depth temporary reaches 64.156815%; putting deferred-node pushes before
length increments reaches 67.197945% on non-final planes. Sequential child
indices reach 68.634964%. Applying the same branch ordering to the final
plane reaches 70.72494%, and sequential final-plane indices reach 71.37789%.
These forms follow the target's branch direction and increment/load sequence.
Separate group/high-group temporary trials regressed and were restored.

All 16,384 lossless round trips pass, including exact predicted-versus-written
bit lengths, guards and DC preservation. All 6,911 nonempty lossy round trips
and 27,644 early-cutoff checks also pass. Full all_source build passes, with
no function-score regressions. Report: build/bitplane-length-report.json.
The unit remains NonMatching; this is not a source-linked playback claim.


## Bink lossless estimator dispatch and running total (2026-10-01)

`LenBPLossless` improves from 71.37789% to 80.14396%. Moving the high-group
case before the group case and letting branch nodes fall into the shared
child path reaches 76.04113% for non-final planes and 79.14653% for both
loops. Reusing the decoded entry as its child index reaches 79.32648%.
A single running length counter, with each node-presence bit counted before
its child or coefficient bits, reaches 80.14396% and removes the redundant
per-node total copy.

All 16,384 lossless round trips pass, including exact estimator-versus-writer
bit lengths, guards, and DC preservation. The 6,911 nonempty lossy round
trips and 27,644 early-cutoff checks pass as well. The full all_source build
passes with no function-score regressions. Report:
build/bitplane-length-dispatch-report.json. The unit remains NonMatching;
normal DOL identity does not establish source-linked movie playback.


## Bink lossless writer node traversal (2026-10-01)

`WriteBPLossless` improves from 78.95659% to 87.199104%. Retail's high/group/
branch dispatch order reaches 80.56437%; deferred-child-first branches reach
86.86976%; constructing the first deferred child from its decoded index
reaches 86.952095%. Reusing the entry as its decoded index reaches the final
87.199104%, provided the group branch is also constructed from that index
rather than attempting to mask base bits from an already decoded value.
The round-trip checker caught that intermediate mistake; only the corrected
version is retained. Sequential child-index trials regressed and were dropped.

All 16,384 lossless round trips, exact predicted/written lengths, guards and
DC checks pass. All 6,911 nonempty lossy round trips and 27,644 early-cutoff
checks also pass. Full all_source build and normal retail DOL SHA-1 pass;
only WriteBPLossless changes in the deduplicated report, with no regressions.
Report: build/bitplane-writer-dispatch-report.json. Bitplane remains
NonMatching, so this does not establish source-linked movie playback.


## Bink length-estimator high-node index reuse (2026-10-01)

`LenBPLossless` improves from 80.14396% to 81.87918%. An unsigned plane
counter matches retail's comparison and reaches 80.45758%. Decoding each
high node's index before both its high-group lookup and shared child path
reaches 81.655525% on non-final planes and 81.87918% on both loops. The
separate branch-node path still decodes its packed entry before joining.
A next-plane cursor lifetime trial regressed and was restored.

All 16,384 lossless round trips pass with exact estimator/writer bit-length
agreement, guards, and DC preservation. All 6,911 nonempty lossy round trips
and 27,644 early-cutoff checks also pass. Report:
build/bitplane-length-high-report.json. Bitplane remains NonMatching;
normal DOL identity is not a source-linked playback claim.

Full all_source build and retail DOL SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6 pass. No function-score regressions;
the other report changes are the integrated babinwor/bameshop matches.


## Bink packed-node arithmetic and writer high-node traversal (2026-10-01)

Grouping the packed index and node kind before adding the level in the
branch/high-group entry macros reproduces retail's arithmetic order. Along
with decoding the writer's high-node index once and advancing its tree-end
cursor between child stores, the deduplicated scores improve as follows:

- LenBPLossless: 81.87918% -> 89.83034%.
- WriteBPLossless: 87.199104% -> 89.79641%.
- WriteBPLossy: 77.53779% -> 77.710014%.

The writer-only high-node change reached 87.52395%, and its cursor-update
change reached 87.86527%. The shared branch-entry grouping then improved
all three functions, and the high-group grouping added further lossless
gains. No other function scores change.

All 16,384 lossless round trips pass, including exact estimator/writer bit
lengths, guards, and DC preservation. All 6,911 nonempty lossy round trips
and 27,644 early-cutoff checks pass. Full all_source build passes. Report:
build/bitplane-packed-report.json. Bitplane remains NonMatching; the normal
retail DOL hash does not establish source-linked movie playback.


## Bink lossy writer child traversal (2026-10-01)

WriteBPLossy improves from 77.710014% to 87.27416% in the deduplicated
report. Dispatch now places high/group nodes before the branch-node shared
child path. Each child handles deferred coefficients before active emission,
and the child index advances sequentially. High nodes decode their index
once for both the high-group lookup and child traversal. These changes
preserve the stream traversal and sign-bit order; no assembly is added.

All 16,384 lossless round trips and 6,911 nonempty lossy round trips pass,
along with 27,644 early-cutoff checks. The full all_source build and retail
DOL SHA-1 pass. No other function scores change against
build/bitplane-packed-report.json. New report:
build/bitplane-lossy-writer-report.json. Bitplane remains NonMatching;
normal DOL identity does not establish source-linked movie playback.


## Bink lossy writer depth temporary lifetimes (2026-10-01)

WriteBPLossy improves from 87.27416% to 88.521965%. The coefficient-depth
scan uses a loop-local coeff_bits temporary, and high-group construction
and root setup each use their own block-local group_bits. This separates
those lifetimes from the later lenbits index and stream-header value.
The analogous ordinary-group-loop trial and first-child entry reuse both
regressed and were restored. No arithmetic or stream format changes.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and
27,644 early-cutoff checks pass. Full all_source build and retail DOL
SHA-1 pass, including the integrated rpusrdat match/link commit c8e48b916.
No other local function scores change; the integration adds only the
expected UserDataListCopy improvement from 98.519554% to 100%. Report:
build/bitplane-lossy-depth-integrated-report.json. Bitplane remains
NonMatching, so normal DOL identity is not source-linked playback evidence.


## Bink lossy writer decoded-node reuse (2026-10-01)

WriteBPLossy improves from 88.521965% to 92.63972%. Separating the
initial scan index from the active-coefficient count reaches 88.724075%;
advancing the stream cursor before the header's bit-count update reaches
88.7522%. Reusing node_entry after decoding its packed index gives the
remaining gain. The group-node path extracts its group index before
overwriting the packed entry. This preserves all child traversal and
coefficient emission order without introducing assembly.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and
27,644 early-cutoff checks pass. The full all_source build and retail DOL
SHA-1 pass. Final report build/bitplane-lossy-node-final-report.json shows
only WriteBPLossy changing against the previous integrated report.
An intermediate build reported check_hide_entities at 91.04651%; rebuilding
its unchanged source returned 100%. A diagnostic explicit-null-comparison
edit was discarded; no cutscene source change is retained. The cause of
that transient build discrepancy is not established. Bitplane remains
NonMatching; normal DOL identity is not source-linked playback evidence.


## Bink lossy writer presence and child-depth traversal (2026-10-01)

WriteBPLossy improves from 92.63972% to 93.88225%. Direct node-presence
comparisons avoid materializing a boolean in the shared count variable.
An advancing child_lens pointer follows retail's update-form depth loads.
The next-node label moves to the loop bottom, with expanded nodes continuing
without an extra cursor increment. Finally, group indices derive from the
already-decoded coefficient index, using the packed-group/index shift
difference (two bits). No assembly is added.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and
27,644 early-cutoff checks pass. Full all_source build and retail DOL
SHA-1 pass. No other function scores change. Report:
build/bitplane-lossy-traversal-report.json. Bitplane remains NonMatching;
normal DOL identity does not establish source-linked movie playback.


## Bink lossy writer plane cursor and residual emission (2026-10-01)

WriteBPLossy improves from 93.88225% to 94.43585%. The pending insertion
pointer persists across bitplanes, with the traversal cursor initialized
from it at each plane, matching retail's pointer lifetimes (94.06678%).
The residual loop loads its coefficient and increments the counter before
bit emission (94.40949%). Standalone coefficient nodes also decode their
index in place (94.43585%). A separate byte-sized child-depth temporary
regressed and was discarded.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and
27,644 early-cutoff checks pass. Full all_source build and retail DOL
SHA-1 pass; no other function scores change. Report:
build/bitplane-lossy-cursor-report.json. Bitplane remains NonMatching;
normal DOL identity does not establish source-linked movie playback.


## Bink lossless writer presence and plane cursor (2026-10-01)

WriteBPLossless improves from 89.79641% to 91.47605%. Moving the
next-node path to the loop bottom reaches 90.20659%; direct node-presence
comparisons instead of a materialized sign temporary reach 91.19461%.
Keeping the pending insertion pointer across planes and initializing the
traversal cursor from it each plane reaches 91.47605%. Expanded nodes
continue without an extra cursor increment, preserving tree traversal.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and
27,644 early-cutoff checks pass. Full all_source build and retail DOL
SHA-1 pass, including integrated baimage commit 8ded2f347. No function
scores regress; the only other change is the incoming RwImageApplyMask
99.089554% -> 100%. Report: build/bitplane-lossless-presence-report.json.
Bitplane remains NonMatching; normal DOL identity does not establish
source-linked movie playback.


## Bink magnitude reservoir store order (2026-10-01)

WriteBPLossless improves from 91.47605% to 91.50898% by storing the
magnitude bit buffer before its bit count, matching retail's order.
An explicit else for the empty reservoir, advancing the stream cursor
before the remaining-count calculation, and advancing child-depth pointers
all regressed in the tested forms and were discarded.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and
27,644 early-cutoff checks pass. Full all_source build and retail DOL
SHA-1 pass. No other function scores change. Report:
build/bitplane-magnitude-store-report.json. Bitplane remains NonMatching;
normal DOL identity does not establish source-linked playback.


## Bink length estimator child-depth pointers (2026-10-01)

LenBPLossless improves from 89.83034% to 92.04113%. An advancing
child_lens pointer supplies each child's depth while the packed-node index
advances independently. Applying this to the non-final loop reaches
90.35989%, and to both loops reaches 92.04113%. A shared next-node label
trial generated identical code and was discarded.

All 16,384 lossless round trips pass with exact estimator/writer bit-length
agreement; all 6,911 nonempty lossy round trips and 27,644 early-cutoff
checks pass. Full all_source build and retail DOL SHA-1 pass. Integrated
RenderWare commits 13baf74ea and 045d8935e also pass the combined build.
No function scores regress. Report:
build/bitplane-length-pointer-integrated-report.json. Bitplane remains
NonMatching; normal DOL identity does not establish source-linked playback.


## Bink length estimator original-level guard and plane cursor (2026-10-01)

LenBPLossless improves from 92.04113% to 92.95373%. Retaining the
original maximum level for the final-plane guard reaches 92.19537%,
matching retail's early comparison. The pending-node insertion pointer
then persists across planes, and each plane initializes the traversal
cursor from it, reaching 92.95373%. The final plane initializes the cursor
the same way. Zero-level inputs still skip the final traversal.

All 16,384 lossless round trips pass with exact estimator/writer bit-length
agreement, including zero-level inputs. All 6,911 nonempty lossy round trips
and 27,644 early-cutoff checks pass. Full all_source build and retail DOL
SHA-1 pass; no other function scores change. Report:
build/bitplane-length-guard-report.json. Bitplane remains NonMatching;
normal DOL identity does not establish source-linked playback.


## Bink length estimator final-plane group index (2026-10-01)

LenBPLossless improves from 92.95373% to 93.018% by giving the final
plane its own group_index rather than reusing maxbits for group lookups.
The separate final-plane node-entry, high-group-depth scope, and ordinary
group-depth scope trials regressed and were discarded.

All 16,384 lossless round trips pass with exact estimator/writer bit-length
agreement; all 6,911 nonempty lossy round trips and 27,644 early-cutoff
checks pass. Full all_source build and retail DOL SHA-1 pass. No other
function scores change. Report: build/bitplane-length-final-report.json.
Bitplane remains NonMatching; normal DOL identity does not establish
source-linked playback.


## Bink length estimator root-entry lifetime (2026-10-01)

LenBPLossless improves from 93.018% to 93.133675% by giving root setup
a block-local packed root_entry, independent of the later traversal entry.
The separate root-depth and explicit high-group-table-pointer trials
regressed and were discarded.

All 16,384 lossless round trips pass with exact estimator/writer bit-length
agreement; all 6,911 nonempty lossy round trips and 27,644 early-cutoff
checks pass. Full all_source build and retail DOL SHA-1 pass. No other
function scores change. Report: build/bitplane-length-root-report.json.
Bitplane remains NonMatching; normal DOL identity does not establish
source-linked playback.


## Bink lossy decoder level and buffered-word lifetimes (2026-10-01)

readlossy improves from 95.585365% to 96.59424%. Separating the decoded
maximum level from the remaining-plane counter reaches 95.61862%.
Eleven buffered-bit paths then use a block-local word loaded before the
bit-count decrement, reaching 96.59424%. Giving refill words their own
locals produced identical code, so that extra change was discarded.

All 6,911 nonempty lossy round trips and 27,644 early-cutoff checks pass,
including exact coefficient results, stream consumption, and guard checks.
All 16,384 lossless round trips also pass. Full all_source build and retail
DOL SHA-1 pass; no other function scores change. Report:
build/bitplane-readlossy-buffered-report.json. Bitplane remains NonMatching;
normal DOL identity does not establish source-linked movie playback.


## Bink lossy decoder root-pointer lifetime (2026-10-01)

readlossy improves from 96.59424% to 97.0377%. Root setup initializes
the persistent pending-node pointer directly instead of using and copying
a separate roots temporary. Shared refinement-result/counter-order trials
regressed, while a signed-word mask produced identical code; all were
discarded.

All 6,911 nonempty lossy round trips, 27,644 early-cutoff checks, and
16,384 lossless round trips pass. Full all_source build and retail DOL
SHA-1 pass. No other function scores change. Report:
build/bitplane-readlossy-root-report.json. Bitplane remains NonMatching;
normal DOL identity does not establish source-linked movie playback.


## Bink lossless writer root-entry lifetime (2026-10-01)

WriteBPLossless improves from 91.50898% to 91.67365% by using a
block-local packed root_entry during root setup, separate from the later
traversal entry. A separate 16-bit child-index trial regressed to 91.547905%
and was discarded.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and
27,644 early-cutoff checks pass. Full all_source build and retail DOL
SHA-1 pass; no other function scores change. Report:
build/bitplane-writer-root-entry-report.json. Bitplane remains NonMatching;
normal DOL identity does not establish source-linked playback.


## Bink lossless writer magnitude-bit lifetime (2026-10-01)

WriteBPLossless improves from 91.67365% to 91.696106% with a plane-local
magnitude_bits variable, independent of the header/root lenbits temporary.
A zero-reservoir branch using the known-zero count register regressed and
was discarded.

All 16,384 lossless round trips, 6,911 nonempty lossy round trips, and
27,644 early-cutoff checks pass. Full all_source build and retail DOL
SHA-1 pass, including incoming material-list commit 401809d4b. No function
scores regress. Report: build/bitplane-writer-mag-integrated-report.json.
Bitplane remains NonMatching; normal DOL identity does not establish
source-linked playback.


## Bink Huffman-table reader exact code match (2026-10-01)

ReadHuffTable improves from 99.01529% to 100% in the deduplicated report.
The merge-mode pair shuffle derives its right-hand index as i + 1 instead
of maintaining a separate right counter. This restores the initialization
schedule and indexed-store operands, while simplifying the source. Both
forms visit the same pairs: i starts at zero and advances by two. An
explicit pointer-addition-order trial regressed and was discarded.

The raw diff retains only relocation-name differences. No other function
scores change. Existing Huff4 checks pass 67,584 cases and 1,000 countdown
cases; run-block checks pass 32,768 cases. These cover the decoding helpers
and run expansion, not an independent full ReadHuffTable round-trip test.
Full all_source build and retail DOL SHA-1 pass. Report:
build/expand-huff-table-report.json. The expand unit remains NonMatching;
this exact function match is not a source-linked playback claim.


## Bink Huff8 lookup-mask lifetimes (2026-10-01)

The buffered and refill paths of exp_read_huff8 now each declare their
own lookup mask. CheckReadHuff8Bundle improves from 93.44827% to
93.487686%; NewCheckReadHuff8Bundle improves from 96.02564% to 96.14359%.
No other function scores change. Reordering symbol/used extraction and
making the refill word local produced identical code and were discarded.

Existing related Huff4 checks pass 67,584 cases and 1,000 countdown cases;
run-block checks pass 32,768 cases. These do not independently exercise
Huff8's state-dependent tables. The retained change only narrows the scope
of the identically initialized mask, with no lookup or bit-consumption
changes. Full all_source build and retail DOL SHA-1 pass. Report:
build/expand-huff8-mask-report.json. Expand remains NonMatching; normal
DOL identity does not establish source-linked playback.


## Bink Huff8 buffered-word reuse (2026-10-01)

exp_read_huff8 retains the original bit buffer for both symbol lookup and
bit consumption. The lookup masks the expression directly; consuming bits
shifts that saved buffer. Both buffered and refill paths use this form.
CheckReadHuff8Bundle improves from 93.487686% to 93.84236%, and
NewCheckReadHuff8Bundle from 96.14359% to 96.28205%. No other function
scores change. The buffered-only variant regressed the newer caller;
separate lookup-value/code scopes produced identical code and were dropped.

Related Huff4 checks pass 67,584 cases plus 1,000 countdown cases; run-block
checks pass 32,768 cases. These do not independently exercise Huff8's
state-dependent tables. The retained expressions use the same original
buffer, refill word, mask, and consumed-bit count. Full all_source build
and retail DOL SHA-1 pass. Report: build/expand-huff8-buffer-report.json.
Expand remains NonMatching; normal DOL identity is not playback evidence.

## Bink Huff8 buffered-path store order (2026-10-01)

The buffered path in `exp_read_huff8` now writes the remaining bit count
before the shifted buffer, matching the target instruction order.
`CheckReadHuff8Bundle` improves from 93.84236% to 93.867%, and
`NewCheckReadHuff8Bundle` from 96.28205% to 96.30769%. No other function
scores change in the deduplicated project report. An earlier branch-local `used` trial failed C89 compilation; its
stale-object score was mistakenly treated as unchanged and provides no
evidence about that variant.

Validation: 67,584 Huff4 cases, 1,000 countdown cases, and 32,768 run-block
cases pass; the full source build succeeds and the normal DOL retains
retail SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6. These existing
helper checks do not independently exercise Huff8 state-dependent tables.
The retained change only reorders independent field stores. `expand`
remains NonMatching, so the normal DOL hash is not source-linked Huff8 proof.

## Bink Huff8 packed-code lifetime (2026-10-01)

`exp_read_huff8` now reuses the packed code byte for the consumed-bit
count after looking up the decoded symbol. This follows the target's
register reuse and removes the separate `used` temporary. The high
nibble is in 0..15, so storing it back to `u8` loses no information;
subsequent arithmetic still combines it with unsigned bit counts.
`CheckReadHuff8Bundle` improves from 93.867% to 95.113304%, and
`NewCheckReadHuff8Bundle` from 96.30769% to 97.63077%. The earlier branch-local buffered-word trial failed C89 compilation.
Its stale-object score was incorrectly recorded as unchanged; the corrected
trial and exact match are documented below.

Validation: the new `tools/check_bink_huff8.py` extracts the production
helper and table structure and checks 135,168 cases with four chained
state-dependent reads each against a bit-at-a-time reader. It covers
all 16 initial states, lookup widths 1..8, and buffered lengths 0..32,
checking the symbol, cursor, buffer, and remaining bit count after each
read. All pass. Existing Huff4/countdown/run-block checks also pass.
The full source build succeeds; the deduplicated project report changes
only these two scores, and the normal DOL retains retail SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. `expand` remains NonMatching;
this does not establish source-linked playback or retail table parsing.

## Bink Huff8 buffered-word consumption (2026-10-01)

The buffered path now shifts the local word in place before storing the
remaining count and word. Keeping the original count temporary produces
a better result than also decrementing it in place. This brings
`CheckReadHuff8Bundle` from 95.113304% to 96.87192%, and
`NewCheckReadHuff8Bundle` from 97.63077% to 99.46154%. The newer caller's
remaining raw differences are register assignments, with instruction
order now matching.

Validation: 135,168 Huff8 cases (four state-dependent reads each) pass
on the retained version. The related Huff4/countdown/run-block checks
also passed during this pass. Full source build and retail DOL SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6 pass, and the deduplicated
project report shows only the two intended improvements. `expand`
remains NonMatching; no source-linked playback claim is made.

## Bink New Huff8 exact match (2026-10-01)

Giving each branch of `exp_read_huff8` its own buffered-word temporary
makes `NewCheckReadHuff8Bundle` match all 780 bytes (99.46154% to 100%).
The older `CheckReadHuff8Bundle` also improves from 96.87192% to 97.16749%.
Declarations are at the start of each block, as required by the ProDG C
compiler. Earlier scope trials mixed declarations with statements and
failed compilation, but the trial shell commands still compared the prior
object. Those notes are corrected above; trial commands now explicitly
stop on a failed compile before running objdiff. Retained earlier changes
had successful full builds and fresh reports and remain valid.

Validation: 135,168 Huff8 cases (four stateful reads each), 67,584 Huff4
cases, 1,000 countdown cases, and 32,768 run-block cases pass. Full source
build succeeds and the normal DOL retains retail SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. The initial project report
showed the previously observed untouched `zCutsceneMgr::check_hide_entities`
object anomaly (100% to 91.04651%). Rebuilding its byte-identical source
restored 100%; the final deduplicated report changes only the two Huff8
scores. No cutscene source change was retained; the anomaly's cause is
not established. The whole `expand` unit remains NonMatching, so the
exact function match is not a source-linked playback claim.

## Bink old Huff8 decoded-byte lifetime (2026-10-01)

`CheckReadHuff8Bundle` now decodes the high nibble into its byte-sized
`packed` temporary, saves the next table state, and then reuses the
temporary for nibble packing. This improves 97.16749% to 98.99015%;
`NewCheckReadHuff8Bundle` stays at 100%. A separate high-nibble temporary
regressed to 97.044334%, and a shared u32 packed temporary reached only
97.14286%; neither is retained. The decoded table symbol is a byte, and
the masked high nibble combined with a byte-valued low result is also
within 0..255, so the narrowing preserves every value. An exhaustive
check of all 65,536 decoded-byte pairs confirmed this.

Huff8's 135,168 stateful cases and Huff4's 67,584 cases plus 1,000
countdown cases pass. These are helper checks, not complete old-format
bundle playback tests. Full source build succeeds, the final deduplicated
project report shows only the intended old-Huff8 improvement, and the
normal DOL retains retail SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6.
The whole `expand` unit remains NonMatching.

## Bink signed Huff4 sign-bit lifetime (2026-10-01)

`CheckReadHuff4SBundle` improves from 98.78199% to 98.85308% by giving
the per-symbol sign read its own loop-local bit temporary. Packet flags
and repeat-packet signs retain the existing outer temporary. A separate
repeat-packet temporary regressed to 98.75829%; a loop-local symbol,
word-sized caller symbol, and helper branch-local packed codes also
regressed and were discarded.

The retained change only narrows a temporary's scope. Existing Huff4
checks pass 67,584 cases plus 1,000 countdown cases; Huff8 passes
135,168 cases with four stateful reads each. These do not independently
exercise complete signed-bundle packets. Full source build succeeds, the
deduplicated project report shows only this improvement, and the normal
DOL retains retail SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6. Both
Huff8 scores are preserved, including the newer caller's exact match.
The whole `expand` unit remains NonMatching.

## Bink signed Huff4 table setup (2026-10-01)

Loading the signed bundle's lookup width before its decode pointer improves
`CheckReadHuff4SBundle` from 98.85308% to 98.90995%. The destination
assignment remains between the two loads. This follows the target's
setup order and changes no values or decoding logic. A refill-local word
was unchanged; branch-local signed return values regressed to 96.86256%
and were discarded.

The Huff4 helper's 67,584 cases and 1,000 countdown cases pass. They do
not independently exercise complete signed-bundle packets. Full source
build succeeds; the deduplicated project report shows only this gain.
The normal DOL retains retail SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. `expand` remains NonMatching.

## Bink lossless header overflow count (2026-10-01)

`WriteBPLossless` improves from 91.696106% to 91.711075% by keeping the
header overflow count in `bit_count` instead of repurposing `bit_buf`.
The expressions and stream updates are unchanged. Capturing the shared
single-bit macro's count before its buffer update regressed both writers;
storing the incremented count in that macro was unchanged. Both macro
trials were discarded.

Validation passes 16,384 lossless roundtrips with guards and bit-length
checks, 6,911 nonempty lossy roundtrips, and 27,644 early-cutoff checks.
Full source build succeeds; the deduplicated project report changes
only `WriteBPLossless`. The normal DOL retains retail SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. `bitplane` remains NonMatching,
so this does not establish source-linked playback.

## Bink byte IDCT first-pass even pair (2026-10-01)

`fastidct8x8` improves from 58.927124% to 60.765182% by storing the first
pass's even difference/sum pair into scratch columns 2 and 1 before
computing the odd terms. The target also stores this pair early. The
other even values stay live until the final output combination. A nested
scope keeps subsequent declarations valid for the ProDG C compiler.
Merely moving the scalar even calculations earlier did not change the
score, nor did additionally moving the odd-result stores; those extra
store-order changes are not retained.

The independent scalar-transform checker passes 32,768 IDCT cases
covering byte/doubled outputs and 16,384 motion blocks, quantization
levels, and padded pitches. Arithmetic and rounding expressions remain
unchanged. Full source build succeeds; the deduplicated project report
changes only this function. The normal DOL retains retail SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. `dct` remains NonMatching;
this is not a source-linked playback claim.

## Bink byte IDCT first-pass odd intermediates (2026-10-01)

`fastidct8x8` improves from 60.765182% to 60.785423% by computing odd
intermediates directly into scratch columns 6, 5, and 4, rather than
keeping three scalar results and storing them later. Arithmetic and
rounding are unchanged. Delaying the cross-term store lost the gain;
earlier odd dequantization and earlier odd-sum storage added no gain,
so those follow-ups are not retained.

The scalar-reference checker passes 32,768 IDCT cases for byte/doubled
outputs plus 16,384 motion blocks, including padded pitches. Full source
build succeeds; the deduplicated project report changes only this score.
The normal DOL retains retail SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. `dct` remains NonMatching.

Before pushing, rebased onto concurrent staging commit `b14699bc9` (Core/x
matching work). The combined full build and retail hash pass. Its nine
new exact functions and `UpdateRain` improvement are preserved; the
post-rebase project report has no regressions and the IDCT score is
unchanged from the validated local result.

## Bink doubled IDCT first-pass scratch results (2026-10-01)

`fastidct8x8d` improves from 67.41296% to 67.510124% with the same
first-pass scratch staging used by the byte variant, but stores the even
sum to column 1 before the difference to column 2, matching this target's
order. Copying the byte variant's opposite order regressed to 66.97976%.
An additional attempt to reproduce the later odd-store order was unchanged
and is not retained. Arithmetic and output expansion are preserved.

The independent scalar checker passes 32,768 byte/doubled IDCT cases and
16,384 motion blocks, including padded pitches. Full source build succeeds;
the deduplicated project report changes only this function. The normal
DOL retains retail SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6.
The `dct` unit remains NonMatching.

## Bink motion IDCT paired output stores (2026-10-01)

`FastmIDCT8x8WithMotion` improves from 55.77239% to 56.36194%. First-pass
scratch staging with even columns 1 then 2 reaches 55.80597%; the larger
gain comes from writing each final output pair as its odd intermediate
becomes available, following the target's interleaved arithmetic/stores.
Pixel write order, arithmetic, rounding, and prediction addition are
unchanged. Removing the intermediate byte cast added no gain and was
discarded.

The independent scalar checker passes 32,768 byte/doubled IDCT cases
and 16,384 motion blocks with padded pitches. The local full source build
and retail DOL hash pass, and its project report changes only this score.
Rebased onto the concurrent game commits `1a52d2e77` and `ba03aebed`.
The combined full build and retail SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6 pass; the final deduplicated
report preserves the IDCT improvement and incoming game/NPC gains with
no regressions. The `dct` unit remains NonMatching.

## Bink doubled IDCT even-sum scratch lifetime (2026-10-01)

`fastidct8x8d` improves from 67.510124% to 67.89474%. The first-pass
even sum is written directly into row scratch instead of keeping an
`even2` temporary alive through the odd transform. The even difference
remains a scalar; staging both gives only 67.51417%. Arithmetic and output
store order are unchanged. The byte and motion variants did not benefit
from the same scratch change and were restored.

The scalar checker passes 32,768 byte/doubled IDCT cases and 16,384 motion
blocks, including padded pitches. The full source build and retail DOL
SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6 pass. The deduplicated
project report preserves the incoming `84e64e3af` zEntPlayer gains and
shows no regressions; this is the only additional score change. The `dct`
unit remains NonMatching, so the retail hash does not validate source-linked
IDCT playback.

Rebased onto concurrent platform commit `5780419b5`; the combined full
source build and retail hash pass. The final report retains this IDCT gain,
all incoming platform improvements, and no regressions.

## Bink doubled IDCT outer-pair packing (2026-10-01)

`fastidct8x8d` improves from 67.89474% to 68.38057%. Its final pass
computes the first and last packed pixel words once odd row 6 is available,
before computing odd rows 5 and 4. This shortens the live ranges of their
even/odd inputs; rounding, byte duplication, and output-store order are
unchanged. Moving each duplication next to its initializer alone had no
effect. Individual even scratch changes in the byte/motion variants
regressed and were discarded.

The scalar checker passes 32,768 byte/doubled IDCT cases and 16,384 motion
blocks with padding. Full source build and retail DOL SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6 pass. The deduplicated project
report changes only this function. The `dct` unit remains NonMatching;
the DOL hash does not establish source-linked IDCT playback.

## Bink doubled IDCT outer-word duplication (2026-10-01)

`fastidct8x8d` improves from 68.38057% to 68.44129% by duplicating the
bytes of packed words 0 and 3 immediately after each pair is calculated.
All output stores retain their existing order. Moving the other two
duplications adds no gain, and splitting the pair calculations into
earlier partial words adds no gain or regresses, so those forms were
discarded.

The scalar checker passes 32,768 byte/doubled cases and 16,384 motion
blocks with padding. The full source build and retail DOL SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6 pass. The deduplicated report
changes only this function. The `dct` unit remains NonMatching; the DOL
hash does not validate source-linked IDCT playback.

## Bink doubled IDCT scaled even difference (2026-10-01)

`fastidct8x8d` improves from 68.44129% to 69.57085%. The first-pass
even difference is initially the fixed-point scaled difference of rows
2 and 6; subtracting their sum is a separate update after the remaining
even terms are calculated. The arithmetic and output order are unchanged.
Keeping the unscaled difference until later regresses; reusing the sum
variable gives a smaller gain. Applying the retained form to byte/motion
IDCT regresses, so those variants are unchanged.

The scalar checker passes 32,768 byte/doubled cases and 16,384 motion
blocks with padding. After integrating `a4fd0cd8f`, the full source build
and retail DOL SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6 pass.
The deduplicated report retains the three incoming vector-helper matches
and changes only this additional function, with no regressions. Plankton
impart_velocity remains 91.12676%. The `dct` unit remains NonMatching;
the DOL hash does not validate source-linked IDCT playback.

## Bink doubled IDCT scaled odd intermediate (2026-10-01)

`fastidct8x8d` improves from 69.57085% to 70.42105%. Its first-pass
odd1 term is scaled into a named intermediate before odd_out0 and the
remaining odd scratch values are computed. The final tail uses that
intermediate with the same subtraction and addition. Arithmetic and
output-store order are unchanged. Naming the other odd scale or pair
difference gives no improvement, nor does separating the final-pass
even scaling from its subtraction in any of the three IDCT variants.

The scalar checker passes 32,768 byte/doubled cases and 16,384 motion
blocks with padding. Full source build and retail DOL SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6 pass. The deduplicated report
changes only this function. The `dct` unit remains NonMatching, so the
retail hash does not validate source-linked IDCT playback.

## Bink byte and motion IDCT scaled odd intermediate (2026-10-01)

The same named first-pass scaled odd1 intermediate now improves
`fastidct8x8` from 60.785423% to 61.06073% and
`FastmIDCT8x8WithMotion` from 56.36194% to 58.171642%. Arithmetic,
rounding, prediction addition, and output-store order are unchanged.
Completing the doubled variant's subtraction from odd_rot earlier
adds no gain, whether done in the initializer or a separate update,
so those trials were discarded.

The scalar checker passes 32,768 byte/doubled cases and 16,384 motion
blocks with padding. Full source build and retail DOL SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6 pass. The deduplicated report
changes only these two functions. The `dct` unit remains NonMatching;
the retail hash does not establish source-linked IDCT playback.

## Bink doubled IDCT final-pass odd scale (2026-10-01)

`fastidct8x8d` improves from 70.42105% to 70.72065%. The final pass
names the scaled odd tail input immediately after the shared rotation
is calculated, and reuses it in row 4 after rows 6 and 5 are available.
Arithmetic and output-store order are unchanged. This final-pass change
regresses byte IDCT and leaves motion unchanged, so only doubled IDCT
is edited. Naming the remaining first-pass odd scale and pair difference
in byte/motion variants also gave no gain.

The scalar checker passes 32,768 byte/doubled cases and 16,384 motion
blocks with padding. Full source build and retail DOL SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6 pass. The deduplicated report
changes only this function. The `dct` unit remains NonMatching; the
retail hash does not establish source-linked IDCT playback.

## Bink IDCT odd-intermediate reuse (2026-10-01)

`fastidct8x8d` improves from 70.72065% to 71.28745% by updating its
first-pass odd_sum with the scaled rotation instead of keeping a separate
odd_rot variable. `FastmIDCT8x8WithMotion` improves from 58.171642%
to 58.182835% by updating odd1 with its scaled tail value instead of
keeping a separate odd_scaled1. Arithmetic and output-store order are
unchanged. The equivalent forms in the other paths were unchanged or
regressed, and were discarded.

The scalar checker passes 32,768 byte/doubled cases and 16,384 motion
blocks with padding. Full source build and retail DOL SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6 pass. The deduplicated report
changes only these two functions. The `dct` unit remains NonMatching;
the retail hash does not establish source-linked IDCT playback.

## Bink byte IDCT odd0 scale reuse (2026-10-01)

`fastidct8x8` improves from 61.06073% to 63.603237% by updating odd0
with its scaled value before constructing odd row 6. This preserves
the fixed-point arithmetic and output-store order. Reusing odd_pair0
for the scaled pair difference independently reaches 62.137653%, but
combining both changes reaches only 62.68826%, so only odd0 reuse is
retained. The equivalent doubled/motion trials regress and were restored.

The scalar checker passes 32,768 byte/doubled cases and 16,384 motion
blocks with padding. Full source build and retail DOL SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6 pass. The deduplicated report
changes only byte IDCT. The `dct` unit remains NonMatching; the retail
hash does not establish source-linked IDCT playback.

## Bink IDCT final-pass intermediate reuse (2026-10-01)

Reusing a8 for its fixed-point scaled value in the final pass improves
`fastidct8x8` from 63.603237% to 63.765182% and `fastidct8x8d` from
71.28745% to 71.951416%. Motion IDCT instead benefits from reusing
b1 for the scaled odd-pair difference, improving from 58.182835% to
58.33209%. Each update occurs after the old value's last use. Fixed-point
arithmetic and output-store order are unchanged. The alternative reuse
form in each path regresses and was discarded.

The scalar checker passes 32,768 byte/doubled cases and 16,384 motion
blocks with padding. Full source build and retail DOL SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6 pass. The deduplicated report
changes only these three functions. The `dct` unit remains NonMatching;
the retail hash does not establish source-linked IDCT playback.

## Bink motion-IDCT tail reuse (2026-10-01)

`FastmIDCT8x8WithMotion` improves from 58.33209% to 58.347015% by
subtracting the odd rotation into the already-scaled `odd1` temporary before
computing the cross term. The arithmetic and fixed-point rounding stay the
same. Byte and doubled IDCT scores remain 63.765182% and 71.951416%.

The scalar reference check passes 32,768 byte/doubled cases and 16,384
motion blocks, including padded pitches. The full source compilation and
retail DOL hash check pass, with no other function-score changes. `dct.c`
is still NonMatching; the normal DOL link uses its original object, so that
hash does not verify source-linked Bink playback.

Other rotation-sum reuse forms regressed or were unchanged. This pass also
rejected new iAnimSKB multiply-order, zCombo local-lifetime, and glare-render
macro/indexing experiments; all three game sources were restored and rebuilt.

## Bink audio reinitialization match (2026-10-01)

`NGC_SoundReinit` now matches all 348 bytes (94.655174% -> 100%). Resetting
an ARQ request's owner through `NGC_SOUND_STATE(snd)` instead of the cached
`state` local gives the retail register allocation, including the owner copy
before the cleanup loop. Both expressions identify the same embedded sound
state; the calls, busy-latch checks, and owner values are unchanged.

`Lock` improves from 92.21429% to 92.78571% by reading the channel stride
through its cached state pointer in both cursor branches. Separate owner
locals, later state initialization, and other direct-access forms did not help.

The full source build and retail DOL SHA-1 check pass. The project-wide
report changes only these two function scores, both upward. `ngcsnd.c`
remains NonMatching, so the normal DOL still links its original object;
these checks do not establish source-linked audio playback.

## Bink audio upload addressing (2026-10-01)

`NGC_SoundPlay` improves from 97.395065% to 99.87654%. A branch-local
right-channel task index reproduces retail's separate address calculation;
the same index form is already used by starvation recovery. Loading the
play cursor before adding the upload length further improves register
allocation. The remaining difference is the cursor load and its use in the
addition (r29 instead of retail r11), with both bodies measuring 324 bytes.

The full source build and retail DOL SHA-1 check pass. The project-wide
report changes only this function's score; the previous reinitialization
and lock gains are preserved. `ngcsnd.c` is still NonMatching and the normal
DOL uses its original object, so the hash does not verify source-linked
Bink audio playback. Other tested pointer, cursor, and array forms were
unchanged or worse and were discarded.

## Bink readiness and starvation recovery (2026-10-01)

`NGC_StarvedClear` improves from 98.19588% to 99.93814%. Once a free ARQ
task is found, the completed polling counter is reused as the selected
staging-buffer index. Busy iterations branch before this assignment, so
the retry bound is unchanged. Reading the stereo buffer through `state`
also fixes the callee-saved register permutation. The only remaining
instruction difference loads that field from state+0x2c instead of
snd+0xa4; the addresses are equivalent.

`Ready` improves from 96.223305% to 96.660194% by reading the address shift
through `NGC_SOUND_STATE(snd)`. Other cursor-order and lock-buffer access
variants did not improve their baselines and were discarded.

The full source build and retail DOL SHA-1 check pass, and the project-wide
report changes only these two scores, both upward. `ngcsnd.c` remains
NonMatching: the DOL uses its original object and does not validate
source-linked audio playback.

## Tiki source linking (2026-10-01)

After staging's `thunderCountCB` match, `zNPCTypeTiki.cpp` now links from
source: all 48 functions, 14,492 code bytes, and 1,424 data bytes are exact.
The promotion requires only changing the unit to Matching; no source,
split, weak-helper placement, or compiler changes are needed.

The full source build reproduces retail DOL SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. The generated link selects the
source Tiki object, the report marks the unit complete, and every function
score elsewhere remains unchanged.

## OOB state source linking (2026-10-02)

`zEntPlayerOOBState.cpp` now links from source: all 86 functions, 11,124 code
bytes, and 2,576 data bytes are exact. Its previous object-level 100% hid
layout differences in virtual tables, inline callbacks, and the float pool.

Declaring the state virtual methods inline makes CodeWarrior emit the tables
in retail's constructor-driven order. An implicit `ztalkbox::callback`
constructor and the tutorial callback body in the `.cpp` restore their code
positions. Two discarded C++ helpers retain the early 0.5f/3.0f pool entries
and emit the empty base callbacks before `oob_state::update`. These helpers
are layout scaffolding, not recovered original function bodies; neither adds
runtime code. No compiler, split, linker, or assembly changes are needed.

The full source build passes, the report marks OOB complete, and the linked
DOL reproduces retail SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6. Every
project-wide function score is unchanged from the verified Tiki baseline.

## Standard NPC goal completion and source linking (2026-10-02)

`zNPCGoalStd.cpp` now matches and links all 96 functions: 15,916 code bytes
and 784 data bytes. Its last missing function was the 56-byte
`xListItem<xGoal>::Remove`, which now matches exactly.

The source-likely fix is the list-item destructor calling `Remove`. It both
instantiates the missing method and places the list-item constructor before
the factory inline group, matching retail. The template removal body moves
from `xBehaveMgr.cpp` into an included `xListItem.inl`, so the destructor can
instantiate it in each consuming unit. `NPCC_DstSq` moves unchanged into
`zNPCSupport.h` as an inline, and `GIDOfSafety` moves unchanged into
`xBehaveMgr.h`; these placements restore the remaining weak-function order.
No artificial callers, constructor specializations, compiler changes, or
assembly remain in the final implementation.

The full source build reproduces retail DOL SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6 with the standard-goal source object
selected. The report marks the unit complete. Its missing `Remove` is the
only project-wide function-score change, from absent to 100%; no symbols
or earlier matches regress.

## BinkOpen completion (2026-10-02)

`BinkOpen` improves from 98.50099% to 100% in the deduplicated report,
matching the retail function's 4,040 bytes. Shared failure paths sit at their
first use, and preload-allocation failure now frees the Bink object before
reporting out of memory, as retail does. A shared track-loop index and the
height-before-width scaling-case order recover the remaining register and
branch layout. No assembly, compiler patches, or split changes are involved.

Raw objdiff still annotates equivalent R_PPC_NONE instructions and names the
integer-to-double bias constants differently; the deduplicated report resolves
these to 100%. The full source build passes and the normal DOL retains SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. `binkread.c` remains NonMatching:
this does not establish source-linked Bink playback or complete the unit.

The verified project report changes only BinkOpen and loses no symbols.
An unexpected check_hide_entities regression disappeared after rebuilding
zCutsceneMgr without source changes; the compiler file hash was unchanged.
The cause of that inconsistent object remains undetermined. Audio, keyframe,
DoFrame-tail, and ngcrad3d dimension experiments produced no retained gains.

## Bink audio-open refinement temporaries (2026-10-02)

`BinkAudioDecompressOpen` improves from 99.55556% to 99.82222% (900 bytes).
The second and third Newton refinements use their error expressions directly,
keeping the first error temporary. Arithmetic, double precision, and operation
order are unchanged; the remaining six operand differences are register choices
in the first refinement at the two inline sites. No assembly was added or changed.

All-nested, separate-product/correction, and initial-estimate reuse variants
scored lower and were discarded. The final all-source build uses a private
copy of the unchanged compiler distribution under build/compilers-isolated.
This avoids sharing mutable compiler files with another worktree. Its full
report changes only BinkAudioDecompressOpen, removes no symbols, and retains
check_hide_entities at 100% without a corrective compile. Earlier full builds
using the shared compiler path reproduced that function's intermittent 91.047%
result; six direct recompiles produced identical exact objects. Isolation's
successful build is not proof of the discrepancy's cause.

The normal DOL remains byte-identical to retail, SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. Audio decoding remains NonMatching;
source-linked playback is not established. Validation artifacts:
build/binkacd-isolated-validation.log and build/binkacd-isolated-report.json.

## Bink DVD frame-reader completion (2026-10-02)

`BinkFileReadFrame` improves from 98.44388% to 100%, matching all 784 bytes.
A shared scratch value holds the buffered-seek distance, timer sample, and
final buffer-size calculation at their separate uses. Updating ForegroundTime
directly in each read path recovers the remaining register allocation. The
read, interrupt, volatile-access, and timing operations keep their behavior.
No compiler, assembly, split, or linker changes are involved.

The full source build with the isolated compiler copy passes. The full report
changes only BinkFileReadFrame, removes no symbols, and retains all previous
matches. The normal DOL retains retail SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. Reports and log:
build/ngcfile-frame-final-report.json and build/ngcfile-frame-final-validation.log.

The unit now has 12 of 13 exact functions; BinkFileReadHeader remains 99.75%.
Cursor lifetime, signedness, and evaluation-order trials did not improve that
holdout and were restored. ngcfile.c remains NonMatching, so this is not yet
source-linked DVD I/O or a playback validation.

## Bink frame-decoder cursor lifetime (2026-10-02)

`BinkDoFrame` improves from 98.74114% to 99.57493%. Walking the input past
its decoded-size word restores retail's initial cursor stack store. Advancing
the compressed-frame cursor at the end of each track removes the saved next
pointer and restores the branch and add scheduling. The payload temporary is
also unnecessary. Updating both decoded-frame fields with one assignment
chain slightly improves the final store order. These are ordinary C changes;
no assembly, compiler, split, or linker edits are involved.

The source function is now 1,464 bytes against retail's 1,468. One register
copy, a pair of load positions, and final bookkeeping-store order remain.
Cached byte counts, wider local scopes, signedness, raw pointer variants, and
register hints did not improve this result. Keyframe shared-exit and loop-shape
experiments also scored below their baseline and were restored.

Full all-source compilation passes with the isolated compiler copy. The full
report changes only BinkDoFrame and removes no symbols; the normal DOL retains
retail SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6. Artifacts:
build/binkread-doframe-cursor-final-report.json and
build/binkread-doframe-cursor-final-validation.log. binkread.c remains
NonMatching, so this does not establish source-linked movie playback.

## Tiled RGB16 chroma-table correction (2026-10-02)

The four colored RGB16 kernels selected the opposite clamp tables from retail:
the U-derived contribution must bias clamp_r, and the V-derived contribution
must bias clamp_b. The target object's address arithmetic and the existing
scalar RGB565 conversion in yuv.cpp independently show this pairing. Correcting
it fixes the reconstructed tiled conversion's channel placement. Halfword luma
extraction, clamp-pointer lifetime, and alpha expression order also recover
more of retail's code generation without assembly or compiler changes.

The deduplicated ngcrgb unit score rises from 84.92959% to 85.381485%:

| Function | Before | After |
| --- | ---: | ---: |
| YUV_16_4x2_even | 77.10185% | 77.07407% |
| YUV_16x2_4x2_even | 76.943726% | 78.09091% |
| YUV_16a4_4x2_even | 62.104694% | 64.595665% |
| YUV_16a4x2_4x2_even | 61.277027% | 62.68919% |

The first kernel's 0.02778-point decrease is an intentional tradeoff for the
confirmed table correction; retaining the old pairing would preserve wrong
pixel values. No other project function changes score and no symbols disappear.
Delta-bundle countdown experiments produced no gain and were restored.

The new tools/check_bink_rgb16.py compiles the production kernels with a host
compiler and compares them to a separate scalar pixel and tile-address oracle.
It uses synthetic valid lookup tables and packed words modeling big-endian
loads. All 13,568 cases pass across normal/doubled output, alpha, padded pitches,
row/tile boundaries, pointer advances, and untouched output guards. The prior
source fails the first case (0x58005800 versus expected 0x000b000b), confirming
that the check detects the repaired channel error.

Full all-source compilation passes and the normal DOL retains retail SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. Artifacts:
build/ngcrgb-chroma-integrated-report.json and
build/ngcrgb-chroma-integrated-validation.log. ngcrgb remains NonMatching;
the host check does not establish source-linked GameCube movie playback.

## Bink tiled RGBA conversion (2026-10-02)

`YUV_32a_4x2_even` improves from 79.65000% to 99.58182% in the
full deduplicated report (880 bytes). The low alpha pair shifts alpha and
red independently before combining them, matching retail's expression
shape. Context pointers load after destination tile setup, and chroma words
load before luma and alpha. These are ordinary C changes; the packing
expression is algebraically equivalent. Remaining differences concern
input pointer registers and context load/store scheduling.

The ngcrgb unit improves from 85.381485% to 86.83108%. No other function
scores change, and no symbols disappear. The doubled RGBA experiments
were rejected because their match scores decreased.

`tools/check_bink_rgb32.py` validates all four colored RGB32 kernels against
an independent scalar pixel/tile oracle: 13,568 cases cover alpha, color,
normal and doubled pixels, multiple pitches and row/tile boundaries,
context advancement, and output guards. A negative control that drops the
last alpha byte fails as expected. Non-alpha kernels write zero alpha,
as their matching retail implementations do. Input words are represented
as big-endian numeric values for host testing.

The full source build passes and the linked DOL retains retail SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. ngcrgb remains NonMatching;
that DOL check does not validate playback of these source kernels.

## Bink doubled-pixel RGBA conversion (2026-10-02)

`YUV_32ax2_4x2_even` improves from 71.53846% to 76.44551% in the
full deduplicated report. Alpha is extracted as an unshifted byte before
constructing the duplicated AR halfwords, matching retail's shift sequence.
Channel lookups use the shared YUV table structure directly; their evaluation
order and green-sum operand order now agree more closely with retail.
These are ordinary C expression and local-variable changes, with no compiler
patches or assembly. Remaining differences include register allocation,
context-pointer spills, and loop scheduling; the function is not exact.

All 13,568 cases in `tools/check_bink_rgb32.py` pass. The complete source
build passes, no other function scores change, and no symbols disappear.
The linked DOL retains retail SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. ngcrgb remains NonMatching, so
that hash does not validate source playback. Further context-load ordering,
alpha temporary types/lifetimes, and pointer-advance experiments decreased
scores and were restored.

## Bink doubled RGB16 table access (2026-10-02)

The doubled RGB16 kernels now access clamp-table bases directly instead of
keeping redundant pointer locals. The alpha variant also accesses channel
contributions through YUVTables and constructs biased pointers in RGB order.
These are ordinary C lifetime/evaluation-order changes; the previously
verified U-to-clamp_r and V-to-clamp_b mapping is preserved.

The final deduplicated report improves `YUV_16x2_4x2_even` from 78.09091%
to 78.13420% and `YUV_16a4x2_4x2_even` from 62.68919% to 64.26689%.
The ngcrgb unit rises from 87.33719% to 87.49487%. No other function scores
change, and no symbols disappear. The analogous non-doubled alpha cleanup
was restored because its raw-score increase vanished after normalization.
Additional halfword-alpha extraction, alpha/channel packing order, explicit
luma-table caching, and pointer-increment experiments were rejected.

`tools/check_bink_rgb16.py` passes all 13,568 scalar-reference cases. The
full source build passes and the linked DOL retains retail SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. ngcrgb remains NonMatching;
source playback is not established by this DOL hash.

## Bink lossy bitplane traversal (2026-10-02)

`readlossy` improves from 97.03770% to 99.756096% in the full deduplicated
report. Node traversal uses a guarded do/while loop, with its refill count,
empty-node value, and active-coefficient pointer initialized before the
negative mask. Explicit byte conversion of the next plane matches retail's
counter truncation. Refinement reuses the scratch word for the coefficient
index and the existing delta temporary for arithmetic; an explicit cutoff
count snapshot preserves the update on both exit and continuation paths.

These are ordinary C changes, without compiler patches or assembly. The
remaining difference is a missing counter-copy instruction and its compare
operand: source is 1,800 bytes versus retail's 1,804. The alternative that
keeps this copy schedules the increment differently and scores lower.

The host bitplane checker passes 6,911 nonempty lossy round trips, 27,644
early-cutoff checks, and 16,384 lossless round trips with guard, bit-length,
and reservoir validation. The full source build passes, no other function
scores change, and no symbols disappear. The linked DOL retains retail
SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6; bitplane remains
NonMatching, so this hash does not establish source playback.

## Bink lossless final-plane coefficient stores (2026-10-02)

`ReadBPLossless` improves from 89.64075% to 90.614456% in the full
deduplicated report. Its maximum-plane counter uses a 32-bit local while
the stream read still extracts the same four-bit value. Final-plane child
and deferred-coefficient destinations are computed before their sign bits
are read. Signs use the existing signed 16-bit coefficient temporary,
matching retail's negative-one value instead of constructing an unsigned
65535 in the bit-reading scratch word. Stored coefficient bits are unchanged.

The bitplane checker passes 6,911 nonempty lossy round trips, 27,644 early
cutoff cases, and 16,384 lossless round trips, including bit lengths,
reservoir state, and output guards. The full source build passes; no other
function scores change and no symbols disappear. The linked DOL retains
retail SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6. bitplane remains
NonMatching; the DOL hash does not establish source playback.

Boolean had-level flags, shared node/base indices, broader counter changes,
and explicit traversal-constant caches were tested and rejected. Remaining
lossless differences include register allocation, address evaluation order,
and final-plane instruction scheduling.

## Bink lossless writer table lifetimes (2026-10-02)

`WriteBPLossless` improves from 91.711075% to 92.128746% in the full
deduplicated report. The group-depth loops share one `group_bits` temporary,
and a pointer to the high-group depth table is initialized after the first
group loop and reused for table construction, root setup, and traversal.
The tree and encoded bitstream operations are otherwise unchanged.

The bitplane checker passes 6,911 nonempty lossy round trips, 27,644 early
cutoff cases, and 16,384 lossless round trips with bit-length, reservoir,
and guard checks. The full source build passes, no other function scores
change, and no symbols disappear. The linked DOL retains retail SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6; bitplane remains NonMatching.

Decoder address-expression and early input-cursor variants were rejected.
Writer header ordering, separate root-depth locals, and byte-cast level
updates also scored lower. Retail visibly increments the 16-bit entry for
successive children; that shape was tested with all table-lifetime variants
but currently scores below the retained version and was restored.

## Bink lossless root setup and length calculation (2026-10-02)

`LenBPLossless` improves from 93.133675% to 94.40360% in the full
deduplicated report. A high-group pointer is initialized after the group
loop and used for table construction and traversal; root depths remain
direct array accesses. The restart pointer is preserved when roots are
initialized, and the initial end pointer is derived from the root count
before the individual coefficient roots are filled. This equals the former
tree.nodes address and follows retail's pointer setup more closely.

Direct root-depth accesses also improve `WriteBPLossless` from 92.128746%
to 92.25000%, while its cached high-group traversal pointer remains.
Other child-depth temporary, flag-type, root-depth scope, and combined
lifetime variants were rejected after successful compilation and scoring.

The bitplane checker passes 6,911 nonempty lossy round trips, 27,644 early
cutoff checks, and 16,384 lossless round trips with bit-length, reservoir,
and output-guard checks. The full source build passes; no other function
scores change and no symbols disappear. The linked DOL retains retail
SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6. bitplane remains
NonMatching, so that hash does not establish source playback.

## Bink lossy writer root setup (2026-10-02)

`WriteBPLossy` improves from 94.43585% to 95.90685% in the full
deduplicated report. Its high-group pointer is initialized after the group
loop and reused for table construction and traversal, while root-depth
reads remain direct array accesses. The insertion pointer is preserved
when roots are initialized, and the child-node pointer is set before the
DC root store. The next-plane update explicitly casts to a byte, following
retail's truncation and comparison instructions.

The bitplane checker passes 6,911 nonempty lossy round trips, 27,644 early
cutoff checks, and 16,384 lossless round trips with bit-length, reservoir,
and output-guard checks. The full source build passes; no other function
scores change and no symbols disappear. The linked DOL retains retail
SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6. bitplane remains
NonMatching, so this hash does not establish source playback.

Shared group-depth locals and byte-sized child-depth temporaries were
also tested and rejected. Remaining differences include register
allocation, child-depth truncations, and instruction scheduling.

## Bink lossy writer child-depth reads (2026-10-02)

`WriteBPLossy` improves from 95.90685% to 96.14938% in the full
deduplicated report. A coefficient-depth table pointer is established
before the plane loop, and child depths are read directly instead of
passing through the root-entry temporary. The first child bit is written
before the advancing child pointer is assigned. The absolute-value pass
also reuses its signed scratch value for the resulting magnitude.

The bitplane checker passes 6,911 nonempty lossy round trips, 27,644 early
cutoff checks, and 16,384 lossless round trips with bit-length, reservoir,
and output-guard checks. The full source build passes; no other function
scores change and no symbols disappear. The linked DOL retains retail
SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6. bitplane remains
NonMatching, so the DOL hash does not establish source playback.

Byte-sized child temporaries, reordered child increments, direct stores
inside the root-selection branches, and alternate header scratch reuse
were tested and rejected. Remaining differences include register
allocation and instruction scheduling.

## Bink lossless writer pointer setup (2026-10-02)

`WriteBPLossless` improves from 92.25000% to 92.58383% in the full
deduplicated report. The restart pointer is saved as soon as the roots
are selected, and the child-node end pointer is initialized before the
individual coefficient roots. The absolute-value pass reuses its signed
scratch value for the resulting magnitude, as in the lossy writer.

The bitplane checker passes 6,911 nonempty lossy round trips, 27,644 early
cutoff checks, and 16,384 lossless round trips with bit-length, reservoir,
and output-guard checks. The full source build passes; no other function
scores change and no symbols disappear. The linked DOL retains retail
SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6. bitplane remains
NonMatching, so this hash does not establish source playback.

Separate coefficient-table caches, incrementing child entries and pointers,
and explicit reservoir-flush branch variants were tested and rejected.
Several resemble retail locally but worsen the overall function match.

## Bink lossless decoder magnitude helper (2026-10-02)

`ReadBPLossless` improves from 90.614456% to 91.87185% in the full
deduplicated report. Both non-final-plane magnitude reads now use the
existing `VarBitsGet` helper instead of duplicated refill logic. Its local
refill word improves instruction scheduling and register allocation; the
explicit bit-mask local is no longer needed. The signed 16-bit extraction
preserves the former coefficient conversion and bitstream operations.

The bitplane checker passes 6,911 nonempty lossy round trips, 27,644 early
cutoff checks, and 16,384 lossless round trips with bit-length, reservoir,
and output-guard checks. The full source build passes; no other function
scores change and no symbols disappear. The linked DOL retains retail
SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6. bitplane remains
NonMatching, so the DOL hash does not establish source playback.

Alternate flag widths, initial-depth lifetimes, early root-pointer setup,
and separate sign-result temporaries were tested and rejected. An inline
local-word form gives the same score as the existing helper; the helper
is retained to remove duplicated decoding logic.

## Bink lossless decoder final-plane guards (2026-10-02)

`ReadBPLossless` improves from 91.87185% to 92.44140% in the full
deduplicated report. The final-plane cursor is initialized inside the
nonzero-plane guard, followed by a separate cursor/end comparison. This
follows retail's cursor assignment and branch order more closely. Final
positive/negative unit coefficients reuse the existing code scratch word;
the negative constant still passes through a signed 16-bit conversion,
and stores preserve the same 16-bit coefficient representation.

The bitplane checker passes 6,911 nonempty lossy round trips, 27,644 early
cutoff checks, and 16,384 lossless round trips with bit-length, reservoir,
and output-guard checks. The full source build passes; no other function
scores change and no symbols disappear. The linked DOL retains retail
SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6. bitplane remains
NonMatching, so the DOL hash does not establish source playback.

Workspace-pointer address forms, wider/inverted plane flags, skipping
zero planes earlier, and alternate coefficient-address scheduling were
tested and rejected. Remaining differences include register allocation,
the initial-plane condition lifetime, and coefficient address formation.

## Bink lossless writer magnitude helper (2026-10-02)

`WriteBPLossless` improves from 92.58383% to 97.60479% in the full
deduplicated report. Magnitude writes use the existing `VarBitsPut` helper,
which follows retail's local size/value handling and reservoir-flush
sequence more closely. The now-unused `PUT_BP_BITS` macro is removed.
Successive children advance the 16-bit entry directly, and the next-plane
update explicitly casts the remaining bit count to a byte.

The bitplane checker passes 6,911 nonempty lossy round trips, 27,644 early
cutoff checks, and 16,384 lossless round trips with bit-length, reservoir,
and output-guard checks. The full source build passes; no other function
scores change and no symbols disappear. The linked DOL retains retail
SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6. bitplane remains
NonMatching, so this hash does not establish source playback.

Replacing the initial stream-header write with the helper scored lower,
as did advancing child-depth pointers and alternate root temporaries.
The retained helper removes duplicated bit-writing logic without compiler
changes or new assembly. Remaining differences include register allocation,
header scheduling, and child-depth addressing.

## Bink lossless header remainder and magnitude scratch (2026-10-02)

`WriteBPLossless` improves from 97.60479% to 97.65120% in the full
deduplicated report. Header flushes advance the output cursor before
computing the remaining bit count, using a local remainder instead of
reusing the earlier total count. `LenBPLossless` improves from 94.40360%
to 94.41645% by reusing the signed scratch value for the absolute magnitude
before its existing 16-bit mask and leading-zero count.

The bitplane checker passes 6,911 nonempty lossy round trips, 27,644 early
cutoff checks, and 16,384 lossless round trips with bit-length, reservoir,
and output-guard checks. The full source build passes; no other function
scores change and no symbols disappear. The linked DOL retains retail
SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6. bitplane remains
NonMatching, so the hash does not establish source playback.

Narrower plane counters, child-pointer combinations, alternate plane-loop
forms, and splitting the length calculator's final-plane guard produced
no additional gain and were restored.

## Parallel Robot rendering and YUV column matching (2026-10-02)

Separate worktrees produced six gains, verified together in the full
deduplicated report:

| Function | Before | After |
| --- | ---: | ---: |
| zNPCFodBzzt::DiscoRender | 77.588234% | 82.513370% |
| zNPCSleepy::NightLightUVStep | 67.700000% | 91.700000% |
| zNPCSleepy::RendConeOfDeath | 91.297030% | 93.282180% |
| dounaligned32colm | 95.166664% | 100.000000% |
| dounaligned32acolm2h | 94.270836% | 96.041664% |
| dounaligned32acolm2wh | 94.916664% | 97.333336% |

Robot rendering uses memset's returned vertex pointer, explicit position
component locals, expanded UV assignments, and a directly initialized
immutable direction vector. Mutable UV-rate tables were rejected despite
higher code matching because retail places those tables in .sdata2.
Robot remains at 318/328 exact functions; its overall fuzzy score improves
from 99.10535% to 99.27990%.

YUV column routines use separate row temporaries and advancing byte sample
pointers while preserving unsigned alpha shifts and output stores. The
plain grayscale column routine adds 120 exactly matched code bytes and
one exact function, bringing YUV to 86/97 exact functions. The combined
all-source build and normal link pass, with no other function-score
changes or removed symbols. The DOL retains retail SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. Both units remain NonMatching;
the DOL hash does not establish source rendering or playback.

`python tools/check_bink_yuv_columns.py --self-test` passes 27,456
independent scalar-oracle cases covering the three changed production
routines, grayscale/alpha pixels, doubled dimensions, both rows, positive
and negative pitches, phase wrap, cursor updates, and buffer/context
guards. Deliberate pixel-channel and input-cursor mutations both fail with
the expected diagnostics. This checks numeric packed output on the host;
it does not simulate the GameCube ABI or source playback.

Bungee's bounded scheduling experiments and Plankton's new initializer
variants were restored after failing to beat their baselines. Player
speech casts, rotation-pointer order, streak-zero lifetimes, and animation
copy-loop forms likewise produced no retained root-worktree changes.

## Player slippery-floor normal calculation (2026-10-02)

`zEntPlayer_Update` improves from 98.53860% to 98.71497% in the full
deduplicated report. A local computes the floor normal's horizontal squared
length before the slippery-floor X displacement; the Y displacement then
passes this value to `xsqrt`. The expression keeps the same multiplication
and addition order. Updating the entity frame's position does not modify the
separate `globals.player.floor_norm` fields. This reproduces retail's normal
loads and products before the X-position store, without assembly, compiler
changes, or volatile accesses.

All-source compilation passes. No other function scores change, no symbols
are lost, and no previously exact functions regress. The normal linked DOL retains
SHA-1 `306526d90b48e99894c3138f5fc8f2716d9fecf6`. The player unit remains
NonMatching, so that hash does not validate source-linked player gameplay.

A separate Plankton follow-up retains no change: inline diff helpers,
initializer-sequenced zeroing, nested/union aggregates, and 24 selective
volatile word-copy variants did not improve `impart_velocity` (91.12676%).
The restored unit remains 179/180 exact with all data exact. Player NPC-loop
pointer forms and volatile-count diagnostics were also discarded. Retail
reloads the NPC count and retains a dead four-byte induction variable; DWARF
records NPC shock-distance locals absent from the current empty loop.

## Bink doubled RGBA retail alpha handling (2026-10-02)

`YUV_32ax2_4x2_even` improves from 76.44551% to 86.70513% in the
full deduplicated report. Byte-sized alpha locals match the input samples;
explicit unsigned casts keep every high-byte shift defined for alpha >= 128.

Two independent inspections of the retail object confirm that the second
copy of sample 2 in the second source row uses sample 3's alpha. The source
and independent scalar oracle now preserve this behavior. For input alpha
word 0x11223344 and red 0x55, the retail AR word is 0x33554455. The checker
records the relevant retail instruction offsets and includes a deterministic
distinct-byte case; the previous idealized duplication fails that case.

Both RGB32 and RGB16 host checkers pass 13,568 cases each. Combined with the
Robot, Player, and YUV work above, the full source build and deduplicated
report verify eight function-score gains, no regressions, and no removed
functions. The normal linked DOL retains SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. These units remain NonMatching;
the host checks exercise pixel conversion, addressing, context, and guards,
while the retail hash does not establish source playback or gameplay.

## Player catch-tunnel iteration setup (2026-10-02)

`zEntPlayer_Update` improves from 98.71497% to 98.76138% in the full
deduplicated report. The catch-tunnel sweep counter is reset before forming
its endpoint instead of in the following loop initializer. The counter is
still reset on every traversal entry, and the endpoint calculation cannot
access this local. This follows retail's earlier counter initialization.

All-source compilation passes; no other function scores change, no symbols
are lost, and the exact-function count is unchanged. The normal linked DOL
retains SHA-1 `306526d90b48e99894c3138f5fc8f2716d9fecf6`. The player unit
remains NonMatching; source-linked gameplay is not established by this hash.

NPC shock-loop calculations inferred from DWARF were diagnostic only and
were discarded. They either retained unwanted position-accessor calls or
changed empty-loop unrolling without reproducing retail's count reload and
dead pointer stride. No speculative arithmetic, volatile access, assembly,
or compiler change is retained.

## Parallel Player, Robot, and Bink follow-up (2026-10-02)

The combined full deduplicated report verifies five gains:

| Function | Before | After |
| --- | ---: | ---: |
| zEntPlayer_Update | 98.714970% | 98.761380% |
| zNPCSleepy::RendConeRange | 85.966805% | 86.423230% |
| YUV_32ax2_4x2_even | 86.705130% | 89.522440% |
| dounaligned32acolm2h | 96.041664% | 97.708336% |
| dounaligned32acolm2wh | 97.333336% | 98.833336% |

The Player change is described above. Robot's cone fade stores the distance
above its inner threshold in the existing scalar before converting it to a
fraction. Generated subtraction, division, and subtraction remain in the
same single-precision order. Its register allocation improves, although the
source function grows from 968 to 972 bytes (retail is 964); it remains a
partial match. Vector-storage and other clamp experiments were restored.

Doubled RGBA reads its six independent input words in retail's order:
v, y0, a0, u, y1, a1. The alpha-column kernels use separate luma cursors for
the two rows; the doubled-height kernel also caches its first output address.
Unsigned alpha arithmetic and the previously confirmed retail quirk remain.
The YUV checker's deliberate cursor mutation is now scoped to its grayscale
control function because the same cursor name also occurs in alpha kernels.

The full source build passes. RGB32 passes 13,568 independent pixel cases;
YUV columns pass 27,456 cases and both deliberate mutation checks. No other
function scores change, no symbols disappear, and exact code/data totals
and source-linked totals are unchanged. Overall fuzzy matching improves
from 99.43971% to 99.44198%. The linked DOL retains retail SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. These units remain NonMatching,
so this does not establish source-linked rendering, gameplay, or playback.

## Player ceiling collision pointer reuse (2026-10-02)

`zEntPlayer_Update` improves from 98.76138% to 98.80427% in the full
deduplicated report. The ceiling collision's entity pointer is read through
the existing `ceil` local instead of repeating its parent/member expression.
Both expressions access the same field, with no intervening writes. This
matches retail's collision-pointer setup and object-load ordering more closely.

All-source compilation passes; no other function scores change and no symbols
are lost. The normal linked DOL retains retail SHA-1
`306526d90b48e99894c3138f5fc8f2716d9fecf6`. The player unit remains
NonMatching, so this does not establish source-linked gameplay. Wand-pointer
declaration, reference, and consistent-access variants were rejected after
compilation and scoring; the shorter consistent-access form also scored lower
in the deduplicated report. No compiler, volatile, or assembly changes were made.

## Robot exact setup/data and YUV exact scaling (2026-10-02)

`zNPCFodBzzt::Setup` improves from 95.34722% to 100% (288 bytes).
Its two unchanged local colors are now const, matching ColorSet's const
parameters and retail's complete template-load sequence before the stores.
No array, volatile access, assembly, or compiler workaround is needed.

Robot's data improves from 6,312/10,664 bytes to 10,664/10,664 (100%).
The TubeSlave animation switch had two equivalent idx=3 case bodies in the
opposite source order: TUBEATTACK and TUBEDUCKLING. Restoring retail's case
order makes their two jump-table destinations exact without changing any
animation result or function score. Exact-section accounting had excluded
the entire 4,352-byte .data section for that small table mismatch. All six
retail data/BSS sections now report 100%; Robot has 319/328 exact functions.
Further bounded DiscoUpdate and ParseChild variants produced no gain and
were restored.

`setup_scaling` in YUV improves from 99.563866% to 100% (1,284 bytes).
Four direct conditional-expression maxima replace a repeated temporary and
if-update sequence. The independent setup checker covers all mode patterns,
grayscale and channel inversion, prior table order, pixel sizes, row widths
and pitches, even/odd step combinations, dispatch functions, zoom requests,
and unchanged context/table state. It passes 37,632 cases and rejects a
maximum changed to a minimum. This exercises setup state, not playback.

Normal RGBA improves from 99.58182% to 99.704544% by writing back its U
cursor before its V cursor. The Player ceiling-pointer improvement above
is included. Full combined source compilation and deduplicated reporting
verify exactly four function-score gains, no regressions or lost symbols,
+2 exact functions (9,929 total), +1,572 exact code bytes, and +4,352 matched
data bytes. RGB32 passes 13,568 pixel cases; YUV columns pass 27,456 cases
and both mutation controls. Combined with scaling, 78,656 host cases and
three negative controls pass.

The linked DOL retains retail SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. These units remain NonMatching;
source-linked totals are unchanged, and the hash does not validate source
rendering, gameplay, or playback. Overall fuzzy matching is 99.44312%.

## Robot UV/cone and Bink alpha-column follow-up (2026-10-02)

The full combined deduplicated report verifies four gains:

| Function | Before | After |
| --- | ---: | ---: |
| zNPCSleepy::NightLightUVStep | 91.700000% | 99.600000% |
| zNPCSleepy::RendConeOfDeath | 93.282180% | 93.816830% |
| dounaligned32acolm | 94.952380% | 100.000000% |
| YUV_32ax2_4x2_even | 89.522440% | 95.006410% |

The UV step reuses one scalar rate across the four coordinate updates.
Static const rate tables and fused arithmetic are preserved; the remaining
instruction difference is address-load ordering. The cone renderer names
const references to its existing static colors, improving byte-load/store
ordering without changing their storage or values. Both functions retain
their retail sizes. Other qualifier, reference-scope, and pointer forms
were restored; all Robot data remains 100% matched.

The unscaled alpha grayscale column uses separate luma/alpha cursors and
pixel locals for its two source rows. Existing RGB32_M_A keeps alpha shifts
unsigned. This adds 168 exact code bytes and one exact function; YUV now has
88/97 exact functions and 22,008/29,376 exact code bytes. The scalar column
oracle now covers this fourth production kernel and passes 36,864 cases,
including both expected pixel/cursor mutation failures.

Doubled RGBA writes its chroma cursors back before luma and alpha cursors,
matching retail's register allocation more closely. The known retail alpha
quirk is preserved. RGB32 passes 13,568 independent pixel/context/guard
cases. Alternate channel temporaries and context-load groupings were rejected.
Player's six new immutable-vector trials produced no gain and were restored.

Combined source compilation passes. Exactly these four scores improve;
no functions disappear, no other scores regress, and matched data totals
are unchanged. Exact functions reach 9,930; overall fuzzy matching reaches
99.44714%. The linked DOL retains retail SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6. These units remain NonMatching,
with unchanged source-linked totals; the hash does not establish source
rendering, gameplay, or playback.

## Bink doubled color row and RGBA stack layout (2026-10-02)

`dounaligned32row2wh` improves from 96.07595% to 100% (316 bytes).
It follows the existing row2h sibling's count lifetime: the zero-count guard
post-decrements count, and the loop reuses that counter instead of a separate
remaining temporary. Zero counts still return without changing output or
context. YUV now has 89/97 exact functions and 22,324/29,376 exact code bytes.

The independent row oracle passes 10,368 cases covering saturated color
conversion, chroma phase/parity and unsigned wrap, zero and positive counts,
2x2 duplication, signed pitch, cursors, unchanged tables/input, and output
and context guards. Pixel-channel and chroma-advance mutations both fail as
expected. It checks host packed-word behavior, not GameCube playback.

`YUV_32ax2_4x2_even` improves from 95.00641% to 96.503204%. Pointer
declarations follow chroma then luma order, matching retail stack slots.
The final high alpha is retained in the existing unsigned av1 word through
a mask and shift instead of a new byte temporary. The retail mixed-alpha
copy is preserved. RGB32 passes all 13,568 pixel/context/guard cases.

Combined source compilation and the full deduplicated report verify exactly
these two gains, no regressions or removed functions, and unchanged matched
data totals. Exact functions reach 9,931 with 316 added exact code bytes;
overall fuzzy matching reaches 99.44842%. The normal DOL retains retail
SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6. Units remain NonMatching
and source-linked totals are unchanged; this hash does not establish source
playback or gameplay.

Unsuccessful bounded follow-ups were restored: DiscoUpdate's reused-rate
and shared alpha/UV scalar forms; four new Delta16 narrow-counter/loop forms;
six Player AbsControl shared-scratch forms; and Robot's two-element animation
array const/aggregate/pointer variants. The animation-array callee was verified
read-only, but even safe local const forms produced no gain, so no cast or
wrapper is retained. Robot data remains 100%.


## Exact doubled RGBA and alpha-column follow-up (2026-10-02)

YUV_32ax2_4x2_even improves from 96.503204% to 100%, adding one exact
function and 1,248 exact code bytes. Capturing alpha pointers before chroma
and luma, then explicitly advancing the five non-V input pointers after the
first output block, reproduces retail register allocation and scheduling.
The existing mixed-alpha retail copy and unsigned shifts are preserved.
No assembly or compiler modifications are involved.

dounaligned32acolm2w improves from 98.36957% to 99.021736% by naming the
second row's luma sample separately. The independent column oracle now
covers all five grayscale/alpha column kernels: 45,504 cases and both
negative controls pass. RGB16 and RGB32 each pass 13,568 cases, for 72,640
combined pixel/context/guard checks.

After incorporating staging's independent SkinNormals/NPCC_GenSmooth
updates, combined source compilation and a full deduplicated report verify
exactly these two additional gains, no removed symbols or score regressions,
and unchanged matched data and source-linked totals. Overall fuzzy matching
is 99.451%; exact functions reach 9,934 with 2,208,072 exact code bytes.
The linked DOL retains SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6;
these Bink objects remain NonMatching, so this does not establish target
playback. Evidence: build/parallel-sixth-{baseline-report,report}.json and
build/parallel-sixth-validation.log.

## Player bubble-spin Boolean expression (2026-10-02)

`zEntPlayer_Update` improves from 98.80427% to 98.91313% in the full
deduplicated report. The bubble-spin predicate is expressed as a short-circuit
Boolean chain instead of two explicit U8 flag locals. The animation-name,
nonzero-speed, minimum-frame, and maximum-frame tests retain their original
order and short-circuit behavior. Each form stores exactly zero or one.

This recovers retail's zero-register copy and surrounding string-address/load
schedule. It revises the earlier compiler-only classification of this specific
`li` versus `mr` site: expression-generated Boolean temporaries reproduce the
copy that explicit named flags did not. DWARF's U8 floor/wall locals belong to
the later physics block, supporting a re-examination of these earlier names.
A separate S32 condition local regressed and was discarded.

All-source compilation passes; no other function scores change, no symbols
are lost, and the exact-function count is unchanged. The normal linked DOL
retains SHA-1 `306526d90b48e99894c3138f5fc8f2716d9fecf6`. The player unit
remains NonMatching, so this hash does not establish source-linked gameplay.
No compiler, volatile, or assembly changes were made.


## Combined Player/Robot source simplifications (2026-10-02)

DiscoRender improves from 82.51337% to 85.7754% by using its unchanged
local uv_bot array directly when emitting the eight ring vertices, removing
two redundant scalar snapshots. Robot data stays 100% matched. Combined with
the bubble-spin expression above, the full source build and deduplicated
report verify exactly two improved functions, no lost symbols or regressions,
and unchanged exact-code, exact-function, data, and source-linked totals.
Independent staging changes to zLasso and NPCHazard are included in the
baseline. Evidence: build/parallel-seventh-{baseline-report,report}.json and
build/parallel-seventh-validation.log.

Bungee review confirms that four anonymous assignment bodies (176 bytes)
contain identical relocation-free instructions but different generated class
identities. start_detaching has two independent address calculations in the
opposite order. Prior source and include-order forms did not resolve these;
no artificial symbol mapping is retained. Six additional save/load shared-zero,
chained-zero, and return-initialization trials were unchanged or worse; all
were restored and the original source rebuilt.


## xClimate exact source-link completion (2026-10-02)

UpdateRain improves from 99.40476% to 100% (1,008 bytes). Both snowfall
percentage expressions name the normalized value before subtracting it from
one; the particle loop declares and computes xx before zz, as supported by
DWARF. Arithmetic and calls are unchanged. The obsolete compiler-mismatch
comment is removed.

xClimate now matches all 12 functions, all 1,772 code bytes, and all 136 data
bytes. The worker's isolated real-link test passed, and integration independently
rebuilt the full source set and linked with xClimate marked Matching. The DOL
retains retail SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6. The full
deduplicated report confirms only UpdateRain's score changes; no symbols are
lost and no other function regresses. Source-linked units increase from 450 to
451, adding 1,772 source-linked code bytes and 136 source-linked data bytes.
Both isolated MWCC hashes remain unchanged. Evidence:
build/parallel-eighth-{baseline-report,report}.json and
build/parallel-eighth-validation.log.

The root Bink pass tested 27 bounded rectangle-countdown, audio initial-estimate,
and YUV temporary/cursor forms without a retained gain. Every experiment restored
and rebuilt the original source; the restored full report exactly matched the
published baseline before xClimate integration. No compiler modifications were
made. The zTalkBox worker recorded a separate compiler-behavior investigation
packet for its two absent unreachable tail branches; this is a concrete CFG
residue, not proof that a compiler patch rather than original-source differences
is required. Register-allocation-only holdouts remain weaker patch evidence.

## 2026-10-02: xScene first ray endpoint assignment order

A bounded xScene/xModelBucket pass improves `xRayHitsGrid` from 99.820564%
to 99.8243% in the authoritative deduplicated report. Compute the first
endpoint's Y component before its X component; leave the expressions and Z
assignment unchanged. These are independent writes to a local vector, with
read-only ray fields and delta operands. No arithmetic operation or externally
visible access order changes. The function remains 2,140 bytes; xScene has
35/36 exact functions and all 184 data bytes exact. This is a small scheduling
improvement, not a completed TU, so its configuration remains NonMatching.

The other first-endpoint component orders, aggregate delta initialization,
copy-then-scale delta, and named min_t scalar did not improve the baseline.
The sole xModelBucket holdout, FullAtomicDupe, did not improve with explicit
zero fields, immutable or mutable named zero-template copies, or named/reused
RwFrameCreate results. Those changes were restored. The residual is the
second RwMemory template's load/store register scheduling.

Validation: full all_source build passes. The full deduplicated report changes
only xRayHitsGrid and its containing unit (99.959984% to 99.960815%); no other
function, section, size, or exact-function count changes. The unchanged retail
link verifies SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6. This hash check does
not assert that the still-NonMatching source object links exactly. Trial
sources, raw diffs, and scene-p1_yxz-report.json remain under the isolated
worktree's ignored build directory. Compiler binaries were not changed.

## 2026-10-02: iMath3 sphere slab scratch and xString output cursor

Two bounded source-lifetime changes improve the full deduplicated report:

- iBoxIsectSphere: 98.47305% to 98.98203%, still 668 bytes. Snapshot the
  sphere radius for the three slab classifications and reuse hi for each
  center coordinate before adding the radius. This matches retail's reuse of
  its center register; remaining differences are FP register allocation.
  The snapshot spans no calls or continuing-path writes. Any early slab
  rejection writes the result and returns. The later post-collision radius
  read remains p->r, so no value is cached across the collision helper calls.
- xStrParseFloatList: 98.809525% to 99.61905%. Assign the input cursor within
  its null check and initialize a separate output cursor after the early
  return. This recovers retail's combined move/test and removes the extra
  instruction (source 424 to 420 bytes). The remaining differences swap the
  maximum-count and output-cursor registers. Parsing, input restoration,
  output writes and null-input behavior are unchanged.

Rejected trials included separate null assignment/declaration forms, cursor
  declaration positions, comparison-expression/character-local rewrites of
  imemcmp, and radius/hi/lo scratch variants. imemcmp remains unchanged; the
  target's extra byte-to-argument move is still absent. The trial evidence is
  retained under build/imath-* and build/string-* in the isolated worktree.

Validation: all_source passes. imath-string-gains-report.json changes only
these two function scores and their containing units. No missing symbols,
regressions, data changes or exact-function-count changes. iMath3 retains
17/18 exact functions and 40/40 data bytes; xString retains 12/14 and 48/48.
Neither unit is promoted to Matching. The normal retail link SHA1 remains
306526d90b48e99894c3138f5fc8f2716d9fecf6; this is not a claim of exact source
linking for these still-NonMatching units. Compiler binaries are unchanged.


## zDiscoFloor source-link completion and combined near-TU gains (2026-10-02)

Making refresh_bound's unchanged mid_center const raises it from 96.21591%
to 100%, completing all 50 functions, 8,432 code bytes, and 992 data bytes.
The first real source link still differed from retail: independent binary
comparisons found exactly eight single-byte differences, all get_tile
relocations exchanging bit_index/init .sbss displacements. Declaring bit_index
before init restores the actual static storage order. The corrected isolated
flip test and retained Matching build pass. Integration independently rebuilt
all source and linked the Matching object, retaining retail DOL SHA-1
306526d90b48e99894c3138f5fc8f2716d9fecf6.

CruiseBubble's add_trail_sample improves from 91.456985% to 99.91936% by
making vel_rnd, off0, and off1 immutable. This corrects an obsolete source
comment that attributed every mismatch to layout: source vector-copy
scheduling was also involved. The remaining trail offsets and reset_quadrants
holdout keep this TU NonMatching; its data stays 11,028/11,028 exact.

Together with the xScene, iMath3, and xString changes above, the final full
deduplicated report verifies exactly five improved functions, no missing
symbols or regressions, unchanged matched data, and one additional exact
function (+704 exact code bytes). Source-linked units rise from 451 to 452,
adding 8,432 linked code bytes and 992 linked data bytes. Compiler hashes
remain unchanged. Evidence: build/parallel-ninth-baseline-report.json,
build/parallel-ninth-final-report.json, and
build/parallel-ninth-final-validation.log.

Eight root Bink trials (three lossless plane-guard/flag forms and five YUV
initializer forms) produced no gain and were restored/rebuilt. The immutable
flag's initial invalid C89 declaration was corrected to block scope before
scoring. Near-TU worker trials on iPad, xFont, xstransvc, zGame, and zCamera
also produced no retained change; their saved artifacts document the exact
remaining instruction differences. Earlier blanket compiler-only claims
remain suspect: source-only changes have now closed both xClimate and
zDiscoFloor and substantially improved CruiseBubble. No compiler patch was
made or presented as necessary.

## 2026-10-02: Duplotron smoke phase scratch

VFXSmokeStack improves from 99.016396% to 99.79508% in the deduplicated
report by reusing its F32 scratch for the smoke-cycle value and the subsequent
sine result. The angle expression remains `(cycle * 2.0f) * PI`; particle
counts, emission control flow and floating-point operations are unchanged.
The compiler now reads the cycle before the angle constants, as retail does.
Remaining differences are FP register allocation and multiplication operand
order. The 488-byte function remains the only holdout: 34/35 exact functions,
unit 99.89593% to 99.97832%, and 2608/2608 exact data bytes.

Separate phase locals, factor permutations and staged expressions were tested
before retaining this form. A phase multiplied through separate assignments
changed constant ordering and was rejected. The retained object's actual
.sdata2 bytes match the prior source object, including its existing 12-byte
tail after the 40-byte retail prefix; normalized data matching alone would
not have established that. No new constant or speculative calculation is
introduced, and the unit remains NonMatching.

Full all_source compilation passes. The complete deduplicated report changes
only VFXSmokeStack and its unit's text score, with no regression or other
function/section change. The normal retail-link SHA1 remains
306526d90b48e99894c3138f5fc8f2716d9fecf6; source-link exactness is not claimed.
Raw trials and final report are retained under build/duplotron-* in the
isolated worktree. No compiler or inline-assembly changes were made.


## Bink aligned motion copies and parallel TU gains (2026-10-02)

`ExpandPlane` improves from 48.724136% to 50.689655% in the full
relocation-deduplicated report. Retail's residue, inter-DCT and motion-only
blocks each branch between aligned double copies and word copies. The source
previously used only eight-iteration word loops. Restore the two unrolled
paths and advance the motion source by its pitch after each row. The residue
and inter copies target the linear stack block; motion-only copies target
pitched output. Small row macros keep the three sites readable. No compiler
changes or inline assembly are involved.

`python tools/check_bink_motion.py --self-test` passes 12,288 host cases
covering all three production copy sites, four pitches, four-byte-aligned
source/output offsets that exercise both eight-byte alignment branches,
varied bytes, padding and guards. Its negative control detects a missing
source-row advance. This tests the copy operations, not movie decoding or
GameCube playback. The full source build and normal retail DOL SHA-1 check
pass; expand remains NonMatching, so that DOL does not execute this source.

The same integration includes CruiseBubble `reset_quadrants` improving from
93.17204% to 93.27957% by capturing the selected bit in the clear branch, and
Duplotron `VFXSmokeStack` improving from 99.016396% to 99.79508% (documented
above). No other function scores or matched-data totals regress. Exact and
source-linked totals are unchanged: 9,937 exact functions and 452 complete
units. Overall fuzzy progress is 99.47452%, up from 99.46954%.

Other bounded trials were restored: DCT const inputs and per-pass row
lifetimes; scaled raw bundle advancement; normal pattern unrolling; direct
scaled mask-table output. The latter two structures are visible in retail
but the tested forms still reduce the overall function match. Revisit them
with further layout work rather than treating a lower score as completion.
The initial restored-source object was newer than a copied candidate file;
the final validation explicitly refreshed the source timestamp and rebuilt
expand before generating `build/parallel-tenth-final-report.json`.

Near-TU passes on iSystem, zAssetTypes, zMain and save/load retained no changes.
zAssetTypes' data relocation names resolve to the same verified boundary
address; introducing an unrelated symbol cast would obscure the source.
CruiseBubble's early-inline literal-order experiment makes the trail symbol
exact but shifts another method's code position, so it was rejected. No
new evidence here establishes a required compiler patch. Any patch proposal
still needs a focused reproducer, explanation and exact-neighbor regression
checks for review.


## Bink sequential pattern rows (2026-10-02)

`ExpandPlane` improves from 50.689655% to 51.981068% in the full
deduplicated report. The normal pattern block now emits eight row operations
and advances its destination cursor after each row, matching retail's
unrolled body. A small inline row helper reads one pattern byte and combines
the same two colors through the existing mask tables. Neither pixel values
nor color/pattern bundle consumption change. Indexing unrolled rows from the
original base was worse; the sequential cursor is the relevant source detail.

`python tools/check_bink_patterns.py --self-test` passes 262,144 host cases:
all 256 row patterns, 256 generated color pairs, four pitches, padded output
and bundle consumption. Expected pixel words are built independently from
the pattern bits; source mask words are interpreted in target big-endian
order. Two negative controls detect a missing destination-row advance and
incorrect mask selection. This checks block expansion, not movie playback.
The full source build and retail DOL SHA-1 check pass. Expand remains
NonMatching; exact and source-linked totals do not increase.

Baseline `build/parallel-tenth-merged-report.json` includes the independently
published zCameraUpdate and zEntPlayer_Init exact matches. The first full
report for this pass, `build/parallel-eleventh-report.json`, changes only
ExpandPlane, with no symbol or data regression. Its overall fuzzy score is
99.47781%, versus the combined baseline's 99.47465%.

Direct scaled-pattern output and an extra advancing motion destination
cursor were again measured with the new structure and were lower; restored.
Worker camera const/reference variants and iTRC rectangle/token lifetime
variants were neutral or worse and were restored. The remaining camera
scheduling edge is already recorded in docs/COMPILER_VARIANTS.md; this pass
adds no compelling new case for a compiler patch.


The final integration also narrows Robot `RendConeRange`'s temporary vertex
to its loop, improving 86.42323% to 86.46473%. The combined full report
(`build/parallel-eleventh-combined-report.json`) confirms only that additional
gain, no data or function regressions, and an overall fuzzy score of 99.47782%.
Exact functions remain 9,939 and complete units remain 452. Bungee trials
were restored. All-source compilation and the normal retail hash pass.


## Bink fill-row cursor (2026-10-02)

`ExpandPlane` improves from 51.981068% to 55.563896% in the full
deduplicated report. Its normal fill block now advances a local destination
cursor after each of the first seven rows instead of computing every row
from the original base. The eight two-word stores, repeated color, work mark
and one-byte color-bundle consumption are unchanged. Retail's fill path at
801A7E10-801A7E94 uses this sequential destination structure.

`python tools/check_bink_fill.py --self-test` passes 8,192 host cases covering
all byte colors, four pitches, two word-aligned output offsets and four work
indices. It checks pixels, padding, guards, work marks and bundle consumption.
Negative controls detect a missing row advance and missing bundle consumption.
This exercises the production case body on a host, not a movie or GameCube.
Full source compilation and the normal retail DOL hash pass. The expand unit
remains NonMatching, so the linked DOL does not execute this decoder source.

The complete report `build/parallel-twelfth-report.json` improves only
ExpandPlane against `build/parallel-eleventh-combined-report.json`; data,
function identities and exact/source-linked totals are unchanged. Overall
fuzzy progress increases from 99.47782% to 99.48658%.

Raw-copy destination cursors helped alone but reduced the combined fill
result. Skip cursors, staged fill construction, direct scaled patterns, and
the remaining-width loop also scored lower. All those trials were restored.
No compiler changes were made. Workers' four bounded zScene/zLightning
scope/lifetime variants were likewise neutral or worse and restored.


## 2026-10-02: Dutchman animation template restores exact data

A direct retail-object audit corrects the earlier claim that Dutchman's
animation list was unterminated. `ZNPC_AnimTable_Dutchman` references the
52-byte `.rodata` object `@1674` at offset 540. Its thirteen words are:
`1, 11, 4, 5, 6, 12, 13, 14, 16, 17, 18, 19, 0`.
The source template instead contained
`1, 4, 5, 6, 7, 11, 12, 13, 14, 16, 17, 18, 19`.
Function-code matching had hidden the incorrect copied data.

Move Death01 (11) after Idle01, remove Taunt01 (7) from this standard-transition
list, and include the retail zero explicitly. The Taunt animation state itself
remains present, as in retail. Both matching and NON_MATCHING builds now have
the same thirteen-entry terminated list; Dutchman no longer uses the conditional
terminator macro. A follow-up audit also confirms Prawn already uses its
terminated retail list, and the now-unused macro is removed. The stale header
comment and PCPORT explanation are corrected accordingly.

Validation: all_source rebuilds all 55 affected objects successfully. The full
deduplicated report changes only Dutchman's data: 5280/8840 to 8840/8840 bytes
(59.728508% to 100%). All 227 function records are unchanged, with 224 exact;
there are no other unit changes or regressions. A separate compilation with
-DNON_MATCHING into ignored scratch output and the normal matching object both
contain the exact thirteen-word retail template once. The normal retail link
passes SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6. The three existing code
holdouts remain, so the unit is not promoted and exact source linking is not
claimed. Evidence: build/dutchman-anim-report.json, dutchman-anim-modes.log and
the ELF/template audit helpers in the isolated worktree. Plankton's exhausted
impart_velocity holdout is unchanged; no compiler or assembly edits were made.


## 2026-10-02: Prawn terminator follow-up removes stale workaround claim

The Prawn audit follows relocations inside ZNPC_AnimTable_Prawn to retail
@1446, .rodata offset 224, size 40 bytes. Its ten words are
`1, 4, 5, 7, 12, 13, 14, 9, 10, 0`.
The current source references @597 at offset 260 with the identical ten words.
Thus retail and source both have nine animation indices and a terminator.
Prawn already uses an explicit zero and does not need a conditional workaround.

Remove the now-unused NPCC_ANIM_LIST_END macro and correct the remaining
Prawn-workaround statements. A repository source search finds no call sites.
No initializer, gameplay logic, compiler setting, or Matching marker changes.
Prawn remains Object(Matching), with 128/128 exact functions and 4600/4600 data
bytes. Full all_source and normal link validation pass; the full deduplicated
report is identical to the preceding Dutchman correction. Retail DOL SHA1
remains 306526d90b48e99894c3138f5fc8f2716d9fecf6. Both earlier claims of
unterminated retail lists were based on insufficient initializer-data evidence.


## Combined fill, FX and Dutchman verification (2026-10-02)

The final batch also improves `SkinXformVertAndNormal` from 86.95489% to
91.54135%. Each weighted loop captures its selected scratch matrix before
shifting the packed bone indices, and the normal-loop counter is initialized
before resetting the accumulator. The same matrices, weights, arithmetic and
iteration limits are used; this recovers retail's pointer/counter lifetimes.
zFX data remains 26696/26696, and no other FX function changes.

The full integrated report `build/parallel-twelfth-final-report.json` changes
only that function and Bink ExpandPlane versus staging 46e0b5f10, plus the
Dutchman data correction documented above. No function scores, identities or
matched-data totals regress. Matched data increases by 3560 reported bytes to
1264032/1280684 (98.69975%). Overall fuzzy matching is 99.48759%; exact
functions remain 9939 and source-linked complete units remain 452.

All-source compilation and the normal retail DOL link/check pass with SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. Removing the unused terminator macro
leaves the entire combined report unchanged. Both MWCC compiler hashes are
unchanged; no compiler patch or new inline assembly was used. Bink and FX
remain NonMatching, and Dutchman still has three code holdouts.


## Bink scaled fill row pairs (2026-10-02)

`ExpandPlane` improves from 55.563896% to 55.901283% in the full
deduplicated report. The scaled fill case writes two successive 16-byte rows
per iteration, advancing a local output cursor through eight row pairs. This
matches retail's paired-row loop structure. The previous source iterated over
sixteen indexed rows; both write the same 16x16 block and consume one color.

The fill checker now exercises both production fill cases. Its 14,336 cases
cover every byte color, normal and scaled widths, valid padded pitches, word
alignment offsets, output guards, normal work marks and color-bundle advances.
Three negative controls detect missing normal/scaled row advances and missing
bundle consumption. Scaled dispatch's later work marking is outside the case
body checked here. This remains a host block test, not movie playback.

All-source compilation and the normal retail DOL check pass. The complete
`build/parallel-thirteenth-report.json` changes only ExpandPlane relative to
`build/parallel-twelfth-final-report.json`; data and exact/source-linked totals
are unchanged. Overall fuzzy progress is 99.48842%. The unit remains
NonMatching. Sequential pattern-color reads helped alone but lost the gain
when combined with paired fill; direct scaled patterns also scored lower.
Those variants and the neutral motion-offset ordering trial were restored.


## 2026-10-02: Dutchman update_turn matches through the established yaw scratch

Dutchman's update_turn improves from 94.30769% to 100%, adding 260 exact code
bytes. Use the same one-element F32 yaw array already present in the Plankton
and SB2 turn helpers, plus Plankton's matrix local and immutable facing-vector
initializer. Name the target yaw before computing the difference. The array
retains retail's frsp at the stored-yaw/target-angle sum, and the matrix/facing
form recovers the template-copy registers. Wrap thresholds, atan calls,
acceleration arguments and matrix update are unchanged. This is an explicit
source workaround for the observed rounding, not a claim that retail's local
was originally an array.

The historical scalar/double NO-GOs above did not exhaust source possibilities.
An array-only candidate recovered the rounding but retained the initial copy
register difference; the established sibling form resolves both. The final raw
diff has only anonymous zero-template symbol-name differences, and the
normalized function is exact. Coordinate snapshots in turn_to_face and explicit
or shared default labels in LassoNotify did not improve their baselines and
were restored. Those two holdouts remain; the TU stays NonMatching.

Validation: full all_source passes. The complete deduplicated report changes
only update_turn and its containing unit, with no missing symbols, regressions
or data changes. Dutchman advances from 224/227 to 225/227 exact functions,
38928/39484 to 39188/39484 exact code bytes, unit 99.93152% to 99.969%, and
retains 8840/8840 exact data bytes. The normal retail link verifies SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6; exact source linking for the whole
unit is not claimed. Evidence remains under build/dutchman-turn-* and
build/dutchman-other-* in the isolated worktree. No compiler changes or inline
assembly are used.


Final integration also retains the FX skinning loop's explicit index
initialization before packed-shift setup, improving 91.54135% to 91.57895%.
`build/parallel-thirteenth-final-report.json` confirms exactly three improved
functions versus staging 5a4ca721c: Bink ExpandPlane, the FX skinning helper,
and Dutchman update_turn. No function identities, scores or matched-data totals
regress. Exact functions increase to 9940 and exact code to 2217572 bytes;
complete source-linked units remain 452. Overall fuzzy progress is 99.489044%.
Full source compilation and the retail DOL link/hash check pass. Dutchman has
two code holdouts and remains NonMatching. No compiler changes were made.


## Correct scaled-block lower-row dispatch (2026-10-02)

A retail control-flow audit found a real ExpandPlane error. On a lower block
row (`row & 8`), the source broke out of the scaled case before its extra
column advance and work marks. Retail's branch at 801A8410 goes to 801A8950,
which marks both covered work positions and advances output, previous-frame
and column cursors by an extra eight pixels before the common eight-pixel
advance. It consumes one block-type byte, no subtype byte and no payload.
The old source advanced only eight pixels, omitted the marks, and would read
another block type for the second half of the same sixteen-pixel block.

Guard only the subtype decoding with the row-parity check, leaving scaled
advancement and work marking common to both rows. The simpler sixteen-row
fill loop matches better with the corrected control flow than the preceding
paired-row form, so that equivalent fill implementation is restored.

`python tools/check_bink_scaled_dispatch.py --self-test` compiles production
scaled dispatch and common advancement with payload decoders stubbed. It
passes 112 cases across both row parities, luma/chroma plane scales and seven
starting columns, checking bundle consumption, both work positions, cursor
advancement and guards. Before correction it fails at row 8 / plane 1 / column 0:
an eight-pixel advance instead of sixteen. Reinserting that early exit is a failing negative
control; removing subtype consumption is a second negative control. The
separate fill checker passes 14,336 pixel/state cases and three negative
controls. These are host dispatch/block tests, not movie playback.

There is a small, explicit local score tradeoff: ExpandPlane changes from
55.901283% to 55.890465% in the authoritative deduplicated report. The retail
behavioral correction is retained; it is not called a Bink score gain. Other
corrected source forms measured lower. A temporary trial script's CRLF
handling caused one rejected compilation; it was corrected, the original
source rebuilt, and failed output was never scored.

The same batch improves iModelAnimMatrices from 98.10667% to 98.4% by naming
the existing matrix-stack push destination before copying the current matrix.
The complete `build/parallel-fourteenth-final-report.json` has exactly those
two score changes versus staging 3120c8fad. All function identities, matched
data, exact function/code and source-linked totals are unchanged. Overall
fuzzy progress increases slightly, from 99.489044% to 99.48905%. All-source
compilation and the normal retail DOL link/hash pass. Both changed units
remain NonMatching, so that DOL check does not execute their source. No
compiler changes or new assembly were used.

Independent audit confirmed the scaled-row branch and consumption contract.
The bounded Dutchman follow-up and xBehaveMgr audit produced no additional
changes; their prior source/compiler hypotheses remain unresolved.


## Hazard immutable render scales (2026-10-02)

`NPCHazard::Render` improves from 98.09605% to 99.94915% by directly
initializing four const wave/fountain scale vectors with their computed
`dim_flux`. Previously each vector was initialized with a zero y component
and then assigned its actual y value. The arithmetic and model calls are
unchanged; four aggregate-copy/call scheduling clusters now match retail.
Only entry register allocation remains. The Hazard unit rises from
99.71361% to 99.90262%; all 20,612 data bytes remain exact. It is still
NonMatching, with no new inline assembly or compiler changes.

## Bink masked YUV cursor and edge corrections (2026-10-02)

`YUV_blit_mask` improves from 59.317543% to 62.74776%. Retail disassembly
also exposes several behavioral errors in the old reconstruction, corrected
together here:

- Both block-width pitches are copied from the original pitch before either
  `setup_scaling` call. Retail `.text` 0xc1c..0xc64 passes separate stack
  slots 0x70 and 0x78. Copying after the first call compounded 2XH/2XWH
  scaling: an original pitch of 640 became 2,560 instead of 1,280.
- Masked row-pair advancement uses the caller's scaled row pitch, matching
  retail loads from stack 0x78. `S.pitch` remains the underlying pitch used
  inside the zoom callback.
- The right-half destination offset is 16 pixels times pixel size and
  horizontal scale, not 16 bytes. Its luma and alpha offsets are both 16
  source samples; chroma advances by eight. Retail 0x10dc..0x114c shows
  all eight cursor updates. One context save encloses this adjustment and
  the eight row calls. Splitting row iteration from its save/restore wrapper
  removes the old redundant nested context copy.
- Pair and single-block postludes advance alpha alongside luma, even when a
  mask is clean. Retail 0x1360..0x13cc and 0x1534..0x1594 contain these
  updates; the previous source omitted them.
- Row-end luma/alpha and chroma skips subtract the entire processed width.
  With P = adjusted source pitch, C = P / 2, and B = srcw rounded down to
  16, retail computes 15P - B and 8C - B / 2. The former source subtracted
  one block regardless of width, drifting on widths of 32 or more.
- The bottom fallback converts full rows back to original source coordinates
  with `mult64anddiv(full_rows, old_srch, srch)`. Destination conversion uses
  the original source pitch, not the source image height. Remaining height
  is measured in original source rows. Retail 0x169c..0x1728 establishes
  both conversions. Retail's fallback omission of incoming source origins
  is preserved rather than replaced with a guessed API contract.
- Signed division by 16 restores retail's arithmetic mask-origin shift.
  Positive origins are unchanged; the previous unsigned expression differed
  for high-bit origin encodings. No claim is made about caller validity of
  negative origins.

The all-source build and retail DOL SHA-1 pass. The authoritative report
changes only these two functions versus staging 4bd8f821c, with overall
fuzzy matching 99.48905% -> 99.496185%. Data, exact function/code totals,
and source-link totals are unchanged. The normal DOL still uses the original
objects for these NonMatching units; its hash is not a playback test of the
corrected source. Compiler hashes are unchanged.

The earlier ExpandPlane scaled-helper, fill-replication, row-loop and raw-copy
cursor trials in this pass all scored lower and were restored. RW material,
memory, camera, geometry and device/dependency trials also retained no gain.
An explicit output cursor raised standalone RwImageResample to 99.1453%,
but regressed its exact inlined RwImageCreateResample caller to 97.82007%;
the full report rejected it. Common's apparent data differences were unused
header templates, pool ordering and padding, not evidence for a data fix.

`tools/check_bink_yuv_mask.py --self-test` checks 16,800 cases and ten
negative controls. It compiles the production setup, mask dispatcher and row
helpers, then compares callback cursor geometry to an independent rectangle
oracle. Coverage includes modes 0-6, pixel sizes 2-4, grayscale/channel inversion,
five mask patterns, multiple block rows, aligned origins, and partial right and
bottom edges. The old source fails this checker. Callbacks and edge rendering
are mocked: this is not a pixel, target-ABI or playback test, and signed/unaligned
origins are not covered. The existing scaling checker also passes 37,632 cases
and its inverted-maximum negative control.


## Bink RGB shift slots and initialization layout (2026-10-02)

`YUV_init` improves from 65.0962% to 68.080536%. The RGBshift downshift
entries now occupy retail's even-numbered words: red at 6, green at 8,
blue at 10, with zero words at 7, 9 and 11. Previously the header assigned
green to 7 and blue to 8, leaving word 10 zero. This is an initialized-table
layout correction, not a compiler workaround. Source searches find no current
reader of RGBshift besides its initializer, declaration and storage definition;
no claim is made about an observed playback symptom.

The target's register meanings are independently traceable: `.text` 0x1a2c
computes r4 = 8 - green_bits, 0x1a30 computes r5 = 8 - blue_bits, and 0x1a44
computes r6 = 8 - red_bits. Packing white at 0x1a48..0x1a74 confirms those
roles. Stores at 0x1b38, 0x1b48 and 0x1b58 write r6/r4/r5 to RGBshift byte
offsets 0x18/0x20/0x28. Interleaved zero stores use r8, explicitly zeroed
at 0x1acc. For RGB565 the three downshifts are 3, 2, 3; for RGB655 they
are 2, 3, 3. The table remains twelve words and its storage is unchanged.

Two source-layout changes preserve table values while improving the match:
the saturated-white grayscale case precedes the linear-range case, matching
retail block order, and the alpha clamp store follows the middle clamp-table
stores. Low-format-first dispatch and grouping high-word tables together
scored lower and were restored. The function remains 1,760 bytes versus
retail's 1,788 and the YUV unit remains NonMatching.

The all-source build and retail DOL SHA-1 pass. The full deduplicated report
changes only YUV_init versus staging a969b39d7; overall fuzzy matching rises
from 99.496185% to 99.4984%. Exact code/functions, object data and source-link
totals are unchanged. Object data matching cannot validate a table initialized
at runtime, and the normal DOL still uses the original NonMatching object.
Compiler hashes are unchanged.

The accompanying near-TU pass retained no Glyph/xString, zThrown/zLightning
or zFX changes. Each worktree restored its baseline report and retail hash.
The remaining register lifetime, address association and load-order residues
supplied no new focused cross-TU mechanism that justified a compiler patch.

`tools/check_bink_yuv_init.py --self-test` passes 418 initialization calls:
19 cold flags, 19 poisoned same-layout returns, 19 invalid returns, and all
361 ordered warm format pairs for flags -2, -1 and 0..16. The independent
integer oracle checks every luma, chroma, packed-color, alpha and RGBshift
entry, the original UV copy, unused trailing entries, per-array guards,
untouched context and cache state. All eight deliberate regressions fail,
including the legacy shift slots, luma coefficient/saturation, signed chroma
rounding, alpha mask and cache guards. The original source/header also fails.
This is host table/state coverage, not a target-ABI or movie playback test.

## zEntPlayer: exact retail damage-timer constant (2026-10-02)

At staging `d04b1f26f`, `globals.player.DamageTimer = 0.333333f` emitted
`0x3eaaaa9f`, one float step below retail's `0x3eaaaaa0`. The decimal
`0.33333302f` emits the exact target value (0.33333301544189453). Both ELF
objects have exactly one reference to this constant: `zEntPlayer_Update`
+0x11e4; target `.sdata2` +0x298, source +0x284. This corrects a value,
not a relocation-label or anonymous-pool matching artifact.

Full `all_source` and authoritative deduplicated report verification:

- `.sdata2`: 99.893616% -> 100%, all 952 bytes credited exact.
- Unit matched data: 12052 -> 13004 / 27820 bytes.
- Every function entry in the complete project report is unchanged; no
  other unit changes. zEntPlayer stays 335/342 exact, code 99.861046%.
- Full normal build preserves retail SHA1
  `306526d90b48e99894c3138f5fc8f2716d9fecf6`.
- Compiler hashes remain GC/2.0p1a
  `a78a5fdb6c1d5677e987636b2e0743dbaefe9542` and GC/2.0p1e
  `9d445725489050035740aaff35860eddbaf3c3c9`.

The `.rodata` residual remains 99.91898%. Raw source section length is
14848 versus target 14816: sequence alignment finds 36 extra zero bytes
before `patsock_totals` (three header aggregate templates) and four fewer
trailing padding bytes. String content is unchanged. Constant-pool order
also still differs physically; report exactness does not establish a source
link. No Matching flag or whole-TU source-link claim is made.

Bounded orientation trials were restored: naming `floor_norm` through a
reference is neutral at 98.96635%; deferring `eup` initialization into its
normalization call regresses to 97.5%. The residual remains the globals-base
register and `mr r4` scheduling before the first normalization. No exhausted
Update permutations or compiler changes were attempted.

Isolated evidence: `build/player-literal-refs.py`,
`player-literal-report.json`, `player-final-verify.txt`,
`player-literal-build.log`, `player-literal-link.log`,
`player-bounded-trials.py`, and `player-data-small.py`.



### xFX streak position captures and raw data audit (2026-10-02)

Baseline d04b1f26f. Explicit immutable scalar captures for the four streak
positions improve `xFXStreakRender` 92.19259 -> **93.67407**, and xFX TU
99.666824 -> **99.69812**. Retail loads each position's y/z/x components
before writing them, whereas direct macro arguments gave x/y/z loads and
different floating-point registers. Local `const F32` components recover
retail's load order and floating-point assignments without changing arithmetic
or moving reads across calls. Capturing whole `const xVec3` values instead
regressed to 58.044445 and was rejected. No declaration permutations or
shared RenderWare macro edits were tried.

Authoritative full deduplicated reports changed only StreakRender; all other
functions, exact counts, data scores and 452 source-linked units are unchanged.
Full source build passes and retail DOL SHA1 remains
`306526d90b48e99894c3138f5fc8f2716d9fecf6`. GC/2.0p1e SHA1 remains
`9d445725489050035740aaff35860eddbaf3c3c9`. Unit stays NonMatching: six
functions remain partial, so this is not a whole-TU source-link claim.

Raw object data was also checked independently of the 100% report score.
The entire retail .rodata (392 bytes) equals ours after removing 36 leading
zero bytes: three unused 12-byte header vector constants with no relocations.
The .sdata2 pool matches except for inserted 3.0f and 100000.0f at offsets
0x84/0x88, both referenced by the extra emitted weak `xsqrt` body. Remaining
constants retain their sequence. .data/.sdata bytes and relocation sets match.
These existing layout/emission differences are not repaired by fake mapping
or removed constants. No new compiler-deficiency claim follows from this trial.


## 2026-10-02: Recheck branch and narrowing claims against exact source controls

A bounded audit at staging d04b1f26f separates two remaining observations from
claims about compiler deficiencies. No compiler or production source is changed.
The local zTalkBox review packet and xFont reports are diagnostic evidence, not
patch specifications. Failed source permutations alone do not locate a compiler
pass or prove the original source and flags.

The older CalcCombinedDepen caveat above is now superseded: the function is
608 bytes and 100% in the full deduplicated report. Source commit 7dbba28a6
reuses normZ for the second normal instead of introducing nby, and changes the
second clamp to MAX(nddot, 0.25f), following the earlier reuse of nddot. Thus the
previously conflicting branch shape and register allocation are simultaneously
source-reachable. LassoNotify remains 112 bytes at 96.42857%; its empty-case
experiment establishes only that the dead branch can be emitted, not an exact
function or a justification to retain an invented case. Dutchman's update_turn
is another exact control: the documented sibling yaw-array form recovers its
frsp, and the current 260-byte function is 100% without a compiler change.

The older xSndIsPlayingByHandle '+1/-1 whichever end' conclusion is also
superseded. Commit f46dcd2fc changes the wrapper declarations/definition to bool
and zNPCNewsFish::IsTalking's ternary fallback from false to integer 0. The
wrapper, iSndIsPlayingByHandle and IsTalking are all currently 100%. The caller's
other operand type mattered; the earlier isolated return-type trials did not
prove that U8 was required. The exact wrap_block helper in xFX is a separate
control: its u32 return with a U8 cast in the body preserves a deliberate
narrowing site without imposing a U8 return type on its callers.

For zTalkBox, next_state_type::start still differs by a branch to the epilogue
followed by an unreachable branch back to the traversal test: recorded retail
.text addresses 0x2f78 and 0x2f7c, versus source fallthrough. This is a distinct
CFG/emission question, not instruction scheduling, and warrants focused
read-only compiler investigation if that work is prioritized. It is not ready
for a patch proposal. The packet's complete method is not a standalone reduced
reproducer. What is missing is a small case that preserves the absent branch
pair under the recorded flags, plus evidence locating its disappearance in
IR construction, CFG cleanup or final emission. Controls must include the
source-reachable LassoNotify and now-exact CalcCombinedDepen shapes. Their
existence does not explain this loop, but rules out a general claim that the
compiler cannot emit such branches.

For xFont, parse_next_text_jot's baseline U8 local narrows before is_ws and
forwards that narrowed value to bounds; retail saves the loaded value and
narrows at the later bounds call. A char local with explicit U8 casts at the
comparisons and bounds reproduces the desired narrowing sites but rotates the
saved a/c/tb registers, lowering the complete function score. This already
rules out treating the symptom alone as inability to emit the desired
extension placement. A useful reduced case must retain the byte load, bitfield
stores, both char-argument calls, aggregate return and value liveness across
the first call; compare the two typed source forms before reducing them further.
No validated standalone case yet separates value-width tracking, copy
coalescing and register allocation, so no distinct narrowing patch is proposed.

Evidence: build/stagingd04b1f26f-baseline-report.json confirms all exact controls;
build/xfont-d04-*.json and build/zTalkBox-compiler-review.md retain the mismatches.
The isolated GC/2.0p1e compiler remains SHA1
9d445725489050035740aaff35860eddbaf3c3c9. The preceding restored source build,
full-report equality and retail DOL SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6 remain the validation baseline. This
append-only documentation audit requires no new code or compiler build.

## Bink generic blitter: restored cursor experiments and integrated verification (2026-10-02)

Baseline d04b1f26f. Replacing the paired-row global cursor updates with local
base temporaries improved the direct YUV_blit diff from 87.90741 to 88.155556,
but the authoritative full deduplicated score fell from 89.09259 to 88.82222.
The experiment was restored. This is another concrete case where the direct
function score is insufficient for deciding whether to retain a change.

Five follow-up source forms were also restored after full deduplicated reports:
setup base captures 88.659256, shared initial source offset 87.3, shared chroma
base 88.30741, odd-row store ordering 89.09259 (neutral), and paired-plus-odd
row stores 88.82222. These were base lifetime and store scheduling experiments;
no behavior correction or compiler-deficiency evidence was established.
Artifacts are build/yuv-blit17-* and build/yuv-setup17-*.

The combined player literal and xFX scalar-capture changes pass all_source and
the normal build. Comparing parallel-seventeenth-final-report.json against
parallel-sixteenth-tables-report.json changes only xFXStreakRender and the
player constant section: overall fuzzy code 99.4984 -> 99.498726, exact data
1264032 -> 1264984 (+952). All other function entries and unit reports are
unchanged, including restored Bink. Exact functions remain 9940/10147 and
source-linked units 452/543. Retail DOL SHA1 remains
306526d90b48e99894c3138f5fc8f2716d9fecf6; isolated GC/2.0p1a and GC/2.0p1e
hashes remain a78a5fdb6c1d5677e987636b2e0743dbaefe9542 and
9d445725489050035740aaff35860eddbaf3c3c9. The changed TUs remain NonMatching;
the normal retail link does not establish their source-linked runtime behavior.


## zThrown pointer-load audit: provenance matters (2026-10-02)

This corrects the older statement that an sda21 store "simply does not treat"
a pointer load as killed. Six focused probes were compiled using the exact
`zThrown.o` flags, with unmodified stock GC/2.0p1 (SHA1
`74bc177b10d1bbe8a60a21a6c0aa86d2dd9c0668`) and installed GC/2.0p1a
(`a78a5fdb6c1d5677e987636b2e0743dbaefe9542`). All 384 emitted text bytes,
including support accessors, are identical between the two compilers.

The complete standalone diagnostic source is:

```cpp
// Read-only compiler diagnostic: no retail reference exists for these functions.
struct Carry { float timer; };
struct Stats { const char* name; void* callback; Carry* carry; unsigned id; };
static unsigned count;
static Carry fruit = { 24.0f };
static Stats table[23];
Stats* expose_table() { return table; }
unsigned read_count() { return count; }
Carry* fruit_address() { return &fruit; }
float known_static(unsigned i) {
    Stats* stats = table + i;
    if (stats->carry != &fruit) return 0.0f;
    ++count;
    return stats->carry->timer;
}
float no_store(unsigned i) {
    Stats* stats = table + i;
    if (stats->carry != &fruit) return 0.0f;
    return stats->carry->timer;
}
float unknown_stats(Stats* stats) {
    if (stats->carry != &fruit) return 0.0f;
    ++count;
    return stats->carry->timer;
}
float aliasing_member(Stats* stats, Stats* destination, Carry* replacement) {
    if (stats->carry != &fruit) return 0.0f;
    destination->carry = replacement;
    return stats->carry->timer;
}
float distinct_member(unsigned i) {
    Stats* stats = table + i;
    if (stats->carry != &fruit) return 0.0f;
    ++stats->id;
    return stats->carry->timer;
}
float unknown_call(unsigned i, void (*call)()) {
    Stats* stats = table + i;
    if (stats->carry != &fruit) return 0.0f;
    call();
    return stats->carry->timer;
}
```

`known_static` loads carry once, tests it, stores count, then loads timer
through the cached carry. `unknown_stats` loads carry for the test, stores
count, **reloads carry**, then loads timer. The known table's address really
escapes through `expose_table`; this is not a proof based on a never-exposed
array. Other controls in the same source:

| Intervening operation | Carry reload after operation, both compilers |
| --- | --- |
| None, table-derived pointer | No |
| `++count`, table-derived pointer | No |
| `++count`, parameter pointer | Yes |
| `destination->carry = replacement`, unknown destination | Yes |
| `++stats->id`, table-derived pointer | Yes |
| Unknown function-pointer call, table-derived pointer | Yes |

The sibling-member control is especially useful: this compiler conservatively
kills carry when another member of the same known table entry changes, while
preserving carry over a store to the separate count object. Therefore the
repro is consistent with alias-object provenance, not a blanket failure to
invalidate pointer-valued loads or a regression introduced by current patches.

Retail witnesses remain `zThrown_AddFruit` (96.01852%, target reloads carry
after count increment) and `zThrown_LaunchVel` (93.820755%, target schedules
its carry load after the count store). The prior cross-TU witness
`zCutsceneMgrPlayStart` still reloads `cutsceneHackTable[i].alphaBits` after
`s_atomicNumber = 0`, while our compiler retains it. Its authoritative score
is 99.12949%; the isolated raw score prints 98.777%. It is supporting evidence,
**not an exact counterexample**.

Exact same-TU controls were inspected: `zThrown_Remove` and
`zThrown_LaunchStack` remain 100%. Remove preserves its explicitly snapshotted
callback across the list assignment/count decrement and reads the unknown
entity's baseType after the count store. That is consistent with the probe,
but it is not the equivalent cached-table-field expression. No exact retail
function with that identical expression/store pair was established in this
bounded audit. The broad-rule regression boundary therefore remains unproven.
The earlier volatile-counter trial already lost exact functions; it is not a
source fix or a substitute for such a boundary.

**Decision:** preserve a narrowly framed provenance/CSE hypothesis for review;
do not claim a compiler deficiency, safe predicate, or expected match gain.
A reduced probe has no retail target, and source/alias provenance differences
in the real functions remain possible. A future candidate needs an actual
alias-query/pass attribution and exact retail controls before a patch scope
can be justified. No compiler files or running compiler process were patched.

Reproduction artifacts in the isolated worktree: `build/thrown-alias-repro.cpp`,
`thrown-alias-run.py`, `thrown-alias-results.txt`, `thrown-alias-meta.json`,
`thrown-remove-control.txt`, `thrown-stack-control.txt`, and
`thrown-cutscene-control.txt`. The runner extracts the actual zThrown rule
from `build.ninja`; essential flags are `-O4,p -proc gekko -fp hardware
-fp_contract on -lang=c++ -common on -char unsigned -inline off
-use_lmw_stmw on -str reuse,pool,readonly -RTTI off -Cpp_exceptions off`,
with the project's unchanged includes/defines and pragma flags recorded in
`thrown-alias-meta.json`. This is documentation-only; no production source,
Matching marker, or build configuration changed.

### Concurrent TimerUpdate match and real link check (2026-10-02)

Concurrent staging commit 04b4a32d3 resolves xPsyche::TimerUpdate through an
ordinary loop over en_xpsytime with a switch on the timer type. The one-iteration
loop retains the duplicated default arm seen in retail. Combined verification
makes xBehaveMgr 59/59 exact, all 7280 code bytes and 24 data bytes matched.
This supersedes the earlier unresolved TimerUpdate CFG hypothesis: the branch
shape is source-reachable without changing the compiler.

A real Matching-marker flip still fails the DOL hash, so the marker was restored.
Fresh symorder diagnosis shows extra header constants, extra weak inline bodies
and different weak/template definition order. The 100% report is not a source
link. Evidence: build/xBehaveMgr-link17.log, xBehaveMgr-link17-restored.log and
xBehaveMgr-symorder17.txt. No layout edits were retained in this pass.

After merging that concurrent change, parallel-seventeenth-merged-report.json
adds only the TimerUpdate result to the verified player/xFX changes: overall
fuzzy code 99.500046, exact code 2217640 bytes and 9941/10147 exact functions.
Source-linked units remain 452/543. The restored normal link passes again.

## Bink and near-TU follow-ups restored (2026-10-02)

At bd62963f8, eight doubled-alpha column lifetime forms were tested with full
deduplicated reports. Explicit luma/alpha cursors in the width-doubled routine
regressed to 80.652176/80.978264; separate alpha cursors or output pixels in
the height variants also regressed. Separate row-luma samples and a captured
first output pointer were neutral. All source was restored (89/97 YUV functions
remain exact). Evidence: build/yuv-cols18-*.

Byte IDCT reusing the dequantized DC value for its even sum and hoisting the
first-pass scalar declarations were neutral at 63.765182. Reusing row2 for the
scaled even difference regressed to 59.94332. YUV_init's target keeps the
clamp_bh base live where ours keeps clamp_bb; explicit outer, inner and register
blue-high pointers all regressed from 68.080536 to 66.8613. Those experiments
are restored; they establish no compiler deficiency. Evidence: build/dct18-*
and build/yuv-init18-*.

Independent workers found no new literal/data discrepancy in Dutchman's two
holdouts and no improvement from Common's two explicit-default placements.
Robot's direct-red cone load regressed and was restored. A direct scaled-pattern
ExpandPlane implementation with explicit mask bases was pixel-correct across
262144 oracle cases but reduced matching to 54.427315 from 55.890465 in both
scoped and helper forms. The original source was restored, with exact full-report
equality and pattern/dispatch regression controls passing. These rejected
structures should not be repeated without a new layout or lifetime hypothesis.


### Concurrent source-only whole-TU wins, independently verified (2026-10-02)

Staging advanced with 97e496eea, 607e0ab80 and 1f7aacce0 while the isolated
follow-ups ran. The existing weak-inline xBehaveMgr solution is retained:
expression-transition/history bodies belong in xBehaveMgr.h rather than
xBehaviour.h, Next in xListItem.h, and the other list templates in xListItem.inl.
This restores retained helper order without changing weak binding. Our separate
worker also proved an ordinary out-of-line strong-binding alternative can link,
but that broader alternative was not integrated. Its audit establishes that
extra unused constants/weak bodies are stripped; the observed initial 351-byte
DOL mismatch was retained definition order, not the extra symbol sets themselves.

zCombo_Update reaches 100% using comboReward[toShow] for the text lookup and
removing redundant counter/HUD snapshots. zMainMemCardSpaceQuery reaches 100%
with a named signed progress-bar index, one function-scope result shared across
both call sites, and the retained local initialization layout. These supersede
older claims that their remaining register choices could not be source-reached.
As with TimerUpdate, failed declaration-only sweeps did not establish a necessary
compiler patch. No compiler change is involved in these wins.

Independent root all_source and retail-link verification passes. The full report
against bd62963f8 changes only the intended zCombo_Update, memory-card query and
small zLasso_Render gain, plus the three completion markers. xBehaveMgr, zCombo
and zMain all source-link: complete units 452 -> 455, complete code 1338564 ->
1362288 (+23724), complete data 685168 -> 692220 (+7052). Exact functions rise
9941 -> 9943 and overall fuzzy code 99.500046 -> 99.501755. All Bink experiments
are restored and no other function or matched-data score regresses.

Evidence: build/parallel-eighteenth-merged-report.json,
parallel-eighteenth-merged-validation.log and parallel-eighteenth-compare.py.
Retail DOL SHA1 remains 306526d90b48e99894c3138f5fc8f2716d9fecf6; isolated
GC/2.0p1a and GC/2.0p1e remain a78a5fdb6c1d5677e987636b2e0743dbaefe9542
and 9d445725489050035740aaff35860eddbaf3c3c9.

## Bink masked-blitter dispatch and mode lifetimes (2026-10-02)

YUV_blit_mask improves from 62.74776 to 66.67222 in the full deduplicated
report. Retail .text 0xf14..0xf30 dispatches the signed mask value through a
switch, testing 2 then 1/3; the reconstruction previously used an if/else
chain. Use a switch with signed word-sized bits/lower temporaries. Their
values remain 0..3, so removing byte narrowing preserves the value domain.
Retail .text 0xea8..0xed0 reads the right dirty byte before the left and sums
the two Boolean contributions; .text0xedc..0xf10 does the same for the lower
mask row. Reproduce that order without changing which blocks are selected.

The destination surface mode is now read through YUV_SURFACE_MODE(flags) at
each decision instead of keeping a cached mode through the setup calls.
Retail explicitly reloads/recomputes it at .text0xbe4 and .text0xc68..0xc6c.
The flags parameter and mask bytes are ordinary nonvolatile inputs; no memory
writes or callbacks are moved between the paired mask reads. All block calls,
context saves/restores, cursor advances and edge fallbacks retain their behavior.
Source size improves from 3084 toward retail 3124, now 3116 bytes.

Ablation reports: switch alone 63.816902, right-first reads alone 63.167732,
both with byte mask 64.51729, unsigned word mask 64.95903, uncached mode 65.99232,
then signed word mask 66.67222. int and s32 gave the same final score; s32 follows
the surrounding SDK types. No duplicate-condition or compiler workaround is
introduced. Evidence: build/yuv-mask19-*.

The independent host checker passes all 16800 callback-geometry/mask/pitch/
alpha/context/edge-argument cases and all 10 deliberate negative controls.
Only the negative-control region locators changed from if/else to case labels;
expected values and mutation behavior are unchanged. Independent read-only review
also confirmed all 512 selected dirty-mask truth-table combinations. This checks dispatcher
geometry with mock core callbacks, not pixel conversion, generic-edge execution,
GameCube ABI behavior or movie playback.

Full all_source and normal build pass. The complete deduplicated report changes
only YUV_blit_mask versus 21eabc1c2; overall fuzzy code 99.501755 -> 99.50682.
YUV remains 89/97 exact and NonMatching; all data, exact-function and source-linked
totals remain unchanged. Retail DOL SHA1 stays
306526d90b48e99894c3138f5fc8f2716d9fecf6. Isolated GC/2.0p1a and GC/2.0p1e
hashes remain a78a5fdb6c1d5677e987636b2e0743dbaefe9542 and
9d445725489050035740aaff35860eddbaf3c3c9. Logs/report:
build/yuv-mask19-final-check.log, parallel-nineteenth-final-validation.log,
parallel-nineteenth-final-report.json and parallel-nineteenth-compare.py.

Other bounded trials were restored. Five readlossy counter forms did not beat
99.756096: unsigned/register snapshot and inverted cutoff were neutral,
materialized cutoff 98.95787, conditional delta 99.32372. Audio raw .rodata is
already byte-identical (96 bytes); its uncredited anonymous pool is not evidence of
a wrong constant. Workers' zTalkBox traversal, savegame type/reference and iModel
hierarchy-type forms produced no gain, and their restored full reports equal
baseline. No compiler deficiency or new patch requirement follows from them.

## Bink masked-blitter row setup (2026-10-02)

YUV_blit_mask improves from 66.67222 to 72.2548 in the full deduplicated
report. Compute the mask-row skip before setup_scaling, and compute the luma,
alpha, chroma and destination row deltas beside chroma_pitch before initializing
the plane pointers. Retail computes these invariants in that region rather than
keeping their inputs live through the plane setup.

Express the destination row skip as pitch32 minus the full scaled width, plus
the scaled remainder, plus fourteen pitch32 rows. Retail .text 0xcd0 loads the
second setup's pitch from stack 0x78; 0xcdc multiplies it by 14, and 0xd20/0xd28/
0xd38 subtract width, add remainder and add the fourteen rows. Both setup calls
start with equal pitch and the same flags. setup_scaling changes pitch only by
a width-independent doubling for 2XH/2XWH, so pitch16 equals pitch32 for every
mode. The replacement equals the old fifteen-pitch expression modulo 32 bits,
including overflow, before the same signed result conversion. Independent
review checked all mode cases and 1536 boundary-value arithmetic cases.

Place the inverted U/V branch first, as retail .text 0xda4..0xdec does, while
preserving its existing shared chroma base calculation and both assignments.
Moved computations read unchanged locals; their BLITS reads use the distinct
static tables passed by the actual callers, so plane-state stores cannot alias
them. No callback, allocation, mask read or setup call changes order.

Ablations from 2a2fdf851: row-skip expression 67.35723; early mask skip 67.40333;
early deltas 70.87324; combined setup 72.2484; inverted-first shared base 72.2548.
Local chroma bases in each branch regressed to 70.16261, callback-row base
captures regressed to 66.24328, and final-row pitch32 substitution was neutral;
those follow-ups were restored. Source is 3112 bytes versus retail 3124 and stays
NonMatching. Evidence: build/yuv-mask20-*.

The masked-blitter checker passes 16800 geometry, cursor, context and fallback
argument cases plus all 10 deliberate regressions. Its compounded-pitch mutation
now moves the unique copy line across the first setup call without requiring
adjacent lines; the faulty behavior and independent expected values are unchanged.
This is mock-callback geometry coverage, not pixel, target-ABI or playback proof.

Full all_source and normal build pass. The complete report changes only this
function: overall fuzzy code 99.50682 -> 99.51403; exact functions 9943/10147,
matched data 1264984/1280684 and source-linked units 455/543 remain unchanged.
Retail DOL SHA1 stays 306526d90b48e99894c3138f5fc8f2716d9fecf6. Both isolated
MWCC hashes remain a78a5fdb6c1d5677e987636b2e0743dbaefe9542 and
9d445725489050035740aaff35860eddbaf3c3c9. Validation: build/yuv-mask20-check.log,
parallel-twentieth-validation.log, parallel-twentieth-report.json and
parallel-twentieth-compare.py. No compiler or assembly changes are involved.


## Bungee anonymous helpers and real link audit (2026-10-02)

On staging `2a2fdf851`, the four reported-missing hook_asset anonymous
assignment operators are emitted: source class IDs 167-170 versus retail
910-913. Their 12/28/52/84-byte bodies (176 bytes total) are byte-identical.
This is actual object-byte comparison, without object symbol renaming or
objdiff pairing configuration. Bungee remains 115/120 exact functions,
99.30076% code, all 6528 data bytes exact in the deduplicated report.

A natural private-definition trial moved hook_asset from its public header
into the implementation, leaving a forward declaration for hook_type's
asset pointer. Generated IDs became 283-286; the four report holes remained.
The trial was restored. No dummy declarations or generated-name padding
were added, and prior start_detaching permutations were not repeated.

The first real source-substitution diagnostic in this pass shows a broader
link blocker: normal source Bungee produces a 2859136-byte DOL with 19396
changed bytes and SHA1 `46f91b6ddfca7eb998a9a6530c6c397dde50aaf3`.
The first changed instruction, at 0x80063404, is an external call to
bungee_state::active: target branch destination 0x8011246c, source destination
0x80112fa8. This is a retained function-address shift, not an anonymous
assignment instruction mismatch. The marker was restored immediately.

Fresh symorder finds different retained definition positions: the eight
hanging-state movement/detach/render functions occur much earlier in source
than in retail; update_camera also occurs earlier. The target data order is
shared, tweak_cord_off, attaching vtable, base-state vtable, hanging vtable,
then the 132-byte anonymous table. Source instead has shared, tweak_cord_off,
the anonymous table, hanging/attaching/base vtables. Constant-pool positions
also differ despite deduplicated data scoring 100%.

A bounded trial moved the nine ordinary function definitions to their retail
relative positions. It regressed hanging_state::start from 100% to 99.949554%
(1348 bytes), kept all data exact, and increased real-link differences to
19757 bytes. It was restored. Definition ordering alone therefore does not
resolve the interacting weak-helper/pool layout; these failures do not prove
that matching or source linking is impossible.

Restored all_source and complete deduplicated report equality pass. The normal
DOL again hashes to `306526d90b48e99894c3138f5fc8f2716d9fecf6`; GC/2.0p1a
and GC/2.0p1e remain `a78a5fdb6c1d5677e987636b2e0743dbaefe9542` and
`9d445725489050035740aaff35860eddbaf3c3c9`. No production/header/configuration
change or Matching marker is retained, and no compiler or assembly changes
were made. Evidence in the isolated worktree: `build/bungee-layout-assignments.json`,
`bungee-layout-trial.py`, `bungee-layout-link-diff.json`,
`bungee-layout-source-link.log`, `bungee-layout-source.dol`,
`bungee-layout-symorder.txt`, `bungee-reorder-trial.py`,
`bungee-reorder-report.json`, `bungee-reorder-link-diff.json`, and
`bungee-layout-final-report.json`/`bungee-layout-final-link.log`.


## Bink masked-blitter block stride (2026-10-02)

YUV_blit_mask improves from 72.2548 to 80.47119 in the full deduplicated
report. Compute the destination byte stride for one 16-pixel mask block once,
then reuse it for right-half positioning and horizontal advancement. Retail
keeps this block stride in r31 after .text 0xce8; the old source repeatedly
formed the expression from the BLITS table and xscale around callbacks.
The actual callers use the same fixed internal BLITS tables throughout the
operation. Multiplying the shared stride by two for a 32-pixel block preserves
the unsigned arithmetic of the original expression.

Keep pitch32 as the main pitch from initialization onward. Give the first
setup call its own short-lived pitch16 copy, and use pitch32 for final-row
advancement. Retail initializes stack 0x78, copies it to 0x70 for the first
setup, then uses 0x78 for subsequent rows. Both setup calls apply the same
width-independent pitch scaling, so their resulting pitches are equal.

The pitch change alone gives 72.2612; narrowing the old pitch16 scope alone
is neutral. Reusing the first pitch variable as a row delta regresses to
67.79257 and is restored. Deriving row-skip pixel sizes from the cached block
stride regresses to 69.193344 alone or 78.18438 combined with block advances;
retain the original row-skip expression. Retail-inspired second-row-first
plane stores and V-before-U origin stores are neutral and restored.
Evidence: build/yuv-mask21*, yuv-mask21b*, yuv-mask21c*, and yuv-mask21d*.

Full all_source and normal build pass. The full report changes only this
function: YUV unit 94.661354 -> 95.53513, overall fuzzy 99.51403 -> 99.52464.
Exact functions, data, and source-linked totals are unchanged. The unit stays
NonMatching. Retail DOL SHA1 remains 306526d90b48e99894c3138f5fc8f2716d9fecf6;
GC/2.0p1a and GC/2.0p1e remain a78a5fdb6c1d5677e987636b2e0743dbaefe9542 and
9d445725489050035740aaff35860eddbaf3c3c9. Validation uses score/build/link checks;
no additional behavior cases or compiler/assembly changes were introduced.
Report/log: build/parallel-twentyfirst-report.json and
build/parallel-twentyfirst-validation.log.


## Old-format Huff8 byte-return boundary (2026-10-02)

`CheckReadHuff8Bundle` improves from 98.99015% to 99.01478% by routing its
high-symbol decode through a small `u8` forwarding wrapper. The old reader
already assigned this result to a byte before copying it to the next Huff8
state and packing it with the low nibble. The wrapper preserves that
conversion and all bitstream operations. It replaces one `clrlwi` with `mr`;
source size remains 816 bytes against the 812-byte target, so the extra move
and remaining register differences are not resolved.

The previous global helper-return-type experiment improved this reader but
regressed the exact newer reader. Restricting the byte boundary to the old
caller retains `NewCheckReadHuff8Bundle` at 100%. A duplicated byte-return
helper body produced the same output and was discarded in favor of the
forwarding wrapper. This is a source-level narrowing control, not evidence
that a compiler patch is required.

Full all_source compilation passes. The complete deduplicated report changes
only CheckReadHuff8Bundle and its unit (80.92753% to 80.92896%); all 1,272 data
bytes remain exact, with no missing functions or regressions. The normal DOL
retains SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6 and both isolated compiler
hashes are unchanged. Expand remains NonMatching, so this hash does not prove
an exact source link of the unit. No ancillary behavior tests were added or
run. Trial/report artifacts use build/expand-worker-* in the isolated worktree.


## Bink doubled alpha-column sample lifetime (2026-10-02)

`dounaligned32acolm2h` improves from 97.708336% to 97.916664% by naming
its second row's alpha sample separately as `u32 a1`. Retail loads/shifts
the first alpha in r11 and the second in r9; the shared source local used
r11 for both loads. The second load and shift now match. Pointer traversal,
unsigned alpha shifts, table lookup and destination stores are unchanged.
Splitting the alpha cursor as well regressed to 94.166664% and was restored.
Prior IDCT/dequant/output and alpha-column cursor trials were reviewed rather
than repeated. No compiler changes or new assembly were used.

Against staging f7d228e19, the authoritative full deduplicated report changes
only this 192-byte function. YUV moves 94.661354% to 94.66272%; all exact
function/code, data and source-linked counts are unchanged. Full source build
passes and the normal DOL retains SHA1
`306526d90b48e99894c3138f5fc8f2716d9fecf6`. The TU remains NonMatching;
this is a source matching gain, not a playback or whole-TU source-link claim.
No ancillary tests or test infrastructure were added for this lifetime change.


Combined verification also includes concurrent staging 759a79625: deferred
inline helper placement makes cruise-bubble add_trail_sample exact, leaving
that unit at 243/244. The complete report changes only that function and the
three Bink gains above. Overall fuzzy reaches 99.52468; exact functions rise
to 9944 and exact code to 2220856. Data and source-linked totals are unchanged.
Full all_source/normal build and retail SHA1 pass. Evidence:
build/parallel-twentyfirst-merged-report.json and
build/parallel-twentyfirst-merged-validation.log. No compiler patch is needed
for these gains; the cruise-bubble result is another source-layout control.


## Bink masked-blitter branch merge and pitch loads (2026-10-02)

YUV_blit_mask improves from 80.47119 to 81.54674. Form the destination byte
offset before adding the destination base, matching retail .text 0xbf0/0xbf4.
For the lower dirty-mask row, merge bits inside both arms of the left-bit
condition; retail .text 0xefc..0xf10 performs the OR separately in each arm.
The tested bit values and selected blocks are unchanged.

Pass the setup-produced pitch delta and row pitch to the inline block/row
helpers by const u32 reference, retaining the signed delta conversion at its
pointer-add use. Retail reloads these setup outputs after the EVEN callback
(e.g. .text 0xfc8 and 0xfe0); the value parameters captured them beforehand.
setup_scaling does not retain these stack addresses, and core callbacks do
not modify the pitch locals. This preserves behavior while recovering the
observed lifetime. No volatile accesses, assembly, or compiler changes.

Ablations: destination offset alone 80.60051; branch merge with it 81.103714;
pitch references with it 80.9808; combined 81.54674. From the combined form,
keeping only the delta reference gives 79.984634 and only the row reference
81.270164. Removing the unchanged source-width snapshot, capturing mask pitch
before doubling, and a lower-bit ternary all regress and were restored.
Artifacts: build/yuv-mask22*, yuv-mask22b*, yuv-mask22c*, yuv-mask22d*.

The complete deduplicated report changes only YUV_blit_mask; YUV reaches
95.65087 and overall fuzzy 99.52607. All data, exact-function and source-linked
totals remain unchanged. Full all_source and normal build pass; retail SHA1
remains 306526d90b48e99894c3138f5fc8f2716d9fecf6. The unit stays NonMatching.
Validation: build/parallel-twentysecond-report.json and
build/parallel-twentysecond-validation.log. No ancillary behavior tests run.


## Bink masked-blitter caller scopes (2026-10-02)

YUV_blit_mask improves from 81.54674 to 82.31114 by keeping the masked
row loops in their caller's scope. Shared scoped macros replace the two
inline helpers, retaining one copy of each source body and the same four
call sites. They are private to this function and undefined afterward.
Every call site is a standalone statement inside a compound block; arguments
are constants or existing locals, with no side effects from reevaluation.
The loop reads the setup-produced pitch variables directly after EVEN,
without forwarding references or keeping their addresses in extra registers.

Moving only context-save ownership into the caller gives 81.43406. Expanding
all row loops directly gives 82.31114; the retained plain-scope macros give
the same score as that expansion. Wrapping both macros in do/while
zero gives 79.61332 and is not retained. The stack frame is now 0x118, matching
retail, rather than the prior larger frame. Source length is 3088 bytes versus
retail 3124. Reusing setup pitch scratch as a later luma delta still regresses
(78.78617) and was restored. Evidence: build/yuv-mask23*, yuv-mask23b*, and
yuv-mask23c*. This is source lifetime evidence, not a compiler patch requirement.

Full all_source and normal build pass. The complete deduplicated report
changes only this function; YUV reaches 95.73216 and overall fuzzy 99.52706.
Exact functions, all data, and source-linked totals are unchanged. Retail
SHA1 remains 306526d90b48e99894c3138f5fc8f2716d9fecf6. YUV stays NonMatching.
Validation: build/parallel-twentythird-report.json and
build/parallel-twentythird-validation.log. No ancillary behavior tests run.

Parallel bounded YUY2 sample-lifetime, lightning color-reference, and player
SpeakStart helper trials produced no gain and were restored with full-report
baseline equality and passing builds. Player builds with -inline off, so the
Bink byte-return helper technique introduced real calls there and regressed;
the inlining context must be checked before transferring such patterns.


Combined verification includes concurrent staging 584230526. xString now
source-links at 14/14 exact (3648 code, 48 data): direct destination indexing
fixes xStrParseFloatList and the byte-cast boundary fixes imemcmp. Both
xShadowSimple holdouts also reach 100 through the entry-pointer lifetime
inside the loop and testing castOnEnt before capturing it. iModelCullPlusShadow
reaches 100 with the existing RenderWare dot-product macro; iFile async_cb
improves 92.19388 -> 97.5 with the file's ordinary round-up expression.
These source-reachable results supersede any older register-only exhaustion
claims for those functions and add no compiler patch requirement.

The merged full report changes only these six game functions and the masked
Bink gain. xShadowSimple is 13/13 exact but is not marked source-linked here.
Source-linked units rise 455 -> 456; complete code 1362288 -> 1365936 (+3648),
complete data 692220 -> 692268 (+48), exact functions 9944 -> 9949. Overall
fuzzy reaches 99.52953. All matched data remains unchanged, and full all_source
and normal retail-hash verification pass. Evidence:
build/parallel-twentythird-merged-report.json and
build/parallel-twentythird-merged-validation.log.


Concurrent follow-up 81f2a2e2b also source-links xShadowSimple: its quick-cull
wrapper moves from the implementation into xQuickCull.h, with the established
-sym on setting for this unit. The retained matrix/quick-cull helper order
then links without a body change or conditional header suppression. Independent
combined validation passes with the retail SHA1. The full report changes only
xShadowSimple's completion status versus the preceding merged report: complete
units 457, complete code 1370984, complete data 706516; all match scores and
matched data remain unchanged. Evidence: build/parallel-twentythird-linked-report.json
and build/parallel-twentythird-linked-validation.log.


## Bink row-plane calculation order (2026-10-02)

YUV_blit improves from 89.09259 to 90.87037 and YUV_blit_mask from 82.31114
to 82.67094. In their paired-row postludes, calculate the luma and alpha row
positions before the destination positions, then advance chroma. Retail
loads/calculates those row bases in that order (generic .text 0xa74..0xa94).
The fields are distinct and the calculations read unchanged pitch/delta
locals, so no callback, pointed-to pixel access, or traversal step changes.

The generic blitter's odd-row postlude now uses three u8 pointer locals in
that block instead of reusing dest, ybase and abase from the outer setup.
They hold the same computed addresses through the same stores. Generic
plane order alone gives 89.36667; a local destination alone gives 89.31111,
and all three row-local pointers give 90.87037. Swapping the first two odd
stores is neutral and restored. Masked plane order alone gives 82.67094;
base-capture temporaries in that loop regress to 82.10115 and are restored.
Evidence: build/yuv-mask24*, yuv-planes24*, yuv-odd24*, yuv-odd24b*.

Full all_source and normal build pass. The complete deduplicated report
changes only these two functions, with no data or exact/source-linked count
changes. YUV reaches 95.835785 and overall fuzzy 99.53079. Retail DOL SHA1
remains 306526d90b48e99894c3138f5fc8f2716d9fecf6; YUV stays NonMatching.
Validation: build/parallel-twentyfourth-final-report.json and
build/parallel-twentyfourth-final-validation.log. No compiler/assembly changes
or ancillary behavior tests. Worker iSystem, camera and xFX type/macro audits
found no additional gain; unchanged/restored reports and builds pass.


## Bink generic-blitter working pitch parameter (2026-10-02)

YUV_blit improves from 90.87037 to 93.577774. Save the original destination
pitch in old_destpitch and use the destpitch parameter itself as the working
pitch passed by address to setup_scaling. The initial destination origin still
uses the original pitch; scaling and subsequent row advances use the same
working values as before. This recovers the retail prologue's destination-Y
and original-pitch copies (.text 0x6f8/0x714) and initial pitch stack home.
The parameter form alone gives 93.35185 and grows source length from 1068 to
1076 bytes toward retail 1080. It is a natural parameter/local lifetime
choice, not a forced address escape or compiler change.

Move chroma pitch and row-delta calculations immediately after setup_scaling,
before the plane-pointer stores. Retail performs these invariant calculations
before those stores. This gives 93.577774. A named pitch pointer is neutral;
reusing the source-pitch parameter for chroma regresses to 90.9 and is restored.
Division by two and copy-then-half chroma forms are neutral and restored.
Artifacts: build/yuv-blit25*, yuv-blit25b*, yuv-blit25c*, yuv-blit25d*.

Full all_source and normal build pass. The complete deduplicated report
changes only YUV_blit; YUV reaches 95.93532 and overall fuzzy 99.532. All
matched data, exact-function and source-linked totals remain unchanged.
Retail DOL SHA1 remains 306526d90b48e99894c3138f5fc8f2716d9fecf6; YUV stays
NonMatching. Validation: build/parallel-twentyfifth-report.json and
build/parallel-twentyfifth-validation.log. No ancillary behavior tests run.
Header, GetKeyFrame and audio parameter/helper follow-ups produced no gain
and were restored; a useful parameter-lifetime fix does not transfer blindly.


## Branch-emission audit: TalkBox and Hangable (2026-10-02)

At staging d90e7695f, these two residues are not the same missing-dead-code
pattern. Fresh successful compilations with configured GC/2.0p1e show:

- zEntHangable_UpdateFX remains 260 bytes and 99.76923% after relocation
  normalization. Retail .text 0x280/284/288 is `bne 0x28c; b 0x304; b 0x304`;
  source is `beq epilogue; b default; b epilogue`. Both retain the third,
  unreachable branch. The current difference is polarity and block ordering,
  not deletion of that branch. This supersedes the earlier broad reading that
  Hangable proves the compiler always drops the unreachable end-of-case branch;
  historical runs may have used different compiler variants.
- TalkBox next_state_type::start remains 384 target / 376 source bytes,
  97.916664%. Retail .text 0x2f78 branches to the epilogue at 0x2f80, followed
  by an unreachable branch at 0x2f7c back to the traversal test at 0x2ee0.
  Source falls into the epilogue and lacks both instructions.

There are stronger exact controls in the same freshly compiled TalkBox TU:
parse_tag_sound is 824 bytes at 100%, including unreachable second branches
at retail .text 0xc68 and 0xe1c; cb_dispatch is 424 bytes at 100%, including
`b 0x1ee4; b 0x1ee4` at 0x1db8/1dbc. No direct branch targets those second
instructions. The parse function uses if/else/return; dispatch uses a switch.
CalcCombinedDepen was also recompiled and remains 608 bytes at 100%, preserving
the previously source-recovered clamp branch form. Thus neither dead-branch
retention generally nor a broad switch-emission limitation is established.

The complete standalone probe below is a control for source-sensitive branch
emission, not a reproduction of retail TalkBox's absent back-edge. It preserves
Hangable's current source dispatch shape while removing game types and calls.
The equivalent if form removes its two unconditional branches. The reduced
traversal falls through after copy_wait and does not preserve the retail pair.

```cpp
extern void emit_effect(int state);
extern bool trigger(int index);
extern bool wait_needed();
extern void copy_wait();
struct Traversal { int end; int page_end; };
extern Traversal traversal;

void switch_return(int enabled, int state)
{
    if (enabled) {
        switch (state) {
        case 2: return;
        default: emit_effect(state); break;
        }
    }
}

void if_return(int enabled, int state)
{
    if (enabled) {
        if (state == 2) return;
        emit_effect(state);
    }
}

void traversal_tail()
{
    while (traversal.end < traversal.page_end) {
        if (!trigger(traversal.end++)) break;
    }
    if (traversal.end == traversal.page_end) {
        if (!wait_needed()) copy_wait();
    }
}
```

All three compile successfully under both units' identical current flags with
stock GC/2.0p1 and configured GC/2.0p1e. Each produces the same 228 .text bytes:
switch_return 60, if_return 52, traversal_tail 116. Section SHA1 is
b23ea962cad122d67b8b73da1aeaee967a324a3d. External-call relocations are unresolved;
this standalone probe has no retail reference binary. Byte identity across
these compilers does not establish the original retail compiler or source CFG.

Compiler SHA1: stock2.0p1 74bc177b10d1bbe8a60a21a6c0aa86d2dd9c0668;
configured2.0p1e 9d445725489050035740aaff35860eddbaf3c3c9. Invocation uses the
unchanged executable via sjiswrap, the flags below, then
`-c build/branch-audit-probe.cpp -o build/branch-audit-probe.o`:

```text
-nodefaults -proc gekko -align powerpc -enum int -fp hardware -Cpp_exceptions off -W err -O4,p -inline auto -pragma "cats off" -pragma "warn_notinlined off" -maxerrors 1 -nosyspath -RTTI off -fp_contract on -str reuse -multibyte -i include -i src/PowerPC_EABI_Support/include -i src/dolphin/include -i src/dolphin/src -i src/bink/include -i src/bink/src -i src -i build/GQPE78/include -DBUILD_VERSION=0 -DVERSION_GQPE78 -DNDEBUG=1 -lang=c++ -common on -char unsigned -str reuse,pool,readonly -use_lmw_stmw on -pragma "cpp_extensions on" -inline off -gccinc -i include/inline -i include/rwsdk -i src/SB/Core/gc -i src/SB/Core/x -i src/SB/Game -DGAMECUBE -sym on
```

No production or compiler bytes changed. This bounds the hypothesis: current
switch/if lowering can retain unreachable blocks, but no reduced case yet
explains why retail TalkBox retains that particular loop back-edge. The original
source structure and exact retail flags remain unknown; the responsible stage
(IR construction, CFG cleanup, or emission) is not located. No shared patch
mechanism or safe patch scope is inferred from these two holdouts.

Private artifacts: build/branch-audit-run.py, branch-audit-meta.json,
branch-audit-results.txt, branch-audit-byte-equality.json, the four probe .o/.s
pairs, branch-audit-talkbox-raw.json, branch-audit-hangable-raw.json, and
branch-audit-exact-clamp.txt. This appendix preserves the complete probe and
identities if ignored build artifacts are later removed.


## Bink chroma cursor and destination-row stores (2026-10-02)

YUV_blit improves 93.577774 -> 95.14445; YUV_blit_mask improves
82.67094 -> 82.76312. Advance the generic blitter's existing cbase pointer
between its first and second chroma-plane assignments. Retail reuses the
base/result register for this add (.text 0x8b4/0x8dc). The final cbase value
is otherwise unused; both plane addresses and inversion behavior are unchanged.
In both paired-row loops, assign S.dest1 after advancing U/V, recovering
retail's store order without changing callback or traversal behavior.

Destination-store placement alone gives generic 93.64445 and masked
82.76312. Chaining the second chroma address through S.u/S.v is neutral;
advancing cbase gives generic 95.14445. Applying the cursor pattern to luma,
alpha, or masked chroma regresses and is restored, as is alpha-parameter reuse.
Artifacts: build/yuv-blit26*, yuv-blit26b*, yuv-blit26c*, yuv-blit26d*.

Full all_source and normal build pass. The complete report changes only the
two blitters: YUV reaches 96.00272, overall fuzzy 99.532814. Data, exact-function
and source-linked totals are unchanged. Retail SHA1 remains
306526d90b48e99894c3138f5fc8f2716d9fecf6; YUV remains NonMatching.
Validation: build/parallel-twentysixth-report.json and
build/parallel-twentysixth-validation.log. No compiler/assembly changes or
ancillary behavior tests. Save/load and doubled-IDCT follow-ups produced no
gain; ordinary switch break/default forms also failed to reproduce Hangable's
retail branch order, so no production or compiler change was made for those.


## Bink chroma-delta lifetime (2026-10-02)

YUV_blit improves 95.14445 -> 95.9963. Initialize c_delta from chroma_pitch,
then subtract the half-width in place. This separates the shared half-pitch
value from the row delta and restores a register copy; source size grows from
1076 to retail's 1080 bytes. The copy is not yet in retail's exact register or
position, and initial-plane/odd-row scheduling remains unresolved. This is
ordinary C++ with unchanged arithmetic, no compiler or assembly changes.

Full all_source/normal build passes. The deduplicated report changes only
YUV_blit; YUV reaches 96.03404, overall fuzzy 99.533195. Exact functions, data,
and linked totals are unchanged. Retail SHA1 remains
306526d90b48e99894c3138f5fc8f2716d9fecf6; YUV remains NonMatching.
Validation: build/parallel-twentyseventh-report.json and
build/parallel-twentyseventh-validation.log. Related half-pitch temporary,
row-pointer capture, signedness, and assignment-placement trials are neutral
or worse (build/yuv-blit27*, 27b*, 27c*, 27d*); only the improvement is retained.


## iModel animation matrix stack initialization (2026-10-02)

`iModelAnimMatrices` improves from 98.4% to 100% (300 bytes). Initializing
its stack cursor at the base, then copying the identity through
`*++pMatrixArray`, recovers the remaining loop-entry instruction order.
The hierarchy now uses its actual `RpHAnimHierarchy` type and named
`numNodes`/`pNodeInfo` fields instead of a node-info cast and integer pointer.
The stack contents, quaternion conversion, and parent push/pop behavior
are unchanged. Earlier isolated hierarchy and index-initialization forms
were neutral; the initial stack-copy expression was the missing interaction.

The full deduplicated report changes only this function, with no function,
inline-caller, data, or exact-count regressions. iModel now has 38/38 exact
functions, 7,532 matched code bytes, and 1,336 matched data bytes. Full source
compilation and the normal retail DOL hash pass. The TU remains NonMatching:
an actual source-link trial differs in just the zero/one floating-point
literal slots and 14 referring load operands (18 bytes total). Resolving
that genuine literal creation order is still required before Matching.
No compiler modifications, new assembly, fake mappings, or ancillary
behavior tests were used. Evidence: build/imodel-exact-*-trials.py and
build/imodel-exact-candidate.json in the isolated RGB worktree.


## xFont: both remaining functions source-exact (2026-10-02)

Baseline dde4bde5e. parse_next_text_jot reaches 100% from 94.202896% by
retaining the debug-recorded char local while explicitly converting text[0]
to U8 at initialization and converting comparisons and the bounds argument
to U8. This combination preserves the loaded value across is_ws and narrows
at the later bounds call with the retail registers; the previous char-only
and U8-only failures did not establish a compiler limitation. The current
xFont flags use -char unsigned; the obsolete contrary source comment is removed.

parse_tag_tex reaches 100% from 99.87261% by binding the selected font
dimension as a const F32 reference, capturing the other size component by
value, and assigning the scaled result after restoring the selected dimension.
No call or font write occurs during this reference lifetime.

Full all_source build and authoritative deduplicated report pass: all 184
xFont functions, all 26392 code bytes and all 69096 data bytes are exact.
Only these two function scores change across the full report; no code or data
regresses. Evidence: build/xfont-agent-exact-allsource.log and
build/xfont-agent-exact-report.json against xfont-agent-baseline-report.json.
GC/2.0p1a SHA1 remains a78a5fdb6c1d5677e987636b2e0743dbaefe9542;
GC/2.0p1e remains 9d445725489050035740aaff35860eddbaf3c3c9.
Normal build retains retail DOL SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6.

The Matching marker is deliberately withheld pending real source-link layout
closure. Initial source-link differs in 21566 bytes. Moving substr::create
from the file start to its retail helper position after xVec2::assign reduces
this to four bytes: references at 0x8002582b/0x80025857 and
0x8002632b/0x8002632f select reversed 8-byte zero aggregate templates for
get_texture_size and substr::create. These layout experiments remain uncommitted;
the retained commit contains only the verified function improvements.

## xFont: verified whole-unit source link (2026-10-03)

Follow-up to af1013dc7 closes the actual link, not just symbol scores. The
private xFontHelpers.h implementation include retains the real trailing helper
bodies as a separate debug code group under -sym on. It is included only by
xFont.cpp, after tex_render: parsing it earlier moves the first 0.0f literal
and shifts the float pool; parsing its substr initializer after get_texture_size
reverses their zero templates. This boundary preserves both allocation and code
order without extra objects, forced sections, dummy symbols or compiler changes.

The existing rwGameCube2DVertex and xtextbox custom assignment bodies move from
the public headers to their retail positions in xFont.cpp. Their declarations
and bodies are unchanged; making these definitions out-of-line prevents debug
header grouping from moving them away from their first callers. This is a
documented source-layout compromise: the source definitions have strong binding
where retail used weak copies, but the linked bytes and all caller scores agree.

The shared-header all_source rebuild compiled 311 affected objects. The full
deduplicated report has no function or data changes against the preceding exact
xFont checkpoint: 184/184 functions, 26392/26392 code and 69096/69096 data.
The real source-linked DOL is exactly 2859136 bytes and SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6, with zero differing bytes.
Matching is now retained, adding one linked unit and 26392 code/69096 data bytes.
Both isolated compiler hashes remain unchanged (2.0p1a a78a5fdb6c1d5677e987636b2e0743dbaefe9542,
2.0p1e 9d445725489050035740aaff35860eddbaf3c3c9).

Evidence: build/xfont-agent-helpergroup-report.json,
build/xfont-agent-link-allsource.log, build/xfont-agent-link-link-diff.json,
and final retained build/report artifacts build/xfont-agent-final-*.


## Plankton returned-vector copy checkpoint (2026-10-03)

`impart_velocity` improves from 91.12676% to 95.49296% with an explicitly
8-byte-aligned local wrapper around the existing `xVec2` offset. This is a
small compiler-layout workaround, not a claim about the original source.
Only local storage alignment changes: the same two `location()` calls build
the same `xVec2`, and `length2()` still receives that member by const reference.
The returned velocity copy now uses retail's r5/r4/r0 loads and contiguous
stores, followed by the y-zero store. The residual is the offset template
copy: source uses `lfd/stfd`, while retail uses two `lwz/stw` pairs. Source is
276 bytes against retail's 284 bytes; the unit remains NonMatching, 179/180
exact, and no source-link claim is made.

A header-free reproduction and logging-only alias capture isolate the
interaction: the compiler orders the velocity stores before the scalar
zero's whole-object literal load, but permits both 4-byte subrange loads
from the offset template to pass those stores. Exact controls rule out a
blanket aggregate-copy ordering rule: `xOBBHitsOBB` interleaves returned-vector
and template loads, and `world_to_ring_vel` interleaves a register-returned
`xVec2` with its next initializer. This overlaps the previously documented
whole-object/subrange alias question; it does not establish a new compiler
patch or show that source reconstruction is exhausted. No compiler decisions
or binary bytes were changed.

The full source build passes. The authoritative deduplicated report changes
only `impart_velocity`; all 6,728 Plankton data bytes remain exact. Normal
link reproduces retail SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6, using
the original object for this still-NonMatching unit. Isolated compiler hashes
remain p1a a78a5fdb6c1d5677e987636b2e0743dbaefe9542 and p1e
9d445725489050035740aaff35860eddbaf3c3c9.


## iModel actual source-link completion (2026-10-03)

After the exact-code checkpoint, the sole real link difference was literal
creation order: retail places 0.0f before 1.0f after the stream-reader's -1.0f
and 1.05f constants. The source placed one before zero, changing two literal
words and 14 referring load operands (18 differing bytes in total).

Keep xsqrt's existing arithmetic and weak linkage, but defer its definition
only in iModel until immediately before iModelAnimMatrices. Emitting that
actively called helper as an explicit `__declspec(weak)` definition creates
its zero before the matrix identity's one while preserving direct calls.
This is a documented compiler-specific C++ compromise, not a claim that the
original source used the same guard. The definition remains physically in
xMathInlines.h under a separate implementation guard; every other TU sees
its ordinary inline definition at the original include position. The
existing per-TU `-sym on` remains. No unused anchors, forced sections, fake
symbol mappings, padding, assembly, or compiler binary edits are involved.

Moving only the inline definition was neutral because emission remained
lazy. An address/reference binding created the right pool order but made
calls indirect, and a one-element zero object moved the identity flags
store; both were rejected. The explicit weak definition retains all code
and data matches and resolves the actual link, so iModel is now Matching.

Full all_source and normal builds pass. The complete deduplicated report
against the exact-code checkpoint has no function or data changes and no
regressions: 38/38 functions, 7,532 code bytes and 1,336 data bytes remain
exact. Completion increases by exactly those code/data bytes and one TU.
The source-linked DOL is byte-identical to retail, SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. Evidence in the isolated RGB
worktree: build/imodel-exact-linked-report.json,
build/imodel-exact-final-validation.log, and
build/imodel-exact-compare-final.py. No ancillary behavior tests were added.


### zTalkBox next_state::start inline definition (2026-10-03)

The unchanged method body becomes exact when its source definition is declared
`inline`: 97.916664 -> 100, 376 -> 384 emitted bytes. The compiler now retains
both retail tail branches. Earlier isolated loop probes did not establish a
compiler deficiency; they omitted this source-level compilation distinction.
No goto restructuring, assembly, or compiler modification is retained.

Full deduplicated report changes only this function; zTalkBox is now 118/118
functions, 14396/14396 code bytes and 37492/37492 data bytes. All-source and
normal build pass with retail SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6.
The first actual source-link attempt still differs due to physical method and
static-data ordering, so the unit remains NonMatching while that is audited.
Evidence: build/talkbox-next-agent-exact-report.json and
build/talkbox-next-agent-exact-allsource.log (local ignored artifacts).


## xEntMotion whole-TU completion (2026-10-03)

`xEntMotionDebugDraw` reaches 100% from 99.93198% (588 bytes). Remove the
explicit cached `src` local from the move-point case and compare `omp`
directly against `xem->mp.src`. The debug local list includes xmp/idx/omp/jdx,
but no src. The compiler still hoists the source-field load into exactly
the retail loop-entry position. All later instructions remain identical;
the first owner/target draw now preserves its target position in retail's
r29 instead of r30. Thus the old allocator-only/unreachable classification
for this residue was incorrect: the source-level cache was the blocker.

Named position captures, function-level debug-local scopes, scratch pointer
bindings, equivalent entry/switch CFGs, and inline/deferred helper forms did
not solve it. A shared scalar scratch capture obtained r29 but introduced a
redundant move and was rejected. The retained direct-field form is the
minimal reconstruction; no casts, pragmas, helper changes, compiler flag
changes, assembly, or compiler binary modifications are needed.

Full all_source and normal builds pass. The full deduplicated report changes
only DebugDraw: +588 exact code bytes and +1 exact function, without any
function, inline-caller, or data regressions. xEntMotion is now 41/41 exact,
with 11,184 code bytes and 1,464 data bytes exact. Actual source linking
produces zero differing bytes across the 2,859,136-byte DOL and retail SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6, so the TU is Matching. The existing
`-sym on` flag remains. No ancillary behavior tests were added.

Evidence in the isolated RGB worktree: build/motion-exact-sourcefield.py,
build/motion-exact-code-report.json, build/motion-exact-linked-report.json,
build/motion-exact-code-validation.log and
build/motion-exact-link-validation.log.


### Dutchman LassoNotify: exact switch reconstruction (2026-10-03)

The former 112-byte 96.42857% holdout is now exact from a one-token source
correction: the empty case is `LASS_EVNT_YANK` (4), not `LASS_EVNT_GRABEND`
(3). The compiler splits cases 2 and 4 at midpoint 3, producing retail's
`cmpwi 3; beq; bge; cmpwi 2; bge` and the previously missing redundant
branch. Thus the comparison against 3 did not identify the original empty
case. This supersedes the earlier compiler-only/dead-branch claims and the
partial BEGIN-case counterexample; no new event value or compiler change
is needed. The original strong out-of-line definition is preserved.

Behavior is unchanged: GRABSTART still selects DUTCHMANCAUGHT; every event,
including YANK and GRABEND, still forwards its original value to
`zNPCCommon::LassoNotify`. Empty-case/default classification has no other
effect. Definition-inline, header/class ownership, scope, and forwarding
label forms were neutral; examining existing protocol cases exposed the
incorrect switch assumption instead.

Validation at fa0ea7608: all_source and normal build pass; full deduplicated
report changes only LassoNotify, 96.42857 -> 100. Dutchman rises 225/227 ->
226/227 functions, 99.969 -> 99.97913, with all 8,840 data bytes still exact.
Normal DOL SHA1 remains 306526d90b48e99894c3138f5fc8f2716d9fecf6. Isolated
p1e compiler SHA1 remains 9d445725489050035740aaff35860eddbaf3c3c9. The
unit remains NonMatching pending turn_to_face; this is not a source-link
claim. Evidence is in ignored build/dutchman-empty-event-* and
build/dutchman-lasso-exact-report.json.


### Bink Huff4 fast-path symbol scratch (2026-10-03)

CheckReadHuff4Bundle improves from 99.56364 to 99.624245 at the same
660-byte size. In the no-refill branch, reuse the decoded-bit scratch for
the byte symbol before writing and advancing the destination. This recovers
the retail output byte register and store without changing bitstream update
order. The refill branch retains its original expression and word lifetime.

Compiler register-allocation diagnostics helped distinguish the remaining
mask/refill-word allocation from this symbol temporary. Inline decoder
helpers, explicit mask hoisting, and separate named symbol locals did not
improve the baseline and were restored. No compiler or assembly changes.

Combined with the Dutchman LassoNotify checkpoint, the full deduplicated
report changes exactly those two functions and no matched data totals.
All-source and normal builds pass; the normal DOL remains retail SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. Both units remain NonMatching;
this hash is not a claim that either complete unit is source-linked.
Evidence: build/parallel-thirtythird-report.json,
build/parallel-thirtythird-validation.log and build/expand33d-*.


### Bink stereo quantizer completion (2026-10-03)

quanttos16chans2 is now exact: 99.88372 -> 100, all 172 bytes. Give the left
and right channel conversions separate ordinary local scopes, rather than
reassigning the same output/sample temporaries for the second channel. The
first output-pointer increment now uses the running samples register, as in
retail. Separate channel-named locals also match, corroborating a local
lifetime distinction. Store order, clamping, multiplication and loop behavior
are unchanged. No assembly or compiler changes.

The full report changes only quanttos16chans2, adds one exact function and
172 exact code bytes, and loses no matched data. binkacd is now 7/8 exact
functions with BinkAudioDecompressOpen still at 99.82222. Its reported matched
data remains 320/416 bytes; the unit remains NonMatching.

The all-source and normal builds pass; the normal DOL retains retail SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. This does not claim a complete
source link for binkacd. Evidence: build/parallel-thirtyfourth-audio-report.json,
build/parallel-thirtyfourth-audio-validation.log and build/binkacd34b-*.
The loop-form trials and first-refinement-only square-root trials yielded no
additional gains and were restored.


### Bink audio source-link layout closure (2026-10-03)

An actual source-link check exposed layout failures hidden by isolated object
scores: the DOL grew by 64 bytes, with 16857 differing bytes. The named static
floating constants emitted unused .sdata2 storage as well as the correct
96-byte .rodata literal pool. Expressing those eight constants as typed literal
macros removes the unused storage without changing any generated function or
the literal pool bytes.

The remaining large data shift began at 0x802B0378, immediately after the final
audio array. Retail's next Huffman pointer table begins at 0x802B0380. Restoring
16-byte alignment on that table and its split metadata reproduces this boundary
without invented padding objects. Sixteen is the minimum alignment explaining
the observed gap; the binary does not distinguish it from a larger alignment
satisfied by the same address. The source declaration and split now agree.

With both fixes, an actual source-linked binkacd DOL has the retail size and
only 10 differing bytes: the six known floating-register operands in the two
inlined square-root sites. All other bytes, including all data and relocations,
match. BinkAudioDecompressOpen still scores 99.82222, so binkacd remains
NonMatching. Normal all-source/build and retail SHA1 checks pass. No function
or matched-data score changes come from this layout repair.

Evidence: build/binkacd35-link-diff.json (initial),
build/binkacd35-literals-link-diff.json (literal-only),
build/binkacd35-alignment-link-diff.json (10-byte residue), and
build/parallel-thirtyfifth-layout-report.json / validation.log. The combined
report also confirms the four independent improvements from concurrent staging
commit c5b00d4dd, with no regressions.


### zTalkBox complete source link (2026-10-03)

The unchanged state bodies need consistent inline declarations and definitions:
this restores deferred state-method order and the start/next/wait/base/stop
vtable order. Inline trigger_jot overloads emit index then jot at first use.
The earlier inline pad_pressed definition allocates its zero static first in
.sbss, and the empty base methods precede set_text so their first use places
them correctly. The unchanged pointer/location asset definitions have their
own header, which puts their helpers after the sound queue group.

The last mismatch was physical ownership of the exact 40-byte __sinit and
56-byte shared_type constructor. With generic query instantiations visible,
these landed after xSnd.h helpers. Constructor/play/push definitions now live
in xSndQueue.h, included by their existing xHudMeter owner. The four-entry
playing/recent queries have explicit inline specializations in xSnd.h;
generic implementations remain in xSndQueue.h. This is a documented compiler
layout compromise, not a claim that retail used identical specializations.
All queue arithmetic and behavior are unchanged. Ordinary generic inline
query definitions, or removing either specialization, restored the wrong
initializer placement. No compiler edits, assembly, fake symbols, or padding
are used.

Validation: explicitly rebuilt all 138 transitive shared-header source users,
including both implementation owners. MW dependency output omitted xSnd.h
for zTalkBox, so the rebuild list came from a conservative include graph,
not solely Ninja's dependency cache. The full deduplicated report preserves
all 10147 function scores, all matched code, and all matched data relative to
the exact-function checkpoint. zTalkBox is 118/118 functions, 14396/14396 code
bytes and 37492/37492 data bytes. With Matching retained, linked progress rises
by exactly one unit, 14396 code bytes, and 37492 data bytes.

The actual source-linked DOL is byte-for-byte retail: 2859136 bytes, SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. Both compiler SHA1s are unchanged:
a78a5fdb6c1d5677e987636b2e0743dbaefe9542 (2.0p1a) and
9d445725489050035740aaff35860eddbaf3c3c9 (2.0p1e). Local evidence:
build/talkbox-next-agent-retained-report.json, retained-build.log,
final-allsource.log, transitive-rebuild.json, and final-link-summary.txt.


### Bink audio whole-TU completion (2026-10-03)

BinkAudioDecompressOpen reaches 100 from 99.82222, all 900 bytes. The existing
reciprocal-square-root estimate has a float temporary, which is then promoted
to the double-precision guess. All three Newton refinements use the same ordinary
expression. Earlier double-estimate variants either reused the estimate register
for the first product or eliminated the retail estimate-to-guess copy. The float
estimate preserves that copy and recovers the six remaining register operands.
The existing frsqrte assembly statement is unchanged; no assembly or compiler
changes were added. The refinement arithmetic remains double precision.

With the preceding literal/table-alignment repair, the complete source-linked
DOL has retail SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6, and binkacd is now
Matching. All 8 functions and 3300 code bytes are exact; complete progress adds one
TU, 3300 code bytes and 416 data bytes. Objdiff still reports only 320/416 matched data
because the 96-byte literal pool has no source object symbols, although its bytes
and every linked relocation match retail. No synthetic pool symbols were added.

Full report changes only BinkAudioDecompressOpen, adds 900 exact code bytes and
one exact function, and regresses no code/data scores. All-source and actual
source-link builds pass. Evidence: build/binkacd37-private/float_estimate_nested.*,
build/parallel-thirtyseventh-audio-code-report.json,
build/parallel-thirtyseventh-audio-linked-report.json and their validation logs.


### Bink DVD-reader whole-TU completion (2026-10-03)

BinkFileReadHeader reaches 100 from 99.75, all 160 bytes. Increment the
volatile read cursor directly, then assign CurBufSize using the conditional
expression for the remaining/limit choice. Both ordinary source changes are
needed together: compound increment alone gives 98.875 and the conditional
expression alone gives 98.5. Together they restore the retail temporary
lifetimes without dummy values, assembly additions, or compiler changes.

The previous actual source link differed by just four bytes, all at the two
remaining header instructions. The retained exact form now source-links to
retail SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6. ngcfile is Matching:
13/13 functions, 2784/2784 code bytes and 42592/42592 data bytes exact.
The complete progress increase is exactly one TU and those code/data sizes.

All-source and actual source-link builds pass. The full deduplicated report
changes only BinkFileReadHeader, adds one exact function and 160 exact code
bytes, and regresses no code/data scores. Direct-field volatility, signed/int
cursor, scope, and in-place subtraction controls did not improve the match
and were not retained. Existing assembly elsewhere in the file is unchanged.
Evidence: build/ngcfile38d-private/compound_ternary.*,
build/ngcfile38-link-diff.json and build/parallel-thirtyeighth-ngcfile-report.json
with build/parallel-thirtyeighth-ngcfile-validation.log.


### ngcrad3d: texture-format capture completes actual source linking (2026-10-03)

`Open_RAD_3D_image` now captures the first GX texture format in a local
`const GXTexFmt format` immediately before `GXGetTexBufferSize`. The call
uses that value; dimensions and the later `GXInitTexObj` table lookup are
unchanged. Separating the third argument's value from the call expression
makes the compiler prepare height before width, matching retail's two `clrlwi`
instructions around the height store. Earlier dimension captures had left
these instructions swapped; capturing the height assignment instead fixed
the ordering but changed the table-address temporary from retail r3 to r5.
The format-value capture alone reproduces the entire function naturally.

Authoritative full deduplicated report: `Open_RAD_3D_image`
99.68254 -> 100; no other function, data, or exact-code regression. The TU
is now 12/12 exact, with 1704/1704 code bytes and 96/96 data bytes. Both
`all_source` and the normal build pass. Selecting this object as `Matching`
produces a byte-identical retail DOL, SHA1
`306526d90b48e99894c3138f5fc8f2716d9fecf6`. Compiler binaries are unchanged:
GC/2.0p1a `a78a5fdb6c1d5677e987636b2e0743dbaefe9542`, GC/2.0p1e
`9d445725489050035740aaff35860eddbaf3c3c9`. No assembly or compiler changes
and no extra behavior-test infrastructure were needed.

Worker artifacts: `build/rad-exact-baseline-report.json`,
`rad-exact-linked-report.json`, `rad-exact-linked-source-build.log`,
`rad-exact-link.log`, and `rad-exact-callprep-notes.json`.


### Bungee detachment: preserve the existing camera reference (2026-10-03)

`hanging_state_type::start_detaching` now passes its existing `cam` reference
into `xQuatFromMat` instead of recomputing `&globals.camera.mat`. Keeping that
reference live removes the two remaining address-preparation scheduling
mismatches: 99.848274% -> 100%, 580 bytes. The one-line source change is
independent of the ongoing helper/layout work; no shared headers changed.

The full deduplicated report changes only this function, with no function or
data regressions. Bungee now has 116/120 reported exact functions,
25,120/25,296 exact code bytes, and all 6,528 data bytes exact. It remains
NonMatching: anonymous helper identities and actual source-link layout are
still unresolved. The normal full source build and retail DOL hash pass;
that hash is not a claim that the Bungee object is source-linked.

Worker verification rebuilt all 224 SB source objects against restored
baseline headers. Root evidence: `build/parallel-fortieth-bungee-report.json`
and `build/parallel-fortieth-bungee-validation.log`. No compiler edits or new
assembly were used.


## 2026-10-03: Duplotron source-link layout preparation

Reconstruct the retail definition layout without claiming the final smoke-phase
match. `ScenePrepare` belongs before `SceneFinish` and `ScenePostInit`. The
constructor and six trivial overrides have weak bindings in retail; place their
unchanged bodies in the class header as inline definitions. Enable this unit's
existing compiler `-sym on` mode so deferred emission puts those definitions and
the local matrix helper in retail order. Moving the header bodies alone does not
solve the layout; deferred emission is the necessary companion change.

A temporary actual source link now differs from retail in only five bytes,
down from 1,076 before preparation, at DOL offsets `0x122c09`, `0x122c11`,
`0x122c15`, `0x122c17`, and `0x122c19`. All five belong to the known phase
register/operand residue in `VFXSmokeStack`; there are no remaining differences
in data, other functions, weak ordering, padding, or downstream addresses. Both
DOL files are 2,859,136 bytes. The source-linked SHA1 is
`9469c7c1a4ba89741fad6e955f1739c77af808de`, so the unit stays **NonMatching**.
The extra source-object constant tail noted previously does not create an
actual linked-data difference.

Freshly rebuild all six transitive consumers of the changed header:
`zAssetTypes.cpp`, `zNPCGoalDuplotron.cpp`, `zNPCMgr.cpp`, `zNPCTypeCommon.cpp`,
`zNPCTypeDuplotron.cpp`, and `zNPCTypes.cpp`, all under `src/SB/Game`.
The full deduplicated report is exactly equal to the baseline, including all
other units and data. Duplotron remains 34/35 functions, smoke-stack 99.79508%,
unit 99.97832%, and 2608/2608 exact data bytes. `all_source` and the restored
normal retail-link hash pass; compiler hashes remain unchanged. There is no
compiler modification, assembly addition, or behavioral change.

Raw source-link inventories and logs are retained as
`build/duplotron-layout-retained-*` in the isolated worktree. Root-usable affected
source/object lists are `build/duplotron-layout-affected-sources.txt` and
`build/duplotron-layout-affected-objects.txt`. New immutable/reused phase,
partial-staging, and half-period-division forms were neutral or worse and
restored. The half-period form emits `fdivs`, so it is not the retail multiply
lowering. The tested member helper boundary remained an actual call and must
not be represented as an exhausted inline-parameter experiment.


## 2026-10-03: Duplotron named cycle scale

Keep the existing mutable phase scratch and name its `2.0f` cycle scale as an
`F32` local. Later constant propagation now preserves the cycle as the first
operand of the initial multiply, matching retail. No instruction, arithmetic
operation, emitted constant, or state change is added. A `const` coefficient
returns to the old operand order; naming both coefficients gives no further
benefit, so only the useful cycle scale is retained.

`VFXSmokeStack` improves from 99.79508% to 99.83607%, still 488 bytes. The full
deduplicated report changes only that function and its unit's fuzzy score
(99.97832% to 99.98265%). Exact-function counts and all 2608 data bytes are
unchanged. `all_source`, the normal retail DOL hash, and both isolated compiler
hashes pass. A temporary actual source link improves from five differing bytes
to four: `0x122c09`, `0x122c11`, `0x122c15`, and `0x122c19`; its SHA1 is
`59c1eae09e1de1bd8ade4dda975d136fe70b3142`. Only the cycle/PI `f1`/`f2`
allocation remains. The unit stays NonMatching.

The earlier helper caveat was resolved diagnostically: inline settings must
remain active through deferred member emission to inline the helper. That
actual-inline trial produced the earlier, worse phase load order and expanded
other helpers, so it was restored. Direct count construction, double-literal
boundaries, and the other phase forms were also rejected. Artifacts and full
verification are saved under `build/duplotron-scale-checkpoint-*`.


### Bink reader conversion constants belong to the reader (2026-10-03)

Move .rodata 0x80274DF0..0x80274E00 from ngcsnd.c to binkread.c in the
split configuration. The two doubles (bits 0x4330000000000000 and
0x41E0000000000000) are referenced only by BinkOpen at 0x801995B8/0x801995C0
and 0x80199688/0x80199698. They exactly equal binkread.o's compiler-generated
16-byte conversion pool. Their historical SOUND symbol labels did not prove
sound-unit ownership; the retail references and emitted reader pool do.

An actual reader source-link audit previously grew the DOL from 2,859,136 to
2,859,200 bytes by emitting this pool again. Correcting ownership restores
the retail DOL length. The reader remains NonMatching with three code
holdouts; this is layout preparation, not a complete link claim.

The full source/normal build retains the retail SHA1. All Bink function scores
are unchanged; the 16 target data bytes move between units, and project-wide
matched data remains 1,264,984 / 1,280,684. The compiler-generated anonymous
pool remains uncredited in the report; no fake symbols were introduced.
Evidence: build/binkread41-link-diff.json, binkread41-split-link-diff.json,
binkread41-retained-split-report.json and retained-split-validation.log.


## 2026-10-03: Duplotron fully exact and source-linked

Combine the mutable cycle scale with an immutable phase value, then keep the
sine result separate. The coefficient alone fixed multiply operand order; the
immutable phase alone was worse. Together they recover retail's phase in `f1`,
`2.0f` in `f0`, and PI in `f2`, with both multiply instructions exact. This is a
source-lifetime interaction, not a compiler patch. The arithmetic remains
`(cycle * 2.0f) * PI`, with the same sine and particle-count conversion.

`VFXSmokeStack` reaches 100%, completing all 35 functions, 4612 code bytes, and
2608 data bytes. Before changing the marker, the full deduplicated report
changes only this function and its unit, with no regression in any other
function, inline instance, or data. The earlier header/order/deferred-emission
preparation now permits a byte-identical actual source link: all 2,859,136 DOL
bytes equal retail, SHA1 `306526d90b48e99894c3138f5fc8f2716d9fecf6`.
Only after that check is the unit marked Matching, adding one complete TU,
4612 source-linked code bytes, and 2608 source-linked data bytes.

The full source build, normal DOL hash check, and unchanged isolated compiler
hashes pass. All six header consumers were freshly compiled during the layout
checkpoint; the final change is confined to this cpp and its Matching marker.
No compiler modification, new assembly, speculative arithmetic, or ancillary
behavior test was required. Final proof is retained in
`build/duplotron-exact-final-report.json`, `build/duplotron-exact-source-link.json`,
and the corresponding build/link logs in the isolated worktree.


### Bink sound/reader read-only pools reconstructed (2026-10-03)

All four error strings currently emitted by ngcsnd.c are referenced only by
BinkOpen. Move their existing definitions into binkread.c and assign
.rodata 0x80274D80..0x80274E00 to the reader. Sound owns the preceding
0x80274D50..0x80274D80 range. The reader now emits its error strings followed
by the two conversion doubles identified in the earlier ownership audit.

Replace the sound unit's unused named scalar float objects and its one-element
pan-center array with equivalent typed literal macros. The compiler emits the
retail literal pool without duplicate objects. Both source .rodata sections
are byte-identical to their retail sections: sound48bytes, reader128bytes.
A deferred real center-array definition also reproduced the pool, but the
simpler literal form produces the same code and data and is retained.

Actual links with both sound and reader sourced shrink from2,859,200 to the
retail2,859,136 bytes. Code holdouts remain in both units, so neither is marked
Matching. This removes a layout discrepancy; it is not a claim that the linked
DOL is otherwise exact. No fake symbols or padding objects were added.

All function scores and project-wide matched-data totals are unchanged in the
full deduplicated report. Per-unit target data totals move with their true
ownership. The full source/normal build passes and retains the retail SHA1.
Evidence: build/parallel-fortysecond-bink-pools-report.json and corresponding
validation log, bink42-link-comparison.json, and bink42-pools-private artifacts.


## 2026-10-03: xScene exact arithmetic and actual source-link completion

`xRayHitsGrid` improves from 99.8243% to 100%, completing xScene's 36/36
functions, 9,596 code bytes, and 184 data bytes. The only code residue was
scheduling/register allocation in the first ray endpoint calculation. Three
`const F32&` bindings keep the rounded product temporaries alive until the
endpoint additions: Z first, then X and Y, followed by natural X/Y/Z endpoint
assignments. Scalar value declarations alone were neutral; temporary bindings
alone and Z-first bindings alone were incomplete. This is an explicit C++
temporary-lifetime compromise, not a claim that these were the original source
qualifiers, and it introduces neither assembly nor a compiler change.

Exact function scores initially still left actual source-link differences.
Moving the callback constructor after `xRayHitsSceneFlags` restored its retail
position. A narrow xGrid opt-in then gives xScene an explicit weak grid-index
assignment, the concrete callback specialization, and a weak `get_grid_index`
definition at the retail boundaries. The grid algorithm remains shared in
xGrid.h; normal callers keep the existing definitions and header grouping.
The existing range-limit and box helper bodies move unchanged into private
xSceneHelpers.h. Its include sits after the grid callback use and before the
matrix helper use, reproducing the final group order without moving shared
matrix/vector definitions. The TU retains `-sym on` and is now Matching.

Validation: the final authoritative deduplicated report changes only
`xRayHitsGrid`; every other function score and unit's matched data/code/function
counts are unchanged or increase. The full all_source build and normal build
pass. The actually source-linked DOL is byte-for-byte identical to retail,
SHA1 `306526d90b48e99894c3138f5fc8f2716d9fecf6`. Isolated compiler hashes remain
GC/2.0p1a `a78a5fdb6c1d5677e987636b2e0743dbaefe9542` and GC/2.0p1e
`9d445725489050035740aaff35860eddbaf3c3c9`. No ancillary behavior suite was added.
Evidence in the isolated worktree: scene-exact-baseline-report.json,
scene-exact-final-report.json, scene-exact-final-build.log,
scene-exact-final-link.log, and the zero-difference diagnostic source-link
inventory. Header work was coordinated with the Bungee worker; its separate
ordinary-template spelling change can be combined with this opt-in.


### Bink lossless writer child-length cursor (2026-10-03)

WriteBPLossless improves from 97.6512% to 97.92515%. Retail walks the four
adjacent coefficient lengths with a byte pointer: an indexed first load plus
a saved address, followed by three update-addressing loads and plain reloads
after bitstream writes. Reconstruct that cursor alongside the coefficient
index, capturing each child depth before writing its presence bit. The reload
after the bitstream macro remains explicit, matching the original access
sequence. No arithmetic, tree format, compiler flag, or assembly is changed.

The generated function remains 2668 bytes against retail's 2672;
register allocation and a remaining copy still prevent an exact match. A
pointer without the initial depth capture, narrower temporary scopes, and
direct increment expressions were worse and were not retained. These are
private assembly comparisons, not behavioral test cases.

The authoritative full deduplicated report changes only WriteBPLossless. All
other function scores and all matched-code/data/function counts are unchanged.
The full source build passes; the normal DOL retains its retail SHA1. Bitplane
remains NonMatching, so that normal-link check is not a claim of an exact
source link for this unit. Evidence: build/parallel-fortyfourth-writer-report.json,
parallel-fortyfourth-writer-validation.log, and bitplane44*-writer-private.


### zScene per-model mask scope retained (2026-10-03)

On staging `d92ee32f0`, `zScene` still has two holdouts: the 340-byte
`PipeForAllSceneModels` and 2,108-byte `zSceneInit`; `zSceneSetup` is already
exact. The primary worktree had no concurrent zScene source change.

Retained the previously measured but unbanked mask-scope improvement: declare
`remainSubObjBits` in the outer per-model iteration, before `model`, and assign
it after counting subobjects as before. This puts `model` in retail's r24 and
reduces differing register operands from twelve to eight. The remaining swap
is `k` (source r23, retail r25) versus `remainSubObjBits` (source r25, retail
r23). No new wrong role, extra instruction, changed control flow, or changed
function size is introduced. Removed the oversized source comment that treated
a limited declaration experiment as a proof of impossibility.

Authoritative full deduplicated report changes only PipeForAllSceneModels:
99.17647 -> 99.411766. Unit fuzzy score is 99.7793 -> 99.78364; exact totals
remain 72/74 functions and 15,984/18,432 code bytes, with all 4,040 data bytes
exact. This is a limited fuzzy improvement, not an exact-function or linked-unit
completion. zScene remains NonMatching.

Fresh baseline all_source and the retained all_source build pass, as does the
normal retail DOL SHA1 `306526d90b48e99894c3138f5fc8f2716d9fecf6`. Compiler
hashes remain GC/2.0p1a `a78a5fdb6c1d5677e987636b2e0743dbaefe9542` and
GC/2.0p1e `9d445725489050035740aaff35860eddbaf3c3c9`. Full report comparison
confirms every other function/unit and all data unchanged. Evidence is in the
isolated robot worktree's `build/zscene-layout-mask-validation.json`,
`build/zscene-layout-mask-report.json`, and `build/zscene-layout-mask-link.log`.

New unretained controls included an outer-for declaration of k with and without
moving i, a table-loop scope for k, an immutable model pointer/reference, a
block-local const subobject mask, a register hint on k, and inline helper
ownership. None improved on the retained control. In Init, the equivalent color
struct, late global definition, const descriptor/callback references were
neutral. Folding the global assignment into b's initializer added a redundant
byte store; its higher raw score was an alignment artifact and was rejected.
No helper, pragma, register hint, volatility, compiler change, or behavior test
was retained. These controls do not prove the remaining forms unreachable.


## 2026-10-03: zThrown fruit-entry reservation ownership

`zThrown_AddFruit` improves from 96.01852% to 96.203705% in the full
deduplicated report (raw 95.97222% to 96.15741%). Reserve its new entry as
`newThrown = &zThrownList[zThrownCount++]`, combining the existing entry
selection and counter increment into one expression. This preserves their
order and behavior, restores the stats pointer to retail r8 through its lookup
and later store, and removes the separate counter-update statement. It does
not recover the missing carry-pointer reload: source remains 428 bytes versus
retail 432. zThrown remains NonMatching, with 28/32 exact functions and all
15,000 data bytes exact. This is a small source-expression gain, not whole-TU
completion or evidence for a compiler patch.

The fresh pass reviewed the prior table-versus-parameter alias-provenance
probe and the exact Remove/LaunchStack controls. Real inline lookup and
allocation helpers were tested under isolated inline controls and confirmed
actually inlined; neither recovered the retail carry reload/order, so those
changes and flags were restored. LaunchVel remains unchanged. Capturing the
old counter in a local and narrowing the new-entry pointer gave no advantage
over the retained one-line expression. Shared types match the debug records;
no volatile global or speculative compiler modification was introduced.

Validation: all_source and normal builds pass. The full deduplicated report
changes only AddFruit; there are no other function-score, matched-code,
matched-data, or exact-count regressions. The selected-object DOL remains
byte-identical to retail, SHA1 `306526d90b48e99894c3138f5fc8f2716d9fecf6`.
Because zThrown is still NonMatching, that DOL check is not a claim that its
source object links exactly. Evidence in the isolated worker build directory:
thrown-exact-baseline-report.json, thrown-exact-gain-report.json,
thrown-exact-gain-build.log, thrown-exact-gain-link.log, and the reservation
and actual-inlining trial sources/diffs. No ancillary tests were added.


### Bink plane read alignment restored (2026-10-03)

ExpandPlane now uses the existing VarBitsGetAlign macro before returning its
read cursor. Bitstream refills already advance cur past the word being
consumed; unused buffered bits are discarded at the plane boundary. Retail's
epilogue clears bitlen and returns cur without advancing it. The previous
conditional cur++ incorrectly skipped another word when buffered bits remained.
The macro reproduces retail's operation directly, with no new helper or flag.

ExpandPlane improves from 55.890465% to 56.033806% in the authoritative full
deduplicated report. Source size changes from 5376 to 5360 bytes against
retail's 5916; the function still has substantial unrelated holdouts and the
unit remains NonMatching. Raw comparison is 55.430695% to 55.44557%; the full
report also normalizes relocation effects.

Combined verification includes the separately documented zScene mask-scope
and zThrown entry-reservation gains. Exactly those three functions improve;
all other scores, matched-data totals, exact-function/code counts, and complete
TU counts are unchanged. All-source build passes and the normal selected-object
DOL retains retail SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6. That normal
link is not evidence that these NonMatching source units link exactly. No
behavioral suite, compiler modification, or inline assembly was added.
Evidence: build/parallel-fortysixth-final-report.json, its validation log,
expand46d-private/read_align.json, and expand46-left.txt retail epilogue.

The direct scaled-pattern mask expansion and paired-row fill reconstructions
were privately compared but lowered overall matching and were not retained.
These overlap earlier rejected structures; subsequent work should use the
recorded assembly and a new surrounding lifetime/control-flow hypothesis.


## zCamera source-link preparation (2026-10-03)

The unit still has one 160-byte code holdout, `zCameraFlyStart` (94.85%
deduplicated), so it remains NonMatching. Its first actual source-link audit
changed 4,671 DOL bytes despite 41/42 exact functions and 552 exact data bytes.

The major layout error was the documented old-mwld common-BSS inflation bug:
`zcam_backupcam`, the first common, grew from 0x31c to the entire 0x648-byte
common block. Restore the unreferenced `char buffer[16]` recorded at the top
of this TU's debug symbols. It is stripped from retail, just as in the
existing zVolume reconstruction, and prevents the camera backup from
absorbing the whole block. This reduces the actual link residue to 273 bytes.

The remaining layout differences identify two misplaced existing helpers.
Retail owns weak `xVec3Dist2` in zCamera and weak `xVec3Dist` in xCollide;
move their unchanged arithmetic into their existing xVec3Inlines.h header.
Header ownership of `xVec3Dist2` fixes its order after `zCamera_FlyOnly`.
Making `xVec3Dist` available at TranSpeed causes its called xsqrt helper to
create the 1e-5 literal before MatrixSpeed's 114.59155 literal, exactly as
retail does. The other unused xsqrt constants strip normally. xCollide has
no reconstructed local call to xVec3Dist, so a narrow explicit-weak emission
guard preserves its retail helper and avoids losing its 80 exact bytes.
Also recover the retail local binding of zCameraFreeLookSetGoals and use
this TU's deferred `-sym on` emission path.

The resulting actual source-link residue is confined to 16 bytes in
FlyStart's frame-load/store cluster. Its source hoists TOCINFO.mempos above
the pause-state store and TOCINFO.size above the data-pointer store; retail
uses serial r0 load/store pairs. Struct, array, union, reference, immutable
copy, and genuinely inlined activation-helper trials did not resolve that
ordering and were restored. No compiler-only conclusion follows from them.

Validation artifacts are build/camera-flystart-retained-report.json,
build/camera-flystart-retained-all-source.log and the camera-flystart layout
link inventories. The affected source list is
build/camera-flystart-affected-sources.txt (186 header consumers). No compiler
binary changes, new assembly, or ancillary behavior tests were used.

Final checkpoint checks: all 186 header consumers were rebuilt, and the
entire deduplicated report is identical to the pre-change report. Normal
link retains retail SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6.
The actual camera source-linked DOL has the original 2,859,136-byte length,
16 differing bytes, and SHA1 78638dbe2495c05c0b581213376159ed3098fbf6.
Removing only this TU's `-sym on` increases that residue to 5,376 bytes,
so the emission flag is necessary for this header layout. The unit remains
NonMatching. Isolated p1a/p1e compiler hashes are unchanged.


### zLightning: name the repeated segment's vertex-array element (2026-10-03)

On the zScene checkpoint `7ce41aba1`, bind `RwIm3DVertex* const& drawVerts =
vert[i]` at the start of the repeated-segment draw loop, and use that element
for both vertices. This preserves the element's reloads across vector helper
calls while giving its lifetime an explicit source identity. The initial pair
needs no change. Authoritative deduplicated `zLightningFunc_Render` improves
**99.6076 -> 99.6405**, at the same 1,580 bytes; its differing instruction rows
fall **25 -> 22**. The axis cursor now uses retail's r25. The vertex cursor and
alpha still differ, and the two cursor increments are exchanged; this is a
partial improvement, not an exact-function claim.

The unit improves **99.720116 -> 99.7243**. Exact totals remain 15/17 functions,
7,920/12,448 code bytes, and all 7,808 data bytes. `RenderLightning` remains
99.028496. A full report comparison found this one function change only;
`all_source` and the normal build pass. Normal DOL SHA1 is
`306526d90b48e99894c3138f5fc8f2716d9fecf6`. Compiler hashes remain
2.0p1a `a78a5fdb6c1d5677e987636b2e0743dbaefe9542` and
2.0p1e `9d445725489050035740aaff35860eddbaf3c3c9`.
`zLightning` remains NonMatching: the normal checksum does not establish an
actual source link for this TU.

Focused controls: delaying the reference until the first vertex write loses
the gain; naming the axis too is neutral against baseline. Splitting the inner
alpha and making the element reference mutable do not improve on the retained
form. Shared-counter forms with this reference score 99.551895 (all loops) or
99.60253 (draw phase). Moving UV-phase initialization to the repeated segments
shortens the function and regresses; an initial vertex-pointer loop also
regresses. A local diagnostic of the PS2 DWARF `_col` macro form changes both
function sizes and regresses, so it does not justify changing the shared GC
header. An aggregate RGB snapshot for the initial pairs also regresses. None
of these alternatives or macro/header changes is retained; the earlier broad
claims of source impossibility are not conclusions supported by this pass.

Worker evidence under ignored `build/`: `zlightning-layout-retained-report.json`,
`zlightning-layout-retained-allsource.log`, `zlightning-layout-retained-link.log`,
`zlightning-layout-retained-validation.json`, and private variants in
`zlightning-layout/`. Validation compares against the preceding verified
`zscene-layout-mask-report.json`.


### zLightning: paired UV parity fixes the second-loop register cluster (2026-10-03)

Following `85d1e0935`, give the UV parity reused by each vertex pair a named
`S32 odd = i & 1` at its first use in both `RenderLightning` loops. This is
**99.028496 -> 99.15061**, with the same 2,948 bytes. All **18** differences in
the second loop's direction/parity/color/position-register cluster disappear;
differing rows fall **106 -> 88**, and the new differing-row set is a strict
subset of the old one. This is not merely a higher numerical score. Retail's
r0 parity and r3 direction-result roles are recovered, together with the
registers derived from their interference. The first loop stays byte-exact.

This result supersedes the earlier assertion that the two loops' allocation
asymmetry was not source-reachable. Applying the same source pattern to both
loops produces the required different allocation naturally. Capturing parity
before the direction calculation instead grows the function by four bytes
and regresses; the first-use lifetime is material. Naming parity in the end
caps is neutral. U32 color temporaries and a camera reference are neutral;
boolean direction, named tail-vertex references, and initial color aggregates
regress. No header, compiler, macro or alternative trial change is retained.

Full deduplicated comparison against the preceding verified report changes
only `RenderLightning`. Unit score is **99.75321**, exact totals remain 15/17
functions and 7,920/12,448 code bytes, and all 7,808 data bytes remain exact.
The retained `zLightningFunc_Render` score stays 99.6405. `all_source`, normal
build, retail DOL SHA1 `306526d90b48e99894c3138f5fc8f2716d9fecf6`, and the
unchanged 2.0p1a/2.0p1e compiler hashes pass. The unit is still NonMatching;
no actual source-link success is claimed. Remaining render differences are
in color-load scheduling and end-cap/address register allocation.

Ignored worker evidence: `build/zlightning-layout-parity-{report.json,
allsource.log,link.log,validation.json,mismatches.json}` and private trial
`build/zlightning-layout/render_parity_both_loops/`. The mismatch artifact
records the eighteen removed rows and an empty newly differing row list.


## Cutscene manager pointer-to-Boolean lowering (2026-10-03)

`check_hide_entities` now spells its pointer test explicitly:
`bool mgrNotNull = (globals.cmgr != NULL);`. This preserves the intended
null test and avoids an independently reproduced, intermittent wrong lowering
of the implicit conversion. The good 172-byte function is exact. The bad
160-byte function replaces `lwz` at `globals + 0x1fbc` followed by
`neg / or / srwi` with a single `lbz` at that address, then stores that byte
to `ents_hidden`. Reading one byte of a pointer is not the required Boolean
conversion (for example, a non-null GameCube address beginning with 0x80
must produce 1, not 0x80).

The first observation followed camera checkpoint `0074ec3ec`, but an
old-header/new-header paired rebuild was exact in both cases. Root then
reproduced an exact new-header rebuild too. The header change is therefore
not established as the cause. A private fixed-environment repetition used
baseline `6a3eee3e7` with only `xVec3Inlines.h` taken from `0074ec3ec`:

- Configured GC/2.0p1e, implicit conversion: 3 bad and 5 good objects in 8 runs.
- Stock GC/2.0p1, implicit conversion: initially 8/8 good, then one identical
  bad object in a second 8-run cohort (15 good / 1 bad overall).
- GC/2.0p1e, explicit null comparison: 8/8 good, byte-identical whole objects.
- Existing GC/2.0p1a and GC/2.0p1d controls: each 8/8 good; these finite
  results do not exclude intermittent failure in either version.

Stock reproducing the same bad object rules out claiming that the observed
failure requires our compiler patches. The precise cause is not identified;
there is no proposed compiler modification. Finite successful repetitions do
not establish an absolute guarantee. The source uses the ordinary explicit
null comparison as the bounded workaround.

SHA-1 identities:

- Stock p1 compiler: `74bc177b10d1bbe8a60a21a6c0aa86d2dd9c0668`.
- p1a compiler: `a78a5fdb6c1d5677e987636b2e0743dbaefe9542`.
- p1d compiler: `0a4878bb49f808bc137f7a8ae2fa08ae99c0c3b5`.
- p1e compiler: `9d445725489050035740aaff35860eddbaf3c3c9`.
- Good complete zCutsceneMgr object: `feeae14c8669bc5cb5249b2245aae65804fa981c`.
- Bad complete object: `a1232f288bcb17fbebf4adf19ad3b0c68c2ffdb4`.

The private RGB worktree preserves the exact compile command in
`build/camera-audit-command.txt`, repetition scripts and results in
`build/camera-audit-repeat.py`, `camera-audit-repetitions.json`,
`camera-audit-earlier-patches.py` and `camera-audit-earlier-patches.json`,
and both object images under `build/camera-audit-objects/<compiler>/<sha>.o`.
The command comes from `ninja -t commands
build/GQPE78/src/SB/Game/zCutsceneMgr.o`; each repetition executes its final
sjiswrap/mwcceppc command afresh, with TEMP and TMP both set to the private
`build/tmp-camera-audit` directory. Relevant settings are `-O4,p`,
`-proc gekko`, `-enum int`, `-fp hardware`, `-fp_contract on`, `-char unsigned`,
`-inline off`, `-common on`, `-use_lmw_stmw on`, and the normal GameCube SB
include paths/defines. The scripts replace only the compiler directory for
existing-binary controls and restore the temporary header/source edits.
No compiler binaries were changed and no behavioral test suite was added.

The retained one-line source change passes the full deduplicated comparison
without score/data regressions, `all_source`, and the normal build;
`zCutsceneMgr` remains NonMatching, so its retail object is still selected
for that link. The built DOL retains retail SHA-1
`306526d90b48e99894c3138f5fc8f2716d9fecf6`.


### Integrated camera/lightning verification (2026-10-03)

Root report `build/parallel-fortyseventh-final-report.json` against
`parallel-fortysixth-final-report.json` changes exactly two scores:
`zLightningFunc_Render` 99.6076 -> 99.6405 and `RenderLightning`
99.028496 -> 99.15061. All other function scores and all per-unit/global
exact-code, data, function and completion measures are unchanged (466/543
complete units). The explicit cutscene null comparison retains its exact
function. `all_source` and the normal retail link pass.

The root camera source-link audit independently reproduced the worker's
16 differing bytes, solely at DOL offsets 0x4d2f4..0x4d303, with original
length 2,859,136 and SHA1 78638dbe2495c05c0b581213376159ed3098fbf6.
Evidence: `build/camera47-source-link.json` and its saved DOL/link log.
Normal object selection was restored. These are partial gains; no additional
unit is marked Matching and no compiler binary was changed.


### zLightning: promoted color snapshots remove the endpoint register rotations

After `4f5612234`, each single-write RGB snapshot in `RenderLightning` binds
`const U32&` to an explicit `U32(l->color.channel)` value. The conversion creates
a value snapshot, rather than aliasing the original byte field. The references
remain local to their one color write. This small lifetime compromise improves
**99.15061 -> 99.95116** at unchanged 2,948 bytes: **82 differing rows disappear**,
leaving just the two initial three-load RGB scheduling clusters. Every tail
and end-cap register operand is now retail-exact. Plain U32 locals, plain
per-write const U8 values, and references to the original byte fields do not
produce this result; promotion plus reference lifetime is material.

Full deduplicated comparison changes only this function. Unit **99.9428**,
15/17 exact functions, 7,920/12,448 exact code bytes, and 7,808/7,808 data bytes;
Func_Render remains 99.6405. `all_source`, normal build and retail/compiler
hashes pass. An actual source-link check keeps the retail DOL size and leaves
36 differing bytes, all inside the two remaining render functions; no helper,
constant-pool or data-layout discrepancy is present. NonMatching is restored
and the normal retail checksum passes. The function and TU are not yet exact.

Controls retained only in ignored artifacts: explicit promoted snapshots at
the initial pairs are neutral, reordering those captures to match the observed
load order regresses, and byte-typed snapshot references regress. The same
promoted RGB snapshots in Func_Render are neutral and are not retained there.
Artifacts: `build/zlightning-layout-snapshots-{report.json,validation.json,
allsource.log,link.log,source-diff.json,source-main.dol}`. Prior source-link
inventory was 162 differing bytes; the retained gain removes the large shared
endpoint/tail clusters without a linked layout change.


### zLightning: RenderLightning is fully exact (2026-10-03)

The last six rows after `504003484` were the two initial pairs' load order.
Read `l->color.r` directly in their RGBA macros, retaining the shared green
and blue snapshots. The compiler still emits one shared red load per pair,
but now schedules it after green and blue without changing their register
roles. A named red-field reference also matches, so the smaller direct form
is retained. **RenderLightning is 100%, all 2,948 bytes**, and the full report
changes only that function. Unit **99.95437**, 16/17 functions exact,
10,868/12,448 exact code bytes, and 7,808/7,808 data bytes. Func_Render remains
99.6405 and is the sole holdout.

`all_source`, normal build, retail and compiler hashes pass. An actual source
link now differs by just **24 bytes**, every one inside Func_Render's range
`0x800A0648..0x800A0C74`; the exact RenderLightning and all other DOL bytes
match retail. DOL size remains 2,859,136. NonMatching is restored until the
last function closes; no layout work or compiler change is needed on this
baseline. Evidence: `build/zlightning-layout-renderexact-{report.json,
validation.json,allsource.log,link.log,source-diff.json,source-main.dol}`.


### Root verification of the exact lightning render (2026-10-03)

Against the preceding staging report, `parallel-fortyeighth-render-report.json`
changes only RenderLightning from 99.15061 to 100: +2,948 matched code bytes
and +1 exact function. All other scores/data and completed-unit totals are
unchanged. All-source and normal retail build pass. Root independently
source-linked zLightning and reproduced exactly 24 differing DOL bytes,
original length 2,859,136 and SHA1 b0cb70a2d8a199d7902a131dc89456b4b2c84999;
normal object selection and retail hash were then restored. Artifacts:
`build/lightning48-source-link.json`, saved DOL, and restoration log.

The private Bink decoder root-cursor forms and guarded next-magnitude shift
did not improve ReadBPLossless (91.80942 / 91.47207 versus 92.4414). They were
not applied to production source. The root-cursor form overlaps older root
initialization experiments and provides no new compiler-deficiency evidence.


### iMath3 sphere slab bound lifetimes (2026-10-03)

`iBoxIsectSphere` now scopes each axis's box lower bound and sphere upper
bound to that axis's classification. The sphere center and radius remain
snapshots; the lower sphere bound is expressed at its two comparison sites,
which the compiler combines into one subtraction. No values, branch cases,
helper calls, or post-helper radius reads change. The debug record names only
`xcode`, `ycode`, and `zcode`; it does not establish the earlier long-lived
`lo`/`hi` scratch arrangement. Retail keeps radius in f2 and the lower sphere
bound in f0. These shorter lifetimes recover that allocation without a new
helper, volatile access, compiler change, or inline assembly.

The authoritative full deduplicated report improves the 668-byte function
from 98.98203% to 99.371254%, and the unit from 99.869026% to 99.919106%.
Every other unit and function is identical to the fresh baseline; the unit
remains 17/18 exact functions with all 40 data bytes matching. Scoped scalar
bounds alone reached 99.19162%; aggregate intervals and temporary references
introduced extra memory traffic, while broader center scopes or comparison
booleans lost matching. Only the strongest verified form is retained.

A real source-selected link remains 55 bytes different at the unchanged
2,859,136-byte DOL size (SHA1 `99ca180599ee0a199fc604e8456c240f6dc8d109`).
Twenty-one bytes are register operands in this function: retail reuses f1
for center/upper sphere bound and the dead lower-box register for its upper
bound, while source uses distinct registers. Another 32 relocation bytes
and two data bytes reflect a pre-existing swap of the 0.5f/0.0f constant
pool entries. Thus deduplicated data equality does not establish actual pool
layout equality. The TU stays NonMatching; no completion is claimed.

All-source and normal builds pass; the restored normal selected-object DOL
has retail SHA1 `306526d90b48e99894c3138f5fc8f2716d9fecf6`. Isolated GC/2.0p1a
and GC/2.0p1e compiler hashes remain `a78a5fdb6c1d5677e987636b2e0743dbaefe9542`
and `9d445725489050035740aaff35860eddbaf3c3c9`. Evidence is saved under ignored
`build/imath3-return-*`: baseline/candidate full reports, source-link byte
inventory, build logs, rejected sources and raw diffs. Only score/build/link
validation was used.


### zLightning: complete Func_Render and source-link the unit (2026-10-03)

`zLightningFunc_Render` reaches 100% at the original 1,580 bytes through
four interacting source lifetimes: keep alpha in `U32` after explicitly
normalizing each random value through `U8`, use direct vertex-array indexing,
share the function's loop counter as in the debug locals, and separate the
initial zero UV phase from the repeated-segment phase initialized to one.
The initial phase remains a named integer, preserving retail's two floating
additions; the earlier literal-zero spelling had removed those instructions.
The explicit byte conversion preserves the original alpha values. The wider
local and separate phase bindings are bounded C++ reconstruction choices,
not claims that the PS2 debug types describe the original GameCube source.

These forms must be assessed together. Normalized word alpha removes the
initial alpha/cursor swap and fixes the repeated alpha register; direct
indexing then fixes both repeated cursors and their increment order. The
shared counter fixes its own lifetime but initially exchanges the UV phase
and vertex count registers. Separating the initial and repeated UV phases
resolves that last exchange. The former vertex-element reference is no
longer needed. This supersedes the old claim that the remaining register
allocation could not be reached from source.

Combined with the independently verified exact `RenderLightning` commits
`504003484` and `218988381`, the TU is now **17/17 exact**, with all
**12,448 code bytes and 7,808 data bytes** exact. The complete deduplicated
comparison against checkpoint `4a05bb532` plus the explicit cutscene-null fix
changes only the two lightning render functions; all other function scores,
matched data and exact counts are unchanged. The combined gain is 4,528 exact
code bytes and two functions. `all_source` passes.

`zLightning.cpp` is marked Matching only after the actual source-object link:
the 2,859,136-byte DOL is byte-for-byte identical to retail, SHA-1
`306526d90b48e99894c3138f5fc8f2716d9fecf6`. The existing `-sym on` remains.
No compiler, header, macro, assembler or behavioral-test changes are involved.
Evidence is preserved in the RGB worktree under `build/lightningfunc-*`:
`base-report.json`, `exact-report.json`, `source-linked-report.json`,
`exact-build.log`, `exact-source-link.log`, and the private source/raw-diff
trials. The retained Func_Render candidate is `separate_initial_phase.cpp`.


### Integrated lightning completion and iMath3 verification (2026-10-03)

Root all-source build and actual retail DOL checksum pass with zLightning
selected from its compiled source object. The full deduplicated report
`build/parallel-fortyeighth-final-report.json` changes only the two lightning
render functions (both now 100) and iBoxIsectSphere (98.98203 -> 99.371254)
against the preceding staging report. No other unit/function regresses.
Matched code increases 4,528 bytes and exact functions increase by two;
complete source-linked units rise 466 -> 467, adding 12,448 complete code
bytes and 7,808 complete data bytes. Actual linked DOL SHA1 remains
306526d90b48e99894c3138f5fc8f2716d9fecf6. iMath3 remains NonMatching.
Evidence: the final report, validation log and report-comparison script
under `build/parallel-fortyeighth-*`.


### bamatlst: immutable material-count snapshot improves the final holdout

On the worker's `218988381` baseline, snapshot `len` as a scoped
`const RwInt32 materialCount` after list initialization, and use that snapshot
for the empty check and `_rpMaterialListSetSize`. The count remains immutable
across the allocation helper's inlined body. The later streamed-index reads
and loop still use the original `len` variable. `_rpMaterialListStreamRead`
improves **99.88064 -> 99.933685**, at unchanged 1,508 bytes. The allocation
byte count now uses retail's r26. Differing rows fall **8 -> 5**, with no new
differing row; the remaining count operand uses r25 instead of retail r27.
This narrows the CSE-pair barrier described in `docs/RW_RESIDUE.md` without
claiming it is fully resolved.

Full deduplicated report changes only this function; unit **99.96464**,
7/8 functions exact and 1,320/2,828 exact code bytes, with data unchanged.
`all_source`, normal build, retail SHA1 and unchanged compiler hashes pass.
Actual source linking yields a same-size 2,859,136-byte DOL with exactly
**five differing bytes**, at `0x80218909`, `0x8021890D`, `0x80218922`,
`0x8021892D`, and `0x802189A1`; all are the residual count-register operands.
There is no independent helper/data/layout discrepancy. NonMatching remains
set, and the normal retail checksum is restored.

The bounded controls were an existing chunk-size scratch reuse (extra stack
store, 1,504 bytes and regression), unsigned count snapshot (regression),
capture before initialization (regression), and reuse of the existing loop
counter (neutral). None is retained. Source/compiler/behavior tests were not
expanded. Evidence under ignored `build/`: `bamatlst-lifetime-count-` report,
validation, mismatch, allsource/link and source-link artifacts; private source
controls are in `bamatlst-lifetime/`.


### iMath3 pool ownership follow-up (2026-10-03)

ELF inspection corrects the preceding checkpoint's original pool label:
retail begins with 0.5f (`3f000000`), then 0.0f (`00000000`), then 1.0f.
The first two retained source entries are reversed. This accounts for two
data-byte differences and 32 relocated load operands in the actual-link
inventory; the other 21 differences remain sphere-slab register operands.
The earlier reference to a 0/1 swap was incorrect.

The actively called `xsqrt` implementation in xMathInlines.h creates these
constants. Its surviving retail copy in xBound loads half and three before
`__fpclassifyf` and preserves them in f31/f30; current source loads them
afterward. The helper is itself only 67.51163% matched, so its source cannot
be treated as an exact control for iMath3's literal creation order. Selecting
its existing explicit-weak emission form in iMath3 and a separate `-sym on`
control both preserved the wrong order. Temporary-reference representations
of half and both coefficients introduced addressable temporary storage,
moved the pool further away, and lowered the xBound helper's raw score. They were rejected.
No artificial pool declarations, padding, compiler changes, or new assembly
were used; no shared-header or configuration change remains.

The retained checkpoint remains 99.371254% for iBoxIsectSphere and 17/18 exact
functions. Fresh all-source build and full deduplicated report reproduce the
checkpoint with no differences; the normal selected-object DOL again has
retail SHA1 `306526d90b48e99894c3138f5fc8f2716d9fecf6`, with both isolated
compiler hashes unchanged. This bounded follow-up does not resolve the
original coefficient lifetime or prove an optimizer defect. Local evidence:
`build/imath3-pool-*` raw diffs, rejected header/source forms, and restored
report/build logs.


## Bink block-copy alternating row cursors (2026-10-03)

`ExpandPlane` improves **56.033806 -> 64.977684** in the complete deduplicated
report. Retail's skip and raw copy paths alternate two row-address chains;
previous source advanced a single cursor or recomputed each row from a base.
Recover those independent even/odd row cursors for skip source/destination,
raw destination, all three motion-copy sources, and the normal fill destination.
Each pair is still copied in scan order, with the same aligned-double versus
word-copy branches, eight rows, pitches, and bundle/work consumption. The last
pair needs no additional cursor advance. This is ordinary C pointer lifetime
reconstruction, with no compiler changes or new assembly.

The source evidence is explicit in the retail add/copy chains (for example,
skip at object text 0x2118..0x2248 and raw at 0x2d94..0x2ef0). The first skip
change alone reaches 57.17985 full-report; raw and motion paths then improve
the combined function further. Extending alternating destination cursors to
the direct motion path instead regresses and is not retained. Earlier scaled
pattern/fill/raw and loop-counter combinations also remain rejected. All
private candidates are under `build/expand49-*-private`; keyframe wrapping
and IDCT helper controls produced no gain and are documented in
`build/bink49-findings.txt`.

Full `all_source` and the normal selected-object retail link pass. Comparing
`parallel-fortyninth-final-report.json` against `parallel-fortyeighth-final-report.json`
changes only ExpandPlane and the separately documented material-list stream
reader gain. All other function scores, matched-data totals, exact counts and
complete totals are unchanged (467 complete units). DOL SHA1 remains
306526d90b48e99894c3138f5fc8f2716d9fecf6. Expand remains NonMatching, so that
normal link does not establish a retail source link for the Bink decoder.
No behavioral test suite was added or run.


### zGame: recover source-link layout, retain the three-store holdout (2026-10-03)

The near-exact report concealed a substantial source-link layout difference.
Selecting the original zGame source object produced a same-size DOL with
9,313 differing bytes. The 276-byte `xUtil_select<char>` body appeared before
`zGameInit`, shifting the main functions by 0x114, and common-object placement
also displaced `globals` by 24 bytes.

Three source-supported changes recover the real layout:

- The existing per-TU `-sym on` setting mechanism moves the template out of
  the main function group; this alone reduces the DOL residue to 2,410 bytes.
- Restore `char buffer[16]` before `dummyPortalAsset`. The actual declaration
  is recorded twice at file scope in `dwarf/SB/Game/zGame.cpp`, lines 8-9.
  It is a genuine common declaration, not synthetic padding. This recovers
  common-object placement and reduces the residue to 345 bytes.
- Keep the existing `xDrawEnd` and `xDrawBegin` implementation bodies in a
  private `zGameDrawHelpers.h` include at the end of the cpp, with inline
  definitions matching their retail weak binding. Their implementation-file
  ownership places them after `xUtil_select<char>` as retail requires.
  Merely adding inline in the cpp fixes binding but leaves their order wrong.

The final diagnostic source DOL has the retail length of 2,859,136 bytes and
only **49 differing bytes**, all inside `zGameScreenTransitionUpdate` at DOL
offsets **0x95e98..0x95f1b**. Its SHA-1 is
`73cc28c7208a9d3c90966e1e1c6e2a2cb1380a67`. Every byte outside the existing
three-store scheduling residue matches. The TU remains **NonMatching**:
23/24 functions exact, 7,680/8,664 code bytes exact, all 3,656 data bytes exact,
and transition score 99.11382% (98.97154 raw).

Source controls retained no code changes: shared quad-coordinate copies,
immutable UV snapshots, normalized color captures and inline transition
emission are neutral or worse. Shared UV copies regress; named screen-edge
reference values add an instruction. A scoped scheduling-off control regresses
the raw function to 80.01626%, so it is not retained and does not justify a
compiler change. The exact adjacent `zGame_HackDrawCard` remains an important
counterexample to a blanket scheduler explanation.

The retained full deduplicated report equals the lightning-complete baseline
exactly; `all_source` and the normal build pass. After restoring NonMatching,
the normal DOL again has retail SHA-1
`306526d90b48e99894c3138f5fc8f2716d9fecf6`. No shared headers, compiler binaries,
assembly or behavioral tests changed. RGB-worktree evidence is under
`build/zgame-close-*`: `retained-report.json`, `retained-build.log`,
`retained-final-link.log`, `retained-source.json`, `retained-source.dol`,
`retained-audit.py`, and the restored source trial scripts/raw diffs.


### Integrated Bink/material/zGame verification (2026-10-03)

Final root all-source build and full report retain exactly two score changes:
ExpandPlane 56.033806 -> 64.977684 and _rpMaterialListStreamRead
99.88064 -> 99.933685. Every per-unit/global data, exact-code/function and
completion measure is unchanged; 467 units remain complete. Bink source size
is 5,640 bytes versus retail 5,916 (previous source 5,360), recovering real
row-address operations rather than adding padding.

Root independently reproduces the zGame worker's actual source link: same
2,859,136-byte DOL, 49 differing bytes confined to offsets 0x95e98..0x95f1b,
SHA1 73cc28c7208a9d3c90966e1e1c6e2a2cb1380a67. Normal object selection is
restored, and retail DOL SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6 passes.
All three units remain NonMatching. Evidence: final report/validation log
`build/parallel-fortyninth-final-*` and `build/zgame49-source-link.json`,
saved DOL, source-link and restoration logs. No new inline assembly,
compiler modification, or ancillary behavioral suite was used.


### Bink ExpandPlane: recover scaled block output (2026-10-03)

Retail scaled-pattern output expands pairs of mask bits directly into two
output rows using mask3/mask4. Reconstruct that path instead of expanding
to an intermediate block first. Scaled raw output advances the color bundle
once per input row, and scaled fills write paired output rows. Recover the
remaining-width loop counter and end-of-row work-column reset. These ordinary
C changes follow the retail block structure without assembly or compiler edits.

Full deduplicated ExpandPlane score improves 64.977684 -> 67.05882.
Source size is 5,644 bytes versus retail 5,916. The full source build and
normal retail build pass; every other function score and all global/per-unit
exact code, data and completion measures are unchanged. Complete units remain
467/543; this is partial Bink progress, not a completed TU.

Evidence: build/expand50-scaled-private and expand50-fill-private contain
isolated comparisons; build/parallel-fiftieth-scaled-report.json and
parallel-fiftieth-scaled-validation.log contain integrated verification.


### Cinematic helper ownership and source-link preparation (2026-10-03)

The actual zNPCFXCinematic source link improves from 8,217 differing DOL
bytes to 3,972 at the unchanged 2,859,136-byte size. Its SHA1 is now
`fdac9623e12b452529a126922fe41e174b5ff7ab`. This is layout preparation, not a
completed TU: NCIN_SleepyDRay_AR remains 436 bytes and 97.477066% matched,
with 92/93 functions and all 27,624 data bytes exact in the full report.

Retail places NoseyClear/NoseySet, the local matrix-vector helper, five cone
setters, KillAll and singleton after the ordinary callbacks, in that order.
The source previously emitted the matrix helper and singleton among the
callbacks and gave the Nosey/cone/KillAll methods strong bindings instead of
retail's weak bindings. `-sym on`, explicit weak Nosey definitions and a
TU-private helper definition group recover that order and those bindings.
Two narrow opt-ins expose declarations for xMat3x3RMulVec and SB2::singleton
in this TU; the private group supplies their existing arithmetic/accessor
bodies in the required positions. Every other TU retains its previous
header definitions. The duplicated local matrix body is a documented small
C++ compromise to avoid moving a widely shared definition's emission group.
No common-BSS workaround is needed: neither object has COMMON symbols and
the named BSS/SBSS layouts already agree. No padding or constants were added.

The existing UV table declarations now follow the two color initializers,
as their retail pool ordering requires. This preserves all code/data scores.
With the earlier pool still displaced, that correct ordering adds five
incidental absolute-byte differences compared with the 3,967-byte helper-only
checkpoint; preserving the incorrect relative order would obstruct closure.

The remaining pool issue is explicit: retail creates 3.0f and the signed
integer-to-double bias `4330000080000000` before PI; source creates them at
later uses. This changes alignment and leaves the linked .sdata2 eight bytes
shorter. All shared global function addresses now agree; the only shifted
shared globals are eight objects following that pool, at -8 bytes. Current
residue partitions as 1,904 .text, 2,066 .sdata2, one .init and one .sdata
byte. No supported earlier literal owner was found, and none was invented.
The separate code holdout still concerns UV-product register assignment and
scroll-load/color-stack-store scheduling. Earlier notes about a heap-store
alias residue do not describe this current source.

All 162 affected source consumers rebuilt after the header changes. The full
deduplicated report is identical to baseline across all units/functions and
data, including inline callers. All-source and normal builds pass; restored
normal DOL SHA1 is `306526d90b48e99894c3138f5fc8f2716d9fecf6`. Both isolated
compiler hashes remain unchanged. NonMatching is retained. No compiler
modification, new assembly, or behavioral test suite was used. Evidence:
`build/cinematic-layout-final-report.json`, `cinematic-layout-affected-sources.txt`,
`cinematic-layout-owned-link.json`, saved linked ELF, and associated build/raw
comparison logs.


### Bink run-block reconstruction and integrated cinematic verification (2026-10-03)

ExpandPlane improves from 67.05882 to 74.951996 in the full deduplicated
report. Retail decodes ordinary run blocks into the 8x8 scratch buffer before
copying to the output. Recover that path, alternating destination cursors for
pattern/motion rows, and chained doubled rows for scaled runs. Use the ordinary
outer row loop, stage fill-word replication, and consume the inter DC value
after its motion copy as retail does. The existing VarBitsGet macro replaces
the custom exp_get_bits implementation. All changes are ordinary C.

The retained source frame is retail's 0x478 bytes; function size is 5,576
versus retail 5,916. A row-cursor generation error in a private intermediate
experiment was corrected during source review before commit. Only the corrected
source and final report are authoritative; earlier private scores are rejected.

All 162 consumers of cinematic's changed headers rebuilt. The final all-source
and normal retail builds pass, and every other function score plus every
per-unit/global exact code, data and completion measure is unchanged. Complete
units remain 467/543. Root also independently reproduces cinematic's actual
source link: 3,972 differing bytes at the retail size, SHA1
fdac9623e12b452529a126922fe41e174b5ff7ab. Normal selection is restored and
retail DOL SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6 passes.

Evidence: build/parallel-fiftyfirst-final-report.json, its validation log,
expand51-corrected-private/corrected_motion.*, and cinematic51-source-link.json.
Both TUs remain NonMatching; no compiler modifications or new assembly.


## zLasso morph-target object references (2026-10-03)

Bind the two existing morph targets to const RpMorphTarget references before
reading their vertex pointers. This preserves object identity and the original
index arithmetic; no aggregate copy or public layout changes. Retail separately
adds the verts field offset to both scaled indices. Direct field accesses in the
previous source instead shared morphTarget+0x14, removing one instruction. The
reference form restores the target 4048-byte function from 4044 bytes.

Full deduplicated zLasso_Render improves 98.70257 -> 99.242096, and the unit
99.36231 -> 99.62749. Only this function changes in the full report; all 14504
data bytes remain exact and the unit stays 19/20 exact and NonMatching. Existing
RenderWare accessor macros and advancing the original index were neutral;
scoping the vertex pointers regressed slightly. Separate branch matrix scopes
were neutral. Sharing the UV scratch variables suggested by the debug names
regressed to 98.64921 / 4044 bytes and was restored. These results do not imply a
compiler defect or that the remaining source search is exhausted.

Actual source selection now differs in 840 DOL bytes, down from 1221211 caused
largely by the previous four-byte text shortfall. Source DOL remains 2859136
bytes, SHA1 499c7b26129eaff41142c2ee87841164a7361275. All 840 differences are
in .text: 153 inside Render; the remaining 687 are at initVertMap, bakeMorphAnim,
vec2vecMat, xMat4x3Rot/C and two AddGuide call bytes, requiring a separate helper
layout audit. This is partial matching progress, not a completed/source-linked
TU. Remaining Render differences include register lifetimes through geometry
setup and vertex emission, plus the last emission loop's increment scheduling.

Evidence: build/lasso-closure-gain-report.json, lasso-closure-gain-link.json,
lasso-closure-gain-linked.elf and lasso-closure-gain-build.log. Full all_source
and normal build pass; normal retail SHA1 is
306526d90b48e99894c3138f5fc8f2716d9fecf6. GC/2.0p1a and GC/2.0p1e remain
respectively a78a5fdb6c1d5677e987636b2e0743dbaefe9542 and
9d445725489050035740aaff35860eddbaf3c3c9.


## zLasso weak rotation-helper layout (2026-10-03)

The retail object places xMat4x3Rot and xMat4x3RotC in a separate weak .text
section, after the local initVertMap, vec2vecMat and bakeMorphAnim bodies. The
source instead defined both strongly before those local helpers. Marking the
definitions inline recovers the weak bindings; placing their definitions after
the local helpers recovers retained order. All changes are private to this TU,
with the existing -inline off preserving the calls and exact helper code.

The complete deduplicated report is identical to the preceding morph-reference
checkpoint. all_source and normal build pass. Actual source selection reduces
840 differing DOL bytes to 146, all inside zLasso_Render; helper order, bindings,
and call destinations now match. This also resolves seven relocation bytes
previously counted within Render. Source SHA1 is
1078ecccdfe9c2a0cbe5dcfa1f8d8f32264db9f3, size 2859136. Normal selection restores
retail SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6. The unit remains
NonMatching, 19/20 exact; Render remains 99.242096. No compiler or shared-header
change is involved.

Evidence: build/lasso-layout-tail.json, lasso-layout-tail-link.json,
lasso-layout-tail-linked.elf and lasso-layout-final-report.json. The remaining
146 bytes are register assignment and emission-loop scheduling, not unexplained
pool or layout displacement. No artificial constants or padding were added.


### Bink normal pattern: assemble both words before writing (2026-10-03)

On staging8f9204062, the normal pattern path at retail .text0x2984..0x2c24
assembles both four-pixel words before the row stores. In particular, its two
ORs at0x2a0c/0x2a10 precede stores at0x2a14/0x2a18. Preserve that operation
boundary with ordinary u32 low_word/high_word locals in expand_pattern_row.
Also consume the two colors with individual cur_ptr increments, matching the
pointer updates at0x299c..0x29b4. The alternating row cursors and scaled pattern
helper are unchanged. No qualifiers, compiler changes, or assembly were added.

Private ProDG3.5 raw scores: baseline74.13049 (5576 bytes), paired word
snapshots78.1021 (5576), snapshots plus separate color consumption78.38472
(5584). The full source build and authoritative deduplicated report confirm
ExpandPlane74.951996 ->79.16565; exactly that one function changes globally.
All other function records, data totals, exact-code/function totals, and unit
identities are unchanged. Both all_source and the normal link pass; normal
DOL SHA1 remains306526d90b48e99894c3138f5fc8f2716d9fecf6. Expand remains
NonMatching, so this checksum is not a whole-unit source-link claim.

Evidence in the RGB worktree: build/pattern-order-private.py and its private
candidate objects/raw reports, pattern-order-baseline-report.json,
pattern-order-retained-report.json, pattern-order-validation.json, and both
baseline/retained build/link logs. Compiler hashes remain unchanged.


### Integrated pattern/run/fill and lasso verification (2026-10-03)

Combining the normal pattern word snapshots with paired scaled-fill row cursors
and the contiguous run-block decoder improves ExpandPlane74.951996->79.55849
in the full deduplicated report. Both run callers consume a contiguous8x8
scratch block, so remove the unused pitch parameter/offset mapping and use
EXPBITS_GET1_BRANCH for retail's branch-local one-bit handling. The scaled
fill retains sixteen16-byte output rows with one consumed color. Independent
source review confirmed unchanged row coverage, pixel arithmetic, bitstream
and bundle consumption for these changes and the pattern helper checkpoint.

zLasso_Render improves98.70257->99.242096 and restores retail function size.
Root reproduces the recovered helper layout's source-selected DOL:146 differing
bytes, all within Render, at the retail size; SHA1
1078ecccdfe9c2a0cbe5dcfa1f8d8f32264db9f3. Normal object selection is restored
and retail SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6 passes.

Full all-source/normal builds pass. Exactly those two function scores improve;
every other function and every per-unit/global exact-code, data and completion
measure is unchanged.467/543 units remain complete; both TUs remain NonMatching.
Evidence: build/parallel-fiftysecond-report.json and validation log,
expand52-combined-private/combined.*, and lasso52-source-link.json/saved DOL.
No compiler edits, new assembly, or behavioral test suites were used.


### Bink scaled pattern: prepare the next word before storing (2026-10-03)

On the verified normal-pattern checkpoint b0253478e, the scaled pattern loop
still completes both stores for each packed word before loading the next mask
pair. Retail starts the next pair earlier: loads at 0x3044/0x304c precede the
first store at 0x3058, and loads at 0x306c/0x3070 precede the store at 0x3074.
Two ordinary u32 word temporaries express that preparation boundary while
retaining all eight stores, row coverage, and bundle consumption. Only
expand_pattern_block_scaled changes; the normal pattern helpers remain intact.

Moving the second color load earlier is byte-identical because ProDG already
schedules it there. Computing all four words before any stores barely improves
raw matching (78.38472 -> 78.40365). Preparing one word ahead with four locals
reaches 78.82691; reusing two rolling word locals reaches 78.93847. The retained
function is 5,596 bytes versus the 5,584-byte baseline and 5,916-byte retail.

Full deduplicated verification changes only ExpandPlane, 79.16565 -> 79.658554;
all other function records, data, exact counts, and unit identities are unchanged.
The source build and normal link pass, retaining retail DOL SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. Expand remains NonMatching. This is
an isolated helper result before interaction with the newer run/fill changes.
Evidence: build/scaled-pattern-order-private/*, the private trial scripts,
scaled-pattern-order-retained-report.json, scaled-pattern-order-validation.json,
and retained build/link logs in the RGB worktree.


## UIFont immutable background rectangle (2026-10-03)

Making the empty-text background rectangle const in zUIFont_Render restores
retail's coordinate-load and delayed field-store ordering during aggregate
construction. The authoritative deduplicated score improves from 97.54153%
to 100%; all 13 functions, 3,160 code bytes, and 96 data bytes are exact.
The full report changes no other function or matched-data total, and the
all_source build passes. A separate coordinate-reference trial regressed
and was discarded; no header or compiler changes are involved.

The actual source-selected link has zero differing bytes and retains retail
DOL SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6, so zUIFont is now Matching.
Evidence in the isolated robot worktree: build/uifont-output-report.json,
build/uifont-output-source-diff.json, and build/uifont-output-source-link.log.


### UIFont completion and integrated Bink verification (2026-10-03)

Root independently verifies zUIFont as Matching: all 13 functions, 3,160 code
bytes and 96 data bytes are exact. The actual main.elf input is
build/GQPE78/src/SB/Game/zUIFont.o, and the resulting DOL retains retail SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. The source change is the immutable
empty-text background rectangle. Completed units increase from 467 to 468;
exact function count increases by one and exact code by 1,204 bytes.

Bink's scaled pattern helper prepares the next packed word before the current
output stores. Combine that with the retail signed filled-pixel comparison
and pitch-first motion multiplication. Full deduplicated ExpandPlane improves
79.55849 -> 80.09398. The count is bounded by block/run lengths, and neither
its signed comparison nor the commuted multiplication changes pixel or bundle
consumption. Offset-consumption rewrites were worse and were not retained.

The full report changes exactly UIFont Render and ExpandPlane. All other
functions and per-unit exact/data/completion measures are unchanged. Global
completion gains are exactly the UIFont totals above. All-source and normal
retail builds pass. No shared headers, compiler binaries or assembly changed.
Evidence: build/parallel-fiftythird-report.json and validation log, plus
expand53-run-count-private, expand53-motion-private and expand53-combined-private.


## UI rectangle construction and source-link closure (2026-10-03)

zUI_Render improves from 94.85797% to 100% by initializing the model's
immutable rectangle directly from asset expressions. This removes four
reconstruction-only scalar locals absent from the debug local list and
restores retail's dimension-conversion and field-store ordering. All 47
functions, 11,936 code bytes, and 10,660 data bytes now match. The full
all_source build passes; no other function or matched-data total changes.

Actual linkage additionally needs the file-scope buffer[16] recorded before
sSorted in dwarf/SB/Game/zUI.cpp. Without that original declaration, the
linker enlarges sSorted from 3,072 to 3,104 bytes despite the input symbol's
correct size, shifting subsequent COMMON storage. Restoring the declaration
reduces the DOL difference from 559 to 193 bytes without changing any score.

The remaining difference is the 44-byte xMat3x3Scale wrapper emitted before
the generated initializer instead of after it. Its unchanged body now belongs
to zUI's private inline implementation header, included before its first use;
the compiler's existing -sym on mode supplies retail's deferred header group.
The private header has no other consumers. No compiler binary or assembly
changes are involved. The retained source-selected link has zero differences
and SHA-1 306526d90b48e99894c3138f5fc8f2716d9fecf6, so zUI is Matching.
Evidence: build/zui-output-sym-report.json, build/zui-output-source-diff.json,
and build/zui-output-source-link.log in the isolated robot worktree.


### Integrated zUI completion verification (2026-10-03)

Root independently rebuilds all_source and the retail link with zUI selected
from build/GQPE78/src/SB/Game/zUI.o. The resulting DOL retains SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. The full deduplicated report changes
only zUI_Render, 94.85797 -> 100; all other function records and per-unit
measures remain unchanged. Completion increases to 469/543, with 11,936 code
and 10,660 data bytes newly source-linked. Exact code increases by 1,380 bytes
and exact functions by one. Evidence: build/parallel-fiftyfourth-report.json
and build/parallel-fiftyfourth-validation.log.

A separate read-only Bink audit explains 76 of ExpandPlane's 296-byte size
deficit: source skip shares 19 instructions with the motion-copy tail. Its
physical two double loads and one store are followed by six loads and seven
stores in the shared tail, preserving all eight copied rows. Retail tails
differ only in cursor register names, so this does not justify an artificial
anti-folding rewrite. ExpandPlane remains 80.09398 and NonMatching. Evidence:
build/expand-double-copy-audit.txt in the RGB worktree and root
build/expand53-combined-private/scaled_signed_pitch.json.


### Bink local run decoding and bit-reader comparisons (2026-10-03)

ExpandPlane improves 80.09398 -> 81.08249 in the full deduplicated report.
Both run paths now decode directly into the actual local motion_block array,
rather than passing it through a reconstructed inline helper's pointer formal.
This improves local lifetimes and scheduling; it does not yet reproduce all
retail frame-indexed stores. Normal runs mark the work block immediately after
reading the scan pattern, matching retail's 0x22b8 store. Scaled runs retain
their existing common work marking. The local patterns bundle is renamed
pattern_bundle so the scan macro still resolves the genuine global table.
Bitstream reads, run lengths, scratch writes, and bundle advances are preserved.

The six positive fixed-width bit reads use the equivalent buffered-count
comparison > count-1. This changes exactly six compare immediates and their
blt branches to retail's ble forms, with no size or other-function changes.
Widths are four and seven bits; no zero-width caller exists. The combined raw
score is 80.38607 and source size 5,632 bytes (retail 5,916). A guarded do-loop
recovered retail's -1 entry guard and zero exit comparison but reduced overall
matching, so it was rejected. No frame padding or compiler changes were used.

All-source build and normal retail link pass with SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. Every other function and all exact
code/data/completion measures are unchanged; completion remains 469/543.
Expand remains NonMatching, so this checksum does not claim source-link closure.
Evidence: build/parallel-fiftyfifth-report.json and validation log, root
build/expand55-threshold-private and expand55-combined-private, and RGB
build/run-lexical-private/lexical_mark_formatted.c with corresponding raw diff.


### Bink scaled-fill loop edges and copy-folding audit (2026-10-03)

The scaled fill now writes its first row, seven interior row pairs, then the
last row. This follows retail's seven-count loop at 0x2f80 instead of the
reconstructed eight paired iterations. It still writes all sixteen 16-byte
rows and consumes one color. ExpandPlane improves 81.08249 -> 81.1474 in the
full deduplicated report (raw 80.38607 -> 80.45774), source 5,632 -> 5,660 bytes.
Only this function changes; all exact-code/data/completion measures are stable.
All-source build and normal retail link pass with SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. Expand remains NonMatching.

A chained odd-row assignment only reversed store offsets without recovering
retail's pointer/store schedule, so it was discarded. Combining the peeled
fill with guarded do-loops for runs scored lower and was also discarded.
Evidence: build/expand56-fill-private, expand56-combined-private, and
parallel-fiftysixth-report.json/validation.log; RGB scaled-fill-chain-private.

The complete skip/motion audit explains 224 bytes of shared-tail folding:
76 for double copies and 148 for word copies, exactly 56 deleted diff rows.
Word skip executes three loads/two stores before branching to a shared tail
with thirteen loads/fourteen stores; all eight rows are preserved. This
leaves a net 32-byte size gap after the scaled-fill change, not proof of any
missing pixel operation. RGB evidence: build/expand-skip-motion-folding-audit.txt.

Bink reader .rodata is already byte-identical (128/128), with all twelve
normalized relocations equal. Its 93.333336 section score reflects named
retail conversion constants versus anonymous compiler constants, not wrong
data. A fresh bidirectional keyframe-loop variant regressed and was restored.
Evidence in the plankton worktree: build/binkread-closure-rodata-proof.json
and build/binkread-closure-findings.md. No reader source changes were retained.


### Bink frame-pointer lifetimes and tail updates (2026-10-03)

Initialize ExpandPlane's existing dest/old/work_row locals at declaration,
before decoder setup calls. Their values are unchanged, but their lifetimes
now recover retail's frame/work register roles (r22/r21/r20) and remove three
late pointer copies. Using the formal parameters directly also improves the
private score but is weaker than these ordinary initialized locals. Move the
independent work-column and frame-pointer updates before the remaining-width
subtraction, following the target tail; row coverage and work marks are intact.

Combined raw matching improves 80.45774 -> 81.362404, with source size
5,660 -> 5,648 bytes. Full deduplicated ExpandPlane improves
81.1474 -> 82.05882. Every other function record and all per-unit/global exact
code, data, and completion measures are unchanged. All-source compilation and
normal retail linkage pass, SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6.
Expand remains NonMatching and completion remains 469/543.

A separate normal-pattern audit finds identical 168-instruction/672-byte
operation inventories, including all mask loads, pixel stores and pattern
cursor reloads. Moving its work mark after both color captures gains alone,
but loses against this pointer-lifetime checkpoint when combined (81.1359 raw).
Preserving the helper with captured color-value arguments has the same loss.
Both pattern variants are therefore saved but not retained. A fresh Dutchman
review also confirms the proposed aggregate-lifetime mechanism was already
tried and regressed; no repeated compilation or source edit was made.

Evidence: build/expand57-formals-private, expand57-tail-private,
expand57-combined-private, expand57-pattern-combined-private, and
parallel-fiftyseventh-report.json/validation.log. RGB evidence is
normal-pattern-audit-rows.txt, pattern-lexical-private and
pattern-captured-helper-private under build.


## YUV initializer format-owned red shift (2026-10-03)

Retain the red-channel packing shift alongside each format's channel widths and
blue shift. All existing formats/defaults use red_shift = 0; write that value to
the existing first RGBshift slot. No format cases, table slots, packed values,
header declarations or storage sizes change. The first word's historical
RGB_SHIFT_RESERVED0 spelling is preserved to avoid an unrelated header change.

Retail YUV_init defines r31 = 0 within its format cases (+0x254, +0x268,
+0x284, +0x2a0, +0x2b8 and +0x2dc), then keeps it through the first RGBshift
store at +0x374. This is distinct from the generic reserved-word zero formed at
+0x38c. The neighboring first-triplet values are the green shift (red_bits)
and blue_shift, supporting red-shift ownership rather than arbitrary register
spelling. Earlier format-switch/table-pointer/blue-high trials did not exercise
this value boundary. Only this single source hypothesis was tried this pass.

Full deduplicated YUV_init improves 68.080536 -> 68.67785; raw objdiff improves
66.447426 -> 67.30872. Source size rises 1760 -> 1768 toward retail 1788. The full
report changes only YUV_init: unit 96.03404 -> 96.0704, all 4804 data bytes remain
exact, and 89/97 exact functions remain unchanged. This is partial source progress;
the YUV unit stays NonMatching and no actual source-link completion is claimed.

Fresh all_source and normal builds pass with retail SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. No compiler, assembly or behavioral-test
changes. Evidence: build/yuv-init-closure-{baseline,red-shift}.json,
yuv-init-closure-gain-report.json and yuv-init-closure-gain-build.log; baseline
full comparison is build/binkread-closure-restored-report.json at dcbec2d53.


### Integrated Bink sentinel loops, pattern boundary and YUV verification (2026-10-03)

Use the ordinary descending for-loop condition run_length != -1 in both
normal/scaled run branches. Each length comes from a byte, and the loop only
decrements it, so iteration counts and scratch writes are unchanged. This
recovers retail's minus-one entry checks, improving raw ExpandPlane
81.362404 -> 81.960785 and source size 5,648 -> 5,664 bytes. The earlier
guarded do-loop form still scores lower and is not retained. A standard
register qualifier on the plane parameter was neutral and is also discarded.

With these loop lifetimes, the saved normal-pattern boundary now gains when
combined: capture both colors before marking the work block. The normal case
contains its former inline helper body, preserving color replication, all
eight row calls and sixteen word stores. The row helper and scaled pattern
path are unchanged. Combined raw score is 82.04868 at the same 5,664 bytes.
This previously lost against the pointer-only checkpoint, so it was measured
again specifically after the run-loop allocation changed.

Root independently integrates the format-owned YUV red shift and verifies
exactly two full-report changes: ExpandPlane 82.05882 -> 82.80257 and
YUV_init 68.080536 -> 68.67785. Every other function record and all per-unit
and global exact-code/data/completion measures remain unchanged. All-source
compilation and normal retail linkage pass with SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. Both units remain NonMatching;
completion remains 469/543 and no source-link closure is claimed.

Evidence: build/parallel-fiftyeighth-final-report.json and final validation
log, expand58-for-private and expand58-run-private; RGB
build/pattern-loop-interaction-private contains the isolated combined source
and raw diff. Its CheckReadRLEHuff4Bundle audit found only register allocation
and scheduling, including one retail argument-copy instruction; no RLE edits
were justified.


## YUV format-switch body ownership (2026-10-03)

Replace the initializer's nested format if-chain with a switch in retail body
order: 565, 5551/555, 4444, 664, 655, default. Keep the explicit format-owned red_shift
from the preceding checkpoint. All accepted/default flag values and the cache
and invalid-flag guards remain unchanged. No forced jumps or padding are used;
identical suffixes remain available for ordinary compiler folding.

The old ascending-case switch trial was not evidence against switch ownership:
its raw disassembly already matched retail's comparison tree (split 9, then 8/7,
and 11/12), while its case bodies occurred in a different order and it lacked
the retained red-shift value. Retail places 565 first and shares its suffix with
the later 655 body. This coherent combination is the only new variant tried.

Full deduplicated YUV_init improves 68.67785 -> 72.979866; raw objdiff improves
67.30872 -> 71.87696. Source size changes 1768 -> 1796 versus retail 1788, so
some operation/lifetime differences remain. The full report changes only this
function; all 4804 data bytes and 89/97 exact YUV functions remain unchanged.
The unit stays NonMatching. all_source and normal build pass; normal DOL SHA1
is 306526d90b48e99894c3138f5fc8f2716d9fecf6. No behavioral tests, compiler edits,
assembly changes or source-link completion claim are involved.

Evidence: build/yuv-init-closure-ordered-switch.{cpp,json,py},
yuv-init-closure-switch-report.json and yuv-init-closure-switch-build.log.
The read-only phase inventory before this change is in
build/yuv-init-closure-residue-review.md.


### Integrated YUV switch and constant final-pixel verification (2026-10-03)

In each run path's existing filled_pixels == BINK_RUN_BLOCK_LAST_PIXEL
branch, index the scan with that constant instead of the counter. Retail
uses two direct byte loads at offset63; the previous source used indexed
loads despite the equality guard. Values, bundle consumption and scratch
writes are identical. Raw ExpandPlane improves 82.04868 -> 82.30088 with
source size unchanged at 5,664 bytes. Only ExpandPlane changes in the raw diff.

Root independently verifies the ordered YUV switch and this final-pixel
change together. The full report changes exactly YUV_init 68.67785 -> 72.979866
and ExpandPlane 82.80257 -> 83.05476. Every other function record and all
per-unit/global exact-code, data and completion measures are unchanged.
The all-source build and normal retail link pass, retaining SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. Both TUs remain NonMatching and
completion remains 469/543; no whole-TU source-link closure is claimed.
Evidence: build/expand59-last-private and parallel-fiftyninth-report.json
with its validation log. No compiler changes, assembly, or behavioral tests.


## Bink trailing dirty-mask cursor (2026-10-03)

On staging94d28d676, retail YUV_blit_mask .text0x13e0..0x13fc reads
maskp[0], conditionally reads maskp[mask_step], and then increments maskp.
The source incremented after its first read and compensated with mask_step-1,
producing an address add plus a displaced byte load instead of retail's lbzx.
Keep the cursor unchanged through both reads, then increment it before the
existing dirty-block test. Both byte addresses and the final cursor are unchanged.

The sole candidate improves full deduplicated YUV_blit_mask82.76312 ->83.02945
(raw81.300896 ->81.51856), reducing source size3088 ->3084 versus retail3124.
Exactly one function record changes globally; other function scores, data,
exact counts, and source-linked totals are unchanged. all_source and normal
link pass; retail DOL SHA1 remains306526d90b48e99894c3138f5fc8f2716d9fecf6.
YUV remains NonMatching; this is not a whole-unit source-link claim. Compiler
hashes are unchanged; no new assembly, compiler edits, or ancillary tests.

Evidence in the RGB worktree: build/masked-tail-baseline-report.json,
masked-tail-retained-report.json, masked-tail-retained-raw.json,
masked-tail-validation.json, and baseline/retained build and link logs.


### Integrated masked-tail and run-copy lifetime verification (2026-10-03)

Initialize normal RUN's destination row cursors in the copy phase, after
decoding the scratch block. They need not remain live across bitstream and
run decoding. The copy body, eight rows, alignment branch and all stores are
unchanged. This restores retail's initial double-copy sequence, including
two indexed stores and alternating f0/f13 values. Raw ExpandPlane improves
82.30088 -> 82.6883 and source size falls 5,664 -> 5,660 bytes. The raw-copy
branch was separately audited: its cursor reloads already agree with retail,
so no source snapshot or other raw-copy change was justified.

Root independently integrates the masked-tail cursor fix. Full deduplicated
verification changes exactly ExpandPlane 83.05476 -> 83.47262 and
YUV_blit_mask 82.76312 -> 83.02945. Every other function record and all per-unit
and global exact-code/data/completion measures are unchanged. All-source
compilation and normal retail linkage pass with SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. Both units remain NonMatching and
completion remains 469/543; no source-link completion is claimed. Evidence:
build/expand60-run-copy-private, parallel-sixtieth-report.json and its
validation log. The YUV_init format-owned green_shift trial regressed and was
restored; the published ordered-switch checkpoint remains intact.


## Bink bottom fallback inclusive endpoint (2026-10-03)

Retail YUV_blit_mask .text0x16a4 computes the original last row, old_srch-1,
before both mult64anddiv calls. At0x16d4/0x16dc it derives the remaining height
as last_row-full_h+1. Preserve that phase boundary with an ordinary unsigned
last_row local inside the existing bottom-fallback guard. The two conversions,
guards, and fallback arguments otherwise stay unchanged. Unsigned subtraction
and addition preserve the prior height modulo32bits, including zero values.

On the verified trailing-mask checkpoint c548c81b8, the sole candidate improves
full deduplicated YUV_blit_mask83.02945 ->83.979515 (raw81.51856 ->82.51729).
Source size3084 ->3088 versus retail3124. It recovers the endpoint subtraction
before conversion and subtract/add pair afterward, using retail's r30/r29
endpoint/full-height roles. Exactly one function record changes globally; data,
exact counts, source-linked totals, and other function scores are unchanged.
all_source and normal link pass, with retail DOL SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6 and unchanged compiler hashes.
YUV remains NonMatching. No compiler edits, new assembly, or ancillary tests.

Evidence: RGB worktree build/masked-endpoint-{report,raw,validation}.json,
masked-endpoint-build.log and masked-endpoint-link.log; comparison baseline is
build/masked-tail-retained-report.json.


## Integrated Bink motion marking and fallback endpoint (2026-10-03)

Mark the motion block dirty before reading its X/Y motion bytes. Retail does
this store at 0x2c2c, before either byte load; the previous source placed the
mark after both reads. The independent work mask and motion bundles retain
the same final values and advancement, and the copy body is unchanged.
ExpandPlane improves 83.47262% to 83.5355% in the full deduplicated report
(raw 82.6883% to 82.78161%, unchanged 5660-byte source function).

Integrated with the inclusive bottom-fallback endpoint, YUV_blit_mask improves
83.02945% to 83.979515%. These are the only two changed function records;
all other functions, data measures, exact totals and completion totals are
unchanged. Completion remains 469/543. All-source compilation and the normal
retail link/check pass, with DOL SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. Both Bink units remain NonMatching;
this normal retail hash is not source-selected closure evidence.

The separately isolated xAnim EffectSingleLoop count-first snapshot trial
regressed 99.781815% to 99.145454% and was discarded. It retained the wrong
load order and changed register roles; no xAnim source change is included.
No new inline assembly, compiler patch or ancillary behavioral tests.
Evidence: build/expand61-motion-mark-private and
build/parallel-sixtyfirst-{report.json,validation.log}.

Before publication, staging independently gained 7fa246016 (EffectSingleLoop).
Rebased these Bink commits onto it and repeated the combined build/report.
That independent pointer-walk change matches the neighboring
EffectSingleDuration idiom and makes the 220-byte function exact. The rebased
report adds only that function match relative to the integrated report above;
Bink scores, data and 469/543 completion are unchanged. Retail DOL hash passes.
Evidence: build/parallel-sixtyfirst-rebased-{report.json,validation.log}.


## Bink scaled raw row cursor ownership (2026-10-03)

Capture colors.cur_ptr once at each scaled raw row entry and advance that
same source_row after writing the doubled output rows. Retail 0x33a4..0x33ac
loads the row cursor and forms its next value before the output stores;
0x3430 commits that saved next value. The previous source reloaded the bundle
cursor after the stores. The source row and output buffers are independent;
the four halfword reads, sixteen output pixels per doubled row, eight-row
loop and eight-byte input advance are unchanged.

ExpandPlane improves 83.5355% to 83.9236% in the full deduplicated report
(raw 82.78161% to 83.13928%). Source code shrinks 5660 to 5656 bytes by
removing the extra cursor reload. Only this function record changes; all
other functions, data, exact measures and completion totals remain unchanged.
Completion is still 469/543. All-source build and normal retail link/check
pass with SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6. Expand remains
NonMatching; this hash does not prove a source-selected Bink DOL match.
Evidence: build/expand62-raw-row-private and
build/parallel-sixtysecond-{report.json,validation.log}.

Parallel near-TU trials retained no changes: xAnim's allocation if/else was
instruction-identical, its positive-pattern pointer capture regressed, and
bamatlst's explicit serialized-index cursor regressed. Masked-blitter local
chroma initialization and outer-row base captures also regressed. No new
assembly, compiler patch or ancillary behavioral tests were introduced.


## Bink scaled-pattern mask lifetimes (2026-10-03)

The scaled-pattern loop now preloads the next pair of raw mask-table entries
before the current output stores, then combines that pair with the colors
when it becomes the current output word. Retail's next mask loads precede
current stores (0x3044/0x304c before 0x3058 and 0x306c/0x3070 before 0x3074),
but its next packed word is formed later. The earlier two-word pipeline
computed that next packed result too early. Reuse one packed word and a pair
of mask values, preserving all eight stores, four mask pairs, input-byte
consumption and row advances. Raw scaling also captures each halfword into
its word variable before expansion, making the source-sample lifetime explicit.

The sample capture alone raises raw ExpandPlane from 83.13928% to 83.17985%;
the combined mask pipeline reaches 84.643005%. The authoritative deduplicated
report improves 83.9236% to 85.4476%. Source size drops from 5656 to 5644 bytes.
Only ExpandPlane's function record changes; all other functions, data, exact
measures and 469/543 completed units are unchanged. All-source build and
normal retail link/check pass with SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. Expand remains NonMatching, so the
normal retail hash does not establish a source-selected Bink match. No new
assembly, compiler modifications or ancillary behavioral tests.
Evidence: build/expand63-{scale-samples,pattern-masks}-private and
build/parallel-sixtythird-{report.json,validation.log}.


## Bink residue and inter dirty-mark ordering (2026-10-03)

Like the previously corrected motion-only case, retail marks residue and
inter blocks dirty before loading the signed X/Y offsets (0x2584 and 0x2734).
Move each mark ahead of those loads while keeping bundle advances, motion
copy bodies and decoder calls unchanged. Work-mask storage and offset bundles
are independent; each block receives the same mark and consumes the same
motion values. The two paths share substantial code, so evaluate the combined
form: residue alone and inter alone score slightly lower, but together raw
ExpandPlane improves 84.643005% to 84.98242%, with unchanged 5644-byte size.

The full deduplicated report improves 85.4476% to 85.78702%. All other function
records, data, exact measures and completion counts are unchanged (469/543).
All-source build and normal retail link/check pass; DOL SHA1 remains
306526d90b48e99894c3138f5fc8f2716d9fecf6. Expand remains NonMatching; no
source-selected closure is claimed. A separate explicit scratch-destination
pointer trial regressed to 84.54361% raw and was discarded. No compiler
modifications, new assembly or ancillary tests.
Evidence: build/expand64-motion-marks-private,
build/expand64-scratch-destination-private, and
build/parallel-sixtyfourth-{report.json,validation.log}.


## Bink run counter and Goo color capture (2026-10-03)

Initialize the normal RUN filled-pixel count after reading the scan-pattern
bits. Retail initializes the counter at 0x22c0 after the pattern lookup;
its lifetime need not overlap that bit read. This raises raw ExpandPlane
84.98242% to 85.17377% and full deduplicated score 85.78702% to 85.97836%.
The equivalent scaled-path change alone and combined were weaker; retain
only the normal-path lifetime change. Source size remains 5644 bytes.

Before other integration, the full report changes only ExpandPlane; all
other function records, data and exact/completion totals are unchanged.
All-source build and normal retail link/check pass, DOL SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. Expand remains NonMatching.
Evidence: build/expand65-run-count-private and
build/parallel-sixtyfifth-{report.json,validation.log}.

The independently verified zFXGooRenderAtomic color-capture change is
integrated separately: immutable promoted green/blue values and direct red
output recover more retail channel roles in all three vertex blocks.
Worker deduplicated score improves 99.182076% to 99.35014%, with all other
functions and all 26696 zFX data bytes unchanged. Goo and skinning residuals
still prevent zFX closure. Its scoped skin-mask trial regressed and was
restored. No new assembly, compiler modifications or ancillary tests.


### zHud: recover source-link function groups; conversion residue remains (2026-10-03)

The two code holdouts are `zhud::setup` (556 bytes, 97.1223%) and
`zhud::update` (756 bytes, 97.8836%). Their four unsigned-counter conversions
have identical operations to retail, but load the `0x4330000000000000` bias
before the saved-counter store/reload pair; retail loads it after that pair.
DWARF confirms the `old_value` and `old_max_value` arrays are unsigned ints.
Combining assignment and conversion into `(F32)(old_value[i] = *value[i])`
removed the required reloads and regressed both functions (95.647484% and
96.79894%); this trial was restored. This evidence does not establish that a
compiler patch is necessary.

There was also an independent source-link layout mismatch. TU-local `-sym on`
keeps primary functions in source order instead of pulling `show`/`hide`
forward and interleaving weak functions. Actual source-selected DOL differences
fall from 1,232 to 85 bytes with code/data scores unchanged. Retail then places
`meter_widget::changing` in its own final weak section, after the font-meter
`get_asset` section. The opt-in `XHUDMETER_DEFER_CHANGING` declaration and
`zHudMeter.inl` preserve that ownership only for zHud; every other consumer
keeps the original header body. This reduces the actual DOL difference to
**57 bytes**, SHA-1 `e10ffdd59ac1d4c79fcf81040696f73d8c16f3b5`.

The remaining bytes are 19 in setup, 19 in update (the four bias-load moves),
and 19 in the literal pool at `0x8025cd01..0x8025cd14`. The older note describing
zHud's target-only `64006875` as a constant is misleading: those bytes straddle
strings. Retail has `"hud\0hud:meter\0hud:model\0"`; source lacks the first
`"hud\0"`. `xhud::asset::type_name()` supplies this real string elsewhere,
but no surviving zHud debug/source use establishes its original stripped
owner. No synthetic reference or padding was added. The TU stays NonMatching.

Validation: force-rebuilt all five dependency-recorded xHudMeter consumers
(`xHudMeter`, `xHudFontMeter`, `xHudUnitMeter`, `xHud`, `zHud`), then completed
`all_source`, full deduplicated report equality, and normal retail DOL SHA-1
`306526d90b48e99894c3138f5fc8f2716d9fecf6`. The source-selected link reproduced
57 differing bytes after that rebuild. Compiler p1a/p1e hashes remain
`a78a5fdb6c1d5677e987636b2e0743dbaefe9542` and
`9d445725489050035740aaff35860eddbaf3c3c9`.

Local artifacts in the Plankton worktree: `build/zhud-closure-ownership-link.py`
(temporary Matching audit, restores normal configuration in `finally`),
`zhud-closure-ownership-link.json`, `zhud-closure-ownership-linked.elf`,
`zhud-closure-final-report.json`, `zhud-closure-fresh-build.log`, and
`zhud-closure-affected-{sources,objects}.txt`. No behavioral tests were run.

Root independently reproduced the integrated HUD checkpoint after rebuilding
all five affected translation units. The full report is exactly equal to the
combined Bink/Goo report; source-selected zHud DOL is retail size 2859136 with
57 differing bytes and SHA1 e10ffdd59ac1d4c79fcf81040696f73d8c16f3b5.
Temporary source selection was restored and the normal retail SHA1 passes.
Evidence: build/parallel-sixtyfifth-{combined,hud}-report.json,
build/parallel-sixtyfifth-hud-validation.log, and build/zhud65-source-link.json.


## Spline tridiagonal solver loop ownership (2026-10-03)

Tridiag_Solve was semantically reconstructed but scored zero: the source
function was 940 bytes against retail's 572, with loop unrolling absent in
retail. Name the forward predecessor index, then traverse backward from the
already selected last element using while (j-- > 0), naming the successor
index inside that loop. The same coefficients, three component expressions,
allocation/free calls and output order are preserved. For positive n the
backward body still visits n-2 through zero, finishing with j=-1.

The named forward index removes its unwanted unrolling. Reusing the last
index for the backward countdown removes that loop's unrolling as well.
Keeping both neighbor indices in the final form recovers more of retail's
explicit address arithmetic. No optimization pragma, compiler change,
assembly or ancillary behavioral test was added.

The full deduplicated Tridiag_Solve score improves 0% to 84.05595%, source
size 940 to 556 bytes. xSpline improves 89.62188% to 97.738014%. Only this
function's report record changes; all other functions, data, exact measures
and 469/543 completion totals are unchanged. Full source build and normal
retail DOL check pass, SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6.
xSpline remains NonMatching; this does not claim a source-selected closure.

An initial integration check correctly found no gain because copying an
older trial file left the restored baseline object newer than its source.
The retained source timestamp was refreshed and compilation repeated before
both final report and validation. Evidence: build/spline67-{baseline.json,
indices,previous-only,backward-count,count-next} and
build/parallel-sixtyseventh-final-{report.json,validation.log}.

Parallel bacamera and p2define audits retained no change. The conversion-bias
review found distinct scheduling and value-numbering effects in zHud and
Show_frame, with historical broad-rule tradeoffs; it did not establish a
safe common compiler patch. The Bink sound/Huffman/bitplane audits found
prior controls already covering the observed residuals, so no repeated
source permutations were retained.


## HUD unit-meter indexed initialization (2026-10-03)

`unit_meter_widget::unit_meter_widget` advances its inner row index in the
assignment (`model[j++][i]`) rather than in the `for` increment. This keeps the
same twelve model loads and `[6][2]` indexing, while recovering the retail
indexed store and inner byte-offset induction. Retail advances the row/count
before the store at target `0xd0..0xe4`; the previous source folded both loops
into a walking output pointer. This is a source evaluation-boundary change,
not the previously exhausted pointer/reference spelling sweep.

Authoritative deduplicated constructor score: **90.246376 -> 93.21739**;
source size **268 -> 272 bytes**, retail **276**. Unit score improves
98.44213 -> 98.916664. A second, combined model-info cursor trial scored
92.695656 and was rejected. The remaining four-byte gap concerns the outer
output offset/base calculation; the unit remains `NonMatching`.

`all_source` and normal link pass. Full report changes only this constructor;
all other function scores, data, exact counts and linked-unit counts are
unchanged. Normal DOL SHA1 remains
`306526d90b48e99894c3138f5fc8f2716d9fecf6`; isolated p1a/p1e compiler hashes
remain `a78a5fdb6c1d5677e987636b2e0743dbaefe9542` and
`9d445725489050035740aaff35860eddbaf3c3c9`. This is not a source-selected
whole-unit link claim. Worker evidence: `build/unitmeter-audit.json`,
`unitmeter-postincrement.json`, `unitmeter-model-cursor.json`,
`unitmeter-retained-report.json`, and `unitmeter-retained-validation.json`.



### xCamera: recover the missing implicit xBound copy helper (2026-10-03)

`__as__6xBoundFRC6xBound` was genuinely absent from the source object, not a
symbol-pairing artifact. An ordinary `bound = source` emits its exact 224-byte
weak compiler-generated definition using the existing xBound type: eight
quick-cull words, type byte, `__copy` for the three-byte pad, nine union words,
and the matrix pointer. The old commented manual float-member copy was not
this operation and has been removed.

The explicit `__deadstripped_xCamera_bound` reconstruction supplies that copy
reference between `xCameraSetScene` and `xCameraSetTargetMatrix`, matching the
retail helper's emission boundary. Its original caller is unknown; this is a
small documented C++ emission compromise, not a claim to recover that source.
The reconstruction is absent from the linked ELF. No shared header or copy
implementation was changed.

Retail xBinaryCamera::update does not call this helper, nor does any surviving
function in the retail xCamera unit. Its callers survive in other units:
`zThrown_Update`, `NPCC_bnd_ofBase`, and `zCameraFlyRestoreBackup`. Consequently,
the original source-selected xCamera link failed with the missing helper.
With the reconstruction it links successfully, although 16,841 DOL bytes still
differ (SHA-1 `af8ad819b24cd43653382b969ee0a17618b29842`); this does not complete
the TU. xCamera stays NonMatching and xBinaryCamera::update remains a separate
1420-byte, 92.087% deduplicated holdout.

Full deduplicated validation changes only this helper from 0 to100%, adding224
matched code bytes and one exact function. The unit moves75/77 to76/77 exact,
97.59949 to99.19811 fuzzy, with all688data bytes still exact. `all_source` and
the normal build pass; normal DOL SHA-1 remains
`306526d90b48e99894c3138f5fc8f2716d9fecf6`. No behavioral tests or compiler edits.

Private evidence: `build/xcamera-closure-bound-emission.json` (initial exact
copy body), `xcamera-closure-bound-report.json`, baseline-link.log (undefined
references), bound-emission-link.json and bound-emission-linked.elf, and
`xcamera-closure-bound-final-verify.py` in the Plankton worktree.


### xCamera private inline groups: source-link preparation (2026-10-03)

After the exact xBound copy checkpoint, the source-selected xCamera DOL linked
but differed in16,841bytes. Retail's seven text groups distinguish primary
functions from camera accessors, matrix helpers, entity visibility, vector
helpers, math helpers, and the vector2 assignment. Many corresponding source
bodies were still strong definitions in xCamera.cpp.

The retained change moves those existing bodies into six TU-private inline
files and enables `-sym on` only for xCamera, restoring retail weak bindings
and section ownership. The existing `XMATH3_DEFER_RMULVEC` opt-in preserves the
local matrix multiply in its matrix group. A narrow new
`XVEC3INLINES_DEFER_LENGTH` opt-in lets xCamera supply the identical vector-length
body in its private vector group; all other consumers retain the unchanged
shared-header body. Including the camera/matrix groups at the former
xQuatGetAngle definition boundary also restores that helper's literal order
before the binary-camera literals. No arithmetic, public layout, padding, or
synthetic constant owner was introduced.

Actual source-selected DOL differences fall **16,841 ->13,363 ->13,210 ->13,191**
(private groups, vector-length ownership, then known literal-owner ordering).
The final DOL has2,859,136bytes and SHA-1
`ee9f9ff56e2a78ebb3a786fabc732af3d03489a5`. This is a partial layout recovery,
not TU completion: xCamera remains NonMatching,76/77functions exact, and the
1420-byte xBinaryCamera::update still scores92.087% deduplicated.

The principal remaining code-layout error is exact and isolated: the implicit
36-byte xQuat assignment is generated in the primary section at its first use
inside _xCameraUpdate, whereas retail places it between xVec3Inv and xacos in
the vector group. It shifts subsequent primary functions36bytes. Marking the
existing static _xCameraUpdate definition inline did not recover that owner
and was restored. No hand-written replacement copy was added. Remaining
external relocations/data layout still prevent an identical source link.

A useful negative control: omitting xCamera's unused weak xsqrt definition via
the existing deferral mechanism removed3.0f/100000.0f from its *object* pool but
changed no DOL byte. The linker already strips those duplicate-body literals.
That neutral control was restored; extra object literals alone were not a
source-link blocker.

Validation force-rebuilt all186dependency-recorded xVec3Inlines.h consumers,
then completed all_source and normal builds. Full deduplicated report equals
the preceding exact-bound-helper checkpoint: no code/data regression and
all688reported camera data bytes remain exact. Normal DOL SHA-1 stays
`306526d90b48e99894c3138f5fc8f2716d9fecf6`; p1a/p1e compiler SHA-1 values remain
`a78a5fdb6c1d5677e987636b2e0743dbaefe9542` and
`9d445725489050035740aaff35860eddbaf3c3c9`. No behavioral tests or compiler edits.

Private reproduction/evidence: `build/xcamera-layout-pool-order-link.py`
(temporary Matching selection with normal restoration in finally),
`xcamera-layout-pool-order-link.json`, `xcamera-layout-pool-order-linked.elf`,
`xcamera-layout-pool-order-report.json`, `xcamera-layout-fresh-build.log`,
`xcamera-layout-final-build.log`, and
`xcamera-layout-affected-{sources,objects}.txt` in the Plankton worktree.

### xCamera globals declaration ownership (2026-10-03)

The retail xCamera object has no definition of `globals`, but the source
contained a redundant `zGlobals globals;` definition despite including
`zGlobals.h`, which already declares it extern. Both source xCamera.o and
zMain.o consequently emitted an 8136-byte, 8-byte-aligned SHN_COMMON object.
Removing only the xCamera definition leaves the existing zMain definition
and the header declaration intact. xCamera.o now references globals as
undefined, matching its actual ownership; no structure or behavior changes.

This single control reduces actual source-selected DOL differences from
**13,191 to 6,617 bytes**. The resulting 2,859,136-byte DOL SHA-1 is
`276e6d7a6503ba086d2fc6c1e4ebc1d8b5fa4a42`. The full deduplicated report is
identical to the preceding layout checkpoint, all_source builds, and the
restored normal link retains retail SHA-1
`306526d90b48e99894c3138f5fc8f2716d9fecf6`. xCamera remains NonMatching;
the implicit quaternion copy placement and binary-camera update holdout
are still unresolved. No additional source variants were attempted.

Private reproduction/evidence: `build/xcamera-layout-globals-link.py`,
`xcamera-layout-globals-link.json`, `xcamera-layout-globals-linked.elf`,
and `xcamera-layout-globals-report.json` in the Plankton worktree.

### xCamera implicit quaternion copy ownership (2026-10-03)

After removing the duplicate globals definition, all primary functions after
_xCameraUpdate and the first helper groups remained displaced by 36 bytes.
The compiler-generated weak xQuat assignment was emitted after its first
source use in _xCameraUpdate instead of retail's vector-header group between
xVec3Inv and xacos. This single ownership error accounted for most of the
remaining 6,617 source-linked DOL differences.

A TU-private inline stripped-reference reconstruction now performs the
ordinary `dest = source` operation between those two vector helpers. Moving
the existing camera, matrix, entity, and vector private includes before
_xCameraUpdate establishes this first-use owner while preserving retail's
helper group order. The original stripped caller is unknown: this is an
explicit matching compromise, not a claim of recovered original source.
The reconstruction has no emitted caller in either the object or linked ELF;
the compiler-generated assignment retains its exact 36-byte body and weak
binding. No manual copy implementation, padding, or literal owner was added.

The actual source-selected DOL now differs in **205 bytes**, down from
**6,617**. All 205 lie inside the 1,420-byte xBinaryCamera::update function;
there are no remaining differences elsewhere in the DOL. Thus the measured
helper, global, and literal link-layout blockers are resolved, while the
existing 92.087% deduplicated update holdout remains. xCamera stays NonMatching.
The source-selected DOL SHA-1 is
`ea4e9276c185b32cc9fcf1e3bf00a223dd031541`.

Full deduplicated report equals the preceding checkpoint (76/77 exact camera
functions, 688/688 camera data bytes exact), all_source and normal builds pass,
and restored normal SHA-1 remains
`306526d90b48e99894c3138f5fc8f2716d9fecf6`. Only TU-private files changed;
there are no new shared-header consumers to rebuild. Private artifacts:
`build/xcamera-round70-quat-owner-link.py`, corresponding link.json,
linked.elf, report.json, and residue.json. No behavioral tests or compiler edits.


## zEntPlayer external vector-helper ownership (2026-10-03)

The retail player object declares xVec3::create(F32,F32,F32) externally and
contains none of the three 12-byte aggregate initializer templates emitted
by xVec3.h's two create overloads and cross. The source object emitted all
three zero templates plus an extra weak create body. Only the third template
had a reference, from that extra body; the other two were unused.

XVEC3_DEFER_AGGREGATE_HELPERS preserves declarations for these methods while
leaving their existing inline definitions unchanged for default consumers.
zEntPlayer opts in around its includes. No retail function body, arithmetic,
or class layout changes.

The authoritative full report changes only player .rodata from 99.91898% to
100%. Crediting that 14,816-byte section takes player matched data from
13,004/27,820 to 27,820/27,820 (100%), and project matched data from 1,264,984
to 1,279,800 (99.93097%). Every function record and all exact-code/completion
counts remain unchanged; completion is 469/543. Raw relocation comparisons
improve at five call sites, but the authoritative report already normalizes
those differences; they are not additional function gains.

The raw source .rodata is now 14,812 bytes versus retail's 14,816, with trailing
alignment padding excluded from the report comparison. All 183 dependency-
recorded xVec3.h consumers were explicitly rebuilt, followed by all_source
and the normal retail link/hash check. Before/after player source-selected
DOLs are identical: 2,859,200 bytes, SHA1
f7c68a5c8dbd50bc76bead3b88d8c7344b1c8888. The linker already stripped the
removed definitions/templates. Independent player layout/function differences
remain; this is object-data matching, not TU closure. Normal DOL SHA1 remains
306526d90b48e99894c3138f5fc8f2716d9fecf6.

The integrated camera ownership checkpoint independently reproduces 205
source-DOL byte differences, all within xBinaryCamera::update and none
elsewhere, SHA1 ea4e9276c185b32cc9fcf1e3bf00a223dd031541. Both units remain
NonMatching. Evidence: build/player70-{baseline.json,aggregate-visibility,
affected-sources.txt,baseline-source-link.json,candidate-source-link.json};
build/parallel-seventieth-{final-report.json,final-build.log,verify.py};
build/xcamera70-{source-link.json,source-linked.dol}.


### xCamera completed: immutable interpolated position (2026-10-03)

With link layout resolved, all 205 remaining source-selected DOL differences
were inside xBinaryCamera::update. Retail snapshots A's three coordinates and
computes all three interpolated outputs before their stores. Source instead
zero-initialized cam_loc and assigned its components separately, extending
different floating-point and general-register lifetimes. Earlier standalone
scalar snapshots were worse and a mutable runtime aggregate was neutral.

The successful reconstruction initializes `const xVec3 cam_loc` directly from
the three existing xlerp expressions. This local is never modified afterward
and is consumed by xCameraMove through a const reference. Making its complete
value immutable recovers the aggregate construction boundary, including all
register allocation and interleaving with heading initialization. Arithmetic
and consumers remain unchanged; no helper, compiler, or shared-header change
is required. The DWARF dump preserves cam_loc's name/order but is not evidence
that this exact qualifier spelling appeared in the original source.

Full deduplicated scoring changes only xBinaryCamera::update,
**92.087% to 100%** at 1,420 bytes. xCamera is now **77/77 exact functions**,
**14,012/14,012 code bytes** and **688/688 data bytes** exact. Raw objdiff's
99.95775% is solely three literal-symbol pairings; full dedup resolves them,
and actual source-selected linking independently proves there is no residual.
The resulting DOL is byte-identical to retail, SHA-1
`306526d90b48e99894c3138f5fc8f2716d9fecf6`, with zero differing bytes.
The unit is therefore marked Matching while retaining its verified -sym on
configuration and private emission ownership reconstruction.

Final all_source and source-selected normal builds pass; full report has no
code/data regression. Compiler binaries are unchanged. Private evidence:
`build/xcamera-round71-immutable.json`, `xcamera-round71-immutable-link.py`,
corresponding link.json, linked.elf, report.json, and final-report.json.
No behavioral tests, assembly, or compiler edits were used.

### Player COMMON ownership: restore the debug buffer declaration (2026-10-03)

A source-selected player link inflated gust_data from its genuine 36-byte
COMMON definition to 1,036 bytes and still allocated every following COMMON
object. Scanning all root object inputs found no other gust_data definition.
The inflated size is exactly the sum of the eleven player COMMON objects,
matching the documented first-COMMON linker bug in `docs/common_bss.md`.
Retail's split object instead has a leading stripped
__unknown_common_bss_symbol before gust_data.

The missing source owner is evidenced directly: dwarf/SB/Game/zEntPlayer.cpp
lines 6-7 lists global `char buffer[16]` before the player globals. Exact linked
zUI and zVolume preserve the same leading declaration from their debug data.
Restoring it in zEntPlayer makes that unused COMMON absorb the linker grouping
behavior and then disappear from the linked ELF, preserving the real objects'
sizes. This is a recovered declaration, not an invented padding object; no
new reference or fabricated size is used.

On published c6c145768, the isolated before/after source-selected links show:

- gust_data: **1,036 -> 36 bytes**, at 0x803bf640 in both links.
- gPlayerAbsMat: 0x803bfa4c -> 0x803bf664; later COMMON objects likewise
  lose the erroneous 1,000-byte displacement.
- BSS size: **1,123,056 -> 1,122,056 bytes**. The linked global buffer is absent.
- DOL differences: **1,935,330 -> 1,934,872 bytes**; size remains 2,859,200
  versus retail 2,859,136 because the separate player code/layout residue
  still shifts the image. After this fix, player COMMON addresses retain only
  that uniform +0x40 shift.
- Source-selected DOL SHA-1:
  `1ea86c30b428dd4b6e0d01d92e1fc6d951550063`.

Full deduplicated reports are byte-for-byte identical before and after. All
source files were freshly built after syncing the isolated branch; candidate
all_source and restored normal builds pass. Normal retail SHA-1 remains
`306526d90b48e99894c3138f5fc8f2716d9fecf6`; p1a/p1e compiler hashes are unchanged.
Player stays NonMatching. Private reproduction: `build/player72-common-link.py`
with `before`/`after` arguments, corresponding reports/link inventories and
linked ELF/DOL files; `player71-gust-ownership-findings.md` records diagnosis.

### Player animation-table definition ownership (2026-10-03)

Retail groups six exact animation-table builders together in a separate
56,908-byte text section: Sandy, Patrick, player, tongue, boulder vehicle,
and TreeDome. The source had scattered those strong definitions throughout
zEntPlayer.cpp's primary section. The debug data confirms the missing owner
by name: `dwarf/dwarf_data.cpp` attributes the builders to
`C:\SB\Game\zEntPlayerAnimationTables.h`. This is header definition
ownership, not a reason to make these public functions weak or inline.

The unchanged bodies now live together in TU-private
zEntPlayerAnimationTables.inl, included at the former first-builder boundary.
The existing shared zEntPlayerAnimationTables.h remains unchanged and keeps
its public declarations for other consumers. Two existing static callback
forward declarations are needed before the include. All six definitions
retain their strong binding, exact body, and retail relative offsets.

Actual source-selected linking improves from **1,934,872 to 1,904,631 differing
DOL bytes**. The six builder addresses now share a uniform +760-byte
displacement from retail, versus Sandy -89,676, Patrick/player/tongue/boulder
-87,848, and TreeDome -2,380 previously. Source DOL size remains 2,859,200
(retail 2,859,136); other primary/helper ordering and code residues remain.
The candidate source-selected SHA-1 is
`2fb2c0b4d9ec18f9c9059224d5c06681e9ac3250`. This is partial layout recovery;
zEntPlayer remains NonMatching.

The full deduplicated report is byte-for-byte equal to the preceding COMMON
checkpoint, including all 27,820 reported player data bytes exact. all_source
and restored normal builds pass; normal DOL SHA-1 remains
`306526d90b48e99894c3138f5fc8f2716d9fecf6`. The shared declaration header was
restored before validation, and its two other direct consumers rebuilt.
No APIs, shared headers, compiler binaries, or assembly changed. Private
evidence: `build/player73-function-ownership.json` and
`player72-common-anim-header-{report.json,link.json,linked.elf,linked.dol}`;
`player72-common-link.py anim-header` reproduces the link with normal restore.


### Bink IDCT even-input lifetimes (2026-10-03)

Retail's final transform pass loads the four even workspace inputs before
writing the row scratch array. The reconstruction read those inputs again
after scratch stores. Capture inputs 2, 6, 0, and 4 once, then use those
values in the same sum, difference, multiply, and shift expressions. This
removes four redundant input loads in each variant without changing the
integer arithmetic tree or output order. Both arrays are private local
storage, so the snapshots retain the same values.

The full deduplicated report improves only these three functions:
- fastidct8x8: 63.765182 -> 63.97166 (source 948 -> 932 bytes; retail 988).
- fastidct8x8d: 71.951416 -> 73.19433 (1000 -> 984; retail 988).
- FastmIDCT8x8WithMotion: 58.347015 -> 58.41418 (1028 -> 1012; retail 1072).

All other function records, data credit, and completion counts are unchanged.
all_source and the normal retail link pass; SHA1 remains
306526d90b48e99894c3138f5fc8f2716d9fecf6. DCT remains NonMatching. No behavioral
tests, assembly, compiler changes, or dummy storage were introduced. Earlier
first-pass scalar-output staging was neutral; an eight-argument inline
column helper regressed and was discarded. Private evidence:
build/dct73-{baseline,even-inputs,even-inputs-fastidct8x8d,
even-inputs-FastmIDCT8x8WithMotion}.json and
build/parallel-seventythird-{final-report.json,final-build.log,verify.py}.



### Bungee ownership checkpoint ported onto current staging

Port of the previously isolated `7c71df654` preparation onto `a0d34bb80`.
The unchanged private attaching/hanging methods and collision functor return
to their deferred class groups; `drop_asset` returns to its implementation
scope (there are no external users). The real down-vector aggregate and
matrix/vector helper ownership reproduce the earlier method, vtable and
initializer layout. Public hook types, entry points and object layouts stay
unchanged. This is layout preparation, **not a Matching conversion**.

Unlike the original checkpoint, `xBoxFromSphere`'s header body and the ordinary
`xGridCheckBound` template spelling are Bungee-only opt-ins. Other callers keep
their previous definitions. The newer `XMATH3_DEFER_RMULVEC`,
`XVEC3_DEFER_AGGREGATE_HELPERS` and `XGRID_DEFER_BOUND_HELPERS` paths remain
intact. The `XVEC3_MATMUL_INLINE` matrix definition/body must stay identical
to their ordinary xMath3.h counterparts.

Forced all 224 game sources to rebuild, covering every transitive consumer:
xVec3.h 188, xMath3.h 187, xGrid.h 156, Bungee's header 6. The complete
deduplicated report is JSON-identical to the current baseline: Bungee remains
116/120 paired exact functions and 6,528/6,528 data bytes; no caller regresses.
The normal source-selected Matching units still produce retail SHA1
`306526d90b48e99894c3138f5fc8f2716d9fecf6`.

Temporarily source-selecting Bungee produces the same 2,859,136-byte image
with exactly **267 differing bytes**, SHA1
`b08482d6b8930465c578eb8b5decd69e2b747a99`, identical to the original checkpoint.
Thus the remaining barrier is still the genuine 0.5f/0.1f constant-pool
allocation order; no new method, helper or data-layout discrepancy appeared.
No literal padding, compiler changes or Matching marker were retained.
Compiler SHA1s remain `a78a5fdb6c1d5677e987636b2e0743dbaefe9542` (2.0p1a)
and `9d445725489050035740aaff35860eddbaf3c3c9` (2.0p1e).

Private evidence: `build/bungee-port-{baseline,final}-report.json`,
`bungee-port-affected-consumers.json`, `bungee-port-final-build.log`,
`bungee-port-source-link-input.txt`, `bungee-port-source-diff.json`, and the
saved diagnostic DOL/ELF; the diagnostic script restores the normal link.


### Bink IDCT even-output store order (2026-10-03)

After capturing the four even inputs, store the even sum in row[0] before
the difference in row[3], as retail does. Keep the separate input difference
calculation after those stores. This changes only independent local scratch
assignments; arithmetic expressions and final destination writes are unchanged.
The input-snapshot checkpoint makes this previously obscured scheduling
boundary measurable in all three final-pass variants.

Full deduplicated gains: fastidct8x8 63.97166 -> 64.052635,
fastidct8x8d 73.19433 -> 73.51417, and FastmIDCT8x8WithMotion
58.41418 -> 58.61194. Source sizes stay 932/984/1012 bytes, respectively.
Every other function record, data credit, and completion count is unchanged.
all_source and normal retail link pass, with SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. No compiler or assembly change;
DCT remains NonMatching. Private evidence: build/dct74-even-store-*.json
and build/parallel-seventyfourth-{final-report.json,final-build.log,verify.py}.


### Bink doubled-IDCT output cursor lifetime (2026-10-03)

Initialize d0 from dest at the second-pass boundary beside resetting the
workspace cursor. It is unused during the first pass. Retail retains the
original destination across that pass and creates the writing cursor afterward;
the reconstruction initialized the cursor early and lost the corresponding
register move. This source-lifetime correction restores that move and brings
fastidct8x8d from 984 to the retail 988-byte size.

Full deduplicated match improves 73.51417 -> 73.728745. Every other report
record is unchanged apart from aggregate fuzzy percentages. all_source and
the normal retail link pass (SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6).
DCT remains NonMatching. No math, output addressing, compiler flag, or assembly
changed. A separate byte-IDCT odd-input snapshot was byte-neutral and discarded.
Private evidence: build/dct75-{late-output,odd-inputs}.json and
build/parallel-seventyfifth-{final-report.json,final-build.log,verify.py}.


### Bink pattern-byte consumption (2026-10-03)

In expand_pattern_row, retain the low nibble and consume the high nibble by
shifting row_bits in place. This removes an unnecessary high_bits local and
recovers retail's low-mask-before-high-mask load ordering in the normal
pattern rows. All lookup indices, packed colors, stores and bundle advances
are unchanged. The compiler still splits the low mask/scale into two
instructions rather than retail's combined rotate-mask; this is not closure.

Full deduplicated ExpandPlane improves 85.97836 -> 86.06153. Raw comparison
improves 85.17377 -> 85.338066, with source size 5644 -> 5676 versus retail5916.
Full report comparison changes only that function and fuzzy aggregates;
all data, exact-match totals and completion fields remain unchanged.
all_source and normal link/check pass; normal DOL SHA1 remains
306526d90b48e99894c3138f5fc8f2716d9fecf6. Expand remains NonMatching.

A raw four-mask snapshot regressed, and masking only at lookup also regressed;
neither is retained. DCT output cursor char/int scratch type controls and a
paired scalar output reconstruction were byte-neutral and discarded. No new
assembly, compiler changes, or behavioral tests. Evidence:
build/expand76-{baseline.json,row-bits-private},
build/parallel-seventysixth-{report.json,verify.py,build.log}.


### Bink lossy writer child-depth acquisition (2026-10-03)

`WriteBPLossy` improves from 96.14938% to 96.50967% in the authoritative
full deduplicated report. Retail's first child read at 0x2398 is followed by
its persistent cursor address at 0x239c before the presence-bit write. The
later children likewise load their depth through the advanced cursor before
finishing the coefficient-index increment. Capture that depth separately,
using the same input/cursor boundary already present in `WriteBPLossless`.
The post-bitstream-write depth reloads remain intact; tree contents, child
coverage, index truncation and bitstream updates are unchanged.

This recovers the first cursor setup boundary and improves later load order,
while some index-increment scheduling remains different. Source size remains
2,260 bytes versus retail's 2,276. The older child-type/caching trials replaced
or reused post-write values; this change only captures the pre-write input.
No compiler flags, compiler binaries, assembly or behavioral suites change.

All other function records, non-text sections and per-unit non-fuzzy measures
are identical to the baseline report. The full `all_source` build and normal
build pass, retaining DOL SHA1 `306526d90b48e99894c3138f5fc8f2716d9fecf6`.
Bitplane remains NonMatching: normal linking does not establish source-linked
retail equivalence for this unit. Evidence in the isolated RGB worktree:
`build/bitplane-lossy-phase-report.json`, `bitplane-lossy-phase-validation.json`,
`bitplane-lossy-phase-build.log` and `bitplane-lossy-phase-link.log`.


## 2026-10-03: dldevice small-BSS declaration order

The source-selected dldevice DOL differs from retail in 371 bytes despite
identical total size. The C compiler emits its small-BSS definitions in
reverse declaration order: for example, _RwDlCopyClear is at 0x803cc91c
instead of retail 0x803cc8a8. The existing per-TU `-sym on` control is neutral
for this C file and was not retained.

Gathering the 25 existing file-scope small-BSS definitions in reverse retail
order reduces the actual source-selected DOL residue to 165 bytes. Types,
qualifiers, bindings, values, and function bodies are unchanged. The seven
objects from _RwDlWaitingDoneRender through dgGGlobals now have exact retail
addresses. The two function-local statics, swap and gxInit, still precede
this group in emitted storage; the 18 earlier globals remain eight bytes
late. Their lexical ownership has not been changed to force placement.
This is partial layout preparation, not a completed translation unit.

The authoritative full report is identical before and after: 14/16 exact
functions, unit 99.73589%, 968/968 data bytes. CameraBeginUpdate's initial
store/setup ordering and RasterShowRaster's three framebuffer/index-load
swaps remain. A genuine shared inline queue-copy setter was also tried; it
inlined completely but left the load swaps unchanged, and was restored.
No compiler defect is established by these source controls.

Both source-linked DOLs are 2,859,136 bytes. Before SHA1 is
`e165898fb0cc3aae3120c80f74d1d6a8b9b85d3a`; after SHA1 is
`ccc0ed7b263193958cc9bc4336c8518ab8ade440`. The unit stays NonMatching.
The ordinary build retains the exact retail SHA1
`306526d90b48e99894c3138f5fc8f2716d9fecf6`. Full source build passes, and
private p1a/p1e compiler hashes remain `a78a5fdb6c1d5677e987636b2e0743dbaefe9542`
and `9d445725489050035740aaff35860eddbaf3c3c9` respectively.
Evidence in the isolated worker build directory: dldevice76-restored-report.json,
dldevice76-reverse-report.json, dldevice76-source-link.json,
dldevice76-reverse-source-link.json, and dldevice76-link.py. The diagnostic
script temporarily selects only this source object, captures the ELF/DOL,
then restores NonMatching and verifies the ordinary retail hash in finally.


## 2026-10-03: dldevice explicit-zero ownership

A reduced C control establishes the remaining definition distinction under
published GC/2.0p1f: explicit file-scope zero definitions emit first, in
source order; tentative definitions emit later, in reverse order. Both
forms remain in .sbss. Function-local statics retain their separate delayed
emission, including a local explicitly initialized to zero.

The eighteen existing globals from _RwDlCopyClear through _RwDlFrameGo now
use explicit zero initializers in retail order. The seven later globals stay
tentative and reverse-declared. No objects, types, qualifiers, bindings, or
nonzero values are added or changed. All 25 file-scope .sbss offsets now
match retail exactly. This replaces the earlier eight-byte displacement
without changing any instruction body or data score.

Actual source-selected DOL differences fall from 165 to 37 bytes, at unchanged
2,859,136-byte length; new SHA1 is
`fdad768bc469c93e318ae28d2fa5b497b4b5645f`. Eight residual bytes refer to the
swapped function statics: swap/gxInit have source offsets 76/72 instead of
retail 72/76. These affect _rwDlBreakPtCallback (3 bytes),
_rwDlVIPostRetraceCallback (3), and _rwDlSystem (2). The original code
residues remain in _rwDlCameraBeginUpdate (17 bytes) and
_rwDlRasterShowRaster (12). This remains a NonMatching layout checkpoint.

Full deduplicated report equality, full source build, and ordinary retail
SHA1 `306526d90b48e99894c3138f5fc8f2716d9fecf6` pass. Private compiler p1f
SHA1 remains `8641f1a15bab7d961b7b7558e8d0de64449c509c`; no compiler bytes
were modified. Evidence: build/dldevice77-zero-control.c/.o,
dldevice77-baseline-report.json, dldevice77-zero-report.json,
dldevice77-zero-source-link.json, dldevice77-zero-residual.json, and the
self-restoring source-link diagnostic dldevice77-zero-link.py.


### Bink lossy writer root/child scratch ownership (2026-10-03)

`WriteBPLossy` improves from 96.50967% to 96.86995% by using its existing
16-bit entry scratch for the pre-write child-depth acquisitions introduced
in the preceding change. The depth still comes from a byte table, and every
post-bitstream-write reload remains separate. This restores all three retail
root-selection branch pairs at object offsets 0x2138/0x2140, 0x2158/0x2160
and 0x2178/0x2180: each root computes its packed entry inside the chosen arm,
then reaches the common halfword store. Source size grows from 2,260 to
2,272 bytes against retail's 2,276. This is actual branch recovery, although
operand allocation and the remaining copy still prevent an exact match.

The old shared-entry trials also reused the value after bitstream writes;
this reconstruction preserves those reloads and changes only the pre-write
scratch ownership. Separate reader controls were rejected: capturing the
initial nonzero condition explicitly adds boolean-normalization instructions,
and retaining the header depth with a separate mutable counter does not
improve the complete function. Neither reader change is retained.

The full deduplicated report changes only `WriteBPLossy`. Every other function
record, non-text section and unit non-fuzzy measure is unchanged. Full source
and normal builds pass; normal DOL SHA1 remains
`306526d90b48e99894c3138f5fc8f2716d9fecf6`. Bitplane stays NonMatching, so this
normal-link result is not an exact source-link claim. The isolated worktree
uses staging's published p1f variant (SHA1
`8641f1a15bab7d961b7b7558e8d0de64449c509c`); bitplane itself still uses ProDG3.5.
Evidence: `build/bitplane77-entry-scratch.json`,
`bitplane77-retained-report.json`, `bitplane77-retained-validation.json`,
and corresponding build/link logs. No behavioral suites or compiler changes.


### Bink sound task-array base lifetime (2026-10-03)

Lock captures state->tasks after reading the AX cursor, then uses that base
for the left and right lock tasks. This gives cursor-state access and task
addressing their own pointer lifetimes, preserving every task index, source
address, clamp and returned buffer/length. No qualifiers or layout changes.

Full deduplicated and raw Lock improve 92.78571 -> 93.46429; source grows
216 -> 220 bytes against retail224. This is partial: the dedicated task-base
addi is not retail's state-pointer mr, and some base/displacement choices,
channel-stride reads and repeated lock-index reads remain different.

Only this function's score and fuzzy aggregates change in the full report.
All other function records, exact code, data and completion measures remain
unchanged. all_source and normal link/check pass, preserving retail SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. ngcsnd stays NonMatching, so normal
linkage is not a whole-unit source-link claim. The private compile uses the
unit's actual ProDG3.5 -G0 configuration. No new assembly, compiler change or
behavioral test. Evidence: build/ngcsnd78-{baseline.json,tasks-private},
build/parallel-seventyeighth-{report.json,verify.py,build.log}.


### Bink inverse-transform working variables across passes (2026-10-03)

The byte and doubled IDCT now reuse the same butterfly working variables
across the dequantizing and output passes, following the ownership pattern
of the exact forward transforms in this file. Each reused variable has the
same mathematical role in both passes. Expressions, evaluation statement
order, scratch stores and output writes are unchanged. First-pass local
declarations move to function scope to keep the C declaration rules valid.
The doubled variant also shares its existing odd_scaled1 working value.

Full deduplicated and raw scores improve: fastidct8x8 64.052635 -> 65.2915
and fastidct8x8d 73.728745 -> 76.38461. Source sizes remain 932 and 988 bytes
respectively, against retail 988 for each. Opcode multisets are unchanged;
this recovers some scheduling and operand allocation, not missing arithmetic.
The motion variant regresses 58.61194 -> 56.656715 with the analogous change
and is not retained. Neither retained function is claimed exact.

The full report changes only these two function scores and fuzzy aggregates.
All other function records and all exact-code, data and completion measures
are unchanged. all_source and normal link/check pass; the normal DOL retains
SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6. DCT remains NonMatching, so
this does not claim source-linked retail identity. No compiler change or
behavioral test. Private evidence: build/dct79-shared-pass,
build/dct79-shared-fastidct8x8d, build/dct79-shared-FastmIDCT8x8WithMotion;
combined validation: build/parallel-seventyninth-{report.json,verify.py,build.log}.


### Bink inverse-transform even/odd butterfly scratch (2026-10-03)

After saving the even butterfly outputs, reuse its three working scalars
for the odd rotation sum, scaled pair difference and remaining rotation
difference. The same mathematical reuse applies in both passes of the byte,
doubled and motion transforms. All values needed from the even part have
already been saved; no arithmetic expression, output address, rounding or
scratch-array write boundary changes. This follows the ordinary staged
butterfly temporary ownership also visible in the forward transforms.

Full deduplicated and raw improvements:

- fastidct8x8: 65.2915 -> 74.72065 (932 bytes; retail 988).
- fastidct8x8d: 76.38461 -> 82.69231 (988 bytes; retail 988).
- FastmIDCT8x8WithMotion: 58.61194 -> 65.01119 (1012 bytes; retail 1072).

All three opcode multisets and sizes are unchanged; the gain comes from
scheduling and operand lifetimes. The first two retain the preceding
cross-pass working-variable reuse, while motion keeps separate pass scopes.
No new assembly, compiler changes or claimed compiler deficiency.

The full report changes only these three function scores and fuzzy
aggregates; all other function records, exact code, data and completion
measures remain unchanged. all_source and normal link/check pass, retaining
DOL SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6. DCT remains NonMatching;
this is not a source-linked retail-identity claim. Private evidence:
build/dct80-butterfly-scratch-{fastidct8x8,fastidct8x8d,motion}; combined
validation: build/parallel-eightieth-{report.json,verify.py,build.log}.

A separate even-output store-order control on the preceding checkpoint was
nearly neutral for byte and regressed doubled. It is not combined with the
retained scratch reconstruction.


### Doubled IDCT final butterfly scratch values (2026-10-04)

In fastidct8x8d, carry each odd butterfly's final sum or difference through
its existing arithmetic temporary before writing row[6], row[5] and row[4].
The previous checkpoint used those temporaries only for the intermediate
rotation results. Both passes now finish each working value in place; all
operands, arithmetic order and scratch/output store boundaries are preserved.

Full deduplicated and raw score improves 82.69231 -> 82.77328, with source
and retail both 988 bytes. The analogous byte and motion controls regress
to 74.04453 and 60.847015 and are rejected. This is a small operand-lifetime
gain; no exact-function or source-link closure is claimed.

Full report changes only fastidct8x8d and fuzzy aggregates. All other function
records, exact-code, data and completion measures are unchanged. all_source
and normal link/check pass, preserving retail SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. No compiler or assembly changes.
Evidence: build/dct81-final-scratch-{fastidct8x8,fastidct8x8d,FastmIDCT8x8WithMotion}
and build/parallel-eightyfirst-{report.json,verify.py,build.log}.


## Glare per-vertex coordinate captures (2026-10-04)

xScrFXGlareRender now computes each vertex's three coordinates into immutable
F32 scalars immediately before the position macro. These are the same corner
expressions and vertex order, with each vertex prepared before its stores;
UV/color calculations and rendering calls are unchanged. This mirrors the
existing streak position captures. It improves the arithmetic/store schedule
and register assignments without introducing additional instructions; both
source and retail remain 920 bytes. The remaining differences include the
initial coordinate load order and register allocation, so this is not closure.

The authoritative full deduplicated function score improves 93.35217% to
96.74348%; xScrFx improves 98.82294% to 99.4234%. Only this function's score
and aggregate fuzzy measures change. All other functions, exact measures,
data, sizes, and completion totals remain unchanged. All-source compilation
and normal retail link/check pass with SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. No compiler changes, assembly, or
additional behavioral tests were introduced; xScrFx remains NonMatching.

Evidence in the isolated rendering worktree: build/glare81/{baseline,
position_scalars}/{xScrFx.cpp,xScrFx.o,diff.json}, build/render81-{baseline,
final}-report.json, build/render81-{baseline,final}-build.log, and
build/render81-verify.py. The raw private score is 93.17826% to 96.569565%;
the full report above is the authoritative progress measure.


### Doubled IDCT even-butterfly phase (2026-10-04)

In the doubled transform's output pass, evaluate all four even working
values before storing the four even outputs. Compute the input0/input4
sum and difference together, then the input2/input6 sum and scaled
difference, followed by row0, row3, row1 and row2. The four inputs remain
captured before the phase, and every arithmetic expression is unchanged.
This restores more of retail's computation and local-store scheduling at
the current shared-temporary checkpoint.

Full deduplicated and raw fastidct8x8d improves 82.77328 -> 85.23482, with
source and retail both 988 bytes. Applying the same phase ordering to byte
and motion regresses to 72.10121 and 63.00373, so those controls are rejected.
The full report changes only fastidct8x8d and aggregate fuzzy measures; all
other function records, exact code, data and completion totals are unchanged.

all_source and normal link/check pass, preserving DOL SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. No compiler change or assembly.
DCT stays NonMatching; retail-size equality is not an exact-link claim.
Evidence: build/dct84-even-phase-{fastidct8x8d,fastidct8x8,FastmIDCT8x8WithMotion}
and build/parallel-eightyfourth-{report.json,verify.py,build.log}.


## Bink copy-block destination cursor ownership (2026-10-04)

ExpandPlane improves from 86.06153% to 88.09601% in the full deduplicated
report (raw 85.338066% to 87.342125%). SKIP, RUN-copy, MOTION and RAW now
reuse one function-scope dst0/dst1 pair for their alternating destination
rows. Each case still initializes both cursors at its original point; all
source loads, destination stores, arithmetic and cursor advances are unchanged.

The source function grows from 5,676 to 5,900 bytes against retail 5,916.
Previously the compiler merged SKIP copy tails with later copy cases; its
SKIP exits jumped into those later tails. With shared cursor ownership,
those exits reach the common block end and each copy case retains its own
instructions, as retail does. Every floating copy-load/store opcode count
now equals retail. Integer lwz counts rise 189 to 201 (retail 201); stw counts
rise 233 to 245 (retail 244). Remaining scheduling and instruction differences
mean this is a partial source match, not TU closure.

The full all_source build passes. The complete deduplicated report changes
only ExpandPlane and fuzzy aggregates; all other metrics are identical.
The normal retail DOL remains 2,859,136 bytes with SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. Expand remains NonMatching, so
this link check does not establish source-linked playback. No assembly,
compiler patch or new behavioral tests were introduced.

Evidence in the RGB worktree: build/expand84-shared-row-cursors.py and its
private candidate/diff directory, expand84-baseline-report.json,
expand84-candidate-report.json, expand84-{baseline,candidate}-build.log,
expand84-verify.py and expand84-validation.json.


### Doubled IDCT output-pass odd-pair rotation (2026-10-04)

Compute the scaled odd-pair difference in a3 immediately after the pair
sum is stored in row7, before the shared odd rotation. Both input sums are
available at that point. All expressions, remaining scratch writes, packed
output calculations and destination stores retain their order.

Moving this calculation in both passes regresses to 83.29555%; isolating
the passes shows first-pass-only 82.62348%, but output-pass-only improves
fastidct8x8d from 85.23482% to 85.408905%. Only the output-pass change is
retained, at unchanged source/retail 988-byte size. This is a small scheduling
and operand-lifetime gain, not exact-function or TU closure.

The full deduplicated report changes only fastidct8x8d and fuzzy aggregates.
All other function records, exact code, data and completion measures are
unchanged. all_source and normal link/check pass, retaining retail SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. No compiler or assembly changes.
Evidence: build/dct85-odd-pair-{rotation,first_only,second_only} and
build/parallel-eightyfifth-{report.json,verify.py,build.log}.


### Motion IDCT shared butterfly variables across passes (2026-10-04)

FastmIDCT8x8WithMotion now shares the same nine butterfly working variables
across its column and output passes, as the byte and doubled variants already
do. The existing even/odd scratch reuse is preserved. First-pass coefficient
locals move to function scope for C89; every read, arithmetic expression,
assignment boundary, scratch write and prediction/output operation retains
its order. Each working value is assigned before use in each pass.

The full deduplicated score improves 65.01119 -> 68.54851, with source size
unchanged at 1012 bytes against retail 1072. The earlier cross-pass control
predated the retained even/odd reuse and regressed; that relevant source
change justified this follow-up. No instruction-count or exact-TU closure
is claimed.

Full report changes only this function and aggregate fuzzy measures. All
other function records, exact code, data and completion measures are identical.
all_source and normal retail link/check pass, preserving DOL SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. DCT remains NonMatching, so the
normal DOL does not establish source-linked IDCT identity. No assembly,
compiler changes or behavioral tests were introduced.

Evidence: build/dct93-motion-shared-butterfly/{candidate.c,candidate.o,diff.json}
and build/parallel-ninetythird-{report.json,verify.py,build.log}.

### Motion IDCT output cursor ownership (2026-10-04)

The motion transform now gives its output pass a separate byte cursor,
initialized from dest after the column pass, as the byte transform already
does. All arithmetic, prediction reads, output stores and advances retain
their order. The local cursor recovers the missing pass-entry copy, although
its allocated register still differs from retail.

FastmIDCT8x8WithMotion improves 68.54851 -> 68.79478 in the full deduplicated
report; source size rises 1012 -> 1016 bytes against retail 1072. Exactly one
mr is added; the remaining opcode inventory is unchanged. This is a partial
match, not function or TU closure.

all_source passes, and the normal retail link target remains up to date with
DOL SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6. The full report changes only
this function and fuzzy aggregates; every other function record and every
non-fuzzy metric is identical. DCT remains NonMatching, so the normal retail
DOL does not establish source-linked IDCT identity. No compiler or assembly
changes and no behavioral tests.

Evidence: build/dct97-motion-output-cursor and
build/parallel-ninetyseventh-{report.json,verify.py,validation.json,build.log}.


## Bink decoded-audio count lifetime (2026-10-04)

In BinkDoFrame, retain the clamped decoded-byte count in an ordinary u32 local
through the audio-buffer overflow check. Use it for the overflow amount and
queued-byte update, refreshing it after the overflow path's sound-state writes.
This recovers retail's branch around an output-count reload. The original local
spelling is unproven; the small redundant value lifetime is an accepted matching
compromise, with no volatile access, assembly, compiler change or fake symbol.
The round 39 private candidate was previously declined for source complexity;
this pass revalidates it under the user's permission for reasonable locals.

Full deduplicated BinkDoFrame improves 99.57493 -> 99.621254. Source size remains
1464 versus retail 1468; the unit remains NonMatching. Exactly one function score
changes; all exact-code/data/completion measures and other functions are unchanged.
All-source compilation and normal retail linkage pass, retaining SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. The normal DOL still uses the retail object
for this unit, so this is partial source progress rather than source-link closure.

Evidence in the RGB worktree: build/bink103-{baseline,candidate}-report.json,
matching build logs, bink103-validation.json, and binkread103-doframe-private/.


## Skinning model-bone pointer boundary (2026-10-04)

`SkinXformVertAndNormal` now names the selected model matrix, advances that
pointer past the root matrix, and passes it to the existing `xMat4x3Mul`.
This is the same `mat[bi + 1]` input. It recovers retail's six-instruction
call setup: scale the bone index, add it to each matrix base, then advance
the model pointer by one matrix before the call. The previous expression
combined the index and root offset before adding the model base.

The full deduplicated score improves from 91.766914% to 93.646614%, at the
same 532 bytes. The remaining differences include entry scheduling and the
mask/done/weight-cursor register roles. Exactly this function's score changes;
all non-fuzzy report fields, data totals, exact-function counts and other
function scores are unchanged. All-source compilation and the normal retail
link/check pass, with DOL SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6.
The unit remains NonMatching; this normal link does not establish an exact
source-selected DOL. No compiler changes, assembly or behavioral tests were
introduced. Isolated evidence is under build/cutscene107/.

The preceding cutscene/camera audit retained no changes: FinishLoad and
FinishExit still differ in incoming/derived pointer storage and address
rematerialization; FlyStart retains its documented TOCINFO load-order residue.
Their existing aggregate/reference/helper controls were not repeated.


## 2026-10-04: Retain the lossless length child-depth capture

LenBPLossless improves from 94.41645% to 94.429306% by capturing each
main-plane child depth in an ordinary byte local before consuming it in the
comparison and encoded tree entry. The four-child traversal, depth values,
bit-count arithmetic, and final-plane path are preserved. This recovers the
round45 private candidate, which was previously discarded because the gain
was only one register encoding. Its original source spelling is unproven;
the small local lifetime is an accepted matching compromise.

The full deduplicated report changes only LenBPLossless and fuzzy aggregates.
All exact-match, data, and completion measures remain identical. The function
still emits 1532 bytes against retail's 1556 and remains NonMatching. The
all-source build and normal retail check pass; DOL SHA-1 remains
`306526d90b48e99894c3138f5fc8f2716d9fecf6`. No compiler, assembly, layout,
or qualification changes are involved.


## Glyph renderer: restore the verified typed capture (2026-10-04)

An archived complete `zNPCCommon_Glyphs_RenderAll` candidate still reaches
100% with the current compiler. Capture the current `en_npcglyph` before
light-kit setup and use it for the light-kit predicate and list lookup.
The glyph/list locals precede the count/index locals, and the inner traversal
uses `k = 0; while (cnt > k)` with its increment after the unchanged render
condition. All calls, guards and iteration behavior are preserved. This is
an ordinary typed lifetime and equivalent loop form; its original spelling
is not proven. The archive's complete source and minimization records were
checked before transplanting only this function into current source.

The full deduplicated report changes only this function, 98.735634% to 100%,
at 348 bytes. Exact code increases by 348 bytes and exact functions by one;
no other scores, data, symbol identities or unit-completion markers change.
Glyph now has 28/29 exact functions, 4808/5060 exact code bytes and all
10948 data bytes matched. `zNPCGlyph_ScenePrepare` remains 96.87302%, so the
unit remains NonMatching. All-source compilation and the normal retail
link/check pass with DOL SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6;
this does not establish a source-selected Glyph DOL. No compiler, assembly,
volatile or behavioral-test changes. Evidence: build/glyph109/.


## Glyph preparation: recover the per-type model lifetime (2026-10-04)

The complete archived `zNPCGlyph_ScenePrepare` scope candidate also improves
with the published p1g compiler. Keep `mdl_raw` and `cnt` inside the glyph-type
loop, with the existing outer glyph/list/index declarations in the recorded
order. Every initialization, asset lookup and glyph initialization call remains
unchanged. The model pointer now uses retail's r26. The missing separate copy
of the glyph type and other register roles remain unresolved.

The authoritative full report changes only ScenePrepare, 96.87302% to
97.111115%. Source size remains 248 bytes against retail's 252. Exact counts,
all matched data, other function scores and completion markers are unchanged;
Glyphs_RenderAll remains 100%, and the unit remains NonMatching at 28/29
exact functions. All-source compilation and the normal retail link/check pass,
with DOL SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6. Published p1g retains
SHA1 99bd18455ff674337d7a6186164df8a1b1ba13a7. No additional compiler changes,
assembly, volatile accesses or behavioral tests were introduced. Evidence:
build/archive110/, including the archived-candidate transplant, fresh p1g
baseline, full-report comparison and build logs.


## dlFread: advance the destination before the disk cursor (2026-10-04)

After each complete buffered copy, advance `addr` before `posTmp`. These are
independent local updates, and the values passed to every read/copy call and
the function's returns remain unchanged. The complete archived dlFread
candidate recorded this order; its second, unrelated final-accounting reorder
was omitted after verifying that the smaller change preserves the full report.
The retained change recovers retail's two-add sequence at function offsets
0x170 and 0x174.

The authoritative full report changes only dlFread, 99.27007% to 99.34306%,
at the same 548 bytes. All non-fuzzy report fields, matched data, exact counts,
other function scores and completion markers are unchanged. All-source
compilation and the normal retail link/check pass with DOL SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. Published p1g is unchanged. iSystem
remains NonMatching; this is not a source-selected link claim. Evidence:
build/archive111/, including the final minimal-source report.

## ROM font: recover the text-box right-edge operand order (2026-10-04)

The complete archived DrawTextBox candidate improves with p1g, but only its
right-edge addition spelling is needed: use `width + x` in the existing wrap
comparison. The archived declaration and null-check changes were omitted.
This preserves all calls, stores, guards and iteration behavior, and changes
only the generated add at function offset 0x6c from `add r25,r29,r25` to
retail's `add r25,r25,r29`.

The authoritative full report changes only DrawTextBox, 98.60656% to
98.68852%, at the same 244 bytes. All non-fuzzy fields, matched data, exact
counts, other function scores and completion markers are unchanged.
All-source compilation and the normal retail link/check pass with DOL SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6; published p1g retains SHA1
99bd18455ff674337d7a6186164df8a1b1ba13a7. iTRC remains NonMatching; this is
not a source-selected link claim. Evidence: build/textbox112/, including the
complete archive candidate, minimal candidate, reports and build logs.

## Spline arc evaluation: separate search and segment indices (2026-10-04)

The complete archived ArcEvalIterate candidate remains a gain under p1g.
Keep a separate signed `segStart` for the segment's first arc sample instead
of reusing the binary-search lower index, retain the recorded endpoint setup
order, and express the unchanged refinement loop as a for loop. The readable
version omits the archive's redundant cast and formatting changes. All
coefficient/evaluation calls, stores, guards and loop behavior are preserved;
the original spelling is not proven. This recovers retail's r8 search upper
index, r7 sample count and r5 conversion bias, although endpoint conversion
scheduling and other register differences remain.

The full deduplicated report changes only ArcEvalIterate, 96.910995% to
97.40838%, at the same 764 bytes. All non-fuzzy report fields, matched data,
exact counts, other function scores and completion markers are unchanged.
All-source compilation and the normal retail link/check pass with DOL SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6 and unchanged p1g SHA1
99bd18455ff674337d7a6186164df8a1b1ba13a7. xSpline remains NonMatching; no
source-selected link claim is made. Evidence: build/archive113/, including
the remaining-candidate index, complete/readable candidate diffs, assembly,
full reports and build logs.

## Swept sphere: recover box-axis evaluation order (2026-10-04)

The complete archived xSweptSphereToBox candidate recovers a small gain on
current p1g. Only two moves among independent axis-component calculations
are needed: evaluate aZx before aYz and aZz before aZy. The archived syntax
and declaration changes were omitted after the smaller change reproduced
the entire gain. Every expression, collision call, output store, guard and
return remains unchanged. The change adjusts initial axis loads and their
floating-point register assignments; it does not duplicate calculations.

The authoritative full report changes only xSweptSphereToBox, 99.15849% to
99.174835%, with source size unchanged at 2448 bytes. All non-fuzzy report
fields, matched data, exact counts, other function scores and completion
markers are unchanged. All-source compilation and the normal retail link/check
pass with DOL SHA1 306526d90b48e99894c3138f5fc8f2716d9fecf6. xCollide remains
NonMatching; this is not a source-selected link claim. No compiler changes,
assembly, volatile accesses or behavioral tests were introduced. Evidence:
build/swept114/ (complete archive and minimal candidate, private diffs,
validation.json), plus build/swept114-{baseline,candidate}-report.json and
build logs. Archive provenance is the complete gbest.json in scratchpad/cs/gp/
xSweptSphereToBox__FP12xSweptSphereP4xBoxP7xMat4x3; no reduced body that drops
calls was used.

## Thrown collision response: retain projection lifetimes with a shared dot product (2026-10-04)

The complete `big5/gp/thr2` best candidate reproduces 99.941864% under p1g.
Its body was checked against original best_99.9419_w2.c; differences from
current code, excluding historical renames and formatting, are confined to
the collision-reflection loop. A single simplification check restored the
shared `dothdng` expression while retaining recorded projection lifetimes and
the equivalent while traversal. This keeps a smaller gain at 99.93129%, and
is preferred over repeating the dot formula for another 0.010574 points.
All calls, stores, guards and iteration behavior remain intact.

The retained form keeps current debug names and uses an ordinary F32
`tangentX` instead of the archive's typeof temporary. The redundant capture
is a matching compromise, not a claim about original source spelling.
The original complete candidate is preserved in local commit 62ad79379.
The full deduplicated report improves only zThrown_Update, 99.915436% to
99.93129%, at unchanged 3784 bytes. All non-fuzzy fields, matched data, exact
counts, other function scores and completion markers are unchanged.
All-source compilation and normal retail link/check pass with DOL SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6 and unchanged p1g SHA1
99bd18455ff674337d7a6186164df8a1b1ba13a7. The unit remains NonMatching; no
source-selected link claim is made. Evidence: build/thrown114/, including
complete/readable/shared-dot candidates, normalized source diff, assembly,
and the final shared-report.json, shared-report-comparison.json and
shared-build.log. No assembly, volatile or compiler changes were introduced.

## Bink byte IDCT: mirrored output-pair lifetime (2026-10-04)

Write each mirrored byte pair through an ordinary inline helper taking the row
and its input/output column indices. The four calls retain the same integer
sum/difference expressions, rounding, narrowing and eight byte-store order.
The helper fully inlines and adds no emitted function or call. This is distinct
from the previous whole-row helper and scalar-pair helper: the pointer-backed
pair boundary improves the match while those earlier forms regressed or were
neutral. The original source spelling remains unproven.

The full deduplicated fastidct8x8 score improves 74.72065% to 75.76113%.
Source size moves from 932 to 944 bytes toward retail's 988. Relative to the
previous source it gains one lwz, one mr and one addi; the retail load/copy
residue is not solved. All other function scores and all non-fuzzy report
fields, including data and completion totals, are unchanged. All-source build
and the normal retail check pass with DOL SHA1
306526d90b48e99894c3138f5fc8f2716d9fecf6. DCT remains NonMatching; no source-link
completion is claimed. No assembly, volatile accesses or compiler changes.

Evidence: build/dct118-pointer-pairs/ and
build/parallel-hundredeighteenth-{report.json,changes.json,verify.py,build.log}.
The preceding load-provenance investigation is build/dct116-rtl/; the old
helper's address-pseudo/loop-hoisting proof is in the RGB worktree under
build/dct118-helper-rtl/.

## Bink signed Huff4: return the complete signed symbol (2026-10-04)

Move the existing conditional sign-bit read and negation into
exp_read_huff4_signed after its magnitude decode and bitstream updates. The
helper now returns the complete signed symbol; the caller only stores it.
Signed-byte conversion remains in each decode path, zero still consumes no
sign bit, and all bit reads, cursor updates, narrowing and output-store order
are preserved. Repeat packets and the unsigned readers are unchanged. The
helper fully inlines. This replaces the former result-copy/zero-test with a
comparison on the helper's symbol, though other register roles still differ.
The original source spelling remains unproven.

The full deduplicated CheckReadHuff4SBundle score improves 98.90995% to
99.028435%, with source and retail size unchanged at 844 bytes. No other
function score or non-fuzzy report field changes, including all data and
completion totals. All-source compilation and the normal retail check pass;
DOL SHA1 remains 306526d90b48e99894c3138f5fc8f2716d9fecf6. Expand remains
NonMatching; no source-link completion is claimed. No assembly, volatile,
compiler changes or behavioral tests were introduced.

Evidence: build/huff120/ (one complete private candidate, instruction changes,
validation.json), build/huff120-candidate-{report.json,build.log}, and the
unchanged baseline build/dct119-baseline-report.json. Prior output-parameter
and output-macro helper trials were not repeated.


### Bink masked right-half context advance (round 126)

YUV_blit_mask improves from 83.979515% to 84.79642% in the full
deduplicated report. The right-half case now advances luma, alpha and chroma
before its two destination pointers, matching the source order already used
by the paired-block and final single-block horizontal advances. Retail's
right-half instruction path follows that same order. The saved context,
mask reads, arithmetic, callback sequence and restored context are unchanged;
there is no intervening call between these independent field updates.

A private shared inline helper for all three horizontal advances produced
exactly the same instructions as this two-line move. Retain the smaller
source correction. The previous lower-mask merge helper was not repeated.
Source size remains 3088 bytes versus retail 3124; all four callback loops
remain aligned, while setup and other context scheduling differences remain.
Original source spelling is unproven; this is an ordinary statement-order
correction supported by the matching neighboring operations.

The complete deduplicated report changes only YUV_blit_mask and aggregate
fuzzy scores. No other function score or non-fuzzy field changes. All-source
compilation and normal USA retail verification pass; DOL SHA1 remains
306526d90b48e99894c3138f5fc8f2716d9fecf6. The unit remains NonMatching.
No compiler, assembly, volatile, flags or behavioral-test changes.

Evidence: build/mask126/{baseline,shared_order,shared_helper}.cpp,
private-validation.json, validation.json, shared_order-diff.txt, and the
baseline/candidate report and build logs. Baseline yuv.cpp is byte-identical
to the saved round124 baseline, allowing its private object evidence reuse.


### Regional save-game date formatting

PAL and German retail format both save modification dates and new timestamps
as day.month.year. Use the existing regional version defines to select that
format and argument order in iSGFileModDate and iSGMakeTimeStamp; preserve
USA's month/day/year calls unchanged.

The regional 25,648-byte unmatched-data total was two slash characters in
@stringBase0. The 23,616-byte zero initializer for IconData was already exact;
no bitmap or icon extraction is involved. PAL object comparison now gives
100% for both date functions and .rodata. Its full report changes only
isavegame: matched functions 61 -> 63, matched data 83,136 -> 108,784, fuzzy
code 99.89019% -> 99.897766%. PAL all_source and its normal 445-unit source
link pass with retail SHA1 6da9022f06bfb62a203017ec38046ba2566dc0cf.
USA and German integration verification is delegated to the parent worktree.
The three existing holdouts remain; the TU is not promoted to Matching.

Evidence: build/isavegame127/{findings.json,date-functions.txt} and
build/isavegame128/GQPP78-{unit.json,report.json,build.log}, PAL-validation.json.


### Bink pattern lookup byte offset (round 136)

ExpandPlane improves from 88.09601% to 88.40568% in all three full,
deduplicated regional reports. The normal pattern-row helper now computes
the low-nibble table byte offset by scaling the pattern byte before masking:
`(row_bits * sizeof(u32)) & (HUFF4_SYMBOL_MASK * sizeof(u32))`.
Both low-word mask loads use that offset. This selects exactly the same
four-byte table entries for the unsigned input byte; the high nibble,
color arithmetic, reads, and writes retain their existing behavior.

Retail combines each low-nibble mask and scale into one clrlslwi. The old
source emitted separate clrlwi/slwi instructions for all eight rows. The
new expression recovers all eight combined instructions: clrlslwi count
1 -> 9, clrlwi 28 -> 20, and slwi 31 -> 23, matching those retail counts.
The function shrinks from 5900 to 5868 bytes versus retail5916. Raw score
87.342125 -> 87.570656. The earlier round77 offset candidate masked before
scaling; this candidate scales before masking. Original source spelling is
unproven, but the correction is ordinary unsigned table-address arithmetic.
No assembly, volatile, flags, or compiler patch is added.

All-source compilation passes for USA, Europe, and Germany. Every full-report
field outside fuzzy scores is identical, and ExpandPlane is the only changed
function record. All three normal DOL SHA1 checks pass. Expand remains
NonMatching, so these existing source selections do not prove a source-linked
Bink decoder match. No exact-function or completed-TU gain is claimed.

Evidence: build/expand136-shift-mask.py, its candidate and raw diff directory,
build/expand136-baseline.json, and build/parallel-136-final-validation.json.

## Memory-card slot initialization order (2026-10-08)

`iSG_mcidx2slot` improves from 95.671875% to 96.03125% in all three
GameCube versions by initializing the memory-size output before the slot
output and the readiness array afterward. The initial values, card-probing
loop, return values, and 256-byte function extent are unchanged. This is
a partial source improvement; initialization scheduling still differs.

The bounded sweep measured all 120 orders of the five initialization groups.
All three complete source builds and retail DOL checksums pass. Comparing
every function and unit measure finds this function's fuzzy score is the
only change; exact code, data, function counts, and completion are unchanged.
USA overall fuzzy rises from 99.71828% to 99.71831%; Europe and Germany rise
from 99.71839% to 99.71843%. The unit remains NonMatching.

The two other save-card holdouts were also checked: 48 equivalent switch
case-group/default variants do not change their scores. Stock GC/2.0, 2.5,
2.6, and 2.7 retain the same three residuals, so these tests provide no
new compiler-patch justification.

Private evidence: `build/oct08-save-init-probe/results.json`,
`build/oct08-save-probe/results.json`, the `oct08-save-after-<version>.json`
reports, and `build/oct08-save-german-proof.log`.

## Parallel PS2 and France matching (2026-10-09)

Full reports against a fresh `8afd24dc3` baseline retain every previously
measured function score and exact match. The USA report now has 982,784 exact
code bytes / 3,735 exact functions, up 6,576 bytes / 24 functions. France has
118,832 exact bytes / 431 exact functions, up 11,572 bytes / 45 functions.
The full CPU-code denominators remain 2,978,560 and 2,979,968 bytes respectively.
Neither report claims a fully linked retail executable.

France's original-only recovery adds 42 identities / 64,332 known bytes:
pad, Hangable, Group, Event, the Dutchman parameter caller and three helpers,
King Jelly and its float-list helper, and the SB2 parameter caller. The combined
registry now covers 702 functions / 299,568 bytes. The parameter bodies use a
distinct caller/callee-cluster proof kind; they do not claim complete NPC units.
Original typed arrays, complete literals, strict control flow, unique masked
bodies and three authenticated reference versions establish the new extents.

The final production run regenerates all three proof registries, checks the
committed symbols/splits, compiles 66 French source units and exports the full
region report. A dependency regression found during integration is fixed:
Group and Hangable keep their independently compared Event contexts after the
Event functions are registered. Original-backed tests cover both dependency
fences. All 385 previously published TU-proof records remain unchanged.

Source gains include particle loops, pad thresholds/rumble pointer reuse,
culling and group operand order, sound voice bounds and original wrappers,
scoped grid call boundaries and direct FFX returns, spline inlining/address induction, missing
streaming scratch initialization, and MovePoint's shared unit-length local.
Changed source units were compared in all four PS2 regions; direct original
byte reconstruction corroborates the newly exact debug-region bodies where
relocations are available. French unresolved operands remain documented in
their unit notes. All three GameCube full source reports and retail checksums
pass with unchanged scores after the earlier memory-card improvement.

The France fuzzy tool searches large original functions and contiguous blocks
first, uses bit-parallel LCS beyond the quadratic refinement budget, excludes
authenticated occupied ranges, and replays tentative maps as ranking penalties.
The final diagnostic map excludes all 299,568 verified bytes. Tentative candidates
remain ineligible for progress. Compiler probes did not establish an earlier-bug,
later-fix pattern; this batch adds no compiler patch.

Private evidence: `build/oct08-france-final/SLES-53623`,
`build/oct09-us-current/SLUS-20680`, `build/oct09-{france,us}-comparison.log`,
`build/oct09-hangable-test.log`, and the per-unit artifacts cited in the PS2
notes. The baseline worktree is `C:/Projects/bfbb-verify-oct08-before`; its USA
baseline reused 137 completed objects from the initial serial run and compiled
the rest against the frozen source. The interrupted compiler's partial object
was excluded. The subsequent USA source refresh recompiles the three changed
units against the unchanged verified targets, preserving source hashes and the
full report/export checks.

The next source-only batch adds another 992 exact bytes / six functions in
USA and 528 bytes / four functions in France. The USA report reaches 983,776
exact bytes / 3,741 functions; France reaches 119,360 bytes / 435 functions.
Psyche transition/list access and the three string hashes supply the exact
gains. Sound switch exits and the particle-manager countdown improve fuzzy
matching. The countdown also restores the original three volatile load sites,
removing the extra initialization read in the previous source object.

Both full reports were refreshed against the unchanged verified target sets;
every previous function score and exact match is retained. All three GameCube
full source reports and retail checksums remain unchanged. Evidence:
`build/oct09-{us,france}-second`, their `*-second-comparison.log` files, and
`build/oct09-second-<GameCube-version>.log`. Compiler/scoring settings,
function extents and all regional denominators remain unchanged.

The Plankton/CruiseBubble batch adds two independently proven parameter bodies /
24,552 known French bytes. Full original-only registry regeneration and source
compilation pass, bringing coverage to 704 functions / 324,120 bytes and full
CPU fuzzy matching from 8.623672% to 9.450579%. Their source comparisons improve
after removing five vector copies and restoring an original zero-vector local.
The `auto_tweak.h` change affects only Plankton/Prawn; both complete units were
checked, with every Prawn score retained.

Tokenizer and substring joins add another 512 exact bytes / two functions in
both USA and France. USA reaches 984,288 exact bytes / 3,743 functions; France
reaches 119,872 bytes / 437 functions. Psyche timer clearing and buffer-tokenizer
scope improve fuzzy matching as well. Complete report comparisons find no
regressions, and all three GameCube source reports/checksums remain unchanged.
Ten original-backed parameter-proof/mutation tests pass. Private evidence:
`build/oct09-{france,us}-npc`, their `*-npc-comparison.log` files,
`build/oct09-npc-tests.log`, and `build/oct09-npc-<GameCube-version>.log`.

The utility/serializer/Prawn batch passes another complete USA source build
and full French registry regeneration/source build. USA adds 2,268 exact bytes /
eight functions, reaching 986,556 bytes / 3,751 functions. France adds 4,048 bytes /
four functions, reaching 123,920 bytes / 441 functions. The new Prawn parameter
identity supplies 2,660 French bytes; utility classification, probability and
CRC lifetimes make all eleven utility functions / 2,668 bytes exact in all four
PS2 regions. Serializer inline boundaries add 880 exact bytes in each debug
region. Prawn turning locals/sign tests, regional credits layout/color unpack,
and float-parser pointer scope improve fuzzy matching.

French coverage is now 705 functions / 326,780 bytes. Its production run compiles
69 source units and keeps the 2,979,968-byte CPU denominator. The original ctype
array declaration, all 257 entries including EOF, and the only PS2 profile
consumer were audited; raw debug-region utility/serializer reconstruction uses
genuine call, GP, paired-address and switch-table relocations. Two Prawn proof
mutation tests pass. All previous function scores/exact matches are retained,
and all three GameCube full source reports/checksums remain unchanged.
Evidence: `build/oct09-{us,france}-utility`, their comparison logs,
`build/oct09-prawn-tests.log`, and `build/oct09-utility-<GameCube-version>.log`.

The textbox/screen-effects batch adds 4,676 exact USA bytes / eleven functions,
reaching 991,232 bytes / 3,762 functions. Textbox reset/link/pointer lifetimes,
original inline rectangle/height helpers, reciprocal vertex depth and regional
dimensions recover five complete functions. Screen-effect initialization/reset,
platform rendering, distortion call boundaries and regional glare geometry
recover five more; Prawn's original decompose boundary recovers its death entry.
Credits packed color uses the original four-byte reversed-channel unpack type.

Full USA comparisons retain every previous function score and exact match;
the French report is entirely unchanged. All three GameCube builds and retail
checksums pass. Glare improves from 98.5087% to 98.55218% in each GameCube region,
with no other function or code/data measure regression. Evidence:
`build/oct09-{us,france}-ui`, their comparison logs, and
`build/oct09-ui-<GameCube-version>.log`. The authentic RenderWare RGBA macro
investigation remains a separate private caller sweep, outside this batch.

The first player-animation batch adds three exact French builders / 7,448 bytes.
Full canonical regeneration preserves all earlier proof records and validates
the exact COP1 MOV.S decoder extension, including reserved-bit rejection and
MFC1/CFC1 address-register clobbers. All six decoder/literal/original mutation
tests pass. French coverage reaches 708 functions / 334,228 known bytes; its
70-unit source build reports 131,368 exact bytes / 444 exact functions with
the unchanged 2,979,968-byte CPU denominator. The other builders remain outside
this scoped proof pending complete callback/helper evidence.

The PS2 UI renderer also improves from about 57.99% to 97.62% across the debug
regions after restoring vertex depth, reciprocal depth, regional dimensions
and byte-color locals. USA full CPU fuzzy rises from 64.281576% to 64.316060%,
with exact bytes/functions unchanged. Every previous function score/exact match
is retained; all three GameCube source reports and retail checksums pass.
Evidence: `build/oct09-france-animation`, `build/oct09-us-ui-render`, their
comparison logs, `build/oct09-player-tables-tests.log`, and
`build/oct09-animation-<GameCube-version>.log`.

The SDK/animation-extension batch reaches 1,005,392 exact USA bytes / 3,775
functions, adding 14,160 bytes / thirteen functions. Authentic RenderWare vertex
color assignment, compatible laser color copies, renderer local lifetimes and
inline boundaries, UI setup/portal/button dispatch and shadow call boundaries
supply the exact gains. Emitter helpers, packed credits color, ribbon normals,
and original PS2 VU shadow operations improve fuzzy matching. All VU masks are
checked against raw words because the disassembly display omits destination masks.

Five original-backed animation builders now compare exactly in France, expanding
the earlier three without changing their records. Full callback/sound/global
contexts corroborate the additional German pointer operands; those contexts
remain unpromoted. Three complete CruiseBubble insertion/cheat/callback bodies
reuse the prior parameter anchor and add another 1,148 exact bytes. Together
with exact UI setup, France gains 27,020 exact bytes over the preceding snapshot.
Its full production run recovers 713 functions / 360,832 known bytes and compiles
70 source units, preserving the full CPU-code denominator and earlier scores.

The SDK worker compares 69 affected unit/region pairs plus the final laser helper
closure. Root independently completes the USA source build, all French registry
regeneration/source/export checks and all three GameCube builds/checksums. Seven
animation/Cruise original-backed tests pass. All function/code/data regression
checks are clean. GameCube streak and glare fuzzy scores improve; no exact or data
measure is lost. Evidence: `build/oct09-{us,france}-sdk`, their comparison logs,
`build/oct09-animation-extension-tests.log`, `build/oct09-sdk-<GameCube-version>.log`,
and the raw projection/caller artifacts cited in the unit notes.

The renderer/sound follow-up adds another 2,280 exact USA bytes / three functions,
reaching 1,007,672 bytes / 3,778 functions. Ring lifetime selection, fireworks
setting order and original sound-position scalar lifetimes supply the exact
gains; lightning initialization order and omitting an unused PS2-only counter
improve fuzzy matching. All 37 sound functions / 6,764 bytes are now exact in
the three debug PS2 regions. The fast shadow receiver is restored from original
C/VU operations, with all 66 vector words checked raw including masks.

The complete USA/French report comparisons retain every earlier score and exact
match; the French report is entirely unchanged. All three GameCube full source
reports/checksums pass without new regressions. Evidence:
`build/oct09-{us,france}-render-followup`, their comparison logs, and
`build/oct09-render-followup-<GameCube-version>.log`.

The shadow/cache and effect batch adds 6,252 exact USA bytes / seven functions,
reaching 1,013,924 bytes / 3,785 functions. Five complete shadow leaf,
environment, entity, fill and removal bodies contribute 5,076 bytes; restoring
texture pointer types and one-bit conditions makes bubble/shiny rendering exact
for another 1,176 bytes. Workers validate the affected units in all three debug
PS2 regions. Root recompiles both changed units against the previously verified
214-unit USA snapshot and retains every earlier function/code/data measure.

Nine independent original-backed French listener/voice/delayed-insertion bodies
add 1,988 exact bytes / nine functions, reaching 160,376 bytes / 459 functions.
All prior registry records survive unchanged. Strict production regeneration
equals the serialized registries and all 70 enabled French source units compile.
Coverage reaches 722 functions / 362,820 known bytes. Full CPU denominators stay
2,978,560 bytes for USA and 2,979,968 for France; source data and full executable
links remain pending. All three full GameCube reports/checksums retain the earlier
streak/glare gains without further regression. Four listener mutation/duplicate/
type-bound tests pass, and exact JSON roundtrip equality guards serialization.

Evidence: `build/oct09-us-shadow-effects`, `build/oct09-france-listeners`, their
comparison logs, `build/oct09-listeners-final-tests.log`,
`build/oct09-france-listeners-map{,-query}.json`, and
`build/oct09-shadow-<GameCube-version>.log`. The refreshed fuzzy map excludes
722 proven bodies before ranking remaining large contiguous sections; candidates
remain diagnostic and receive no matching or boundary credit.

The vector/playback batch adds another 2,232 exact USA bytes / four functions,
reaching 1,016,156 bytes / 3,789 functions. World shadow rendering and the shadow
quad add 1,784 exact bytes; both vector normalization bodies contribute 448.
Explicit original effect helper inline boundaries, billboard loop invariants,
ribbon visibility and the shared lightning endpoint loop improve fuzzy matching
without losing earlier matches. The complete USA snapshot recompiles all four
changed units against the previously verified 214-unit target/source set.

French playback proves nine further complete bodies / 4,924 bytes with all
three reference originals, typed storage, complete strings and explicitly scoped
runtime/vector uniqueness checks. Seven source bodies are compared; two HIS
bodies remain coverage-only. The raw-exact vector source change and three exact
playback bodies add 2,696 exact bytes / five functions, reaching 163,072 bytes /
464 functions. Coverage reaches 731 functions / 367,744 bytes. The complete
production run regenerates all registries identically and compiles 72 units.
An initial regeneration failure identified three replaced Hangable vector
contexts; excluding subsequent vector identities preserves the earlier proof.
The original-backed Event/vector dependency test and seven playback tests pass.

All function/code/data regression checks pass. All three GameCube full source
reports/checksums retain their earlier measures, and both full Xbox source
reports remain identical at 15,306 exact bytes / 81 functions. CPU denominators
and source-data/link limitations are unchanged. Evidence:
`build/oct09-us-vector-playback`, `build/oct09-france-playback-retry`, their
comparison logs, `build/oct09-{playback,hangable-vector}-tests.log`,
`build/oct09-vector-playback-<GameCube-version>.log`, and
`build/oct09-vector-playback-xbox` with both regional comparison logs.

The culling/robot batch adds 2,076 exact USA bytes / four functions, reaching
1,018,232 bytes / 3,793 functions. Climate wind lifetime, hazard's steamy-stinky
body and original robotic zoom/appearance operand order provide those gains.
Restoring VU side-plane and scalar near-plane culling raises eleven NPC particle
updates from 29-76% to 98-99.95% and decal update from 48.63% to 94.67508%.
Original lane masks, packed comparisons and actual vmul.w branch delay slots
are checked raw in all three debug regions. Typed hazard union locals, original
call boundaries and PS2 rotation paths produce nine more function improvements.
The combined USA fuzzy measure rises from 64.709509% to 65.105562% with no loss
in any earlier function/code/data measure; all seven changed units are rebuilt
against the previously verified 214-unit source/target snapshot.

French streak/motion/model closure adds 44 complete identities / 9,716 bytes.
All new source bodies are exact, and two previously proven support bodies gain
source comparison. Exact matching rises by 9,892 bytes / 46 functions, reaching
172,964 bytes / 510 functions. Strict original-backed regeneration preserves
every earlier proof; all 76 enabled source units compile. Coverage reaches 775
functions / 377,460 bytes. Twelve cluster mutation/duplicate/dependency tests
pass. The refreshed largest-first fuzzy map excludes those proven bodies and
continues to treat remaining candidates as diagnostics only.

All three GameCube complete source reports/checksums preserve their earlier
scores. The Xbox-only weighted random calculation improves xUtil_yesno from
38.454544% to 79.84849% in both complete 13-unit production builds; exact totals
stay 15,306 bytes / 81 functions and all other scores remain unchanged. Updating
an explanatory reviewed-metadata sentence required a new anonymous-registry
input digest; full re-decoding confirms every one of the 2,440 anonymous records
is unchanged. CPU denominators, source-data and complete-link limitations remain
unchanged for all platforms.

Evidence: `build/oct09-us-culling-robots`, `build/oct09-france-culling-robots`,
their comparison logs, `build/oct09-robot-cluster-tests.log`,
`build/oct09-culling-robots-<GameCube-version>.log`,
`build/oct09-culling-robots-xbox-retry`, both regional comparison logs,
`build/oct09-urand-boundaries` and `build/oct09-france-robots-map{,-query}.json`.

The save/emitter/NPC batch adds another 4,208 exact USA bytes / fourteen
functions, reaching 1,022,440 bytes / 3,807 functions. Save selection, format,
space/slot validation and the previously missing autosave updater add 2,800
bytes / eight functions. Original German wrong-device prompts remain present.
Typed emitter event/result lifetimes contribute 600 bytes / two functions;
hazard cylinder arithmetic adds 240 / one, robot kennel counter scopes add
168 / one, and particle-system predicates add 400 / two. Snow and sprite VU
culling, original recursive call boundaries, emitter bound references and
signed particle limits supply further fuzzy gains. The USA fuzzy measure rises
from 65.105562% to 65.299311%, with all eight changed units recompiled against
the previously verified 214-unit source/target snapshot and no earlier loss.
Interpolation source restores the original unassigned result for unsupported
PS2 modes; supported modes are independently checked, and GC keeps its matched
initializer. Unnamed runtime callees remain unresolved rather than promoted.

French facing/NPC/player-model closure adds 44 complete identities / 10,844
bytes, reaching 819 functions / 388,304 known bytes. Typed nested player-model
operands use the independent complete CalcNewDir anchor; scoped small-helper
uniqueness preserves the generic thresholds and all opaque runtime contexts
remain unnamed. Together with exact DuploNotice source, matching rises by
5,672 bytes / 29 functions, reaching 178,636 bytes / 539 functions. All 76
enabled units compile after strict aggregate evidence regeneration. Sixteen
original-backed mutation/duplicate/dependency tests pass, and every earlier
function/code/data score is retained.

Xbox particle random inlining adds 263 exact bytes / one function in both
complete 13-unit production builds, reaching 15,569 bytes / 82 functions.
Three neighboring commands improve without changing any other score. The
actual source reconstructs all 263 original bytes using eight named address
operands; all 45 actual HIGHLOW fields across the four bodies are authenticated.
The full GameCube reports/checksums remain unchanged in all three regions.
CPU denominators and source-data/full-link limitations remain unchanged.

Evidence: `build/oct09-us-save-npc`, `build/oct09-france-save-npc`, their
comparison logs, `build/oct09-npc-{helper,facing}-tests.log`,
`build/oct09-save-npc-<GameCube-version>.log`, `build/oct09-save-npc-xbox`,
both regional comparison logs, and `build/oct09-france-npc-map{,-query}.json`.
The private Xbox reconstruction and all-section cross-platform proofs are
cited in `XBOX_PARTICLE_RANDOM.md`.

The full-region renderer/save/robot batch rebuilds every target object and all
214 enabled source units in USA, PAL and Germany. This validates the generic
ELF defined-symbol correction against complete reports, including explicit
original-DWARF/inverse-checked recursive calls. The already raw-identical
220-byte updater becomes exact through correct comparison metadata; no source
or compiler instruction is changed by that correction.

| PS2 region | Exact bytes before | Exact bytes after | Exact functions before | Exact functions after |
| --- | ---: | ---: | ---: | ---: |
| USA | 1,022,440 | 1,031,720 | 3,807 | 3,822 |
| PAL | 1,014,880 | 1,024,160 | 3,794 | 3,809 |
| Germany | 1,015,364 | 1,024,868 | 3,794 | 3,810 |

USA/PAL each gain 9,280 exact bytes / fifteen functions; Germany gains 9,504 /
sixteen, including its original extended-character validator. Save/load result,
autosave polling, format/overwrite/directory callbacks and original UI strings
provide the save gains. Typed S32 timer materialization, damage/snore/bonked
branches and death-ray order provide the robot gains. All four previously
missing PS2 particle renderers now have source, improving the complete iParMgr
unit from 38.355755% to 92.8517% while preserving its earlier exact functions.
VU geometry/culling/Euler masks, packed stores, polynomial values, real delay
slots and culled-path pivot restoration are independently checked raw. Remaining
renderer size/register/scheduling differences stay scored.

The complete debug-region comparisons reject no function/code/data regression.
USA fuzzy matching reaches 65.527588%, PAL 65.488686%, Germany 65.362493%.
PAL/German baselines come from independently compiled frozen commit `555924f5e`;
the USA baseline is the previously verified complete 214-unit snapshot. The full
CPU denominators remain 2,978,560 / 2,979,712 / 2,976,512 bytes respectively.

French vector/scalar closure adds ten complete identities / 6,492 known bytes,
reaching 829 functions / 394,796 bytes. Typed full vectors and member paths, the
existing player-model anchor and a typed local float corroborate 33 data operands;
all 33 JALs close through fixed complete identities or unnamed literal contexts.
The complete 76-unit report adds 2,772 exact bytes / eight functions, reaching 181,408 /
547. All earlier proof records and scores are preserved. Eight original-backed
tests and three ELF-symbol tests pass. All three GameCube source reports/checksums
and both Xbox full source reports are unchanged. Source-data/full-link limitations
remain explicit.

Evidence: `build/oct09-full-render-save/<version>`, all three comparison logs,
`bfbb-verify-oct09-555/build/before-555` and baseline logs,
`build/oct09-france-render-save`, its comparison log,
`build/oct09-npc-vector-tests.log`, `build/oct09-render-save-<GameCube-version>.log`
and `build/oct09-render-save-xbox`.

The curve/pool/animation follow-up adds 1,444 report-exact bytes / four functions
in each debug PS2 region. USA reaches 1,033,164 bytes / 3,826 functions, PAL
1,025,604 / 3,813, and Germany 1,026,312 / 3,814. All three complete report
comparisons preserve every earlier function/code/data measure. Three changed
units are rebuilt against the just-verified 214-unit source/target snapshots.
Curve interval arithmetic and the original out-of-line abs behavior contribute
460 bytes; pool scene entry, bucket sort and flush contribute 984. The curve's
single runtime JAL and flush's SDK JAL remain unnamed/unresolved; independent
raw audits prove the remaining 456/460 and 256/260 bytes rather than claiming
new callee identities. Particle transform flags and vertex/index reset order
give further source improvements, with VU/store/delay checks replayed.

French animation closure proves nineteen new complete builders / 10,200 bytes:
seventeen Robot members in one unique 9,884-byte span and two Common members
in an independently unique 400-byte span. All original DWARF ordering, bounds,
zero gaps, typed table/literal/callback roles and local-array copy inventories
are checked. Small members inherit the complete unique cluster's identity;
generic uniqueness thresholds and whole-TU/data-extent exclusions remain intact.
All old registry records survive unchanged and fifteen original-backed tests
pass. The full French production gate regenerates all aggregate evidence and
compiles 76 source units. With exact curve and bucket-sort source, matching
gains 10,996 bytes / twenty-one functions, reaching 192,404 bytes / 568 functions.
Coverage reaches 848 functions / 404,996 known bytes. No earlier score is lost.

All three GameCube full source reports/checksums and both complete Xbox source
reports remain unchanged. CPU denominators and source-data/full-link limits
remain unchanged. Evidence: `build/oct09-curve-animation/<debug-version>`, the
three comparison logs, `build/oct09-france-curve-animation`, its comparison log,
`build/oct09-npc-{animation,common-animation}-tests.log`,
`build/oct09-curve-animation-<GameCube-version>.log`,
`build/oct09-curve-animation-xbox` and both regional comparison logs,
plus `build/oct09-france-animation-map{,-query}.json`.

The player/TRC/SKB/boss batch adds 624 exact bytes / three functions in USA and
PAL, reaching 1,033,788 / 3,829 and 1,026,228 / 3,816 respectively. Germany
gains 984 bytes / four functions, reaching 1,027,296 / 3,818, including its
previously omitted goo-death control/carried-object cleanup. Regional controller
messages, byte-color handling, validity and initialization reproduce the
original layouts and strings without naming stripped camera SDK calls.

Player movement's original TurnToFace boundary improves 75.10625% to 99.375%;
documented slide locals/determinant order improve 35.108143% to 36.631042%.
Original rotation state/reset ordering and the absent PS2 dot clamp improve
80.958466% to 85.722046%. Update's two original integer-abs calls improve
94.44583% to 94.56484%; their runtime name remains unresolved. SKB translation
improves 28.290323% to 99.17742% through the genuine volatile abs.s primitive and
original count/zero-array lifetimes. Eval remains missing in this snapshot and
retains its full denominator. All three combined debug-region report comparisons
preserve every earlier function/code/data measure.

Five individually unique French boss animation builders add 10,472 exact bytes
/ five functions, reaching 202,876 bytes / 573 functions. All original owner and
boundary witnesses, 279 typed table/string/callback/initializer operands and
143 calls are inventoried in every reference. The shared initializer uses the
authenticated function owner; the previous Robot proof output stays identical.
All 556 prior function proofs remain unchanged. Nine boss mutation tests and
eight Robot replay tests pass; strict aggregate regeneration and all 76 French
source units succeed. Coverage reaches 853 functions / 415,468 known bytes.

All three GameCube full source reports/checksums and both Xbox source reports
remain unchanged. CPU denominators and source-data/full-link limitations remain
explicit. Evidence: `build/oct09-player-boss/<debug-version>` and comparison logs,
`build/oct09-france-player-boss`, its comparison log,
`build/oct09-npc-boss-animation-tests.log`,
`build/oct09-npc-robot-animation-replay.log`,
`build/oct09-player-boss-<GameCube-version>.log`,
`build/oct09-player-boss-xbox` and both regional comparison logs.

The SKB/model/townsfolk follow-up adds 280 exact bytes / one function in each
debug PS2 version through Villager's original sine-expression order. USA reaches
1,034,068 bytes / 3,830 functions, PAL 1,026,508 / 3,817, and Germany
1,027,576 / 3,819. Full report comparisons preserve every earlier function,
code and data measure. Four changed units are rebuilt against the verified
214-unit snapshots, with unchanged debug profiles and frozen source hashes.
Overall fuzzy scores reach 65.719248%, 65.680177%, and 65.564430% respectively.

Previously omitted SKB evaluation reaches 78.87838%; model animation matrices
reach 93.62963%, and sphere culling reaches 90.19444%. Their raw VU/MMI words,
lane masks, original locals and typed polynomial/frustum operands are checked
independently. Lasso rendering improves 85.162766% to 88.31542%. Scheduling and
register residuals remain scored; no compiler patch is introduced.

France adds fourteen complete animation identities / 9,092 known bytes through
five independent builders and two complete Ambient/Villager clusters. All 561
previous function records, auxiliary proofs, sequence/call prefixes and metadata
remain unchanged. Twenty-two original-backed tests pass. Strict full aggregate
regeneration succeeds and all 82 enabled source units compile. Exact matching
gains 6,524 bytes / fourteen functions, reaching 209,400 bytes / 587 functions;
known coverage reaches 867 functions / 424,560 bytes. The four-function SKB
profile uses unchanged authenticated boundaries and adds its 32-byte Duration
match; incomplete Sandy/SB1 builders retain their full residuals.

All three GameCube source reports/checksums and both Xbox source reports pass
without a regression. Full CPU denominators and the source-data/full-link
limitations remain unchanged. Evidence: `build/oct09-eval-townsfolk`, its three
comparison logs, `build/oct09-france-eval-townsfolk` and comparison log,
`build/oct09-npc-{remaining,townsfolk}-animation-tests.log`,
`build/oct09-eval-townsfolk-<GameCube-version>.log`,
`build/oct09-eval-townsfolk-xbox` and
`build/oct09-france-townsfolk-{query,map}.json`.
