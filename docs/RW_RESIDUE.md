# RenderWare residue: why the last 50 RW functions do not match

This document lists, for every RenderWare SDK function in the bfbb GameCube
decomp that is below 100%, what exactly differs from retail and what causes
it. The aim is to separate three kinds of residue:

- residue that **source changes can still fix**;
- residue that **needs compiler work**, either a different clause in the
  patched `GC/2.0p1a` or a behaviour that only a later archived compiler has;
- residue whose cause is **known but has no lever** under any available
  compiler (mostly register-allocator colour order).

## How this was produced

1. **Classification.** `tools/residue.py` compiles every unit that has a
   non-matching RW function. It uses the unit's own compiler and then GC/2.0p1,
   2.5, 2.6 and 2.7 from the same source, and diffs each against the target
   with objdiff. The shape of the project-compiler diff is one of:
   - **REG**: identical instructions apart from register numbers;
   - **SCHED**: the same multiset of instructions in a different order;
   - **COUNT**: a different instruction count;
   - **OPS**: the same count but different opcodes.

   The "matches under" column lists the alternative compilers that give 100%
   from the unchanged source.
2. **Per-function experiments.** Each function was reduced to a small C repro
   and compiled with `tools/regalloc/cc.py` under GC/2.0p1a, 2.0p1, 2.5, 2.6 and
   2.7. Some were also compiled with 1.1–1.3.2, 2.0 and 3.0a. Where a clause of
   the alias patch was suspected, it was tested with **ablated copies of
   GC/2.0p1a**:
   - per-clause predicate stubs (`xor eax,eax; ret`);
   - dispatch entries restored to the stock 2.0p1 handlers in process memory;
   - a debugger-driven *conditional* ablation that skips clause V only for
     stores of a given kind (`cablate.py --mode vtmp|vdecl|vfr`).

   The real compiler binaries were never modified.
3. **Register-allocator capture.** REG residues were captured with
   `tools/regalloc/diag.py`. It records the interference graph, the simplify
   order, the final colours and the creation phase of every `@NNN` temp. The
   colouring was then replayed exactly (`replay exact` on every function
   below), and the solver looked for the virtual-register rank moves that would
   give retail's colours on the same graph.

The working notes and repro files are in the session scratchpad
(`residue_sched.md`, `residue_count.md`, `residue_reg.md`, `part_mfx.md`,
`part_*.md`, and the `drv/ col/ misc/ wld/ inst/ mfx/ e/ cse/` repro
directories). None of that is in the repo. This document is the durable
summary.

### Current totals

Measured on the verify worktree (`C:\Projects\bfbb-verify`, built at the
latest verified state) from `build/GQPE78/report.json` and a fresh
`residue.py` run:

| measure | value |
|---|---|
| RW functions matched | **989 / 1039** (95.19%) |
| RW units complete (linked) | **90 / 120** |
| RW code matched | 320,364 / 371,428 bytes (86.25%); the 50 residue functions are the whole 51,064-byte gap |
| RW data matched | 11,764 / 11,764 bytes (100%) |
| non-matching functions | 50, in 29 units |
| residue shapes | REG 18, SCHED 17, COUNT 13, OPS 2 |
| match 100% under some other archived compiler from the same source | 12 / 50 (none of the 12 units matches whole under that compiler) |

The 30th incomplete unit is `rtslerp`. It is 100% (code and data) under its
GC/2.0p1 override but is still configured `NonMatching`. This document did not
investigate why.

**Compiler overrides already in `configure.py`** (all other RW units use the
patched GC/2.0p1a):

| unit | compiler | why (rule below) |
|---|---|---|
| `stdkey` | GC/2.0p1 | R1d + R2 (pun unions) |
| `rtslerp` | GC/2.0p1 | R2 (E3n + A on pun unions) |
| `ptankgcncallbacks` | GC/2.0p1 | E3n + W on a frame aggregate |
| `ptankgcnrender` | GC/2.0p1 | R1t (V on the `fctiwz` temp) |
| `world/pipe/p2/gcn/setup` (`MatFunc1`) | GC/2.0p1 | clause W on the int→float frame round trip |
| `multiTexGcnData` | GC/2.5 | R3 (const-pointer LICM) |

These units are 100%. They are evidence for the model below and are not in
the residue table.

**Fixed since the residue notes were written, so not in the table:**
`_rwPalQuantResolvePalette`, `_rpPTankAtomicCreateCustom`, `WriteHeaders`,
`_rwGCNTriStripGetStats`, `IndexDataCreateRemapped`,
`RpMorphTargetCalcBoundingSphere` and `_rwGCNDisplayListGetStride`. All of them
turned out to be source shape: inline nesting, `const` locals, `p[i+k]` instead
of `p++`, or a named temp.

---

## 1. Compiler behaviour model

### Background: the 2.0p1a alias patch

GC/2.0p1a is GC/2.0p1 plus C predicates injected into two dispatch tables and
one call site. The source is `AliasPatch.c`; the injection is done by
`tools/patch_compiler.py`; `docs/DUPLOTRON.md` has the history.

| site | entries | clauses |
|---|---|---|
| value-numbering (VN) store kill `0x5bd068` → `0x511a30` | 0 (whole) | **V** (a store kills cached small-static values, i.e. `.sdata2` literals), **F** (a store to a whole plain static gets a fresh value number, so the next read reloads instead of forwarding) |
| | 1 (subrange) | **S** (VN half: F for subranges of statics of at most 8 bytes) |
| scheduler / CodeMotion may-alias `0x5bd0bc` → `0x511fc0` | 0 whole×whole | E3n, W, C/C+, then **A** (differing opcodes, both at most 4 bytes, one side static) or **B** |
| | 1 whole×subrange | W, C+, B |
| | 3 subrange×whole | E3n, C+, B |
| | 4 subrange×subrange | **S** only (two subranges of statics of at most 8 bytes: the same object always aliases; different objects alias under the A/B tests) |
| LICM `0x56f472` | call to `isloopinvariant` | a whole static read is never loop-invariant (this replaced clause H) |

- **E3n**: a store to a declared frame object (`Object+0x18 != 0`) may not be
  passed by a later small-static or literal load. Stores into `const` locals
  (flag 0x40) are exempt, which is the documented "const lever".
- **W**: the write-after-read half. A plain store to a declared frame local may
  not pass an earlier `lfs`/`lfd` literal load, and the edge carries the load's
  latency.

The clause map above comes from disassembling the linked blob
(`tools/aliaspatch_blob.py`). Two agents did this independently and got the
same map. It shows E3n being consulted on entry 0 as well as entry 3. That
supersedes the older DUPLOTRON remark that "a whole-scalar store never reaches
clause E3n".

### The rules found in the RW residue

Each rule below states the observed behaviour, the evidence, and the residue
functions it accounts for. "Retail" means the compiler that built the retail
RW objects.

**R1: clause V over-kills cached literals on stores to frame objects.**
The VN entry-0 predicate (`0x60e508`) runs clause V's small-static kill walk
when the stored object's base-expression word is 5 (static) *or* `0x10005`
(frame object). Clause F only applies to 5. So after any store to a stack slot,
every cached `.sdata2` literal is dead and the next use reloads it. Stock
2.0p1, 2.5 and retail keep the literal in its register. The conditional
ablation `cablate.py` split this into two sub-cases.
- **R1t, store to a compiler temporary**, in practice the `stfd` of the
  `fctiwz` float→int conversion slot.
  - Repro `e/t3.c`: two `(int)(w*255.0f+0.5f)`. 2.0p1a emits 3 `lfs`; stock
    2.0p1 emits 2; 2.0p1a with `--mode vtmp` emits 2.
  - Repro `e/p1.c` (`c.r = (u8)(255.0f*x)` into a GXColor): 6 `lfs` under
    2.0p1a and under `vdecl`; 2 under 2.0p1 and under `vtmp`.
  - The byte store into the GXColor goes through VN entry 1, so it is not the
    kill. The conversion temp is.
- **R1d, store to a declared pun-union local**: `_gf`/`_sf` of
  `rwIEEEGetFloatWord`/`rwIEEESetFloatWord`. Repro `e/s7.c`: 3 `lfs` under
  2.0p1a, 2 under 2.0p1, 2 with `vdecl`, 3 with `vtmp`.

Residue: `_rwGCLightsGlobalEnable`, `_rwGCLightsLocalEnable`,
`RwImageCreateResample` and part of `RwImageResample`. Already worked around by
overrides: `ptankgcnrender` (R1t) and `stdkey` (R1d).

**R2: E3n, with clause A as its fallback, pins literal loads below stores to
the IEEE pun unions.**
- `trace.py` on `stdkey`: the only scheduler clause that answers may-alias is
  E3n on entry 0, for the pairs `stw _sf` → `lfs @lit` and `stfs _gf` →
  `lfs @lit`.
- Killing E3n alone is not enough, because clause A then pins the same pairs.
- With `vdecl` plus E3n and A both killed, both `stdkey` functions are 100.
- The same pair is behind `RtQuatSetupSlerpCache`: only `abl_s0`, `v_E3nA` and
  stock 2.0p1 give 100.

