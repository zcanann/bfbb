# RenderWare residue: why the last 24 RW functions do not match

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
   from the unchanged source. Today it is empty for all 24 functions.
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
| RW functions matched | **1015 / 1039** (97.69%) |
| RW units complete (linked) | **100 / 120** |
| RW code matched | 336,412 / 371,428 bytes (90.57%); the 24 residue functions are the whole 35,016-byte gap |
| RW data matched | 11,764 / 11,764 bytes (100%) |
| non-matching functions | 24, in 19 units |
| residue shapes | REG 13, SCHED 9, COUNT 2 |
| match 100% under GC/2.0p1 or GC/2.5 from the same source | 0 / 24 |

The 20th incomplete unit is `rtslerp`. All its functions match, but it cannot
link: its `.sdata2` constant order depends on `RtSlerp*` functions that were
stripped from the retail binary (see 3.8).

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
(N1–N10) were measured with `wcap.py`/`diag.py` on test files
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
  (`ImageConvertDepth`).

---

## 3. Per-function table

Columns:
- **%**: under GC/2.0p1e (the RW compiler).
- **alt**: the GC/2.0p1 / GC/2.5 scores from `residue.py`.
- **class**: the residue-class letter from section 4.

| class | function | unit | size | % | shape | alt (2.0p1 / 2.5) |
|---|---|---|---|---|---|---|
| a | `ExtractNodes` | driver/common/palquant | 616 | 99.51 | REG | 99.51 / 99.51 |
| a | `DlRasterTile` | driver/gcn/dlraster | 660 | 99.67 | REG | 99.67 / 99.67 |
| a | `_PropagateDependenciesAndKillDeadPaths` | src/pipe/p2/p2dep | 1296 | 99.32 | REG | 99.32 / 99.32 |
| a | `_rwStringStreamFindAndRead` | src/babintex | 980 | 99.33 | REG | 99.33 / 99.33 |
| a | `RwTextureStreamRead` | src/babintex | 2400 | 99.59 | REG | 99.59 / 99.59 |
| a | `MatrixOrthoNormalize` | src/plcore/bamatrix | 1132 | 99.52 | REG | 98.11 / 98.11 |
| a | `RwImageApplyMask` | src/baimage | 1340 | 99.09 | REG | 96.49 / 99.09 |
| a | `RpGeometryStreamRead` | world/bageomet | 3620 | 99.91 | REG | 99.91 / 99.91 |
| b | `_rwFreeListAllocReal` | src/plcore/bamemory | 432 | 99.68 | REG | 99.68 / 99.68 |
| b | `RwImageResample` | src/baresamp | 468 | 97.82 | REG | 97.82 / 57.81 |
| b | `CameraBuildPerspClipPlanes` | src/bacamera | 2020 | 99.11 | REG | 99.11 / 99.11 |
| b | `_rpMaterialListStreamRead` | world/bamatlst | 1508 | 97.77 | REG | 97.77 / 97.77 |
| c | `StalacTiteAlloc` | src/pipe/p2/p2define | 116 | 90.69 | SCHED | 68.45 / 68.45 |
| c | `_rwDlCameraBeginUpdate` | driver/gcn/dldevice | 1464 | 98.88 | SCHED | 97.42 / 97.42 |
| c | `_rwDlRasterShowRaster` | driver/gcn/dldevice | 1060 | 99.89 | SCHED | 80.17 / 80.17 |
| c | `RpMaterialStreamRead` | world/bamateri | 1008 | 99.15 | SCHED | 99.21 / 99.21 |
| c + e | `RxLockedPipeUnlock` | src/pipe/p2/p2define | 2760 | 98.06 | COUNT | 97.41 / 97.41 |
| d | `_rwDlNativeTextureWrite` | driver/gcn/dltexdic | 732 | 97.75 | SCHED | 89.72 / 89.72 |
| d | `AtomicForAllLineIntersections` | plugin/collis/ctgeom | 1432 | 98.30 | SCHED | 98.30 / 95.78 |
| d | `AtomicForAllSphereIntersections` | plugin/collis/ctgeom | 936 | 98.24 | SCHED | 98.24 / 98.24 |
| d | `_rpGameCubeMTEffectSend` | plugin/matfx/gcn/multiTexGcnPipe | 1804 | 95.85 | SCHED | 96.15 / 96.15 |
| e | `CalcMeshNBTs` | plugin/matfx/gcn/multiTexGcnPipe | 4340 | 98.72 | SCHED | 98.72 / 95.73 |
| e | `_rwGCNVtxFmtInstClr` | world/pipe/p2/gcn/instance/geominst | 2176 | 97.13 | COUNT | 97.13 / 97.13 |
| f | `UserDataListCopy` | plugin/userdata/rpusrdat | 716 | 98.52 | REG | 96.51 / 98.52 |

