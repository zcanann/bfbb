# PC port — follow-up plan

Not started. This is a design record so the decision context survives, written
while the decomp is still in progress. Nothing here should change what we work
on day to day except one thing, flagged under **Priority consequence** below.

Target shape: the decompiled game code compiled with a modern toolchain against
a PC platform layer, rendering through **librw**, loading **Xbox-extracted
assets**, shipping as code only.

## The premise this rests on: matching and portable are different goals

Matching means reproducing CodeWarrior's exact codegen for big-endian PowerPC.
A port needs code that is *semantically complete and correct*. Most functions
we have marked `NonMatching` are already fine for a port, and byte-exactness
buys it nothing.

So the gate for phase 1 is **not** the project's headline percentage. It is
"every function in `src/SB/**` is written and behaves correctly." Today that is
**7529 / 7673 game-code functions** (tools/srcprogress.py, 2026-08-26): 7243
exact + 257 codegen-only + 29 reload-only = 92.37% by bytes, against 80.41%
exact. Only 144 functions still need source written, and 153 of 221 units are
source-complete. The remaining library work —
`rwsdk` (1039 functions, 4% matched), `bink` (336, 1.9%), `MSL` (324, 49%) —
is *irrelevant to the port*, and in the case of rwsdk and bink actively so,
since we replace one and delete the other.

**Priority consequence:** finishing rwsdk matching would be ~1039 functions of
effort a port discards. Every `src/SB/**` function is dual-purpose. This is an
independent argument for the game-code-first rule already in docs/DUPLOTRON.md,
and against ever treating rwsdk as the biggest lever just because its function
count is large.

## Phases

### Phase 1 — finish the game code
Gate: all of `src/SB/**` written and semantically correct. Matching status per
function does not matter here; *completeness* does. A `NonMatching` function
that behaves right is portable. A missing function is not.

Worth doing as part of this phase, because it is nearly free while the code is
fresh and expensive later: note every place the code casts a raw asset buffer
to a struct. Those are the phase-4 risk sites (see **Asset caveats**).

**The hazard this gate exists to catch: stubs that match but lie.** Matching
rewards a function that produces the right instructions, and sometimes the
cheapest way to get there is a body that is semantically wrong. A live example
is `zNPCFXCutscenePickTable` in `zNPCFXCinematic.cpp`, currently a placeholder
returning `NULL` because the `g_cutmap` table and its 24 per-cutscene
`NCINEntry` tables (~24 KB of `.data`) do not exist in our source. It is
correctly flagged with a TODO and it lets three neighbouring functions reach
100% — but in a port it means cutscene effects silently never play.

These do not show up as missing functions and they do not show up in
`report.json`. Any function whose body was written to satisfy the diff rather
than to do the job needs finding before phase 2. Grepping for `TODO`,
`return NULL;` one-liners, and empty `{ }` bodies (`tools/stubs.py`) is the
starting point.

### Phase 2 — PC implementations of the interfaces
The codebase already has the seam. `src/SB/Core/x/` is platform-agnostic game
code; `src/SB/Core/gc/` is the GameCube implementation behind 25 `i*` headers.
A port is largely `src/SB/Core/pc/` written against those same headers.

This is a much better starting position than SM64 or OoT had — the abstraction
already exists and is already the boundary the game code respects.

| group | headers | notes |
|---|---|---|
| math | `iMath` `iMath3` `iColor` | likely portable near-verbatim; watch PPC `fres`/`frsqrte` estimate semantics |
| collision | `iCollide` `iCollideFast` | pure computation, should port directly |
| animation | `iAnim` `iAnimSKB` | mostly computation |
| system | `iSystem` `iTime` `iMemMgr` | thin; host clock + allocator |
| input | `iPad` | SDL_GameController |
| audio | `iSnd` | SDL_audio / OpenAL / miniaudio |
| files | `iFile` `isavegame` | host filesystem; saves become files on disk |
| rendering | `iModel` `iDraw` `iLight` `iEnv` `iFX` `iScrFX` `iMorph` `iParMgr` `iCutscene` | **the bulk of the work**, all of it via librw |
| video | `iFMV` | re-encoded FMVs, or stub |
| cert | `iTRC` | Nintendo TRC compliance; almost entirely stubbable |
| GC-only | `ngcrad3d` | GameCube radiosity; no PC counterpart, drop or reimplement |