Residue: `RpLightGetConeAngle`. Already worked around by overrides: `stdkey`,
`rtslerp`.

**R3: 2.5-style loop-invariant code motion (LICM) of loads through a pointer.**
GC/2.5, 2.6 and 2.7 hoist an invariant `p->field` load out of a loop. When the
loop contains a call, they do it only if `p` is a pointer-to-const. GC/2.0p1 and
2.0p1a never do it, with or without `const`. This is not caused by the patch:
2.0p1 behaves the same.
- Repro `e/k.c`: `for (i = 0; i < l->n; i++) s += g(l->m[i]);` with
  `const L *l`. 2.5 hoists both `l->n` and `l->m`; 2.0p1(a) reloads them every
  iteration; without `const`, 2.5 reloads them too.
- Repros `e/m1.c` (const) and `e/m2.c` (non-const): only m1 gives retail's
  `_rpMaterialListFindMaterialIndex`, and only under 2.5+.
- `misc/licm.c` repeats the experiment with `fc(L*, const L*)` and `fn(L*, L*)`.
  2.5 hoists in `fc` only.
- A call-free loop (`wld/gs.c`, `wld/gs3.c`) is also hoisted by 2.5 and never by
  2.0p1(a). Whether `const` is required in the call-free case was not tested
  separately.
- Why a hand-written hoist does not match: 2.5 hoists *after* induction-variable
  strength reduction, so the loop keeps retail's `base + offset-IV` form. A
  hand-written `T *base = p->field;` before the loop is strength-reduced into a
  second pointer IV instead. Measured examples: `UserDataListCopy` 96.84,
  `RpGeometryStreamGetSize` 95.64, `_rpMaterialListFindMaterialIndex` 98.57.

Residue: the three bamatlst loop functions, `RwImageCopy`,
`RpGeometryStreamGetSize`, `RpGeometryStreamWrite`, `ImageConvertDepth`,
`UserDataListCopy`, `_rpSkinMatrixBlendUpdate`, and part of `RwImageApplyMask`.
Already worked around by override: `multiTexGcnData`.

**R4: an X-form (reg+reg) load is not hoisted above callee-save spills in
2.0p1/2.0p1a.**
- 2.0p1 and 2.0p1a (and 2.0 and 1.3.2) schedule an `lwzx` after every earlier
  `stw rN,x(r1)` of the prologue. A D-form load from a non-r1 base is not held
  back this way.
- 2.5, 2.6, 2.7 and 3.0a3 hoist the `lwzx` to just after the spill of its own
  destination register. That one dependence is a write-after-read edge on the
  register, and it stays: `mfx/gs3.c` v3 waits for `stw r28` when the
  destination is r28.
- Repros: `col/w1.c` and `mfx/gs.c` (`lwzx`) against the controls `col/w2.c`
  and `mfx/gs2.c` (constant offset, so `lwz r31,0x8(r5)`), which hoist on every
  compiler.
- This is a scheduler dependence rule, not an alias clause; stock 2.0p1 and
  2.0p1a behave identically.

Residue: `CollisionDataStreamWrite`, `MultiTextureStreamGetSize`,
`MultiTextureStreamWrite`. All three are 100% under 2.5+ from the current
source.

**R5: clause F (store, then reload of a just-stored static) is required by
retail.**
- Every `x = Register(...); return x >= 0;` plugin-attach function needs
  retail's `stw r3,X; lwz r0,X; srwi r0,r0,31`.
- Every stock compiler (1.3.2, 2.0, 2.0p1, 2.5, 2.6, 2.7, 3.0a3) forwards r3
  instead. 13 non-volatile spellings were tried (`mfx/att.c`, `mfx/att2.c`,
  `e/g1-g3.c`).
- Examples: `_rpDlLightPluginAttach`, `_rpMultiTexturePluginAttach`,
  `_rpGameCubeMTPipePluginAttach`, `RpPTankPluginAttach`,
  `RpUserDataPluginAttach`, and `RpCollisionPluginAttach` (4 sites).
- These functions are all 100% today under 2.0p1a. They matter because they
  are why the R1/R3/R4 units cannot simply move to 2.0p1 or 2.5.
- Clause S (a static of at most 8 bytes is one alias unit) is needed for the
  same reason, by every module `Open`: `_rpLightOpen`, `_rpMaterialOpen`,
  `_rpGeometryOpen` and `_rwImageOpen`. Repro `wld/open.c`; padding the module
  info to 12 bytes (`wld/open12.c`) brings back stock behaviour.

**R6: E3n over-fires on stores into frame aggregates that are not pun unions.**
A `.sdata2` literal load is held below an earlier subrange store to a declared
frame aggregate. Retail and stock 2.0p1/2.5 hoist the literal.
- `MeshRenderEnvMap`: `stb` to `shiney.a`; repro `mfx/env.c`.
- `_rwDlCameraBeginUpdate`: `stw` to the `RwMatrix viewoffset.flags`; repro
  `drv/cam.c`.
- In the second case removing E3n alone is not enough, because stock compilers
  also hoist the `lfs` to other places. No compiler gives the target there.

This is DUPLOTRON's known "same operands, opposite answer" E3n class. The
documented narrowing attempts on store opcode, object size and partialness all
failed. The game also needs E3n: removing it on entry 3 costs -120/+5.

**R7: clause A over-fires on whole-scalar frame STORE vs static LOAD.**
- Clause A's recorded witness, `xShadowManager_Render`, is a frame *load*
  against a static *store*.
- In RW, retail lets a static load pass a whole 4-byte frame store:
  - `_rwDlNativeTextureWrite`: `lwz _RwGameCubeRasterExtOffset` passes
    `stw bytesLeftToWrite`.
  - `MeshRenderEnvMap`: `lwz MatFXMaterialDataOffset` passes `stw shiney`.
- Retail still pins a static load below a frame *struct-field* store
  (`_rwDlNativeTextureWrite`'s `nativeTexture.id = 6`).
- Repro `drv/e3n.c`. 2.0p1a pins both the scalar and the aggregate case.
  2.0p1, 2.5, 2.6 and 2.7 hoist both. No compiler gives retail's split.
- Proposed narrowing (untested tree-wide): clause A does not fire for
  whole-scalar frame store → later static load.

**R8: retail has ordering edges that 2.0p1a lacks.**
- **R8a, E3n has no entry-4 counterpart.**
  - The case: a store to a subrange of a frame aggregate (`mat.color`) against
    a later load of a subrange of a static of at most 8 bytes
    (`materialModule.globalsOffset`).
  - Retail pins the load; 2.0p1a lets it cross, because entry 4 only consults
    S, and S needs both sides static.
  - Repros `wld/mat.c` and `wld/mat2.c`. Making the offset a whole `static int`
    sends the pair to entry 3, where E3n gives exactly the target order.
  - Function: `RpMaterialStreamRead`.
- **R8b, a direct `@sda21` store to a small-static subrange is ordered before a
  later same-opcode store to a frame-aggregate subrange.**
  - An ablated compiler `v_SB4` (entry 4 falls through to clause B when S
    declines) makes `StalacTiteAlloc` 100%.
  - It breaks `StalacMiteAlloc`, `PipelineTopSort` and
    `RxLockedPipeAddFragment`, so retail's rule is narrower.
  - DUPLOTRON records clause B on entry 4 as -199 tree-wide.
- **R8c, a volatile store to a small static carries a latency edge to a later
  plain small-static store.**
  - With `volatile` on `_RwGCXFBCopy`, the RHS-first load order matches
    (`drv/vtail.c` `h()`).
  - The tail `stw _RwGCXFBCopy; li r0,1; stw _RwDlFSAATop` then needs an edge
    that no compiler adds for a volatile store (`vtail.c` `f()`).
  - Function: `_rwDlRasterShowRaster`.

**R9: retail's alias analysis is more precise than any available compiler
for some pointers.**
- **R9a**: retail treats a load through a pointer whose reaching definitions
  merge `&localV3d` with heap pointers (`&startVerts[i]`) as not aliasing other
  address-taken locals.
  - Every compiler tried (1.1, 1.2.5n, 1.3, 1.3.2r, 2.0, 2.0p1, 2.0p1a, 2.5,
    2.6, 2.7, 3.0a3, 3.0a5.2) answers may-alias.
  - Repros `col/s1..s9.c`. Loads hoist when the pointer is a single known
    object (k3, k5, g3/g5) and are blocked once a local is merged in (h3, h4,
    k1, k4). `#pragma opt_pointer_analysis on` is inert.
  - Functions: `AtomicForAllLineIntersections`,
    `AtomicForAllSphereIntersections`.
- **R9b**: retail treats a store to an address-taken (escaping) frame array as
  not aliasing a load through a call-returned pointer.
  - In `_rpGameCubeMTEffectSend`, `texMtx` is passed to
    `GXLoadTexMtxImm` (that is what makes it escape) and `matrix` is the result
    of `RwMatrixMultiply`. Repros `mfx/mtx*.c`: `n1` (array not address-taken)
    gives exactly the target under 2.0p1a, while `p1`/`p3`/`m1` are serialised
    on 2.0p1, 2.0p1a, 2.5 and 2.7.
  - Retail does serialise the structurally identical `GetTexFrameMatrix` loop
    just below. So this is not a blanket difference, and the discriminator is
    unknown.

**R10: unroller IV-offset folding.**
- In the 3× unrolled plane loop of `RwCameraFrustumTestSphere`, 2.0p1a folds
  the IV increments into displacements (`0x28`–`0x34`, one `addi r5,r5,0x3c`).
- 2.5 keeps retail's three `addi r5,r5,0x14`. Verified on the real function
  under 2.5.

### The inferred retail compiler ("compiler B")

No archived compiler explains all the RW evidence. The evidence is consistent
with one unarchived build that sits between 2.0p1 and 2.5:

> **Retail RW ≈ GC/2.0p1a's alias rules (F, S, W, E3n on scalar frame locals,
> the LICM static-read hook), minus clause V on frame-object stores (at least
> compiler temporaries), minus E3n/A on the IEEE pun unions, plus GC/2.5's
> const-pointer LICM (R3), X-form scheduling (R4) and unroller IV handling
> (R10).**

