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
separate large `xShadowReceiveShadowFastPS2` routine remains unrecovered.

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