`src/dolphin/` (26 subsystems) is replaced rather than ported:

- `os` → threads, allocation, timing
- `pad`, `si` → SDL input
- `dvd` → filesystem
- `card` → save files
- `ai`, `ax`, `dsp`, `ar` → host audio
- `gx`, `vi` → librw backend
- `mtx` → portable math
- `thp` → GameCube video, dropped
- `exi`, `eth`, `ip`, `upnp`, `hio`, `db`, `gd`, `lg`, `OdemuExi2`,
  `odenotstub`, `amcstubs` → debug/dev/network, dropped

`src/PowerPC_EABI_Support` and `src/runtime_libs` are deleted outright — the
port uses the host libc/libc++.

### Phase 3 — librw
BFBB is RenderWare 3.x. We cannot ship a decomp of Criterion middleware even if
we finish one, so rwsdk gets replaced, not completed.

The precedent is exact: **re3 / reVC** (GTA III, Vice City) are RenderWare games
ported to PC via **librw**, an open-source RW reimplementation. Same middleware
generation, same problem.

This is the phase with the most unknowns. It is also the reason phase 4 picks
Xbox assets.

### Phase 4 — Xbox assets
Use assets extracted from the **Xbox** release rather than the GameCube one.

This is the single highest-leverage decision in the plan, because it solves two
problems at once:

1. **Endianness.** Xbox is little-endian x86. The decompiled code constantly
   casts raw asset buffers straight into structs — placement-new over asset
   data, `(xEntAsset*)`, `(xDynAsset*)`, literal offset arithmetic like
   `stringBase + 0x2e2`. Against big-endian GameCube assets every one of those
   is a byte-order bug. Against Xbox assets they are not.
2. **librw format support.** librw's native-format support is strongest for
   PS2/Xbox/PC and weakest for GameCube. Xbox RenderWare models and textures
   are formats librw already understands; GameCube's are not. Choosing Xbox
   assets aligns the asset pipeline with the renderer we are adopting instead
   of fighting it.

Tooling exists — Industrial Park handles HIP/HOP across platforms.

## Asset caveats — what Xbox assets do NOT solve

Recording these now so nobody rediscovers them at phase 4.

**Pointer width is untouched.** Xbox is 32-bit x86. Asset-overlaid structs still
assume 4-byte pointers. On x86-64 every such struct changes size and layout.
Two options, and this is still open:
- build 32-bit — cheap, layouts line up almost exactly with Xbox assets, but
  caps the port's future;
- separate on-disk formats from in-memory structs — correct and invasive.

**The code is GameCube-derived; the assets would be Xbox-derived.** This is the
new risk the decision introduces, and it needs validating per asset type rather
than assumed. Our struct definitions were reverse-engineered from the CodeWarrior
GameCube binary. Xbox asset files serialize layouts produced by MSVC. Padding,
bitfield ordering and enum width can differ between those compilers. Anything
memory-mapped is exposed.

The split is roughly:
- *Gameplay / logical* assets (entity placement, cutscenes, dialog, config) are
  structurally shared across platforms and differ mainly by byte order — these
  are the ones our code casts directly, so this is where the layout risk lives;
- *Renderable* assets (models, textures) are genuinely platform-native and
  different — but that is fine, because librw wants the Xbox ones anyway.

**Version and content drift.** The decomp targets `GQPE78` (GameCube NTSC-U).
The Xbox release is a different SKU and may carry different revisions, fixes or
content. Do not assume asset IDs and contents correspond 1:1.

**The alternative we are not taking, and why to remember it.** Offline-convert
the *GameCube* assets to little-endian. That guarantees layouts match the
GC-derived code exactly, trading the librw format problem back in. If phase 4
hits layout mismatches that are worse than expected, this is the fallback, and
a hybrid is legitimate: GC-converted logical assets, Xbox renderable assets.

