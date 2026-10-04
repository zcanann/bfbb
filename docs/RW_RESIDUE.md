# RenderWare residue: why the last 19 RW functions do not match

This document lists, for every RenderWare SDK function in the bfbb GameCube
decomp that is below 100%, what exactly differs from retail and what causes
it. The aim is to separate the kinds of residue:

- residue that **source changes can still fix**;
- residue that **needs compiler work**: a behaviour that only retail's
  compiler has and that no archived compiler provides;
- residue whose cause is **known but has no lever** under the RW compiler
  (mostly register-allocator colour order or interference-graph shape).

## How this was produced

1. **Classification.** `tools/residue.py` compiles every unit that has a
   non-matching RW function. It uses the unit's own compiler and then GC/2.0p1
   and 2.5 from the same source, and diffs each against the target with
   objdiff. The shape of the project-compiler diff is one of:
   - **REG**: identical instructions apart from register numbers;
   - **SCHED**: the same multiset of instructions in a different order;
   - **COUNT**: a different instruction count;
   - **OPS**: the same count but different opcodes.

   The "matches under" column lists the alternative compilers that give 100%
   from the unchanged source. Today it is empty for all 19 functions.
2. **Per-function experiments.** Each function was reduced to a small C repro
   and compiled with `tools/regalloc/cc.py` under the GC/2.0p1a–e variants,
   2.0p1, 2.5, 2.6 and 2.7, and sometimes 1.1–1.3.2, 2.0 and 3.0a. Suspected
   alias clauses were tested with ablated copies of the compiler (per-clause
   stubs, restored dispatch entries, `cablate.py` conditional ablation) and,
   for the retail-only edges, with the experimental `patch_compiler_rw.py`
   parts `dss`, `volb` and `e3n4`. The real compiler binaries were never
   modified.
3. **Register-allocator capture.** REG residues were captured with
   `tools/regalloc/diag.py` under `RCAP_MW=GC/2.0p1e`. It records the
   interference graph, the simplify order, the final colours and the creation
   phase of every `@NNN` temp. Every capture below replays exactly
   (`replay exact`), and the solver looked for the virtual-register rank moves
   that would give retail's colours on the same graph.

The working notes and repro files are in session scratchpads; none of that is
in the repo. This document is the durable summary.

### Current totals

Measured from `build/GQPE78/report.json` and a fresh
`python tools/residue.py --category RW --compilers 2.0p1,2.5` after `ninja`:

| measure | value |
|---|---|
| RW compiler | **GC/2.0p1e** (all units except `stdkey`) |
| RW functions matched | **1020 / 1039** (98.17%) |
| RW units complete (linked) | **106 / 120** |
| RW code matched | 343,364 / 371,428 bytes (92.44%); the 19 residue functions are the whole 28,064-byte gap |
| RW data matched | 11,764 / 11,764 bytes (100%) |
| non-matching functions | 19, in 14 units |
| residue shapes | REG 8, SCHED 9, COUNT 2 |
| match 100% under GC/2.0p1 or GC/2.5 from the same source | 0 / 19 |

`rtslerp` now links (stripped `RtSlerp*` functions restored for the `.sdata2`
order; see 3.7).

**Compiler override still in `configure.py`** (all other RW units use
GC/2.0p1e, see `docs/COMPILER_VARIANTS.md`):

| unit | compiler | why (rule below) |
|---|---|---|
| `stdkey` | GC/2.0p1 | R1d + R2 (pun unions); also a `const` source-shape question under R3 |

The former overrides (`rtslerp`, `ptankgcncallbacks`, `ptankgcnrender`,
`setup`, `multiTexGcnData`) were dropped when the RW compiler gained 2.0p1b–d
behaviour; those units match on the common compiler.

### Compiler lineage