What each archived compiler lacks:
- **Stock 2.0p1** lacks F and S (it fails every attach and `Open` function) and
  lacks R3/R4/R10.
- **2.5** has R3/R4/R10 but lacks F/S/W/E3n. Examples: `_rpSkinCreate` drops
  100 → 98.84 under 2.5 on exactly an E3n/W literal hoist; `CollisionOpen`
  drops to 47.5 on S-style static hoisting; the attach functions fail on F.
- **2.0p1a** over-fires V, E3n and A, and lacks R3/R4/R10.

Experiments run against the model:

| experiment | RW result | game (SB, 224 units) result |
|---|---|---|
| clause V walk skipped for stores to **compiler temporaries** (`cablate --mode vtmp`, in-process, all 120 RW units) | **+4 / -0**: `_rwGCLightsGlobalEnable`, `_rwGCLightsLocalEnable`, `RwImageCreateResample`, `_rxPTankGameCubeRenderCallBack` (the last is currently matched only via the 2.0p1 override); gclights becomes 6/6 | **+0 / -0**, four partials up (`xFXStreakRender` 76.8 → 92.2, `NCIN_SleepyLamp_AR` 94.0 → 99.7, `xFXShineRender` 94.1 → 97.9, `xScrFXGlareRender` 62.4 → 65.0), consistent with DUPLOTRON's earlier measurement |
| clause V walk skipped for stores to **all frame objects** (`vfr`) | +4 / -0 | +0 / **-1** (`xcsCalcAnimMatrices` 100 → 96.98), so the stdkey union case needs a narrower discriminator than "all declared frame locals" |
| `vdecl` + kill(E3n) + kill(A) on `stdkey` | both functions 100 | not measured tree-wide (it kills E3n/A everywhere, which DUPLOTRON prices at well over -100) |
| `v_E3nA` / `abl_s0` on `rtslerp` | 100 | not measured tree-wide |
| `abl_E3nW` on `ptankgcncallbacks` | 4/4 | not measured tree-wide |
| `abl_W` on `setup` | 7/7 | not measured; W is +35/-0 on the game |
| `v_SB4` (entry 4: S then B) | `StalacTiteAlloc` 100, three p2define functions down | -199 per DUPLOTRON |

R3, R4 and R10 have **not** been reproduced as a patch to 2.0p1a. They are
behaviours of 2.5's optimiser and scheduler, not alias-table answers. The claim
that retail has them rests on: (a) these functions matching 100% under 2.5 from
unchanged source, and (b) the same units' other functions needing
2.0p1a-only clauses.

---

## 2. Register-numbering rules

Many residues below come down to which virtual register (vreg) a web gets,
because the graph colourer is deterministic given that number. The rules
(N1–N10) were measured with `wcap.py`/`diag.py` on test files
`cse/t/t1..t8.c` and on the real units. Replays reproduce the compiler's
colours exactly on 22 residue captures and 16 matched controls. See also
`tools/regalloc/README.md`.

| # | rule |
|---|---|
| N1 | vregs are numbered in this order: params (declaration order), then the function's own named locals, then compiler `@` objects, then objectless lowering temps. A dead variable can keep an empty slot. |
| N2 | own named locals: **reverse textual declaration order across all scopes** (the last declaration anywhere gets the lowest vreg). |
| N3 | `@` objects: reverse creation order. The `@` counter is file-global. |
| N4 | `@` creation order: (1) frontend inline expansion, call by call in source order; (1b) IroVars struct scalarisation; (2) IRO linearisation temps (`?:`, `&&`/`\|\|`); (3) the loop unroller; (4) FindLoops (strength-reduced walkers/IVs); (5) CSE; (6) IROUseDef web splitting. |
| N5 | inside one inline expansion (vreg ascending): modified params last-to-first, top-level locals in **declaration** order, block locals, return temp. This is the opposite of N2, so the same two locals rank oppositely out of line and when inlined. |
| N6 | across families (vreg ascending): own named locals < web-split temps < CSE < FindLoops < unroll < linearise < scalarised struct members < inline objects (the last inline call lowest) < objectless temps. |
| N7 | CSE temps are created in order of first occurrence, so ascending vreg is reverse program order. |
| N8 | a variable with several disjoint webs keeps its own object for the **first** web. Each later web becomes a new IROUseDef `@` object, ranked just above the named-local block and below every CSE temp. |
| N9 | `x = c ? a : b` (or an if/else that IRO turns into `?:`) gives `x` **no web of its own**: the value lives in an objectless temp. Assign-then-conditionally-reassign keeps `x`'s web. |
| N10 | colouring: simplify scans vregs ascending and pushes nodes of degree < 29 (GPR) / 32 (FPR). Survivors pop first in descending vreg. Select takes the lowest free volatile register, else the lowest already-claimed callee-saved register free of neighbours, else claims the next from r31/f31 down. |

A `hyp.py` test re-ranked whole families the other way: all `@` forward, each
family reversed, inline before IRO, and source after `@`. None of these fixes a
residue without breaking the matched controls. So the REG residues are **not** a
systematic numbering difference between our compiler build and retail's. They
are either source shape (a different set of webs) or a different interference
graph.

The one lever that has generalised: when the solver wants an objectless value
ranked inside the named block, store it first into an existing named local
whose later web carries the other value. This fixed
`_rwGCNDisplayListGetStride`.

---

## 3. Per-function table

Columns:
- **%**: the project compiler (GC/2.0p1a for every unit here).
- **alt**: the GC/2.0p1 / GC/2.5 scores from `residue.py`. 2.6 and 2.7 equal 2.5
  for every function.
- **status**:
  - *compiler-limited (X)*: matches 100% under compiler or ablation X from
    unchanged or plausible source, but X breaks other functions in the unit;
  - *compiler-missing*: no available compiler or ablation gives the target;
  - *allocator, rank move known*: the replay names the vreg move, but no
    source lever was found;
  - *graph differs*: no numbering of our interference graph gives retail;
  - *source unknown*: compiler-invariant, and the source shape was not found.

### 3.1 Compiler-dependent: R1 (clause V on temporaries)

#### `_rwGCLightsGlobalEnable`

- **Unit:** world/pipe/p2/gcn/gclights. **Size / % / class:** 544 b, 86.22,
  COUNT (`lfs` +2). **alt (2.0p1 / 2.5):** 100 / 100.
- **Exact difference:** this is the inlined
  `_rpGCHWLightingApplyDirectionalLight`, the GXColor `(u8)(255*c)` part.
  Target loads `@287` once into f3, then 3 `fmuls` / 3 `fctiwz` / 3 `stfd` back
  to back. Ours reloads `lfs @82` after each conversion-temp `stfd` (+2 `lfs`)
  and interleaves the `stb`s.
- **Rule:** R1t.
- **Evidence:** `ablate --off vn0` 100; `cablate vtmp` 100; `vdecl` 86.22.
- **Status:** compiler-limited (2.0p1, 2.5, or 2.0p1a-vtmp). The unit can't
  move: `_rpDlLightPluginAttach` needs F (R5) and is 94.75 under 2.0p1/2.5.
  Under vtmp the unit is 6/6.

#### `_rwGCLightsLocalEnable`

- **Unit:** gclights. **Size / % / class:** 980 b, 92.46, COUNT (`lfs` +2).
  **alt:** 100 / 100.
- **Exact difference:** the same shape as GlobalEnable, from
  `color.r/g/b = (RwUInt8)(255.0f*lightColor->x)`.
- **Rule:** R1t.
- **Evidence:** the same ablation results.
- **Status:** as GlobalEnable.

#### `RwImageCreateResample`

- **Unit:** src/baresamp. **Size / % / class:** 1156 b, 95.11, COUNT
  (`fmr` +2). **alt:** 100 / 78.9.
