# Compiler variants GC/2.0p1b/c/d/e: audit of each change

This audits the changes that `tools/patch_compiler_rw.py` makes on top of
GC/2.0p1a, to decide whether each one is a legitimate model of the compiler
that built the retail objects ("compiler B") and whether game code can move to
GC/2.0p1d. GC/2.0p1a itself (joey's clauses A/B/C/E3n/F/S/V/W/H and the LICM
hook) is taken as given; only the deltas are audited.

Names used below:

- **A**: stock GC/2.0p1, the compiler our patches start from.
- **C**: GC/2.5 (2.6 and 2.7 behave the same on every repro here).
- **B**: retail, as seen through the target objects.
- **2.0p1a/b/c/d/e**: our derived compilers in `build/compilers/GC/`.

## Verdicts

| change | what it does | class | meets the criteria? |
|---|---|---|---|
| (b) V skips temporaries | clause V's literal-kill walk does not fire on a store to a compiler temporary | emulation correction | **Yes** |
| (c-R3) const-pointer LICM | loads through a pointer-to-const are hoisted out of loops even when the loop has calls, stores or early returns | grafted from C | **Yes**, with one documented caveat (`RpHAnimKeyFrameStreamWrite`, a source-shape question) |
| (c-veto) large-loop veto | the >25-instruction veto in LICM tests AltiVec splats instead of FP loads | grafted from C | **Yes** |
| (d-R4) two-register-load alias | a register with no alias does not force worst_case on an X-form access | grafted from C | **Yes** |
| (d-gate) address-taken gate | E3n/A/W/V fire on a frame object only when its address escapes | emulation correction | **Yes** for at-sched and at-w. at-v is weaker: it has no effect alone and pays off only with at-sched |
| (e-rep) const-pointee alias | a const/restrict pointee gets 2.5's plain subrange alias instead of 2.0p1's one-member SET (with `nps`, a guard so 2.0p1a's clauses don't treat the pseudo-object as a static) | grafted from C | **Yes** |
| (e) dss, volb, e3n4 | three extra scheduler edges seen only in retail | retail-only | **No**, kept as experimental parts; see "Not adopted" |

**Game code.** Moving it from 2.0p1a to 2.0p1d measures **+9 exact / −0** over
the 224 SB units (7442 → 7451). 11 partial functions go up and 2 go down. The
current game source was written and tuned under 2.0p1a, so the measurement is
biased toward 2.0p1a, and every part still has zero losses.
**Recommendation: move game code to GC/2.0p1d.** See the last section for the
two partials that go down and what to watch.

## Method

- **Repros.** These are self-written files in `tools/compilerprobe/repros/`:
  `b_vtemp.c`, `r3_licm.c`, `r3_scope.c`, `r3_this.cpp`, `c_veto.c`, `d_r4.c`
  and `d_gate.c`.
  - `run.py` compiles each file with the project's RW flags (or with
    `--flags sb`, the game flags; `.cpp` files always use the game's C++
    flags).
  - For each function it prints the compilers grouped by byte-identical code.
    Literal names and labels are normalised.
  - Every grouping quoted below is identical under RW flags and under game
    flags.
- **Ablation compilers.** `build_parts.py <dir>` derives single-part and
  forced-gate builds from the compilers in `build/compilers` and writes them
  only to `<dir>`. Their SHA-1s:

  | build | SHA-1 |
  |---|---|
  | `c_r3only` | `7e4ee255…` |
  | `c_vetoonly` | `79c4fb97…` |
  | `s_veto` | `305bfd5e…` |
  | `s_r4` | `d194bed2…` |
  | `a_r4` | `5c8a78ba…` |
  | `d_r4`, `d_atsched`, `d_atw`, `d_atv` | equal `P1D_PART_SHA1` |
  | `d_gate` | `2f4a1047…` |
  | `d_nov` | `c3db793e…` |
  | `d_always` | `aa6d3cf5…` |
  | `never_e3na` | `7cc0f576…` |
  | `never_w` | `2d97e040…` |
  | `never_v` | `aed7d46b…` |

  The derived compilers were checked against the recorded hashes:

  | compiler | SHA-1 |
  |---|---|
  | 2.0p1 | `74bc177b` |
  | 2.0p1a | `a78a5fdb` |
  | 2.0p1b | `c8e72ea9` |
  | 2.0p1c | `0a0662b2` |
  | 2.0p1d | `0a4878bb` |