## Latent retail bugs, and the NON_MATCHING escape hatch

A category that is **not** a decomp error and still has to be fixed for a port:
retail code that reads uninitialised stack, where retail's own frame contents
happen to make the read harmless. We reproduce the code exactly, inherit the
read, and do not inherit the luck.

**Confirmed instance.** The Flying Dutchman boss level crashed with
`Invalid read from 0xbed60419, PC = 0x8004c540`, which is `lbz r8,0(r9)` in
`xStrTokBuffer`. `ZNPC_AnimTable_Dutchman` declares a 13-entry `ourAnims` with
no terminator; `NPCC_BuildStandardAnimTran` scans `while (ourAnims[i] != 0)`.
The scan always reads one word past the array, uses it to index
`g_strz_subbanim[23]`, and hands the resulting pointer to the tokenizer.

All three functions are faithful: the target's initialiser blob is 0x34 (13
words, no zero), the target's loop is the same `lwzx / cmpwi 0 / bne+`, and
`ZNPC_AnimTable_Dutchman` and `ZNPC_AnimTable_Prawn` are both **100% matching**.
The array sits at 24(r1)..72(r1) and `stmw r20,80(r1)` starts the saved
registers, so 76(r1) is a hole in retail's frame too. Retail simply had 0 there.

**Why matching harder does not fix it.** The value in that hole is whatever the
preceding execution left at that address, so it is a property of the whole
build, not of any one function. 39 of 44 `ZNPC_AnimTable_*` builders already
match, including the one that runs immediately before Dutchman, and it still
crashed. Only a 100% tree would inherit retail's stack history -- and a PC port
never reproduces PowerPC frame layout at all, so the port needs the real fix
regardless of what the percentage says.

**The escape hatch.** `configure.py` appends `-DNON_MATCHING` to `cflags_bfbb`
when configured with `--non-matching`. Source guarded by it is absent from the
matching build -- which is required, since adding a terminator changes codegen --
and present in every build that has to actually run, including the port.

`NPCC_ANIM_LIST_END` in `zNPCTypeCommon.h` is the first user: it expands to
`, 0` under `NON_MATCHING` and to nothing otherwise. Applied to the Dutchman
and Prawn lists, whose blobs go 0x34 -> 0x38 and 0x28 -> 0x2c in the playtest
build while both objects stay byte-identical in the matching build.

Verify any future use the same way: the matching `.o` must be byte-identical
before and after, and `main.dol` must stay 306526d90b48e99894c3138f5fc8f2716d9fecf6.

Use this sparingly. It is for defects that are provably retail's, where the
matching build must not change. It is not a way to avoid fixing our own bugs.

## Other things that will bite

**Strict aliasing.** The source is full of `*(U32*)&someFloat` casts that exist
precisely because they made CodeWarrior emit the right instructions. Modern
GCC/Clang at `-O2` will miscompile some of it. `-fno-strict-aliasing` is
mandatory, not optional.

**Floating point divergence.** GameCube PPC has paired singles, fused
multiply-add, and `fres`/`frsqrte` estimate instructions with defined but
non-IEEE precision. Physics and gameplay can drift subtly. Usually tolerable;
occasionally the cause of a bug that looks like a logic error and is not.

**CodeWarrior-isms in the source.** `__declspec(weak)`, placement-new over asset
buffers, and the small-data-area (`sdata`/`sdata2`) assumptions are all things
the port's toolchain has to tolerate or have removed.

## Distribution

Code only. The port requires the user to supply their own copy and extracts
assets at first run — the Ship of Harkinian model. RenderWare and Bink are both
proprietary: librw replaces the first, and the FMVs must be re-encoded to a free
format (or the port ships without them) because Bink cannot be redistributed.

## First concrete step, when we get there

Before committing to phase 4, spike it: take one HIP/HOP archive from the Xbox
release, parse a handful of *logical* asset types with our GC-derived struct
definitions, and check the fields land where we expect. That single experiment
resolves the largest open question in this plan — whether GC-derived structs can
read Xbox-serialized data — and it can be done with a standalone tool long
before any of the porting work starts.