- **Exact difference:** inlined pixel averaging. Target keeps the scale literal
  in a register; ours reloads it after the `fctiwz` temp stores, which shows as
  `fmr`/`lfs` churn.
- **Rule:** R1t.
- **Evidence:** `--off vn0` 100; vtmp 100; vdecl 95.11.
- **Status:** compiler-limited (2.0p1, 2.0p1a-vtmp). Under 2.0p1 the unit is
  3/4; `RwImageResample` stays behind.

#### `RwImageResample`

- **Unit:** baresamp. **Size / % / class:** 468 b, 85.72, COUNT (`fmr` +1).
  **alt:** 97.82 / 57.81.
- **Exact difference, two parts:**
  - (a) the R1t literal reloads, as in CreateResample;
  - (b) a callee-saved GPR permutation under every compiler. Target: r31=nY,
    r30=nXDelta, r29=nYPos, r28=nYDelta, r27=dstWidth, r26=dstHeight, r25=src,
    r24=dst, r23=span ptr, r22=`nYPos+nYDelta-1`, r21=nX, r20=nXPos. Ours: r31
    is the `@164` yEnd CSE temp, then nX, nY, nXPos, ...
- **Rule:** R1t + allocator order (N6/N10).
- **Evidence:** 2.0p1 and vtmp remove (a) and give 97.82. `rcap` shows `@164`
  popping first. Declaration permutations, scoping nX/nXPos into the loop and
  naming the yEnd temp give 97.35–97.82.
- **Status:** (a) compiler-limited; (b) allocator, source not found. Needs nX,
  nXPos and yEnd to colour after the params.

### 3.2 Compiler-dependent: R3 (2.5 LICM through const pointers)

#### `_rpMaterialListFindMaterialIndex`

- **Unit:** world/bamatlst. **Size / % / class:** 56 b, 59.29, OPS.
  **alt:** 59.29 / 100.
- **Exact difference:** target hoists `lwz r0,0(r3)` (`matList->materials`) and
  uses an `add r5,r0,r5` pointer IV with a `beqlr` exit. Ours does
  `lwz r5,0(r3); lwzx r0,r5,r6` in the loop and a `mr r3,r7` exit.
- **Rule:** R3 (`const RpMaterialList*`).
- **Evidence:** `e/m1.c` is exact under 2.5+; `e/m2.c` (non-const) is never
  hoisted. Four hand-hoist spellings give 98.57, with the named local in r5 and
  the IV in r0, the reverse of retail.
- **Status:** compiler-limited (2.5). Under 2.5 the unit is 7/8;
  `_rpMaterialListStreamRead` stays behind.

#### `_rpMaterialListStreamGetSize`

- **Unit:** bamatlst. **Size / % / class:** 176 b, 51.77, COUNT (+3:
  `lwzx` +3, `stw` +4, `bl` -2, ...). **alt:** 51.77 / 100.
- **Exact difference:** target hoists `matList->numMaterials`
  (`lwz r29,4(r3)`) and `->materials` (`lwz r28,0(r3)`) out of a loop that calls
  `RpMaterialStreamGetSize`, then strength-reduces them into pointer IVs. Ours
  reloads `0(r28)`, does `lwzx` every iteration, and reloads the bound.
- **Rule:** R3 (across a call).
- **Evidence:** 2.5/2.6/2.7 100 from the unchanged source; 1.3.2, 2.0, 2.0p1
  and 2.0p1a 51.77. Repro `e/k.c`.
- **Status:** compiler-limited (2.5).

#### `_rpMaterialListStreamWrite`

- **Unit:** bamatlst. **Size / % / class:** 596 b, 75.85, COUNT (-8).
  **alt:** 75.85 / 100.
- **Exact difference:** the same loop, through an inlined GetSize.
- **Rule:** R3.
- **Evidence:** 2.5 100.
- **Status:** compiler-limited (2.5).

#### `RwImageCopy`

- **Unit:** src/baimage. **Size / % / class:** 256 b, 92.48, OPS.
  **alt:** 92.48 / 100.
- **Exact difference:** inlined `ImageStraightCopy`. Target hoists
  `sourceImage->stride` (`lwz r29,0x10(r31)`) out of the row-memcpy loop and
  reloads only `destImage->stride`. Ours reloads both.
- **Rule:** R3.
- **Evidence:** 2.5/2.6/2.7 100.
- **Status:** compiler-limited (2.5). Under 2.5 baimage is still 14/18
  (`RwImageSetGamma` 60.34, `_rwImageOpen` 93.6).

#### `RpGeometryStreamGetSize`

- **Unit:** world/bageomet. **Size / % / class:** 252 b, 92.94, SCHED.
  **alt:** 92.94 / 100.
- **Exact difference:** inlined `GeometryStreamGetSizeActual` morph-target loop.
  Target hoists `lwz r4,0x5c(r31)` (`geometry->morphTarget`) to the preheader
  and uses `add r7,r4,r6`. Ours reloads `lwz r0,0x5c(r31)` at the loop top
  (the `bdnz` target).
- **Rule:** R3 (a call-free loop).
- **Evidence:** `wld/gs.c`, `wld/gs3.c`. A manual hoist gives 95.64 (pointer
  walk instead of base+offset); a per-iteration local gives 93.25.
- **Status:** compiler-limited (2.5). bageomet under 2.5 is 15/20.

#### `RpGeometryStreamWrite`

- **Unit:** bageomet. **Size / % / class:** 1228 b, 94.41, COUNT (`lwz` +1).
  **alt:** 94.41 / 99.59.
- **Exact difference:** target hoists `geometry->morphTarget`
  (`lwz r4,0x5c(r26)`) above the per-morph-target loop; ours reloads it every
  iteration.
- **Rule:** R3.
- **Evidence:** under 2.5, 99.59, with only loop-local GPR colour order left
  (`i` r30 vs r29, `morphTarget` r30 vs r31).
- **Status:** compiler-limited (2.5) plus an allocator residue under 2.5. No
  compiler gives 100.

#### `ImageConvertDepth`

- **Unit:** baimage. **Size / % / class:** 604 b, 92.31, COUNT (`mr` +1).
  **alt:** 92.30 / 99.82.
- **Exact difference:** target hoists `srcImage->stride` (`lwz r27,0x10(r4)`)
  out of the row loops; ours reloads `lwz r3,0x10(r28)`. The `mr` delta is the
  knock-on coalescing.
- **Rule:** R3.
- **Evidence:** under 2.5, 99.82, with only the order of three entry loads left
  (target `lwz r8,0x18(r4); lwz r29,0x14(r4); lwz r30,0x14(r28)`; ours loads the
  dest field first). That remainder is probably statement order, which was not
  tested on a writable copy.
- **Status:** compiler-limited (2.5) plus probable source order.

#### `UserDataListCopy`

- **Unit:** plugin/userdata/rpusrdat. **Size / % / class:** 716 b, 96.51,
  SCHED. **alt:** 96.51 / 98.52.
- **Exact difference:** target hoists `lwz r31,0x4(r28)`
  (`srcList->userData`) to the preheader and uses `add r29,r31,r28`. Ours
  reloads `lwz r0,0x4(r27)` each iteration, and the colouring shifts from
  there.
- **Rule:** R3 (a `const T*` param, a loop with calls).
- **Evidence:** `misc/licm.c` (fc vs fn). Under 2.5 the remainder is pure
  register numbering (98.52). A manual hoist gives 96.84 (it creates a second
  IV).
- **Status:** compiler-limited (2.5) plus an allocator residue. Under 2.5 the
  unit is 15/17 (`RpUserDataPluginAttach` needs F).

#### `_rpSkinMatrixBlendUpdate`

- **Unit:** plugin/skin2/gcn/skingcn. **Size / % / class:** 492 b, 92.07,
  COUNT (-5: `addi` -2, `mr` -3). **alt:** 92.07 / 92.07.
- **Exact difference:** first loop. Target loads `usedBoneList` once and copies
  it twice (`lwz r31,8(r27); mr r25,r31; mr r27,r31`), which gives three pointer
  IVs. It also hoists `invBoneToSkinMat`/`numUsedBones` and rematerialises
  `li r0,0` in the loop. Ours has a single IV r24 and hoists `li r31,0` into a
  callee-saved register.
- **Rule:** R3 on a different source shape. Retail CSE'd and hoisted three
  const loads through `skin->boneData`, and each copy became its own IV.
- **Evidence:** rewriting both loops as `skin->boneData.usedBoneList[i]`
  (`e/sk_s4.c`, `i` declared first) gives **100 under GC/2.5**. The same rewrite
  gives 87.19 under 2.0p1a. The current source is 91.99–92.07 everywhere.
- **Status:** compiler-limited (2.5 with the s4 source). Not applied:
  `_rpSkinCreate` drops to 98.84 under 2.5 (it needs E3n/W), and BlendBody and
  RenderCallback fail under every compiler.

#### `RwImageApplyMask`

- **Unit:** baimage. **Size / % / class:** 1340 b, 96.49, SCHED.
  **alt:** 96.49 / 99.09.
