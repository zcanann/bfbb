# PS2 shadow call boundaries

The original shadow initializer calls `SetupShadow` through the inlined camera
creation wrapper, and the eight-byte list wrapper tail-calls
`xShadowManager_Add`. PS2-only `dont_inline` scopes preserve those complete
callee bodies instead of expanding them into the callers. The initializer
also clears `shadow_ent_count` before calling `ShadowMapCreatePipelines`, as
shown by the original call's delay slot.

The 96-byte initializer and eight-byte list wrapper become exact in all three
debug releases: **104 bytes and two functions** each. Units rise from 18/34
functions and 4,760 bytes to 20/34 and 4,864 bytes. Every other function score
is unchanged. The full GameCube USA report remains identical and its retail
DOL SHA-1 passes.

France has no enabled source profile for this unit, so no France gain is
claimed. Compiler flags, profiles, target boundaries and registries remain
unchanged. Private evidence is `build/shadow-oct09/*-{before,calls}.json` and
separate instruction diffs for the original caller bodies.

## VU triangle projection

The original PS2 render and cache-draw routines load their projection matrix
into VU0 registers vf28-vf31. Their triangle callback packs three unaligned
input positions into vf1-vf3, transforms XYZ through that matrix, and writes
the results to its local projected-position array. Those original inline
assembly operations are now restored. The surrounding clipping and vertex
construction remain C. The callback also uses the original PS2 normal bias
of 0.002 and copies the precomputed packed shadow color to each vertex;
GameCube retains its 0.008 bias and channel setters.

The complete **192-byte projection sequence** emitted from source equals the
raw original executable bytes in USA, Europe and Germany. This check includes
the explicit `.xyz` VU destination masks: objdiff's displayed instruction text
and fuzzy score do not distinguish the assembler's default `.xyzw` form.
The core SHA-256 is
`09cf42220202d9fe8428575910905f230fdecc1efb117b8be5ac0327a3d32916`.

| Complete body | Before | After |
| --- | ---: | ---: |
| Triangle callback, 1,100 bytes | 49.4% | 98.501816% |
| Shadow render, 788 bytes | 84.67005% | 98.98477% |
| Cache draw, 1,028 bytes | 77.79378% | 96.420235% |

All three debug releases improve identically. Unit fuzzy matching rises from
82.26242% to 86.47515% in USA and from 82.262024% to 86.474754% in PAL.
Exact totals remain 20/34 functions and 4,864 bytes; every other function score
is unchanged. The full GameCube USA report remains identical and its retail
DOL SHA-1 passes. France still has no enabled profile for this unit. The
separate large `xShadowReceiveShadowFastPS2` routine is restored below.

Private regional reports use `*-vu.json`. `vu-raw-proof.json` records original
executable hashes, original core addresses and source-object hashes for the
raw-byte checks; the 192-byte proof is not a claim that the whole callback is
exact.

## Shadow manager material and render state

The original PS2 manager changes each shadow model's material pipelines from
the a4d skin variants to the corresponding adl variants before rendering the
shadow camera, then restores them afterward. Both ordinary and ADC variants
are handled. The reconstructed loops use the original global pipelines,
independently identified by their DWARF addresses. Manager index zero also
sets a ten-unit shadow volume before drawing and restores the saved PS2 alpha
test afterward. These operations were absent from the shared source.

The manager's NPC receiver selection remains a call to the complete
`xShadow_PickEntForNPC` helper under PS2. Together, these restorations raise
the complete 2,640-byte manager from 83.33182% to 99.84091% in all three debug
releases. Europe/Germany setup also restores the original 512x512 resolution
limits, raising that body's score from 97.072464% to 97.10145%.

Exact totals remain 20/34 functions and 4,864 bytes; every other score is
unchanged. The full GameCube USA report remains identical and its retail DOL
SHA-1 passes. France has no enabled profile. Private reports use
`*-manager.json`; the remaining manager differences are register assignments.


## Fast model receiver