- **Unit sweeps.** `sweep_units.py` runs `tools/solo.py <unit> --mw <compiler>`
  on all 224 SB units and all rwsdk units, then diffs per function.
  - Other agents were committing while this ran, so the sweeps used a frozen
    copy of commit `c2cf5cb6c`: `git archive` of src, include and tools, plus
    the target objects and the real `build/compilers`.
  - Two later commits matched more RW functions:
    - `976c9c28f` matched `VectorMultPoint`, `VectorMultVector` and
      `RpGeometryStreamWrite`.
    - `d10c877fa` linked bavector.
  - Those two units were re-measured live at `d10c877fa`, and the results are
    noted where they matter.
  - RW counts are for every rwsdk unit compiled with the stated compiler,
    including `stdkey`, which `configure.py` builds with GC/2.0p1.
- **Stale build.ninja.** `build.ninja` on disk is stale: it still says
  GC/2.0p1a for the RW units, although `configure.py` says 2.0p1d. A plain
  `solo.py` on an RW unit therefore uses 2.0p1a until `configure.py` is
  re-run. Every measurement here passes `--mw` explicitly.

Headline totals (frozen tree, exact functions):

| | 2.0p1 (A) | 2.5 (C) | 2.0p1a | 2.0p1b | 2.0p1c | 2.0p1d |
|---|---|---|---|---|---|---|
| RW (1027 fns) | 913 | 873 | 970 | 974 | 981 | **991** |
| game (7673 fns) | – | – | 7442 | 7442 | 7444 | **7451** |

RW, 2.0p1a → 2.0p1d: **+22 / −1**. The one loss is `RpHAnimKeyFrameStreamWrite`,
in `stdkey`, which is built with GC/2.0p1 anyway (see (c-R3)). Against stock
2.0p1 the score is +79 / −1 (the same function), and against 2.5 it is
+118 / −0.

---

## (b) Clause V skips stores to compiler temporaries (2.0p1b)

**What it does.** Clause V is 2.0p1a's value-numbering store kill. A store
kills every cached small static, which in practice means every `.sdata2`
literal, so the next use reloads it. 2.0p1a runs that walk when the stored
object's base is a static *or a frame object* (`0x10005`). 2.0p1b adds a stub
in front of 2.0p1a's VN entry-0 handler. When the store's base is a frame
object with no declared `Object` (`*(base+0x18) == 0`, a compiler-invented
slot), the stub takes the compiler's own stock whole-object kill instead.
Every other store goes to clause V exactly as before.

**Class: emulation correction.** No stock compiler has clause V, so the
question is where it is over-broad, not where it came from.

**Code.**

| item | address |
|---|---|
| VN dispatch table | `0x5bd068`, entry 0 (file `0x1BA668`) |
| stock kill | `0x511a53` |
| 2.0p1a's V/F stub | `0x60e5f8` |
| new stub | `.sbpatch+0xF00` (`0x60ef00`) |

The key the stub tests is "frame object with no declared object". That is a
compiler-level property: the temporaries come from `fctiwz`/`stfd` float→int
conversion slots and `stw`/`lfd` int→float slots. It is not tied to symbols,
sizes or functions.

**Repros** (`b_vtemp.c`; L = number of `.sdata2` literal loads):

| function | A 2.0p1 | C 2.5/2.6/2.7 | 2.0p1a | 2.0p1b/c/d |
|---|---|---|---|---|
| `colour`: 3× `(u8)(255.0f*x)` into a local passed by value | L=1 | L=1 | **L=3** | L=1 (2.0p1d byte-identical to A) |
| `loop_conv`: `(int)(v[i]*1000.0f+0.5f)` in a loop | L=4 | L=4 | **L=13** | L=4, byte-identical to A |
| `two_conv`: two conversions sharing literals | ≡A | (2.5 schedules `lfs` above `stwu`) | extra `fmr` copies | byte-identical to A |
| `int_to_float`, `to_unsigned` (controls) | same | same | same | same |

**Retail.** `_rwGCLightsLocalEnable` in the target loads `@287` once
(`lfs f3`) and reuses it across three `fctiwz`/`stfd` conversion temporaries.
2.0p1a reloads it after each `stfd`; this is the `colour` repro exactly. The
same pattern decides these four RW functions:

| function | A | C | 2.0p1a | 2.0p1b |
|---|---|---|---|---|
| `_rwGCLightsGlobalEnable` | 100 | 100 | 86.22 | 100 |
| `_rwGCLightsLocalEnable` | 100 | 100 | 92.46 | 100 |
| `_rxPTankGameCubeRenderCallBack` | 100 | 100 | 82.99 | 100 |
| `RwImageCreateResample` | 100 | 78.9 | 95.11 | 100 |

`RwImageResample` also rises, 85.72 → 97.82.