- **Exact difference, two parts:**
  - (1) target hoists `mask->depth` (`lwz r5,0xc(r29)`) and `mask->stride`
    (`lwz r3,0x10(r29)`) above the row loop; ours reloads both;
  - (2) a callee-saved r30/r31 swap under every compiler. Target:
    `image->width` and the inlined `imagePalette` are in r31 and `tempImage` in
    r30. Ours is the reverse.
- **Rule:** R3 + colouring tie (N10).
- **Evidence:** 2.5 fixes (1) and leaves 60 rows, all of them the swap. A manual
  hoist gives 98.57–98.65. Seven declaration and expression variants did not
  move (2).
- **Status:** (1) compiler-limited (2.5); (2) allocator, source not found.

### 3.3 Compiler-dependent: R4 (X-form load vs callee-save spills)

#### `CollisionDataStreamWrite`

- **Unit:** plugin/collis/rpcollis. **Size / % / class:** 224 b, 89.29, SCHED.
  **alt:** 89.29 / 100.
- **Exact difference:** target is
  `stw r31,0x1c(r1); lwzx r31,r5,r6; stw r30,0x18(r1); cmplwi r31,0; stw r29,0x14(r1); mr r29,r3`.
  Ours does all three `stw` and the `mr` first, then the `lwzx`. Only the
  `RPCOLLISIONDATA(object, offset)` load moves.
- **Rule:** R4.
- **Evidence:** `col/w1.c` vs `col/w2.c`.
- **Status:** compiler-limited (2.5+). Under 2.5 rpcollis is 7/9:
  `CollisionOpen` 47.5 (S) and `RpCollisionPluginAttach` 85.6 (F).

#### `MultiTextureStreamGetSize`

- **Unit:** plugin/matfx/multiTex. **Size / % / class:** 160 b, 85.0, SCHED.
  **alt:** 85.0 / 100.
- **Exact difference:** target is
  `stw r31; stw r30; lwzx r30,r3,r4; stw r29; li r29,0; cmplwi r30,0; stw r28`.
  Ours is all four `stw`, `li r29,0`, then `lwzx`.
- **Rule:** R4.
- **Evidence:** `mfx/gs.c`, `mfx/gs2.c`, `mfx/gs3.c`. Four source shapes give
  the same placement.
- **Status:** compiler-limited (2.5+). Under 2.5 multiTex is 16/17
  (`_rpMultiTexturePluginAttach` needs F).

#### `MultiTextureStreamWrite`

- **Unit:** multiTex. **Size / % / class:** 324 b, 92.59, SCHED.
  **alt:** 92.59 / 100.
- **Exact difference:** the same, with `lwzx r30,r5,r6` between `stw r30` and
  `stw r29` in the target.
- **Rule:** R4.
- **Evidence:** as GetSize.
- **Status:** as GetSize.

### 3.4 Compiler-dependent: R2/R6/R7 (E3n and A over-fire)

#### `RpLightGetConeAngle`

- **Unit:** world/balight. **Size / % / class:** 556 b, 84.50, SCHED.
  **alt:** 100 / 100.
- **Exact difference:** last `else` arm, after `bl _rwSqrt`. Target interleaves
  the `rwACOS` polynomial literals (`lfs @346`, `@345`, `@344`, ...) with the
  bit cast `stfs f1,0xc(r1)` / `lwz r0,0xc(r1)` / `clrrwi` / `stw r0,0x8(r1)`.
  Ours holds every literal load below the whole chain. The tail colouring then
  shifts (f11/f12 instead of f6/f7).
- **Rule:** R2 (E3n + A on the `gf_u`/`sf_u` pun unions).
- **Evidence:** `wld/acos.c`, `wld/acos2.c` (the integer `stw` to the union
  alone triggers it). The const lever
  (`const RwReal sv = s; const RwInt32 iv = *(const RwInt32*)&sv & 0xfffff000; ...`,
  `wld/vC.c`) gives 99.748. The rest is an f6/f7 swap from the pointer-cast
  form, which also costs 2.0p1 its 100. `const` union initialisers are rejected
  in C. 13 more variants were tried.
- **Status:** compiler-limited (2.0p1 or 2.5). Under 2.0p1 balight is 12/13
  (`_rpLightOpen` needs S).

#### `MeshRenderEnvMap`

- **Unit:** plugin/matfx/gcn/effectPipesGcn. **Size / % / class:** 1220 b,
  97.61, SCHED. **alt:** 99.30 / 99.30.
- **Exact difference, two mismatches:**
  - (1) prologue. Target is `lwz r7,@696` (shiney initialiser) first,
    `lwz r4,0x8(r25)`, `lwz r3,MatFXMaterialDataOffset`, `stw r7,0x18(r1)`,
    `mr r28,r6`, `lwzx r29,r4,r3`. Ours loads the literal third and puts
    `stw shiney` after the `lwzx`.
  - (2) inside `if (coef < 1.0f)`: target is
    `lfs f0,@767; li r0,0xff; stb r0,0x1b(r1)`; ours puts the `lfs` after the
    `stb`.
  - Under stock compilers the only remaining diff is the inlined
    `ProjectionMatrixInit` store pair (`stfs 0xac` / `lfs @0.5` /
    `stfs 0xb0`), which needs clause W.
- **Rule:** (1) R7: clause A, a whole frame store against the
  `MatFXMaterialDataOffset` static load. (2) R6: E3n on the `stb` to `shiney.a`.
  Retail also has W.
- **Evidence:** `mfx/env4.c` b0 vs b3 (replacing the static offset with
  `*poff` makes 2.0p1a equal 2.0p1); `mfx/env.c` env1. Three source variants
  were worse or inert.
- **Status:** compiler-missing. It needs W without A-on-frame-store and without
  E3n on `stb`; no single compiler has that. effectPipesGcn is 12/17 under
  2.0p1/2.5.

#### `_rwDlCameraBeginUpdate`

- **Unit:** driver/gcn/dldevice. **Size / % / class:** 1464 b, 98.88, SCHED.
  **alt:** 97.42 / 97.42.
- **Exact difference:** after `bl GXSetCurrentGXThread`. Target is
  `stw r31,dgGGlobals; li r0,0; lis r4,2; lis r3,Inv@ha; stw r0,0x14(r1); addi r0,r4,3`.
  Ours is
  `li r5,0; lis r4,2; stw r31,dgGGlobals; lis r3; addi r0,r4,3; stw r5,0x14(r1)`.
- **Rule:** R6.
  - The E3n edge from `stw 0,0x14(r1)` (`viewoffset.flags`, a subrange of a
    64-byte `RwMatrix` local) to the later `lfs` literals gives each `lfs` two
    predecessors.
  - So `stw dg` releases nothing (release count 0), and `li` wins the pick on
    release count 1.
  - Without that edge, `stw dg` has release count 2 and issues first.
- **Evidence:** `drv/cam.c` reproduces ours. `cam_noflag.c` and `cam_flagk.c`
  issue `stw dg` first; `cam_nolit.c` issues `li` first. Stock compilers also
  hoist the `lfs` elsewhere, so none gives the target. 11 source variants were
  tried; the best other was 98.743.
- **Status:** compiler-missing (E3n narrower than any tested gate).

#### `_rwDlNativeTextureWrite`

- **Unit:** driver/gcn/dltexdic. **Size / % / class:** 732 b, 97.73, SCHED.
  **alt:** 89.72 / 89.72.
- **Exact difference:** target is
  `lwz r6,0(r31); rlwimi; lwz r5,_RwGameCubeRasterExtOffset; stw r7,0x8(r1); add r30,r6,r5; stw r0,0x10(r1)`.
  Ours pins the ExtOffset load below `stw 0x8(r1)` (`bytesLeftToWrite -= ...`,
  an address-taken 4-byte scalar). At the first site the target *keeps*
  `lwz _RwGameCubeTextureExtOffset` below the `nativeTexture.id = 6` field
  store. Stock hoists it there, wrongly, which is why stock scores 89.7.
- **Rule:** R7 (clause A on a whole-scalar frame store → static load); retail
  keeps the struct-field case.
- **Evidence:** `drv/e3n.c` scal/aggr. 2.0p1a pins both; stock hoists both.
  Four source orders: 93.6–97.6.
- **Status:** compiler-missing (the proposed narrowing of A is not built or
  measured).

### 3.5 Compiler-missing: R8 (edges retail has, 2.0p1a lacks)

#### `RpMaterialStreamRead`

- **Unit:** world/bamateri. **Size / % / class:** 1008 b, 99.15, SCHED.
  **alt:** 99.21 / 99.21.
- **Exact difference:** after `bl RwMemNative32`. Target is
  `lwz r0,0xc(r1); stw r0,0x2c(r1)` (`mat.color = tmp`), then
  `lwz r3,RwEngineInstance; lwz r0,materialModule`. 2.0p1a hoists
  `lwz r0,materialModule@sda21` above the `stw`; stock hoists
  `RwEngineInstance` instead.