`xShadowReceiveShadowFastPS2` previously had only a declaration. Its recovered
C body now sets blending, builds the shadow matrix, allocates transformed model
vertices, and batches receiver triangles. The original VU operations project
vertices, reject common outside clip planes, calculate and bias the normal,
and store positions/UVs. Original model/atomic diagnostic register snapshots
are retained. The camera direction loaded with `lqc2` is explicitly aligned
to 16 bytes, as in the original stack layout.

All **66 COP2 instruction words**, in order, are raw-identical to each original
USA, Europe and Germany body. This includes vector masks, register transfers,
clip operations and reciprocal-square-root waits. The check deliberately does
not claim an exact whole body: C/assembly boundaries retain extra branches or
NOPs, scalar/index register differences and constant-store scheduling. Source
emits 1,784 bytes for the original 1,760-byte function.

| Metric, each debug release | Before | After |
| --- | ---: | ---: |
| Fast receiver fuzzy | 0% (absent) | 93.6% |
| Unit fuzzy | 88.649574% | 96.868286% |
| Exact functions | 20/34 | 20/34 |
| Exact bytes | 4,864/20,044 | 4,864/20,044 |

Every other function score is unchanged. The full GameCube USA report remains
identical and its retail DOL SHA-1 passes. France has no enabled source profile
for this unit. Private evidence is `*-fast.json` and `fast-raw-proof.json` in
`build/shadow-oct09`; the latter records executable and source-object hashes,
original addresses, vector-sequence hashes, and the sole changed function.


## Exact cache callbacks

The model-cache leaf callback now follows the original debug locals: one
signed loop index, explicit per-iteration vertex pointers, Z-first dot products
in the vertex-distance tests, and one reused `denom` for the four offset ray
depths. The leaf and environment callbacks also restore PS2's direct
`base - offset` arithmetic instead of negating `offset - base`. GameCube keeps
its existing arithmetic and loop forms.

These are source fixes; no compiler adjustment is involved. Both complete
bodies are exact in USA, Europe and Germany:

| Complete body | Before | After |
| --- | ---: | ---: |
| `shadowCacheLeafCB`, 2,132 bytes | 87.69043% | 100% |
| `shadowCacheEnvCB`, 1,024 bytes | 96.171875% | 100% |

Each region gains **3,156 exact bytes and two functions**. Unit totals rise
from 20/34 and 4,864 bytes to 22/34 and 8,020 bytes; fuzzy matching rises from
96.868286% to 98.37318%. Every other function score is unchanged. The full
GameCube USA report remains identical and its retail DOL SHA-1 passes. France
and Xbox have no enabled source profile for this unit. Private evidence uses
`*-cache.json` and `cache-proof.json` under `build/shadow-oct09`.


## Exact entity callback

The 996-byte `shadowCacheEntityCB` now uses the original conditional reciprocal
expressions when constructing capsule gradients. The prior zero-initialize /
conditional-assignment form omitted several NOPs and chose different floating
registers. Restoring `nonzero ? reciprocal : zero` reproduces the complete
original body; the apparent scheduling residual was a source-form issue.

USA, Europe and Germany all improve from 95.56225% to 100%, gaining another
996 exact bytes and one function. Unit totals are now 23/34 functions and
9,016/20,044 exact bytes, with 98.5937% fuzzy matching. Every other score is
unchanged. This shared rewrite also preserves the full GameCube USA report
and retail DOL SHA-1. France/Xbox have no enabled unit profile. Evidence uses
`*-entity.json` and `entity-proof.json` under `build/shadow-oct09`.


## Exact cache fill

`xShadowVertical_FillCache` restores the original PS2 stack-local order, placing
`cbparam` after the intersection, sorted depths and quick-cull data. Capsule
endpoints read the input position directly, and quick-cull receives the local
intersection directly. These remove unnecessary aliases while preserving the
original load sequence. The original DWARF stack locations independently
confirm the restored layout.

The complete 740-byte function improves from 94.88108% to 100% in USA, Europe
and Germany. Totals are now 24/34 functions and 9,756/20,044 exact bytes, with
98.78268% fuzzy matching. Every other score is unchanged. The full GameCube
USA report remains identical and its retail DOL SHA-1 passes. France/Xbox
have no enabled unit profile. Evidence uses `*-fill.json` and `fill-proof.json`.