### 3.1 (a) Allocator: the needed rank is unreachable by the numbering rules

The interference graph is retail's; the replay names the vreg move that would
give retail's colours, but N1–N10 put no source-expressible web at that rank.

#### `ExtractNodes`

- **Unit:** driver/common/palquant. **Size / %:** 616 b, 99.51.
- **Exact difference:** the outer (out-of-line) copy only. FPR: `recip` v32
  and `weight` v33 swap. GPR: CSE `nodeIndex*4` @649 against the objectless
  `fctiwz` result load v72. The two self-inlined copies already match.
- **Rule:** N2 vs N5. The named-local order is reversed out of line (N2) and
  inlined (N5), so no declaration order fixes both contexts.
- **Evidence:** swapping the declarations fixes the outer copy and breaks both
  inlined copies (96.98). Forms that split the family (including a
  RepresentativeColor-style helper, which exists in Rat DWARF) change the
  self-inline depth: 68–83%.
- **Status:** REG, no lever.

#### `DlRasterTile`

- **Unit:** driver/gcn/dlraster. **Size / %:** 660 b, 99.67.
- **Exact difference:** 32-bit case: the pixel `lhz` v111 is created before the
  offset temps v112–v114 (the RHS is lowered first).
- **Move needed:** v111 to rank 114.5..122.5.
- **Evidence:** Rat DWARF confirms the local set, so no named local can be
  added. Seven address/load/index spellings: inert or worse.
- **Status:** REG, no lever.

#### `_PropagateDependenciesAndKillDeadPaths`

- **Unit:** src/pipe/p2/p2dep. **Size / %:** 1296 b, 99.32.
- **Exact difference:** the objectless `node->nodeDef` load v100 (the base of
  every `iospec->` access) r28→r31, which pushes `pipeline`, `node` and `i` down
  one register.
- **Move needed:** v100 to rank 42.5–44.5, i.e. into the named/split band.
- **Evidence:** IRO always copy-propagates `iospec` and any `nodeDef` local,
  so neither gets a web; declaration moves only move the dead slot
  (99.32/99.37).
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
- **Move needed:** a modified inline param (`length`) numbered outside its N5
  slot, which is impossible. TextureStreamRead also needs the
  `mipmapState`/`autoMipmapState` swap, which contradicts the DWARF
  declaration order (swapping gives 99.49).
- **Evidence:** the assign-then-reassign `bytesToRead` form gives it an object
  web (FindAndRead 99.53) but drops TextureStreamRead to 99.20.
- **Status:** REG, no lever.

#### `MatrixOrthoNormalize`

- **Unit:** src/plcore/bamatrix. **Size / %:** 1132 b, 99.52.
- **Exact difference:** FPR abs-dot block: abs-dot CSE temps @121/@125 against
  the component CSE temps @126–@131.
- **Move needed:** @121/@125 must rank after @126–@131, against N7's
  first-occurrence order.
- **Evidence:** dot-local shapes are inert; splitting dot/abs changes
  contraction (91.7).
- **Status:** REG (FPR), no lever.

#### `RwImageApplyMask`

- **Unit:** src/baimage. **Size / %:** 1340 b, 99.09.
- **Exact difference:** a callee-saved r30/r31 swap: `image->width` and the
  inlined `imagePalette` against `tempImage`.
- **Move needed:** `tempImage` is fully copy-propagated and has no web. The
  merged @218/@216 web must rank 33.5..59.5, below the inlined
  `RwImageAllocatePixels` `imagePalette` webs.
- **Evidence:** seven declaration and expression variants did not move it.
- **Status:** REG, no lever.

#### `RpGeometryStreamRead`