**Why joey's clause was over-broad.** V was introduced for stores to statics,
with a +25 game witness set. It was later widened to frame objects; the
`stdkey` pun unions and `xcsCalcAnimMatrices` needed that. Compiler
temporaries were never part of any witness. DUPLOTRON already recorded the
temporary-skip as 0 / 0 on the game with four partials up.

**Deltas (frozen tree, 2.0p1a → 2.0p1b).**
- RW: **+4 / −0**.
- Game: **+0 / −0**. Four partials go up (`xFXStreakRender` 76.80 → 92.19,
  `NCIN_SleepyLamp_AR` 94.02 → 99.73, `xFXShineRender` 94.06 → 97.89,
  `xScrFXGlareRender` 62.40 → 65.04) and none go down.

**Caveats.** None found. A and C agree with retail in every witness.

**Verdict: meets the criteria.**

---

## (c-R3) LICM of loads through pointer-to-const (2.0p1c)

**What it does.** A load whose base register holds a pointer-to-const
parameter or `this` is loop-invariant. This holds when the loop contains
calls, stores through other pointers, or early returns. It also holds when the
loop stores through a non-const pointer to the same type: 2.5 trusts the
qualifier, and so does the variant.

**Class: grafted from C.**

**Code, A against C.**

| | A (2.0p1) | C (2.5) |
|---|---|---|
| pseudo-object maker for the `mr` of a pointer-to-const register variable | `0x513330` builds a one-member alias set (`mov byte [esi+0x2c],2`, then `0x512e20` / `0x512d80`) | `0x513620` returns the plain alias from `0x513110(obj, 0, 0x7fffff)`; no set |
| LICM flag filter | `0x56f461` (`and eax,0x1a8; jne`) | `0x56fb03`: the same filter, but a plain alias never sets fIsPtrOp (0x20), so the load passes |
| `isloopinvariant` | `0x570f60` rejects alias kind 2 | walks the pseudo-object's defs, of which there are none |

2.0p1c reaches the same result without rewriting the alias builder:

- **R3_FILTER**: lets a "cps" load past the 0x20 filter.
- **R3_INV**: swaps in the set's single member before calling the stock
  `isloopinvariant`. If the member has no use-def node, it enters at
  `0x5710d0`.

The key is compiler-level: a one-member set over an object of the
pseudo-object type `0x5bd008`, with every address register defined only by
`mr` instructions that carry such a set (2.5's "propagate through `mr` only"
rule). The code is at `.sbpatch+0xC00`.

**Repros** (`r3_licm.c`, `r3_this.cpp`, `r3_scope.c`; identical under RW and
game flags). "Split" means [A 2.0p1a 2.0p1b] ≠ [C 2.6 2.7 2.0p1c 2.0p1d],
with the variant byte-identical to C.

| case | result |
|---|---|
| `call_c` (call in loop), `store_c` (store through another pointer), `find_c` (early return), `both_c` (store through a non-const `L*` plus a call), `fsum_c` (float data), `nested_c` (`o->inner->…`), `index_c`, C++ `S::sum_c() const` | **split** |
| controls without `const`: `call_nc`, `store_nc`, `find_nc`, `S::sum_nc` | one group (nobody hoists) |
| cases C does not hoist: `local_c` (const pointer returned by a call), `derived_c` (pointer made by `add`), `global_c` (global pointer-to-const) | one group: the variant does not over-reach |
| soundness: `walk_c` (`p = p->next`), `bump_c` (`p++`), `cond_c` (pointer reassigned on some iterations) | one group: nothing hoisted anywhere |
| `r3_scope.c`: CSE across a call or store, scheduling across a store, outside loops | one group: A and C already agree outside loops, so grafting only the LICM consequence loses nothing observable |

**Retail.** RW functions that need R3. In the table, `c_r3only` is 2.0p1b + R3
with the veto unchanged:

| function | A | C | 2.0p1b | c_r3only | 2.0p1c/d |
|---|---|---|---|---|---|
| `_rpMaterialListFindMaterialIndex` | 59.29 | 100 | 59.29 | 100 | 100 |
| `_rpMaterialListStreamGetSize` | 51.77 | 100 | 51.77 | 100 | 100 |
| `_rpMaterialListStreamWrite` | 75.85 | 100 | 75.85 | 100 | 100 |
| `RwImageCopy` | 92.48 | 100 | 92.48 | 100 | 100 |
| `RpGeometryStreamGetSize` | 92.94 | 100 | 92.94 | 100 | 100 |
| `_rpSkinMatrixBlendUpdate` | 82.05 | 100 | 82.05 | 100 | 100 |
| `_rwFrameListFindFrame` | 88.23 | 100 | 88.23 | 100 | 100 |
| `GameCubeMTEffectStreamWrite` | 85.47 | 100 | 85.47 | 100 | 100 |
| `RpGeometryStreamWrite` (at `d10c877fa`) | 93.01 | 100 | 93.01 | 100 | 100 |
| `ImageConvertDepth` / `RwImageApplyMask` / `UserDataListCopy` | 92.3 / 96.5 / 96.5 | 99.8 / 99.1 / 98.5 | same as A | same as C | same as C |