- **Rule:** R8a.
- **Evidence:** `wld/mat.c`, `wld/mat2.c`. The const lever does not apply
  because `mat` is written by RwStreamRead.
- **Status:** compiler-missing (an entry-4 E3n-style rule). It has not been
  priced tree-wide: DUPLOTRON's full E3n-on-entry-4 is +1/-192, so the rule
  would have to be much narrower.

#### `StalacTiteAlloc`

- **Unit:** src/pipe/p2/p2define. **Size / % / class:** 116 b, 90.69, SCHED.
  **alt:** 68.45 / 68.45.
- **Exact difference:** error block. Target is
  `add r0,r3,r4; lis r3,0x8000; stw r0,gMemoryLimits@sda21; li r0,1; addi r3,0x13; stw r0,0x8(r1)`.
  Ours is `add r5; li r0,1; lis; stw r5,gMemoryLimits; addi; stw r0,0x8(r1)`.
  The `li 1` for `_rwErrorCode.pluginID` rises above the static store, so the
  `add` cannot share r0.
- **Rule:** R8b.
- **Evidence:** `misc/tite.c`: every compiler gives ours and `v_SB4` gives the
  target. `v_SB4` costs StalacMiteAlloc (90.3), PipelineTopSort and
  RxLockedPipeAddFragment. Five source variants: at most 90.69.