- **Unit:** world/bageomet. **Size / %:** 3620 b, 99.91.
- **Exact difference:** morph-target loop: `i` v43 r28→r27; FindLoops offset
  @733 v92 r29→r28.
- **Move needed:** a named `morphTarget` above FindLoops IV @733, which N6
  forbids. The inline-helper lever does not apply: the target has no inline
  helper here.
- **Evidence:** the DWARF-faithful block-scoped `i`s give 99.917; the
  no-variable `sizeTC` form gives 99.07.
- **Status:** REG, no lever.

### 3.2 (b) Interference graph or pre-RA order differs

No renumbering of our interference graph reproduces retail, so the source
builds a different set of webs or interferences, or the pre-RA scheduler
orders the defining instructions differently.

#### `_rwFreeListAllocReal`

- **Unit:** src/plcore/bamemory. **Size / %:** 432 b, 99.68.
- **Exact difference:** CSE v49 against objectless v63/v64.
- **Cause:** our pre-RA order is `addi v63; subi v65; add v64`, which ends
  v49's life before v64 is defined. Retail's order is `addi; add; subi`, so
  v64 interferes with v49. No rank move suffices.
- **Status:** graph differs.

#### `RwImageResample`

- **Unit:** src/baresamp. **Size / %:** 468 b, 97.82.
- **Exact difference:** a callee-saved GPR permutation. Target: r31=nY,
  r30=nXDelta, r29=nYPos, r28=nYDelta, r27=dstWidth, r26=dstHeight, r25=src,
  r24=dst, r23=span ptr, r22=`nYPos+nYDelta-1`, r21=nX, r20=nXPos.
- **Cause:** the pre-RA scheduler places `li nXPos`/`li nX` between the
  `mullw` and the walker `add`, which gives nX/nXPos degree 29 (= K), so they
  become survivors. Deleting the edges v64/v66–nX/nXPos in the capture
  reproduces the target exactly.
- **Evidence:** an `rpDstSpan++` form gives 99.15 but contradicts the DWARF
  `const` pointer, so it is not applied. (The R1t literal reloads are gone
  under 2.0p1b+.)
- **Status:** graph differs.

#### `CameraBuildPerspClipPlanes`

- **Unit:** src/bacamera. **Size / %:** 2020 b, 99.11.
- **Exact difference:** FPR, 29 webs. IroVars scalarisation temps @205..@213
  (vTmp, vTmp2, vRight, vUp, vCOP); `scale` f3→f29 (retail keeps it live across
  calls).
- **Evidence:** no pop order of our graph reaches the target. Rat DWARF has
  **four** `length2` locals (an earlier revision said three normalisations).
  Reorder search best 99.257, only via unnatural statement orders.
- **Status:** graph differs (FPR).

#### `_rpMaterialListStreamRead`

- **Unit:** world/bamatlst. **Size / %:** 1508 b, 97.77.
- **Exact difference:** a callee-saved permutation. Target: matList r31,
  stream r30, matindex-IV r29, i r28, len r27, len*4 r26. Ours: the locals
  first (@295 IV r31, i r30, matindex r29, material r28), then the params
  (matList r27, stream r26). 24 webs shift.
- **Cause:** retail needs the params ranked above the FindLoops walker plus a
  third move; N1/N6 put params below. Rat's locals match ours.
- **Evidence:** the same 97.77 under 1.3.2–2.7; 0 of 720 named-local orders
  work.
- **Status:** graph/structure differs.

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

### 3.5 (e) Source shape unknown, compiler-invariant

#### `CalcMeshNBTs`

- **Unit:** plugin/matfx/gcn/multiTexGcnPipe. **Size / %:** 4340 b, 98.72,
  SCHED.
- **Exact difference:** only the inlined `CalcNBTSetup` `vtxFmt != NULL`
  block. Target issues every table literal and `lbz r3,0xc(r5)` before the
  first `lbzx r0,r6,r12`, and builds the uv GQR before the norm table stores.
- **Rule:** list-scheduler priority over a DAG of a different shape; the same
  under every compiler.
- **Evidence:** all 720 hazard-preserving orders of the six statements: best
  99.08.
- **Status:** source unknown.

#### `_rwGCNVtxFmtInstClr`