**Game.** +1 / −0 from R3 alone: `xfont::irender` 95.61 → 100, a `const`
member function, exactly the `r3_this.cpp` shape.

**Exceptions: one function goes down.**
- **The loss.** `RpHAnimKeyFrameStreamWrite` (stdkey) is 100 under A and
  94.26 under C and under every R3 build. Retail reloads
  `animation->numFrames` on every iteration, although our source declares
  `const RtAnimAnimation *animation`.
- **This does not prove that C differs from B.** The in-tree callback
  typedef `RtAnimKeyFrameStreamWriteCallBack` (`include/rwsdk/rtanim.h:22`)
  takes a non-const `RtAnimAnimation*`, and `rphanim.c` casts the function
  to it.
- **Test.** A private copy of `stdkey.c` with a non-const parameter scores
  **8/8 under 2.0p1d**, and 8/8 under 2.0p1. With that source the unit would
  not need its GC/2.0p1 override.
- **Reading.** Most likely the original signature was non-const, which makes
  this a source question, not a counter-example. If the `const` is genuine,
  this is the only measured case where B does not hoist and C does.

**Deltas.**
- RW, 2.0p1b → R3 alone: +8 / −1 (the loss is the stdkey caveat).
- Game: +1 / −0.

**Verdict: meets the criteria** (the caveat is a source-shape question).

---

## (c-veto) Large-loop veto moved from FP loads to AltiVec splats (2.0p1c)

**What it does.** `moveinvariantsfromloop` refuses to hoist out of a loop of
more than 25 instructions (`[ebx+0x38] > 0x19`) that contains a certain
opcode.
- A tests LFS..LFDUX:
  `0x570985: sub eax,0x89 … sub eax,5; cmp eax,7`.
- C tests VSPLTISB/H/W:
  `0x571035: sub eax,0xd8; cmp eax,2`, the same surrounding code.
- 2.0p1c rewrites A's two immediates to C's values.
- A Gekko never emits a splat, so in practice the veto disappears.

**Class: grafted from C.** It is a literal copy of C's two constants into A's
identical code.

**Repros** (`c_veto.c`). `s_veto` is stock 2.0p1 + the veto move only.

| case | A | s_veto | C | 2.0p1a/b | c_r3only | c_vetoonly | 2.0p1c/d |
|---|---|---|---|---|---|---|---|
| `big_sfield`: a big FP loop reading a global struct field | 13 lit | **≡ C** | 7 lit | ≡ A | ≡ A | ≡ A | ≡ A |
| `big_c`: a big FP loop over a const matrix | 38 `lf` | ≡ A | 28 `lf` | ≡ A | ≡ A | ≡ A | **≡ C** |
| `big_nc`, `big_local` (controls) | one group | | | | | | |

- **Provenance.** `big_sfield` shows it cleanly: A + the graft is
  byte-identical to C.
- **Interaction with R3.** `big_c` shows the veto mattering for our variant:
  hoisting the const matrix out of a big FP loop needs **both** R3 and the
  veto move.
- **Retail.** At `d10c877fa`, `VectorMultPoint` is 12.38 and
  `VectorMultVector` 28.11 under A, 2.0p1b, `c_r3only` and `c_vetoonly`.
  Both are **100** under C and under 2.0p1c/d. Neither half alone matches;
  both together do.

**Exception (C ≠ retail, already handled by 2.0p1a).** C also hoists static
and literal reads out of big loops (`big_static`, `big_sfield`). The 2.0p1a
family never does, because of 2.0p1a's LICM hook: "a whole static read is
never loop-invariant". That hook is retail-evidenced, at −11 game functions
when removed (DUPLOTRON). So the veto graft deliberately brings C's veto but
not C's static hoisting, and on those repros the variant correctly stays with
A.

**Deltas.**
- RW, frozen tree: 0 / 0. At current HEAD it adds the two `VectorMult*`
  functions together with R3.
- Game: **+1 / −0** (`zFX validate_popper` 91.65 → 100), plus
  `xParCmdAnimalMagentism_Update` 81.04 → 85.85.

**Verdict: meets the criteria.**

---

## (d-R4) Alias of two-register (X-form) loads (2.0p1d part `r4`)