`GC/2.0p1e` = 2.0p1a (joey's alias clauses) plus:

| variant | change | rules it implements |
|---|---|---|
| 2.0p1b | clause V skips stores to compiler temporaries | R1t |
| 2.0p1c | const-pointer LICM, large-loop veto from 2.5 | R3 (superseded by e) |
| 2.0p1d | R4 two-register-load alias; address-taken gate on E3n/A/W/V | R4, R1d/R2/R6/R7 for non-escaping locals |
| 2.0p1e | 2.5's plain alias for a const/restrict pointee (`rep` + `nps`) | R3 and R10 both follow from this representation |

The retail-only edges `dss`, `volb` and `e3n4` were **rejected** as compiler
changes: no archived compiler has them, so they fail the provenance criterion.
They remain candidate behaviours of the retail compiler ("compiler B").

### Solved since the previous revision (levers)

The previous revision listed 50 functions. These are now 100%; the lever is
kept because it is reusable.

| function(s) | lever |
|---|---|
| `UserDataListCopy` | DWARF-shaped static `UserDataCopy` helper (indexed string loop); RenderWare's explicit `NULL !=` comparisons keep `UserDataListCopy`'s post-inline complexity above 512 so it stays out of line (see 3.6) |
| `_rwGCLightsGlobalEnable`, `_rwGCLightsLocalEnable`, `RwImageCreateResample` | compiler 2.0p1b (R1t) |
| `_rpMaterialListFindMaterialIndex`, `_rpMaterialListStreamGetSize`, `_rpMaterialListStreamWrite`, `RwImageCopy`, `RpGeometryStreamGetSize` | compiler 2.0p1c/e (R3) |
| `_rwFrameListFindFrame`, `_rpSkinMatrixBlendUpdate` | R3 compiler plus the debug-DWARF direct field reads (no hand-hoisted locals) |
| `VectorMultPoint`, `VectorMultVector` | R3 compiler; read `matrix->right/up/at/pos` inside the loop instead of twelve hand-hoisted locals |
| `RpGeometryStreamWrite` | R3 compiler; test `geometry->flags` directly in the morph-target loop, `sizeTC` at function scope |
| `CollisionDataStreamWrite`, `MultiTextureStreamGetSize`, `MultiTextureStreamWrite` | compiler 2.0p1d (R4) |
| `RpLightGetConeAngle`, `MeshRenderEnvMap` | compiler 2.0p1d address-taken gate (pun unions, `shiney` do not escape) |
| `_rpSkinRenderCallback` | restore the Rat-DWARF helpers: `_rpSkinLoadMatrix` (static, auto-inlined, replacing the `SKINLOADBONEMATRIX` macro) and `static inline _rpSkinLoadMatrixPalette` with `RwUInt8` start/run locals. The "dead IV" was source structure, not a compiler effect |
| `_rpSkinBlendBody` | `posFrac` operand first in the GQR `\|` expression |
| `CameraBuildParallelClipPlanes` | the Rat-DWARF `offset` local |
| `RwImageSetGamma` | `(RwReal)(scaled * (RwReal)255.0)`: the cast blocks `fmadds` fusion and makes the product an objectless temp |
| `WorldSectorStreamRead` | the old-polygon conversion as an auto-inlined static helper; its loop counter becomes an inline object, which ranks above the FindLoops walker |
| `TriStripFollow` | an if/else that reuses the `nextIsLast` flag; its second web becomes an IROUseDef split temp (N8) |
| `RwCameraFrustumTestSphere` | compiler 2.0p1e (R10) plus the RW-style in-loop `sphere->center`/`sphere->radius` reads |
| `ImageConvertDepth` | declare `palette`, `cpSrc`, `cpDst` first, then assign them |
| `RwImageApplyMask` | in the inlined `RwImageAllocatePixels`, initialise `RwBool imagePalette = (imageDepth == 4 \|\| imageDepth == 8);` instead of setting it later. The value then lives in a temp that ranks above `tempImage`'s inline web |
| `RpGeometryStreamRead` | a small static auto-inlined morph-target accessor (`return &geometry->morphTarget[index];`) makes the pointer an inline return value, which ranks above FindLoops IV @733; the morph loop's `i` is block-scoped and `kf` is declared before `morphTarget`, per Rat DWARF. RW's public `RpGeometryGetMorphTarget` is a macro in release builds, so the helper is private |
| `ExtractNodes` | `recip = (root->Leaf.weight > 0.0f) ? (255.9999f / root->Leaf.weight) : 0.0f;` (`recip` becomes an objectless/linearise temp, `weight` a CSE), plus each channel through one reused `RwInt32 c` (`c = (RwInt32)(... * recip); palette[n].red = (RwUInt8)c;`). The extra statements also restore the self-inline depth through complexity (3.6) |
| `DlRasterTile` | the existing local `index` holds the whole byte offset `(tb << 2) + ((tdy + (x & 3)) << 1)`, which creates the offset temps before the pixel `lhz` |

Units newly linked in rounds 3–4: `rpusrdat`, `babinwor`, `bameshop`,
`skingcn`, `baimage`, `bageomet`, `palquant`, `dlraster`.

**General lever found this round:** `p = (T *)((RwUInt8 *)p + s)` instead of
`p += s` changes how the unroller renames the induction variable in the
unrolled copies. It took `_rwGCNVtxFmtInstClr`'s RGB8 loop from 94.33 to 97.13.

---

## 1. Compiler behaviour model

### Background: the 2.0p1a alias patch

GC/2.0p1a is GC/2.0p1 plus C predicates injected into two dispatch tables and
one call site. The source is `AliasPatch.c`; the injection is done by
`tools/patch_compiler.py`; `docs/DUPLOTRON.md` has the history. The RW
variants 2.0p1b–e are built on top of it by `tools/patch_compiler_rw.py`.

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
  (flag 0x40) are exempt, which is the documented "const lever". Under 2.0p1d+
  it fires only when the frame object's address escapes.
- **W**: the write-after-read half. A plain store to a declared frame local may
  not pass an earlier `lfs`/`lfd` literal load, and the edge carries the load's
  latency.

### The rules found in the RW residue

Each rule states the observed behaviour and its current status. "Retail" means
the compiler that built the retail RW objects.

| rule | behaviour | status |
|---|---|---|
| R1t | clause V kills cached literals on a store to a compiler temporary (the `fctiwz` slot) | **implemented** (2.0p1b) |
| R1d | clause V on stores to declared pun-union locals | **implemented** for non-escaping locals (2.0p1d gate); `stdkey` keeps GC/2.0p1 |
| R2 | E3n, with A as fallback, pins literal loads below pun-union stores | **implemented** (2.0p1d gate) |
| R3 | 2.5-style LICM of loads through a pointer-to-const | **implemented**; in 2.0p1e it is a consequence of the const-pointee alias representation |
| R4 | an X-form load is not held below callee-save spills | **implemented** (2.0p1d) |
| R5 | clauses F and S are required by retail (attach functions, module `Open`s) | in 2.0p1a; the reason RW cannot use stock compilers |
| R6 | E3n over-fires on stores into non-escaping frame aggregates | **implemented** (2.0p1d gate). `_rwDlCameraBeginUpdate`, once listed here, is now classified under `dss` (R8b) |
| R7 | clause A over-fires on whole-scalar frame store vs static load | **implemented** (2.0p1d gate) for `MeshRenderEnvMap`; `_rwDlNativeTextureWrite` turned out to be pointer-analysis precision, see R9c |
| R8a | entry-4 subrange×subrange: frame-aggregate store vs small-static load | retail-only edge `e3n4`, **rejected**. `RpMaterialStreamRead` |
| R8b | a direct `sym@sda21` store to a static field is ordered before a later store to a frame-object field | retail-only edge `dss`, **rejected**. `StalacTiteAlloc`, `_rwDlCameraBeginUpdate`, part of `RxLockedPipeUnlock` |
| R8c | a volatile store is ordered before any later store | retail-only edge `volb`, **rejected**. `_rwDlRasterShowRaster` |
| R9a | a load through a pointer merging `&localV3d` with heap pointers does not alias other address-taken locals | no compiler has it. `AtomicFor*Intersections` |
| R9b | a store to an escaping frame array does not alias a load through a call-returned pointer | no compiler has it. `_rpGameCubeMTEffectSend` |
| R9c | `&bytesLeftToWrite` is treated as escaping for loads through `raster` but not through `textureIn` | no compiler 1.3.2–2.7 has it. `_rwDlNativeTextureWrite` |
| R10 | unrolled const-pointer loops keep one `addi` per copy | **implemented**; a consequence of the 2.0p1e alias representation (AddPropagation refuses to fold) |

The detailed evidence for R1–R7 and R10 is in `docs/COMPILER_VARIANTS.md` and
in this document's git history. The still-open rules:

**R8: retail has ordering edges that 2.0p1e lacks.**
- **R8a (`e3n4`).** A store to a subrange of a frame aggregate (`mat.color`)
  against a later load of a subrange of a static of at most 8 bytes
  (`materialModule.globalsOffset`) goes to scheduler entry 4, which consults
  only S, and S needs both sides static. Retail pins the load. Repros
  `wld/mat.c`, `wld/mat2.c`. The `e3n4` part makes `RpMaterialStreamRead`
  100 but costs `zNPCFodBzzt::DiscoRender` 77.59 → 74.14; general forms
  measure −9 and −200.
- **R8b (`dss`).** A direct-symbol (`@sda21`) store to a static struct field
  must precede a store to a frame struct field. DWARF fixes both sides as
  structs, so both are subranges → entry 4 → only S. `StalacMiteAlloc`, which
  does the same store through a base register, has no edge in retail, so the
  direct-symbol condition is fitted to the data. It is now evidenced by three
  functions (`StalacTiteAlloc`, `_rwDlCameraBeginUpdate`,
  `RxLockedPipeUnlock`); the broader `v_SB4` (entry 4 falls through to B)
  is −199 on the game.
- **R8c (`volb`).** With `_RwGCXFBCopy` volatile, the tail
  `stw _RwGCXFBCopy; li r0,1; stw _RwDlFSAATop` needs an edge from the
  volatile store that no compiler adds (`drv/vtail.c` `f()`).

**R9: retail's alias analysis is more precise than any available compiler
for some pointers.**
- **R9a**: every compiler tried (1.1 through 3.0a5.2) answers may-alias for a
  load through a pointer whose reaching definitions merge `&localV3d` with
  `&startVerts[i]`. Repros `col/s1..s9.c`; `#pragma opt_pointer_analysis on` is
  inert. Rat has no `ctgeom`, so the source cannot be cross-checked.
- **R9b**: in `_rpGameCubeMTEffectSend`, `texMtx` escapes (passed to
  `GXLoadTexMtxImm`) and `matrix` is returned by `RwMatrixMultiply`. Repro
  `mfx/mtx*.c`: `n1` (array not address-taken) gives the target; `p1`/`p3`/
  `m1` are serialised on every compiler. Retail does serialise the
  structurally identical `GetTexFrameMatrix` loop, so the discriminator is
  unknown.
- **R9c**: in `_rwDlNativeTextureWrite`, retail lets loads through `raster`
  pass the `bytesLeftToWrite -= ...` store but keeps loads through `textureIn`
  behind it, as though the address escaped only relative to one pointer. No
  compiler 1.3.2–2.7 does this.

### The inferred retail compiler ("compiler B")

No archived compiler explains all the RW evidence. GC/2.0p1e now carries
every behaviour that has a later-compiler counterpart (R3, R4, R10 from 2.5;
V-on-temporaries and the address-taken gate as emulation corrections of
joey's clauses). What remains attributable to compiler B is:

> **Retail RW ≈ GC/2.0p1e plus three narrow scheduler edges (`dss`, `volb`,
> `e3n4`) and pointer-analysis precision (R9a/b/c) that no archived compiler
> has.**

---

## 2. Register-numbering rules

Many residues below come down to which virtual register (vreg) a web gets,
because the graph colourer is deterministic given that number. The rules
(N1–N13) were measured with `wcap.py`/`diag.py` on test files
`cse/t/t1..t8.c` and on the real units. Replays reproduce the compiler's
colours exactly on every residue capture below. See also
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
| N11 | an objectless `?:` temp (N9) coalesces into an N8 split web or a CSE temp, but **not** into a named local's first web. So `x = c ? a : b` on a fresh local leaves `x` webless, while the same `?:` into a reused local's later web joins that web. |
| N12 | reusing an existing local whose first web is dead makes each later assignment an IROUseDef split temp (N8), ranked **below** every CSE temp. This puts a value under the CSE band without a new named local. |
| N13 | a static helper inlined inside another inlined helper creates its inline objects after **all** the outer calls' inline objects, so its group ranks below every outer inline group. |

A `hyp.py` test re-ranked whole families the other way. None of these fixes a
residue without breaking the matched controls, so the REG residues are **not**
a systematic numbering difference between our compiler build and retail's.
They are either source shape (a different set of webs) or a different
interference graph.

Levers that have generalised (each fixed at least one function):
- store an objectless value into an existing named local whose later web
  carries the other value (`_rwGCNDisplayListGetStride`);
- move a loop into an auto-inlined static helper, so its counter becomes an
  inline object ranked above FindLoops temps (`WorldSectorStreamRead`);
- reuse a flag variable in an if/else so its second web becomes an N8 split
  temp (`TriStripFollow`);
- a cast around a product, `(RwReal)(a * b)`, to block `fmadds` fusion and
  make the product objectless (`RwImageSetGamma`);
- declare-then-assign instead of initialising declarations
  (`ImageConvertDepth`), and the reverse: an initialiser instead of a later
  assignment inside an inlined helper (`RwImageApplyMask`);
- a small static accessor that returns a pointer, so the value becomes an
  inline return temp ranked above FindLoops (`RpGeometryStreamRead`);
- a `?:` into a local plus one reused integer local for several channels, so
  the values go through objectless/split temps (`ExtractNodes`, N11);
- widen what an existing local holds (a whole byte offset instead of an index)
  to create its temps earlier (`DlRasterTile`);
- pass a size expression straight to the callee instead of through a local, so
  it becomes a CSE and the local coalesces (`_rpMaterialListSetSize`, which
  lifted `_rpMaterialListStreamRead` 97.77 → 99.88).

Linking facts found while completing units:
- an unreferenced static helper that is fully inlined is still emitted out of
  line by our compiler, but the linker strips it, so it does not block linking;
- `.sbss` statics are emitted in **reverse declaration order**; `rpusrdat`
  linked only after its declarations were reversed with the module struct
  first;
- the auto-inline limit (complexity after the callee's own inlines at most
  512) is in 3.6; `ExtractNodes` depends on it for its self-inline depth.

---

## 3. Per-function table

Columns:
- **%**: under GC/2.0p1e (the RW compiler).
- **alt**: the GC/2.0p1 / GC/2.5 scores from `residue.py`.
- **class**: the residue-class letter from section 4.

| class | function | unit | size | % | shape | alt (2.0p1 / 2.5) |
|---|---|---|---|---|---|---|
| a | `_PropagateDependenciesAndKillDeadPaths` | src/pipe/p2/p2dep | 1296 | 99.32 | REG | 99.32 / 99.32 |
| a | `_rwStringStreamFindAndRead` | src/babintex | 980 | 99.33 | REG | 99.33 / 99.33 |
| a | `RwTextureStreamRead` | src/babintex | 2400 | 99.59 | REG | 99.59 / 99.59 |
| a | `MatrixOrthoNormalize` | src/plcore/bamatrix | 1132 | 99.52 | REG | 98.11 / 98.11 |
| a | `_rpMaterialListStreamRead` | world/bamatlst | 1508 | 99.88 | REG | 99.88 / 99.88 |
| b | `_rwFreeListAllocReal` | src/plcore/bamemory | 432 | 99.68 | REG | 99.68 / 99.68 |
| b | `RwImageResample` | src/baresamp | 468 | 97.82 | REG | 97.82 / 57.81 |
| b | `CameraBuildPerspClipPlanes` | src/bacamera | 2020 | 99.11 | REG | 99.11 / 99.11 |
| c | `StalacTiteAlloc` | src/pipe/p2/p2define | 116 | 90.69 | SCHED | 68.45 / 68.45 |
| c | `_rwDlCameraBeginUpdate` | driver/gcn/dldevice | 1464 | 98.88 | SCHED | 97.42 / 97.42 |
| c | `_rwDlRasterShowRaster` | driver/gcn/dldevice | 1060 | 99.89 | SCHED | 80.17 / 80.17 |
| c | `RpMaterialStreamRead` | world/bamateri | 1008 | 99.15 | SCHED | 99.21 / 99.21 |
| c + e | `RxLockedPipeUnlock` | src/pipe/p2/p2define | 2760 | 98.06 | COUNT | 97.41 / 97.41 |
| d | `_rwDlNativeTextureWrite` | driver/gcn/dltexdic | 732 | 97.75 | SCHED | 89.72 / 89.72 |
| d | `AtomicForAllLineIntersections` | plugin/collis/ctgeom | 1432 | 98.30 | SCHED | 98.30 / 95.78 |
| d | `AtomicForAllSphereIntersections` | plugin/collis/ctgeom | 936 | 98.24 | SCHED | 98.24 / 98.24 |
| d | `_rpGameCubeMTEffectSend` | plugin/matfx/gcn/multiTexGcnPipe | 1804 | 95.85 | SCHED | 96.15 / 96.15 |
| d (inferred) | `CalcMeshNBTs` | plugin/matfx/gcn/multiTexGcnPipe | 4340 | 98.72 | SCHED | 98.72 / 95.73 |
| e | `_rwGCNVtxFmtInstClr` | world/pipe/p2/gcn/instance/geominst | 2176 | 97.13 | COUNT | 97.13 / 97.13 |

### 3.1 (a) Allocator: the needed rank is unreachable by the numbering rules

The interference graph is retail's; the replay names the vreg move that would
give retail's colours, but N1–N13 put no source-expressible web at that rank,
or the only known lever was rejected as not source-likely.

#### `_PropagateDependenciesAndKillDeadPaths`

- **Unit:** src/pipe/p2/p2dep. **Size / %:** 1296 b, 99.32.
- **Exact difference:** the objectless `node->nodeDef` load v100 (the base of
  every `iospec->` access) r28→r31, which pushes `pipeline`, `node` and `i` down
  one register.
- **Move needed:** v100 to rank 42.5–44.5, i.e. into the named/split band.
- **Evidence:** IRO always copy-propagates `iospec` and any named `nodeDef`
  local, so neither gets a web; declaration moves only move the dead slot
  (99.32/99.37). The CSE route ranks too high: a CSE temp lands at ≥46.5,
  behind the `j`/`k` split temps (96.9). The N12 reuse lever needs an existing
  `RxNodeDefinition *` local, and there is none.
- **Status:** REG, no lever.

#### `_rwStringStreamFindAndRead` and `RwTextureStreamRead`

- **Unit:** src/babintex. **Size / %:** 980 b, 99.33; 2400 b, 99.59.
- **Coupled:** the String helper is inlined twice, so any change to it moves
  both functions.
- **Exact difference (FindAndRead, ours → retail):** String inline:
  `bytesToRead` (objectless v73) r26→r31; `baseString` @142 r31→r30;
  `nativeString` @144 r30→r29; `length` @145 r29→r28; outer `string` r30→r29.
- **Exact difference (TextureStreamRead):** `mipmapState` v35 r30→r29 and
  `autoMipmapState` v34 r29→r30; the first FindAndRead inline's @370 r25→r26
  and @350 r27→r28.
- **Move needed:** inline param objects ranked outside their N5 group, which is
  impossible because params are created last in the group. TextureStreamRead
  also needs the `mipmapState`/`autoMipmapState` swap, which contradicts the
  DWARF declaration order (swapping gives 99.49).
- **Evidence:** the best forms reach 99.59 (FindAndRead) and 99.717
  (TextureStreamRead) separately, never both at once. The assign-then-reassign
  `bytesToRead` form gives it an object web (FindAndRead 99.53) but drops
  TextureStreamRead to 99.20.
- **Status:** REG, no lever.

#### `MatrixOrthoNormalize`

- **Unit:** src/plcore/bamatrix. **Size / %:** 1132 b, 99.52.
- **Exact difference:** FPR abs-dot block: abs-dot CSE temps @121/@125 against
  the component CSE temps @126–@131.
- **Move needed:** the raw dots must rank below the CSE band.
- **Known lever (rejected):** reusing an existing dead local (`recipAt`, or
  `recipUp`/`recipRight`) to hold each raw dot before `RwRealAbs` gives 100%:
  the later webs become N12 split temps, which rank below CSE. It is not
  committed because reusing a reciprocal-named variable for dot products was
  judged not source-likely. A fresh local fails: its first web is a named web,
  the `?:` does not coalesce into it (N11), and an extra `fmr` appears (99.33).
  The patch is kept outside the repo.
- **Status:** REG (FPR), known lever rejected.

#### `_rpMaterialListStreamRead`

- **Unit:** world/bamatlst. **Size / %:** 1508 b, 99.88 (was 97.77).
- **What fixed most of it:** passing `size * sizeof(RpMaterial *)` directly to
  `RwRealloc`/`RwMalloc` in the inlined `_rpMaterialListSetSize`. `size*4`
  becomes a CSE, `memSize` coalesces, and the params gain the neighbour they
  were missing, so the callee-saved permutation is gone.
- **Exact difference:** a colour swap between CSE `len` (@299) and `len*4`
  (@300).
- **Move needed:** the two CSE temps in the opposite order. N7 creates CSE
  temps by first occurrence, and the `len == 0` test comes first, so no
  ordering of the source reverses them.
- **Replay check:** a hand replay of the capture with only @300 coloured
  before @299 gives retail's colours exactly, so the residue is just that one
  rank pair (diag's own search runs out of budget and reports it unreachable).
  Inert: memSize assigned twice / block-local / in one call only, store and
  sizeof order, own-type casts, caching materials first; memSize in both calls
  fixes the pair but scrambles the callee-saved params (97.77).
- **Status:** REG, no lever (reclassified from (b)).

### 3.2 (b) Interference graph or pre-RA order differs

No renumbering of our interference graph reproduces retail, so the source
builds a different set of webs or interferences, or the pre-RA scheduler
orders the defining instructions differently.

#### `_rwFreeListAllocReal`

- **Unit:** src/plcore/bamemory. **Size / %:** 432 b, 99.68.
- **Exact difference:** CSE v49 against objectless v63/v64.
- **Cause:** our pre-RA scheduler emits `addi v63; subi v65; add v64`, which
  ends v49's life before v64 is defined, even though the IR order is
  `addi, add, subi`. Retail's order is `addi; add; subi`, so v64 interferes
  with v49. No rank move suffices.
- **Evidence:** about 20 more source variants were inert.
- **DWARF:** Ratatouille declares `link` and `aligned` block-local in the
  every-block-full branch. Block-local `link` is neutral; block-local
  `aligned` gives 98.70 by dropping retail's final `mr r3,r0`, which suggests
  retail's `aligned` and `freeEntry` are separate webs there.
- **Status:** graph differs (pre-RA scheduler).

#### `RwImageResample`

- **Unit:** src/baresamp. **Size / %:** 468 b, 97.82.
- **Exact difference:** a callee-saved GPR permutation. Target: r31=nY,
  r30=nXDelta, r29=nYPos, r28=nYDelta, r27=dstWidth, r26=dstHeight, r25=src,
  r24=dst, r23=span ptr, r22=`nYPos+nYDelta-1`, r21=nX, r20=nXPos.
- **Cause:** the pre-RA scheduler places `li nXPos`/`li nX` between the
  `mullw` and the walker `add`, which gives nX/nXPos degree 29 (= K), so they
  become survivors. Deleting the edges v64/v66–nX/nXPos in the capture
  reproduces the target exactly.
- **Evidence:** unchanged this round. `RwImageCreateResample` inlines the
  resampler, which constrains the forms that can be tried (both must keep
  matching). An `rpDstSpan++` form gives 99.15 but contradicts the DWARF
  `const` pointer, so it is not applied.
- **Status:** graph differs.

#### `CameraBuildPerspClipPlanes`

- **Unit:** src/bacamera. **Size / %:** 2020 b, 99.11.
- **Exact difference:** FPR, 29 webs. IroVars scalarisation temps @205..@213
  (vTmp, vTmp2, vRight, vUp, vCOP); `scale` f3→f29 (retail keeps it live across
  calls). Only the FPR colouring before the loop differs.
- **Cause:** our `scale` v32 has degree 34, so it is a survivor and pops early;
  retail's `scale` must have had degree < 32, i.e. fewer FPR neighbours.
- **Evidence:** no pop order of our graph reaches the target. Rat DWARF has
  **four** `length2` locals. Reorder search best 99.257, only via unnatural
  statement orders.
- **Status:** graph differs (FPR).

### 3.3 (c) Needs a retail-only scheduler edge

These are the candidate behaviours of compiler B. Each matches 100% with the
corresponding experimental part, which was rejected for lack of provenance.

#### `StalacTiteAlloc`

- **Unit:** src/pipe/p2/p2define. **Size / %:** 116 b, 90.69, SCHED.
- **Exact difference:** error block. Target is
  `add r0,r3,r4; lis r3,0x8000; stw r0,gMemoryLimits@sda21; li r0,1; addi r3,0x13; stw r0,0x8(r1)`.
  Ours is `add r5; li r0,1; lis; stw r5,gMemoryLimits; addi; stw r0,0x8(r1)`.
  The `li 1` for `_rwErrorCode.pluginID` rises above the static store, so the
  `add` cannot share r0.
- **Rule:** R8b (`dss`): both stores are subranges (DWARF fixes both as
  structs) → entry 4 → only S.
- **Evidence:** `misc/tite.c`; every compiler gives ours, `dss` gives the
  target. Five source variants: at most 90.69.
- **Status:** retail-only edge.

#### `_rwDlCameraBeginUpdate`

- **Unit:** driver/gcn/dldevice. **Size / %:** 1464 b, 98.88, SCHED.
- **Exact difference:** after `bl GXSetCurrentGXThread`. Target is
  `stw r31,dgGGlobals; li r0,0; lis r4,2; lis r3,Inv@ha; stw r0,0x14(r1); addi r0,r4,3`.
  Ours is
  `li r5,0; lis r4,2; stw r31,dgGGlobals; lis r3; addi r0,r4,3; stw r5,0x14(r1)`.
- **Rule:** R8b (`dss`): the direct-symbol store to the static `dgGGlobals`
  field must precede the store to the frame `viewoffset.flags` field.
- **Evidence:** `drv/cam.c`; 11 source variants, best other 98.743. Matches
  with `dss` only.
- **Status:** retail-only edge.

#### `RxLockedPipeUnlock`

- **Unit:** src/pipe/p2/p2define. **Size / %:** 2760 b, 98.06, COUNT
  (`bge` +1, `ble` −1, `mr` −1).
- **Exact difference, three parts:**
  - a `dss` pair, as in StalacTiteAlloc;
  - the size sum. Retail groups it as
    `(n*0x14 + ((n*u*0x24 + n*u*0x10) + topSort)) + persistent`, which MWCC's
    reassociation does not reproduce from any of ~300 searched forms;
  - the max idiom. `size = (size > end) ? size : end` (`e/pd_tern.c`)
    reproduces retail's `mr; cmplw; ble; mr` exactly, but costs elsewhere
    (97.72), so it is not applied.
  - Further IV/register differences follow from these.
- **Status:** retail-only edge plus source shape unknown (the sum).

#### `_rwDlRasterShowRaster`

- **Unit:** driver/gcn/dldevice. **Size / %:** 1060 b, 99.89, SCHED.
- **Exact difference:** three sites of
  `_RwGCFrameQueue[_RwDlFrameNew].XFBCopy = _RwGCXFBCopy`. Target loads the RHS
  before the volatile index; ours does the reverse.
- **Rule:** load order, then R8c (`volb`). Making `_RwGCXFBCopy` volatile
  fixes six rows and exposes one tail row (`stw _RwGCXFBCopy; li r0,1` vs ours
  `li r0,1; stw`) that only `volb` orders.
- **Evidence:** `drv/vtail.c` `h()` and `f()`. The volatile variant scores
  99.245 without `volb`, so it is not applied.
- **Status:** retail-only edge (plus the `volatile` source change).

#### `RpMaterialStreamRead`

- **Unit:** world/bamateri. **Size / %:** 1008 b, 99.15, SCHED.
- **Exact difference:** after `bl RwMemNative32`. Target is
  `lwz r0,0xc(r1); stw r0,0x2c(r1)` (`mat.color = tmp`), then
  `lwz r3,RwEngineInstance; lwz r0,materialModule`. We hoist
  `lwz r0,materialModule@sda21` above the `stw`.
- **Rule:** R8a (`e3n4`): an 8-byte static `materialModule` subrange load
  against a frame `mat.color` subrange store goes to entry 4.
- **Evidence:** `wld/mat.c`, `wld/mat2.c`. No source lever: `memcpy`, a pun,
  and `RwRGBAAssign` all fold or become a call. The const lever does not apply
  because `mat` is written by `RwStreamRead`.
- **Status:** retail-only edge.

### 3.4 (d) Alias precision that no compiler has

#### `_rwDlNativeTextureWrite`

- **Unit:** driver/gcn/dltexdic. **Size / %:** 732 b, 97.75, SCHED.
- **Exact difference:** target is
  `lwz r6,0(r31); rlwimi; lwz r5,_RwGameCubeRasterExtOffset; stw r7,0x8(r1); add r30,r6,r5; stw r0,0x10(r1)`.
- **Rule:** R9c. Retail treats `&bytesLeftToWrite` as escaping for loads
  through `raster` but not through `textureIn`. No compiler 1.3.2–2.7 does
  this.
- **Evidence:** moving the `-=` after `formatType` gives 98.60, but the
  placement is implausible, so it is not applied.
- **Status:** compiler-missing.

#### `AtomicForAllSphereIntersections` and `AtomicForAllLineIntersections`

- **Unit:** plugin/collis/ctgeom. **Size / %:** 936 b, 98.24; 1432 b, 98.30.
- **Exact difference:** Sphere: after `RtIntersectionSphereTriangle`, the
  target loads `*v0` before both the `distance` store and the
  `collTriangle.point` stores. Line: in `RpCollisTriangleNormalMacro` after
  `_rwInvSqrt`, the target hoists the `collTriangle.point = *v0` loads above the
  three `collTriangle.normal` `stfs`.
- **Rule:** R9a. `v0` merges `&interpV0` with `&startVerts[idx]`.
- **Evidence:** `col/s1..s9.c` across 12 compilers. Rat has no `ctgeom`, so
  the source cannot be cross-checked.
- **Status:** compiler-missing.

#### `_rpGameCubeMTEffectSend`

- **Unit:** plugin/matfx/gcn/multiTexGcnPipe. **Size / %:** 1804 b, 95.85,
  SCHED.
- **Exact difference, three parts:**
  - (a) `ENVMTX` block: target loads all six `matrix->` fields before the first
    `stfs f5,0x160(r1)`; ours serialises per element. This is R9b.
  - (b) a scheduler tie-break that follows from (a).
  - (c) allocator: `config` is objectless v95 and is spilled first.
- **Evidence:** `mfx/mtx*.c`. A locals form reaches 97.96 but is not
  source-likely, so it is not applied.
- **Status:** compiler-missing plus allocator.

#### `CalcMeshNBTs` (inferred)

- **Unit:** plugin/matfx/gcn/multiTexGcnPipe. **Size / %:** 4340 b, 98.72,
  SCHED.
- **Exact difference:** only the inlined `CalcNBTSetup` `vtxFmt != NULL`
  block. Target issues every `type`/`normFrac` table literal and
  `lbz r3,0xc(r5)` before the first `lbzx r0,r6,r12` (`size[pos]`), and builds
  the uv GQR before the norm table stores.
- **Rule (inferred):** retail has **fewer** alias edges from the `size[pos]`
  stores to the `vtxFmt` loads, not a different statement order. All 720
  hazard-preserving orders of the six statements stay at or below about 99.1
  (best 99.08), so ordering cannot produce the target.
- **Evidence:** the DWARF params are non-`const` and the locals match, so
  neither the const lever nor a local change applies. The class is inferred
  from the schedule shape; no compiler or ablation has reproduced it yet.
- **Status:** compiler-missing (inferred; reclassified from (e)).

### 3.5 (e) Source shape unknown, compiler-invariant

#### `_rwGCNVtxFmtInstClr`

- **Unit:** world/pipe/p2/gcn/instance/geominst. **Size / %:** 2176 b, 97.13,
  COUNT (`lwz` +1, `stw` +1).
- **Exact difference:** the RGB565/RGBA4 8× unrolled loops need one more
  callee-saved register than retail.
- **Cause:** the scheduler, not register pressure: `#pragma scheduling off`
  still interleaves the `add`s. In RGB565 retail's 7th pointer and the IV share
  a vreg; ours are separate (@19/@18). RGBA4 differs elsewhere: p6/p7 are
  defined late, which depends on the body DAG.
- **Evidence:** identical under 1.3.2–2.7. A byte-pointer temp
  `{RwUInt8 *p = (RwUInt8 *)dstColor; p += stride; dstColor = (RwUInt16 *)p;}`
  reproduces retail's unrolled 565 block exactly, but breaks the remainder loop
  and RGBA4 and needs a local that is not in DWARF (98.11, not applied). The
  byte-pointer step `p = (T *)((RwUInt8 *)p + s)` fixed the RGB8 loop
  (94.33 → 97.13).
- **Status:** source unknown.

(`RxLockedPipeUnlock`'s size sum is also in this class; see 3.3.)

### 3.6 (f) Inliner decision

Empty. `UserDataListCopy` was here and is now solved (see the solved table):
the unit compiles with `-inline auto`, and a callee is inlined when its
complexity *after its own inlines* is at most 512 (measured with
`#pragma inline_max_size(N)` sweeps; every compiler 1.3.2-2.7 agrees). The
DWARF-shaped `UserDataCopy` helper put `UserDataListCopy` at 494, so it was
inlined into `UserDataObjectCopy`. RenderWare's explicit `NULL != x`
comparisons (as in its `rwstrdup` macro) add complexity without changing the
code; with them it is 520 and stays a `bl`, as in retail. Lever: an
unexpected auto-inline can be a size-accounting difference in code-neutral
style, not a compiler difference. The same accounting sets `ExtractNodes`'s
self-inline depth: its fix needed statements that keep the complexity where
retail's was (see the solved table and section 2).

### 3.7 (g) Link-only

#### `rtslerp`

Solved: linked. Retail's `.sdata2` numbers the 1.0, 0.0 and 2.0 literals at
@304..@306, far below `RtQuatSetupSlerpCache`'s own @500.., so earlier code in
the file created them first. The stripped matrix-slerp API (`RtSlerpCreate`,
`RtSlerpDestroy`, `RtSlerpInitialize`, `RtSlerpGetMatrix`, `RtSlerpSetLerp`,
from the public `rtslerp.h`) is reconstructed ahead of it: `RtSlerpGetMatrix`'s
delta clamp creates 1.0 then 0.0 and its quat-to-matrix conversion 2.0. The
game never calls them, so the DOL link strips them. The bodies are
reconstructions; only the constant order is hard evidence (the label gaps
suggest the real bodies were larger). Lever: when a unit's constant pool order
cannot come from its retained functions, restore the stripped functions.

---

## 4. Summary by residue class

| class | meaning | functions | bytes | fixable how |
|---|---|---|---|---|
| (a) | allocator rank unreachable by the numbering rules (or lever rejected) | 5: `_PropagateDependenciesAndKillDeadPaths`, `_rwStringStreamFindAndRead`, `RwTextureStreamRead`, `MatrixOrthoNormalize` (100% lever known, rejected), `_rpMaterialListStreamRead` | 7,316 | a new source lever; most candidates contradict DWARF |
| (b) | interference graph or pre-RA order differs | 3: `_rwFreeListAllocReal`, `RwImageResample`, `CameraBuildPerspClipPlanes` | 2,920 | source structure (none found) |
| (c) | needs a retail-only scheduler edge (`dss`/`volb`/`e3n4`, candidate behaviours of compiler B) | 5: `StalacTiteAlloc`, `_rwDlCameraBeginUpdate`, `RxLockedPipeUnlock` (`dss`); `_rwDlRasterShowRaster` (`volb`); `RpMaterialStreamRead` (`e3n4`) | 6,408 | compiler, if the edges gain provenance or more witnesses |
| (d) | alias precision no compiler has (R9a/R9b/R9c, plus `CalcMeshNBTs` inferred) | 5: `AtomicForAllLineIntersections`, `AtomicForAllSphereIntersections`, `_rpGameCubeMTEffectSend`, `_rwDlNativeTextureWrite`, `CalcMeshNBTs` | 9,244 | none known |
| (e) | source shape unknown, compiler-invariant | 1: `_rwGCNVtxFmtInstClr` (plus `RxLockedPipeUnlock`'s sum) | 2,176 | source |
| (f) | inliner decision | 0 (`UserDataListCopy` solved) | 0 | - |
| (g) | link-only | 0 (`rtslerp` linked) | 0 | - |
| **total** | | **19** | **28,064** | |

Roughly:
- 10 functions (15,652 bytes) are blocked by retail behaviour that no archived
  compiler has: the three rejected edges (c) and alias precision (d, with
  `CalcMeshNBTs` inferred).
- 8 functions (10,236 bytes) are register-allocator residue (a, b). All have
  exact replays; only `MatrixOrthoNormalize` has a known source lever, and it
  was rejected as not source-likely.
- 1 function is a source question with an unknown shape (e).

**Where the evidence is thin:**
- `dss` rests on three functions but its direct-symbol condition is fitted to
  `StalacMiteAlloc`; `volb` needs a `volatile` that is inferred from load order;
  `e3n4` rests on one function.
- R9b's discriminator is unknown: retail serialises the structurally
  identical `GetTexFrameMatrix` loop.
- `CalcMeshNBTs` is in (d) by inference from the schedule shape (fewer
  `size[pos]`→`vtxFmt` edges), not from a reproduced compiler behaviour.
- The (b) entries' pre-RA orders were shown to reproduce the target by editing
  the capture (`RwImageResample`, `_rwFreeListAllocReal`), not by a source
  change.


### Environment-matrix input capture follow-up (2026-10-03)

The earlier `_rpGameCubeMTEffectSend` locals control is now retained as a
small C source compromise: capture the six scaled x/y matrix components
before storing the upload matrix. These are genuinely used values, with no
extra storage or instructions. Retail likewise reads all six components
before its first upload-matrix store. This recovers that memory-operation
boundary without changing alias rules or compiler binaries.

Full deduplicated matching improves 95.84922 -> 97.95787 at the unchanged
1,804-byte retail size; the unit improves 98.67894 -> 99.086044. Only this
function changes in the full report. All exact code, data and completion
measures are unchanged; the unit remains 10/12 exact with all 64 data bytes
exact. Full source and normal builds pass, and the normal DOL SHA1 remains
`306526d90b48e99894c3138f5fc8f2716d9fecf6`. This is a NonMatching partial,
not proof of the TU's source link. Remaining FPR scheduling and config /
argument / loop register roles are unresolved, as is CalcMeshNBTs.

Evidence: `build/multitex-input-capture`, `build/multitex-{baseline,final}-report.json`
and `build/multitex-validation.json`. The prior blanket "compiler-missing"
label does not apply to this recovered source-level capture boundary.


### Native texture writer accounting follow-up (2026-10-04)

The previously measured format/accounting boundary is now retained under
current source-compromise guidance. `_rwDlNativeTextureWrite` obtains the
raster and builds its packed format before subtracting the completed native
texture header from `bytesLeftToWrite`. No call, write size, failure path,
or counter arithmetic changes. This is a small source ordering compromise,
not a claim that the original statement placement has been recovered.

Current GC/2.0p1f reproduces the gain: 97.748634 -> 98.59563 at unchanged
732-byte retail size. Raster parent/extension reads move before the counter
store, but the counter/format store order and several register roles remain
unmatched. The earlier blanket compiler-missing label overstates this gap.

The full deduplicated report changes only this function and fuzzy totals;
all exact-code, data, function, and completion measures are unchanged.
All-source compilation and the normal retail DOL SHA1 check pass. The unit
remains NonMatching; this is not a source-link closure. Evidence in the
isolated worker: `build/dltexdic88-{baseline,candidate}-report.json`,
`build/dltexdic88-validation.json`, and private diffs in `build/dltexdic88/`.
