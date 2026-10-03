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

## Matching data and the NON_MATCHING escape hatch

The Flying Dutchman source build previously crashed in `xStrTokBuffer` after
`NPCC_BuildStandardAnimTran` read past an unterminated animation list. An earlier
analysis called this a latent retail bug because the animation-table function
matched. The initializer data disproves that conclusion.

Retail's 0x34-byte template, referenced by `ZNPC_AnimTable_Dutchman`, contains
`1, 11, 4, 5, 6, 12, 13, 14, 16, 17, 18, 19, 0`: twelve animation indices and
an explicit zero terminator. The previous source contained thirteen nonzero
entries, included an extra Taunt01 index (7), and placed Death01 (11) later.
The function's instructions matched while its copied data did not. The source
now uses the retail list, including its terminator, in both matching and
`NON_MATCHING` builds. It remains thirteen entries in both modes.

For a verified retail defect that must be repaired in a port, `configure.py`
appends `-DNON_MATCHING` to `cflags_bfbb` when configured with `--non-matching`.
`NPCC_ANIM_LIST_END` in `zNPCTypeCommon.h` still expands to `, 0` under that
flag and to nothing otherwise. The existing Prawn workaround retains that
behavior; correcting Dutchman's initializer does not change the macro or its
other users.

Before adding such an exception, inspect the referenced retail data as well
as function instructions. A 100% function score does not establish that its
initializer data matches or that a source-build bug exists in retail. Verify
matching-build code and data, and retain retail DOL SHA1
`306526d90b48e99894c3138f5fc8f2716d9fecf6` when validating the normal link.
Use the exception for defects established in retail, not as a substitute for
correcting a reconstruction error.

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