**What it does.** The pass at A `0x5123b0` computes the alias of an access.
For an access with two address registers where neither register's defs are
unknown, A answers worst_case:
`0x51258b jne 0x5125ba`, which stores worst_case.
The prologue's callee-save spills carry the worst_case set, so a
`lwzx rD, obj, off` is ordered after every spill.

C rewrote the pass (`0x512570`). At `0x512603`, a register whose helper
(`0x512700`) returns no alias contributes nothing (`test esi,esi; je`), so
the access keeps its own alias. 2.0p1d changes one byte, `75 2d → 75 31`, so
that A's `jne` goes to the "keep the access's alias" tail at `0x5125be`.

**Class: grafted from C.**

**Repros** (`d_r4.c`). `s_r4` is stock 2.0p1 + the byte.

| case | A | s_r4 | C | 2.0p1a/b/c | a_r4 / 2.0p1d |
|---|---|---|---|---|---|
| `xform_c`: `*(T**)((const char*)obj+off)` | `lwzx` after all four spills | **≡ C** | `lwzx r31` right after `stw r31` | ≡ A | **≡ C** |
| `array_c` (`const u8 *a; a[i]`) | | ≡ C | | ≡ A | ≡ C |
| `xform_nc`, `array_nc` (non-const: no alias) | one group, pinned everywhere | | | | |
| `store_x` (X-form store) | one group | | | | |
| `table_c` … `table6_c` (static tables) | one group | | | | |

The docstring's note that static-table indexed loads "keep the table's alias,
as 2.5 does" produced no observable difference in six shapes. A and C agree
there, so the byte introduces nothing.

**Retail.** In the target, `CollisionDataStreamWrite` has `lwzx r31,r5,r6`
directly after `stw r31` and before `stw r30` / `stw r29`. That is C's
ordering; 2.0p1c puts it after all three spills.

| function | A | C | 2.0p1c | 2.0p1d |
|---|---|---|---|---|
| `CollisionDataStreamWrite` | 89.29 | 100 | 89.29 | 100 |
| `MultiTextureStreamGetSize` | 85.00 | 100 | 85.00 | 100 |
| `MultiTextureStreamWrite` | 92.59 | 100 | 92.59 | 100 |

The same three are gained by `a_r4`, which is 2.0p1a + the byte, so R4 does
not depend on R3.

**Deltas.**
- RW: +3 / −0, both on top of 2.0p1c and on top of 2.0p1a.
- Game: **+1 / −0** (`zDiscoFloor set_object_state` 70.17 → 100).

**Verdict: meets the criteria.**

---

## (d-gate) E3n/A/W/V only on frame objects whose address escapes (2.0p1d parts `at-sched`, `at-w`, `at-v`)

**What it does.** `in_wc(obj)` (`.sbpatch+0x700`) walks the worst_case alias
set (`[0x5e9cb4]`) for a member whose object is `obj`; CodeWarrior collects
address-taken objects into that set. Each clause is then wrapped:

| part | clause | entries | wrapper | effect when the frame object does not escape |
|---|---|---|---|---|
| at-sched | E3n (`0x60e13c`) | sched entries 0 and 3 | `+0x740` | answers "no alias" |
| at-sched | A (`0x60e2a0`) | sched entry 0 | `+0x760` | answers "no alias" |
| at-w | W (`0x60e1c8`) | entries 0 and 1 | `+0x7b0` | answers "no alias" |
| at-v | V | VN entry 0 | `+0x7d0` | the stock kill (no V walk, no F) |

**Class: emulation correction.** No stock compiler has any of these clauses.

**What A and C do.** In every `d_gate.c` case, escaping or not, A and C
schedule the literal or static load freely. This holds for C as well where
C's code differs from A's for other reasons.

**Repros** (`d_gate.c`):

| case | A / C | 2.0p1a/b/c | 2.0p1d |
|---|---|---|---|
| `pun_local`: float/int union, address never taken (E3n) | literal hoisted | pinned below the store | **≡ A** |
| `pun_esc`: the same union passed to `take(&u)` | hoisted | pinned | **pinned** (≡ 2.0p1a) |
| `scalar_esc` (A), `w_esc` (W), `v_esc` (V): escaping locals | free | pinned | pinned |
| `v_local`: V on a non-escaping union | ≡ A | differs | **≡ A** |
| `scalar_local`, `w_local`, `col_esc`, `col_local` | one group (registerised, or no clause fires) | | |

Per-clause forced builds (`never_e3na`, `never_w`, `never_v`) confirm that
each escaping case is decided by its own clause.

**Retail evidence for both halves.**