- **Status:** compiler-missing (the retail rule is narrower than "B on
  entry 4").

#### `_rwDlRasterShowRaster`

- **Unit:** dldevice. **Size / % / class:** 1060 b, 99.89, SCHED.
  **alt:** 80.17 / 80.17.
- **Exact difference:** three sites of
  `_RwGCFrameQueue[_RwDlFrameNew].XFBCopy = _RwGCXFBCopy`. Target loads the RHS
  (`lwz r5,_RwGCXFBCopy`) before the volatile index `_RwDlFrameNew`; ours does
  the reverse.
- **Rule:** load order (volatile accesses stay in tree order, RHS first), then
  R8c.
  - Making `_RwGCXFBCopy` volatile fixes all six rows.
  - It exposes one new tail row: target `stw _RwGCXFBCopy; li r0,1`, ours
    `li r0,1; stw`.
- **Evidence:** `drv/vtail.c` `h()` and `f()`. objdiff scores the volatile
  variant lower (99.245, 2 inserted rows) than the current 99.887, so it is not
  applied.
- **Status:** compiler-missing. `volatile` is the likely original source and is
  the source change to pair with an R8c patch.

### 3.6 Compiler-missing: R9 (retail alias precision)

#### `AtomicForAllSphereIntersections`

- **Unit:** plugin/collis/ctgeom. **Size / % / class:** 936 b, 98.24, SCHED.
  **alt:** 98.24 / 98.24.
- **Exact difference:** after `RtIntersectionSphereTriangle` succeeds, the
  target loads `*v0` (`lwz r5/r3/r0,0/4/8(r30)`) before both the `distance`
  store and the `collTriangle.point` stores, and puts `stfs f1,0x8(r1)` last.
  Ours stores `distance` immediately and then interleaves
  `lwz,lwz,stw,stw,lwz,stw`.
- **Rule:** R9a. `v0` merges `&interpV0` with `&startVerts[idx]`.
- **Evidence:** `col/s1..s9.c` across 12 compilers. Five source variants: at
  most equal. Variant (1), which copies before `distance *=` (the order the
  sibling Geometry function uses), is the likely original if R9a is ever
  patched.
- **Status:** compiler-missing.

#### `AtomicForAllLineIntersections`

- **Unit:** ctgeom. **Size / % / class:** 1432 b, 98.30, SCHED.
  **alt:** 98.30 / 95.78.
- **Exact difference:** in `RpCollisTriangleNormalMacro` after `_rwInvSqrt`,
  the target hoists the `collTriangle.point = *v0` loads above the three
  `collTriangle.normal` `stfs`. Ours stores the normal first.
- **Rule:** R9a.
- **Evidence:** as Sphere. Moving the copy before the macro gives 88.17 (the
  target loads after the call).
- **Status:** compiler-missing.

#### `_rpGameCubeMTEffectSend`

- **Unit:** plugin/matfx/gcn/multiTexGcnPipe. **Size / % / class:** 1804 b,
  95.85, SCHED. **alt:** 96.15 / 96.15.
- **Exact difference, two parts:**
  - (a) `ENVMTX` block. Target loads all six `matrix->` fields
    (`lfs f2,0(r6); lfs f1,0x10(r6); lfs f0,0x20(r6); ...`) before the first
    `stfs f5,0x160(r1)`. Ours serialises load/`fmuls`/`stfs` per element.
  - (b) callee-saved permutation. Target r29=effect, r30=coordMap, r31=config;
    ours r30/r31/r29, then r26/r28/r27.
- **Rule:** (a) R9b; (b) allocator.
- **Evidence:** (a) `mfx/mtx*.c` n1 vs m1/p1/p3, and 8 source variants (best
  95.80). (b) declaration moves are inert.
- **Status:** (a) compiler-missing; (b) allocator, not diagnosed with `diag.py`.

### 3.7 Unroller / loop shape

#### `RwCameraFrustumTestSphere`

- **Unit:** src/bacamera. **Size / % / class:** 244 b, 90.77, COUNT
  (`addi` -2). **alt:** 90.77 / 94.13.
- **Exact difference:** 3× unrolled plane loop. Target advances the plane
  pointer after every copy (`addi r5,r5,0x14` ×3, displacements 0x0/0x14).
  Ours folds the increments into displacements 0x28–0x34 with one
  `addi r5,r5,0x3c`.
- **Rule:** R10, plus R3 once rewritten.
- **Evidence:** under 2.5 the increments match (94.13, only FP order left). A
  scratch rewrite that reads `sphere->center`/`-sphere->radius` in the loop
  (const, hoisted by R3) gives 99.26 under 2.5, with only `nDot` f2 vs f1 left.
- **Status:** compiler-limited (2.5) plus an FP colour residue. No compiler
  gives 100. bacamera is diagnosis-only.

#### `_rwGCNVtxFmtInstClr`

- **Unit:** world/pipe/p2/gcn/instance/geominst. **Size / % / class:** 2176 b,
  94.33, COUNT (`lwz` +1, `stw` +1).
- **Exact difference:** 8× unrolled RGB565 loop. Target computes p1..p7 = p0 +
  k*stride and advances the base after the last store
  (`sth r0,0(r3); add r3,r3,r7` at the bottom). Ours computes the next base
  (`add r3,r8,r7`) before the second store, which lengthens one live range and
  needs a fourth callee-saved register (r28: +1 `stw`, +1 `lwz`).
- **Rule:** unroller final-IV placement.
- **Evidence:** identical under 2.0, 2.0p1, 2.0p1a, 2.5 and 2.6. Byte-pointer
  dst 92.53, `mem +=` 92.53, indexed src 91.79, swapped increments 94.33.
- **Status:** source unknown.

### 3.8 Compiler-invariant structure (source shape not found)

#### `RxLockedPipeUnlock`

- **Unit:** p2define. **Size / % / class:** 2760 b, 98.06, COUNT
  (`bge` +1, `ble` -1, `mr` -1). **alt:** 97.41 / 97.41.
- **Exact difference:**
  - Max idiom. Target is `mr r24,r0 ... cmplw r7,r0; ble; mr r24,r7`. Ours is
    `cmplw r24,r0; bge; mr r24,r0`.
  - The depChase/topSort size sum is also associated differently
    (`mullw r10` first, `mulli r22,r10,0x24`).
- **Rule:** source form (the same under 1.3.2–2.7).
- **Evidence:** `size = (size > end) ? size : end` (`e/pd_tern.c`) reproduces
  the max branch exactly, but the function drops to 97.72. 36 sum permutations:
  at most 98.06.
- **Status:** source partly found (the ternary); the sum association was not
  found.

#### `_rpSkinRenderCallback`

- **Unit:** skingcn. **Size / % / class:** 1396 b, 96.14, COUNT
  (`addi` -2, `li` +2, `mr` -2).
- **Exact difference:** in both RLE inner loops, target keeps a **dead IV**
  `bone` (`lbz r17,0(r3); slwi r15,r17,6; addi r17,r17,1`, r17 otherwise unused)
  beside the strength-reduced pointer r15, and sets `j` as a copy of `slot`
  (`li r22,0; mr r19,r22`). Ours drops the dead IV and uses two `li 0`. Retail
  needs one more callee-saved register (`_savegpr_15` vs `_16`).
- **Rule:** dead-IV retention; not a compiler-version effect.
- **Evidence:** 96.11 under 1.3.2, 2.0, 2.0p1, 2.0p1a, 2.5 and 2.6. Repros
  `e/iv1-2.c`, `e/v1-6.c`, `e/w1-2.c`: no tested compiler keeps a dead IV. Eight
  variants: 92.9–96.49.
- **Status:** source unknown (a shape that keeps `bone` live through strength
  reduction).

#### `CalcMeshNBTs`

- **Unit:** multiTexGcnPipe. **Size / % / class:** 4340 b, 98.72, SCHED.
  **alt:** 98.72 / 95.73.
- **Exact difference:** only the inlined `CalcNBTSetup` `vtxFmt != NULL` block.
  Target issues every table literal (`@286`, `@288`) and `lbz r3,0xc(r5)`
  before the first `lbzx r0,r6,r12`, and builds the uv GQR before the norm
  table stores. Ours issues `lbzx Size[pos]`/`stb 0x44` early. GQR registers:
  target r10/r12/r7, ours r8/r9/r4.
- **Rule:** list-scheduler pick over a DAG of a different shape; not an alias
  clause (the same rows differ under every compiler).
- **Evidence:** all 720 hazard-preserving orders of the six statements: best
  99.081. Table declaration swap 98.710; GQR expression spelling 98.715.
- **Status:** source unknown (probably the register-var/asm interface of
  `CalcNBTSetup`).

### 3.9 REG: allocator, rank move known, no source lever

These are identical under 2.0p1 and 2.5 unless noted. "Move" means the vreg
re-ranking that would make the exact replay of our graph produce retail's
colours.

#### `_rwStringStreamFindAndRead`

- **Unit:** src/babintex. **Size / %:** 980 b, 99.33.
- **Exact difference (ours → retail):** String inline: `bytesToRead`
  (objectless v73) r26→r31; `baseString` @142 r31→r30; `nativeString` @144
  r30→r29; `length` @145 r29→r28; outer `string` r30→r29.
- **Rule:**
  - N9: `bytesToRead = c ? 64 : length` has no web of its own.
  - N4/N5: the String-inline `length` is a modified inline param.
- **Move needed:** `bytesToRead` must become an object web (55.5), *and* the
  String-inline `length` must drop below the whole Unicode inline (< 45.5).
- **Evidence:** the assign-then-reassign form gives the object web
  (99.33 → 99.53). The `length` move is impossible, because a modified inline
  param is always its own expansion's object. That form also drops
  RwTextureStreamRead to 99.20.
- **Status:** allocator, rank move known.

#### `RwTextureStreamRead`

- **Unit:** babintex. **Size / %:** 2400 b, 99.59.
- **Exact difference:** `mipmapState` v35 r30→r29 and `autoMipmapState` v34
  r29→r30. Also the first FindAndRead inline's @370 r25→r26 and @350 r27→r28.
- **Rule:** N2 (the DWARF declaration order ranks mipmapState higher), plus the
  FindAndRead blocker above.
- **Evidence:** swapping the declarations gives 99.49 and contradicts DWARF.
- **Status:** allocator; blocked on FindAndRead.

#### `RpGeometryStreamRead`

- **Unit:** bageomet. **Size / %:** 3620 b, 99.91.
- **Exact difference:** morph-target loop: `i` v43 r28→r27; FindLoops offset
  @733 v92 r29→r28.
- **Rule:** N2 vs N6: a named local can never outrank a FindLoops temp.
- **Evidence:** the DWARF-faithful block-scoped `i`s ("hl+blocki") give 99.917
  and leave only "`sizeTC` above @733". The no-variable `sizeTC` form gives
  99.07.
- **Status:** allocator, rank move known.

#### `_PropagateDependenciesAndKillDeadPaths`

- **Unit:** src/pipe/p2/p2dep. **Size / %:** 1296 b, 99.32.
- **Exact difference:** the `node->nodeDef` load (objectless v100, the base of
  every `iospec->` access) r28→r31, which pushes `pipeline`, `node` and `i` down
  one register.
- **Rule:** N1. `iospec = &node->nodeDef->io` is folded by IRO, so `iospec`'s
  slot is dead and the value is objectless.
- **Move needed:** v100 to 42.5..44.5.
- **Evidence:** declaration moves only move the dead slot (99.32/99.37). The
  fix needs a named nodeDef pointer, which is not in DWARF.
- **Status:** allocator, rank move known.

#### `WorldSectorStreamRead`

- **Unit:** world/babinwor. **Size / %:** 1736 b, 99.42.
- **Exact difference:** polygon unpack: the split web of `i` @216 v48 r4→r3,
  and the SR walker of `polygons` @200 v64 r3→r4.
- **Rule:** N8 + N4.
- **Move needed:** split-`i` above 64.5, or the walker below 47.5.
- **Evidence:** a named walker removes retail's `mr r4,r27` (97.6/98.6). Rat's
  version of this code differs (`rpPolygon poly; int b`).
- **Status:** allocator, rank move known; the source may be structurally
  different.

#### `TriStripFollow`

- **Unit:** world/bameshop. **Size / %:** 1264 b, 99.76.
- **Exact difference:** inline `result` of `TriStripEdgeIsAvailable` @482 v74
  r8→r9, and the `&&` linearise temps @497/@503 r9→r8.
- **Rule:** N4/N6: linearise temps are always newer than frontend inline
  objects.
- **Evidence:** Rat DWARF has **no** `TriStripEdge*` helpers (its locals are
  `nextIsLast`, `otherIsLast`, `turnResult`), so ours are a reconstruction
  artefact. The `explicit2` form fixes this pair but loses `mr r8,r9`.
- **Status:** source shape. Needs a DWARF-shaped rewrite, not yet achieved.

#### `RwImageSetGamma`

- **Unit:** baimage. **Size / %:** 340 b, 99.29. **alt (2.5):** 60.34.
- **Exact difference:** FPR: `scaled` v33 f3→f0 and its split web @616 f1→f0;
  the 255.0 loads f3/f1→f0; the 0.5 loads f0→f3/f1.
- **Rule:** N1/N2 + the lowering order of literal loads.
- **Move needed:** both `scaled` webs above the 0.5-load temps.
- **Evidence:** `quantize` (DWARF), `const` half/scale locals, declaration
  moves: inert. Dropping `scaled` fuses to `fmadds` (88.5).
- **Status:** allocator, rank move known.

#### `MatrixOrthoNormalize`

- **Unit:** src/plcore/bamatrix. **Size / %:** 1132 b, 99.52.
  **alt (2.0p1 and 2.5):** 98.11.
- **Exact difference:** FPR abs-dot block: CSE temps @126..@131 (v44..v49) and
  the abs-dot temps @125 v50 / @121 v54.
- **Rule:** N7.
- **Move needed:** @127 above @125, and @121 below all component temps.
- **Evidence:** dot-local shapes are inert; splitting dot/abs changes
  contraction (91.7). Rat's version differs (single `recip`, `vInner`/`vOuter`).
- **Status:** allocator, rank move known.

#### `ExtractNodes`

- **Unit:** driver/common/palquant. **Size / %:** 616 b, 99.51.
- **Exact difference:**
  - FPR outer body: `weight` v33 f1→f2, `recip` v32 f2→f1.
  - GPR: CSE `nodeIndex*4` @649 r6→r0, and the objectless `fctiwz` result load
    r0→r6.
  - The two self-inlined copies already match.
- **Rule:** N2 vs N5: the same declaration pair ranks oppositely out of line
  and inlined. Retail has the inlined order in all three copies.
- **Evidence:** swapping the declarations fixes the outer copy and breaks both
  inlined copies (96.98). A RepresentativeColor-style helper (it exists in Rat
  DWARF) changes the auto-inline depth (83.5).
- **Status:** allocator, rank move known.

#### `DlRasterTile`

- **Unit:** driver/gcn/dlraster. **Size / %:** 660 b, 99.67.
- **Exact difference:** 32-bit case: pixel load v111 r4→r0, against the offset
  temps v112..v114 and CSE @246.
- **Rule:** the lowering order of objectless temps.
- **Move needed:** v111 after v114.
- **Evidence:** 7 address/load/index spellings: inert or worse.
- **Status:** allocator, rank move known.

#### `_rpSkinBlendBody`

- **Unit:** skingcn. **Size / %:** 496 b, 99.80.
- **Exact difference:** the stack-array address v56 r12→r10, and the byte load
  v59 r10→r12.
- **Rule:** lowering order.
- **Move needed:** swap their creation order.
- **Evidence:** no lever found.
- **Status:** allocator, rank move known.

### 3.10 REG: interference graph differs

No renumbering of our interference graph reproduces retail, so the source
builds a different set of webs or interferences.

#### `CameraBuildPerspClipPlanes`

- **Unit:** bacamera. **Size / %:** 2020 b, 99.11.
- **Exact difference:** FPR, 29 webs. IroVars scalarisation temps @205..@213
  (vTmp, vTmp2, vRight, vUp, vCOP). `scale` f3→f29: retail keeps it live across
  calls.
- **Evidence:** no pop order of our graph works. Rat has 3 normalisations where
  ours has 4.
- **Status:** graph differs.

#### `CameraBuildParallelClipPlanes`

- **Unit:** bacamera. **Size / %:** 1304 b, 99.57. **alt (2.5):** 90.97.
- **Exact difference:** FPR: `farPlane` f6→f8, `height` f8→f7, CSE @368..@375.
- **Evidence:** no single or pair move works.
- **Status:** graph differs.

#### `_rwFrameListFindFrame`

- **Unit:** src/babinfrm. **Size / %:** 68 b, 97.65.
- **Exact difference:** `i` r6→r7, SR walker @36 r3→r6. Retail's walker
  interferes with r3 (frameList is still live at the walker init); ours does
  not.
- **Evidence:** DWARF has no `frames` local, but dropping it reloads every
  iteration (88.2). Pointer-walk forms give 98.1.
- **Status:** graph differs (preheader load order).

#### `_rwFreeListAllocReal`

- **Unit:** src/plcore/bamemory. **Size / %:** 432 b, 99.68.
- **Exact difference:** CSE @123 r6→r5, against objectless v63/v64 r5→r6.
- **Evidence:** no single or pair move works.
- **Status:** graph differs.

#### `VectorMultPoint`

- **Unit:** src/plcore/bavector. **Size / %:** 224 b, 94.82.
- **Exact difference:** FPR, 20/21 webs differ. Retail keeps the 12 matrix
  values in f31, f13..f1, x/y/z volatile, and the outputs in f30..f28.
- **Evidence:** no pop order of our graph works.
- **Status:** graph differs.

#### `VectorMultVector`

- **Unit:** bavector. **Size / %:** 152 b, 94.21.
- **Exact difference:** the same pattern as VectorMultPoint, with 16/18 webs
  differing.
- **Evidence:** as above.
- **Status:** graph differs.

#### `_rpMaterialListStreamRead`

- **Unit:** bamatlst. **Size / %:** 1508 b, 97.77.
- **Exact difference:** a callee-saved permutation. Target: matList r31,
  stream r30, matindex-IV r29, i r28, len r27, len*4 r26. Ours: the locals
  first (@295 IV r31, i r30, matindex r29, material r28), then the params
  (matList r27, stream r26). 24 webs shift.
- **Evidence:** the same 97.77 under 1.3.2–2.7. `perm.py`: 0 of the 720
  named-local orders work. In retail the params pop first.
- **Status:** graph/structure differs. This is the only bamatlst blocker under
  2.5.

---

## 4. Summary by root cause

| root cause | functions | bytes | fixable how |
|---|---|---|---|
| R1t: clause V kills literals on `fctiwz` temp stores | 3 (+ part of `RwImageResample`) | 2,680 | compiler: skip V on compiler temporaries (measured +4/-0 RW, 0/0 game) |
| R3: 2.5 LICM through const pointers | 9 (+ part of `RwImageApplyMask`) | 4,376 | compiler: port 2.5's LICM behaviour; 4 of the 9 also leave an allocator residue under 2.5 |
| R4: X-form load vs callee-save spills | 3 | 708 | compiler: 2.5's scheduler rule for `lwzx` |
| R2/R6/R7: E3n and A over-fire | 3 | 3,240 | compiler: narrower E3n/A; no working discriminator yet |
| R7: clause A on scalar frame store → static load | 1 | 732 | compiler: narrow clause A (not built) |
| R8: edges retail has, 2.0p1a lacks | 3 | 2,184 | compiler: narrow new edges (not found) |
| R9: retail alias precision | 3 | 4,172 | compiler: no archived compiler has it; discriminator unknown |
| unroller / loop shape | 2 | 2,420 | 2.5 for FrustumTestSphere (still 99.26); InstClr source unknown |
| compiler-invariant structure, source unknown | 3 | 8,496 | source (not found) |
| REG, rank move known, no lever | 11 | 14,540 | source (DWARF-shaped rewrites for TriStripFollow, RpGeometryStreamRead and WorldSectorStreamRead are the most promising) |
| REG, graph differs | 7 | 5,708 | source structure (Rat-shaped rewrites for CameraBuildPersp and MatrixOrthoNormalize) |
| mixed (R1t + allocator, R3 + allocator) | 2 | 1,808 | both |
| **total** | **50** | **51,064** | |

Roughly:
- 28 functions (20,144 bytes) need compiler behaviour that GC/2.0p1a does not
  have. These are R1–R10 above, counting `RwCameraFrustumTestSphere` and the two
  mixed rows. Seven of them also keep an allocator or ordering residue under the
  best compiler.
- 22 functions (30,920 bytes) are compiler-invariant: 18 REG, 3 structure, and
  `_rwGCNVtxFmtInstClr`. None of them has a known source fix.
- 12 of the 50 match today under an unmodified archived compiler, but not in
  any unit where that compiler also matches every other function.

### What would close the rest

A **RenderWare-specific compiler variant** (a 2.0p1a sibling selected by
per-object `mw_version`, as stdkey, rtslerp, ptankgcn* and setup already are)
could close the compiler-dependent residue. The changes, in order of evidence
strength:

1. **Clause V skips stores to compiler temporaries** (base word `0x10005` with
   `Object+0x18 == 0`).
   - This is the only change that has been measured as an executable compiler.
   - RW: +4/-0. Game: 0/0 with four partials up.
   - It fixes both `_rwGCLightsGlobalEnable` and `_rwGCLightsLocalEnable`
     (gclights becomes 6/6 and linkable) and `RwImageCreateResample`.
   - `RwImageResample` reaches 97.82; its allocator residue remains.
   - `ptankgcnrender` could return to the common compiler.
   - Because it is neutral on the game, it could go into GC/2.0p1a itself
     rather than a variant.
2. **No V walk and no E3n/A pin for the IEEE pun-union locals**
   (`_gf`/`_sf`/`gf_u`/`sf_u`).
   - It fixes `RpLightGetConeAngle` (balight becomes 13/13, given S stays).
   - `stdkey` and `rtslerp` could return to the common compiler.
   - Needs a discriminator narrower than "declared frame local": `vfr` costs
     `xcsCalcAnimMatrices`. Not built.
3. **The R4 X-form rule:** an `lwzx` is not ordered after callee-save spills
   other than its own destination's.
   - Fixes `CollisionDataStreamWrite` and both `MultiTexture*` functions.
   - rpcollis and multiTex would then be expected at 9/9 and 17/17, since their
     other functions match under 2.0p1a. That expectation is not measured.
   - This is a scheduler change, not an alias-table answer. The hook point has
     not been located.
4. **2.5-style LICM through pointer-to-const, after strength reduction** (R3).
   - Fixes the three bamatlst loop functions, `RwImageCopy` and
     `RpGeometryStreamGetSize` outright.
   - Takes `ImageConvertDepth` to 99.82, `RpGeometryStreamWrite` to 99.59,
     `UserDataListCopy` to 98.52 and `RwImageApplyMask` to 99.09.
   - Fixes `_rpSkinMatrixBlendUpdate` together with the `e/sk_s4.c` source
     rewrite.
   - This is an optimiser behaviour, not a clause, so it is the hardest item.
     The alternative is a per-unit GC/2.5 override, which fails because the
     same units need F/S/E3n/W (R5).
5. **Narrow clause A** so that a whole-scalar frame *store* does not pin a
   later static load (`_rwDlNativeTextureWrite`; half of `MeshRenderEnvMap`).
   Not measured on the game; clause A's known witness runs the other way.
6. **The narrow R8 edges** for `RpMaterialStreamRead`, `StalacTiteAlloc` and
   `_rwDlRasterShowRaster`. Each has a repro that isolates the pair, but the
   obvious general forms are heavily negative on the game (E3n-entry-4 -192,
   B-entry-4 -199). These are low priority.

**Not closable by compiler choice:**
- R9 (`AtomicFor*Intersections`, the `_rpGameCubeMTEffectSend` texMtx block),
  `_rwDlCameraBeginUpdate` and the E3n half of `MeshRenderEnvMap`: no archived
  or ablated compiler produces the target.
- The 22 compiler-invariant functions. Their best leads:
  - DWARF-faithful rewrites where our reconstruction has invented structure:
    `TriStripFollow`'s helpers, `MatrixOrthoNormalize`,
    `CameraBuildPerspClipPlanes`, `WorldSectorStreamRead`.
  - The ternary-plus-association search for `RxLockedPipeUnlock`.
  - A source shape that keeps the dead `bone` IV alive in
    `_rpSkinRenderCallback`.

**Where the evidence is thin or the agents disagreed:**
- The claim that `const` drives R3 rests on `e/k.c`, `e/m1.c`/`e/m2.c` and
  `misc/licm.c`, all of which have a call in the loop. For the call-free
  `RpGeometryStreamGetSize` loop it was not tested.
- `_rwDlRasterShowRaster`'s `volatile` source is inferred from load order; it is
  not proven original.
- `_rpGameCubeMTEffectSend` (b) and the 2.5 leftovers of
  `RpGeometryStreamWrite`, `UserDataListCopy` and `RwCameraFrustumTestSphere`
  have not been through `diag.py`.
- The `ImageConvertDepth` remainder under 2.5 is assumed to be statement order.
  It was not tested, because baimage is diagnosis-only.
- The DUPLOTRON note that "a whole-scalar store never reaches clause E3n" is
  contradicted by two independent disassemblies of the current blob, which show
  E3n on entry 0. The current blob is authoritative.