- **Unit:** world/pipe/p2/gcn/instance/geominst. **Size / %:** 2176 b, 97.13,
  COUNT (`lwz` +1, `stw` +1).
- **Exact difference:** the RGB565/RGBA4 8× unrolled loops. Ours places the
  IV `add` early under register pressure, which needs one more callee-saved
  register.
- **Evidence:** identical under 1.3.2–2.7. The byte-pointer step
  `p = (T *)((RwUInt8 *)p + s)` fixed the RGB8 loop (94.33 → 97.13) but not
  these two.
- **Status:** source unknown.

(`RxLockedPipeUnlock`'s size sum is also in this class; see 3.3.)

### 3.6 (f) Inliner decision

#### `UserDataListCopy`

- **Unit:** plugin/userdata/rpusrdat. **Size / %:** 716 b, 98.52, REG.
- **Exact difference:** register residue only.
- **Evidence:** a DWARF-shaped static `UserDataCopy` helper with an indexed
  string loop gives **100% in isolation**. With it, our compiler auto-inlines
  `UserDataListCopy` into `UserDataObjectCopy`, while retail keeps the `bl`.
- **Status:** open question: the inliner's size accounting for the new helper.

### 3.7 (g) Link-only

#### `rtslerp`

All functions match, but the unit cannot link: the `.sdata2` constant order
needs the `RtSlerp*` functions that were stripped from retail. Not a function
residue.

---

## 4. Summary by residue class

| class | meaning | functions | bytes | fixable how |
|---|---|---|---|---|
| (a) | allocator rank unreachable by the numbering rules | 8: `ExtractNodes`, `DlRasterTile`, `_PropagateDependenciesAndKillDeadPaths`, `_rwStringStreamFindAndRead`, `RwTextureStreamRead`, `MatrixOrthoNormalize`, `RwImageApplyMask`, `RpGeometryStreamRead` | 12,044 | a new source lever; most candidates contradict DWARF |
| (b) | interference graph or pre-RA order differs | 4: `_rwFreeListAllocReal`, `RwImageResample`, `CameraBuildPerspClipPlanes`, `_rpMaterialListStreamRead` | 4,428 | source structure (none found) |
| (c) | needs a retail-only scheduler edge (`dss`/`volb`/`e3n4`, candidate behaviours of compiler B) | 5: `StalacTiteAlloc`, `_rwDlCameraBeginUpdate`, `RxLockedPipeUnlock` (`dss`); `_rwDlRasterShowRaster` (`volb`); `RpMaterialStreamRead` (`e3n4`) | 6,408 | compiler, if the edges gain provenance or more witnesses |
| (d) | alias precision no compiler has (R9a/R9b/R9c) | 4: `AtomicForAllLineIntersections`, `AtomicForAllSphereIntersections`, `_rpGameCubeMTEffectSend`, `_rwDlNativeTextureWrite` | 4,904 | none known |
| (e) | source shape unknown, compiler-invariant | 2: `CalcMeshNBTs`, `_rwGCNVtxFmtInstClr` (plus `RxLockedPipeUnlock`'s sum) | 6,516 | source |
| (f) | inliner decision | 1: `UserDataListCopy` | 716 | understand the inliner size accounting |
| (g) | link-only | unit `rtslerp` (0 functions) | 0 | `.sdata2` ordering without the stripped functions |
| **total** | | **24** | **35,016** | |

Roughly:
- 9 functions (11,312 bytes) are blocked by retail behaviour that no archived
  compiler has: the three rejected edges (c) and alias precision (d).
- 12 functions (16,472 bytes) are register-allocator residue (a, b). All have
  exact replays; none has a source lever consistent with DWARF.
- 3 functions are source questions: two unknown shapes (e) and one inliner
  decision (f).

**Where the evidence is thin:**
- `dss` rests on three functions but its direct-symbol condition is fitted to
  `StalacMiteAlloc`; `volb` needs a `volatile` that is inferred from load order;
  `e3n4` rests on one function.
- R9b's discriminator is unknown: retail serialises the structurally
  identical `GetTexFrameMatrix` loop.
- The (b) entries' pre-RA orders were shown to reproduce the target by editing
  the capture (`RwImageResample`, `_rwFreeListAllocReal`), not by a source
  change.