1. **Non-escaping locals: retail lets the load pass**, so the clause is
   over-broad there.
   - `d_always` (in_wc forced to 1, which equals 2.0p1c + R4) loses seven RW
     functions that 2.0p1d matches:

     | function | A | C | d_always |
     |---|---|---|---|
     | `RpLightGetConeAngle` | 100 | 100 | 84.50 |
     | `MatFunc1` | 100 | 95.44 | 53.28 |
     | `RtQuatSetupSlerpCache` | 100 | 98.62 | 91.52 |
     | `_rpPTankGameCubeCreateCallBack` | 100 | 100 | 95.90 |
     | `RpHAnimKeyFrameBlend` | 100 | 97.92 | 88.33 |
     | `RpHAnimKeyFrameInterpolate` | 100 | 97.89 | 88.51 |
     | `MeshRenderEnvMap` | 99.31 | 99.31 | 97.61 |

   - On the game, `d_always` loses six: `BasisBspline`, `render_fade`,
     `zNPCGoalJellyBirth::Process`, `zNPCGoalPatrol::MoveNormal`,
     `zSceneSetup`, `zEntPlayer_AnimTable`.
   - Concretely, in `MeshRenderEnvMap` retail issues the literal `lfs`
     *above* `li r0,0xff; stb r0,0x1b(r1)`, the store to the local colour
     `shiney`. 2.0p1c (E3n) keeps it below the store.
2. **Escaping locals: retail still pins**, so the clause must stay there.
   - `d_never` (no clause on any frame object) loses **18 RW functions** and
     **200 game functions**. RW examples: `nCut` 79.31, `RpCollisionWorldForAllIntersections`
     87.37, `RwMatrixRotateOneMinusCosineSine` 81.35, `_rwDlRasterClear` 88.23,
     `_rwDlRenderStateFogEnable` 90.22.
   - By clause:

     | clause off for escaping objects | RW | game |
     |---|---|---|
     | E3n + A | −18 | −181 |
     | W | −2 | −42 |
     | V | 0 | −2 (`xcsCalcAnimMatrices`, `xSweptSphereToTriangle`) |

   - `MeshRenderEnvMap` itself needs both halves: 100 only under the gate,
     99.31 with the clauses never firing, 97.61 with them always firing.

So the split "does the address escape" is exactly the discriminator that
docs/RW_RESIDUE.md R2/R6/R7 could not find on opcode, size or partialness. It
is a compiler-level property, worst_case membership, and is not tied to
symbols.

**Why joey's clauses were over-broad.** They gate on "declared frame object"
(`Object+0x18 != 0`). Every witness that motivated E3n and W, such as
`stwx` into `anim_list[]`, stores into `pos[100]` and `tranresult[]`, and
GXColor argument temporaries, involves a local whose address is passed on.
The pun unions and plain scalars that are never address-taken were caught
only incidentally.

**Deltas (frozen tree).**

| part (on 2.0p1c) | RW | game |
|---|---|---|
| at-sched | +3 / −0 | **+5 / −0** |
| at-w | +1 / −0 (`MatFunc1`) | 0 / 0 (2 partials up) |
| at-v | 0 / 0 | 0 / 0 |
| all three | +7 / −0 | +6 / −0 (`zEntPlayer_AnimTable` needs at-sched and at-w together) |
| 2.0p1d without at-v (`d_nov`) | 989 | 7451 |
| 2.0p1d | 991 | 7451 |

`d_nov` against 2.0p1d shows at-v's whole yield: with at-sched present, at-v
completes `RpHAnimKeyFrameBlend` and `RpHAnimKeyFrameInterpolate` (96.7 → 100)
and has no game effect.

**Caveats.**
- **The gate is not the complete discriminator.**
  - `d_never` also *gains* `xBoxFromCircle` (77.88 → 100) and
    `zNPCFodBzzt::Setup` (95.35 → 100). These are DUPLOTRON's long-standing
    E3n cost witnesses: retail lets a load pass a store to an escaping
    object there.
  - Both fail under 2.0p1a too, so this is residue, not a regression.
- **Two game partials go down under at-sched.** `xFont get_bounds` goes
  67.11 → 64.02 (61.42 at-sched alone), and `zNPCFodBzzt::DiscoRender`
  79.10 → 77.59. Both are far from matching.
- **at-v.** On its own it is 0/0. Its only measured effect is the two `stdkey`
  functions, which configure.py builds with GC/2.0p1 anyway. The
  escaping-object V it retains is supported by only 2 game functions.
  It is consistent and costs nothing, but it is the least evidenced part.

**Verdict.**
- **at-sched and at-w meet the criteria**: a compiler-level key, retail
  evidence on both sides, and zero losses.
- **at-v is admissible but weakly evidenced.** It is the same key, applied
  to V for consistency, and has no losses.

---

## (e-rep) 2.5's alias for a const/restrict pointee (2.0p1e parts `rep`, `nps`)

**Mechanism.** Both compilers give the `mr` of a pointer-to-const or restrict
variable an alias on a fresh pseudo-object. 2.0p1's maker (0x513330) wraps it
in a one-member alias SET; 2.5's (0x513620) returns
`make_alias(pseudo, 0x7fffff, 1)`. `rep` makes 2.0p1 return 2.5's form (one
site, 0x51336c). Everything downstream is stock code identical in 2.0p1 and 2.5:

- LICM hoists loads through the const pointer. With `rep`, 2.0p1c's R3 hooks
  never see a SET and are inert (objects byte-identical with or without them),
  so R3 turns out to be a consequence of this one representation change.
- AddPropagation (identical code in both) refuses to fold an `addi` whose alias
  covers a whole non-frame object. 2.5's transfer function produces that alias
  for a loop's pointer increment, so an unrolled const-pointer loop keeps one
  `addi` per copy. This is residue class R10 in RW_RESIDUE.md; it is not an
  unroller difference (the unroller is the same code in 2.0p1 and 2.5).

`nps` is required with `rep`: 2.0p1a's clause predicates test "static" as
object kind 5, which a pseudo-object also is. Stock 2.0p1 never exposed a bare
pseudo-object to them. Without `nps`, clause V kills pseudo values and three
iCollide functions are lost. Alone it changes nothing.

**Provenance.** Grafted from C. **Generality.** `e_rep_*.c` (written for this,
not RW source): every function where A and C differ, 2.0p1e equals C
(`f`, `g`, `b2`, `u1`, `u2`); every function where they agree stays in one group.
On the earlier repros (`b_`, `c_`, `d_`, `r3_`), 2.0p1e is identical to 2.0p1d
on all 64 functions. **Measured** against 2.0p1d over every unit: RW +0 / -0 on
the existing source; game **+3 / -0** (`xtextbox::layout::yextent`,
`zNPCSpawner::Owned`, `zNPCBPlankton::player_left_territory`). With the RW-style
in-loop form (`nDot = dot(&sphere->center, ...); nDot -= distance;` compared
against `sphere->radius`), `RwCameraFrustumTestSphere` goes 90.77 -> 100.
`ImageConvertDepth` was fixed by source shape in the same change (100 under any
compiler).

## Not adopted: retail-only edges `dss`, `volb`, `e3n4`

`tools/patch_compiler_rw.py --p1e-parts ... --out DIR` still builds them, for
experiments only. None has a later-compiler counterpart (2.0p1, 2.5, 2.6 and 2.7
all lack each edge), so the provenance criterion fails, and the generality
evidence is too thin to count as an emulation correction:

| part | rule | evidence | why not |
|---|---|---|---|
| dss | a store addressed directly by symbol (`sym@sda21`) to a static field is ordered before a later store to a frame-object field | RW +2 (`StalacTiteAlloc`, `_rwDlCameraBeginUpdate`) | The direct-symbol condition exists only because `StalacMiteAlloc` (the same store through a base register) has no edge in retail. That condition is fitted to the data. |
| volb | a volatile store is ordered before any later store | 0 / 0; +1 (`_rwDlRasterShowRaster`) only if `_RwGCXFBCopy` is also made `volatile` | It needs a source change and a compiler change together to show anything. |
| e3n4 | 2.0p1d's gated E3n also on scheduler entry 4 for small, non-literal-pool statics | RW +1 (`RpMaterialStreamRead`); `zNPCFodBzzt::DiscoRender` 77.59 -> 74.14 | It rests on one function, and the general forms measure -9 and -200. |

They are recorded as candidate behaviours of the real compiler B, to revisit if
more functions turn up the same pattern.

## Game code: can it move to GC/2.0p1d?

Measured per function, frozen tree, all 224 SB units, 2.0p1a → 2.0p1d:
**7442 → 7451 exact, +9 / −0.**

**Gains:**

| function | 2.0p1a → 2.0p1d | part |
|---|---|---|
| `xfont::irender` | 95.61 → 100 | R3 |
| `zFX validate_popper` | 91.65 → 100 | veto |
| `zDiscoFloor set_object_state` | 70.17 → 100 | R4 |
| `BasisBspline` | 96.26 → 100 | gate |
| `zEntPlayerOOBState render_fade` | 85.78 → 100 | gate |
| `zNPCGoalJellyBirth::Process` | 92.95 → 100 | gate |
| `zNPCGoalPatrol::MoveNormal` | 97.43 → 100 | gate |
| `zSceneSetup` | 99.74 → 100 | gate |
| `zEntPlayer_AnimTable` | 97.25 → 100 | gate |

**Partials up (11):**

| function | 2.0p1a → 2.0p1d |
|---|---|
| `xParCmdAnimalMagentism_Update` | 81.04 → 98.27 |
| `xFXStreakRender` | 76.80 → 92.19 |
| `NCIN_SleepyLamp_AR` | 94.02 → 99.73 |
| `zLightningFunc_Render` | 96.89 → 99.50 |
| `xFXShineRender` | 94.06 → 97.89 |
| `SkinXform` | 96.60 → 98.14 |
| `SkinNormals` | 96.42 → 97.18 |
| `SkinXformVertAndNormal` | 85.45 → 86.95 |
| `xScrFXGlareRender` | 62.40 → 65.04 |
| `zEntPlayer_Update` | 97.94 → 98.17 |
| `NPCCone::RenderCone` | 98.92 → 99.13 |

**Partials down (2):** `xFont get_bounds` 67.11 → 64.02 and
`zNPCFodBzzt::DiscoRender` 79.10 → 77.59.

Every part is independently non-negative on the game:
- b: 0 / 0
- R3: +1 / −0
- veto: +1 / −0
- R4: +1 / −0
- at-sched: +5 / −0
- at-w: 0 / 0
- at-v: 0 / 0

The game source was tuned against 2.0p1a, which biases the measurement toward
2.0p1a. It also gives independent evidence: none of these changes was derived
from game code, and they gain on it anyway.

**Recommendation: switch game code to GC/2.0p1d.** It is a strict improvement
in exact functions, uses one compiler model for game and RW, and loses
nothing. When switching:

1. Re-run `configure.py` (build.ninja is stale) and do a full build, so
   `report.json` and the DOL check confirm the solo measurements.
2. R3 now applies to every `const` member function and `const T*` parameter.
   A function that newly fails after a future source edit may be asking for
   the `const` to be removed, or added. The `stdkey` case above is the
   precedent.
3. Look at the two partials that go down (`get_bounds`, `DiscoRender`)
   before attributing their residue to source.

**Update:** game code and RW have since moved to GC/2.0p1e (+3 game functions
over 2.0p1d, -0; see (e-rep)).

## Reproducing

```sh
python tools/compilerprobe/repros/run.py                       # all repros, A/C/2.0p1a-d
python tools/compilerprobe/repros/run.py --flags sb            # same under game flags
python tools/compilerprobe/repros/build_parts.py <scratch>     # ablation compilers (never under build/compilers)
python tools/compilerprobe/repros/run.py c_veto.c --mw 2.0p1,<scratch>/s_veto,2.5
python tools/compilerprobe/repros/sweep_units.py --set sb --mw 2.0p1a,2.0p1d --out sb.json
python tools/compilerprobe/repros/sweep_units.py --diff sb.json 2.0p1a 2.0p1d
```

`sweep_units.py` accepts a scratch compiler as a path relative to
`build/compilers`, for example `../../<rel>/s_veto`.

## Candidate behaviours of the retail compiler, not adopted

GC/2.0p1e is the final compiler model. Research after it (2026-10-01)
characterised more retail-only alias behaviour; none of it is adopted, because
no released compiler (1.3 through 3.0) has it, and closing holdout functions
through ever more compiler patches is not a goal. Measured against 2.0p1e over
all 120 RW and 224 game units:

| candidate | rule | measured |
|---|---|---|
| DS+VND | a store addressed directly (`sym@sda21` or frame offset) to a named static is ordered before later accesses to an address-taken frame object or to another named static through a register (any load, or a pointer-op store), and kills cached whole values of other named statics | +10 / -0 (`StalacTiteAlloc`, `_rwDlCameraBeginUpdate`, `zThrown_LaunchVel`, `zThrown_AddFruit`, `zNPCHazard_ScenePrepare`/`SceneFinish`, `zGameModeSwitch`, `zCutsceneMgrPlayStart`, `zSaveLoad_Tick`, `zCameraFlyStart`); generalises the rejected `dss` |
| L8 | clauses C+/V treat the 8-byte int-to-float constant as a small literal | +3 / -0 (`iParMgrInit`, `zhud::setup`, `zhud::update`); one partial down (`Show_frame`) |
| WE | clause W extended to whole loads of statics of at most 8 bytes | +2 / -0 (`_rwDlNativeTextureWrite`, `zGameScreenTransitionUpdate`) |
| E3nc | clause E3n ignores the load's const flag | +1 / -0 (`zEntPlayer_Render`, since matched by source) |

The functions above are accepted holdouts. `tools/compilerprobe/qprobe.py`
logs a function's `may_alias` queries and finds the decisive one.
