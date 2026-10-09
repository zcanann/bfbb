# PS2 streak and lightning source lifetimes

With the restored PS2 vertex-color setter, two remaining renderer mismatches
come from source-local lifetimes rather than a compiler-version difference.

`xFXStreakRender` now groups its two element pointers and declares the remaining
loop count before the ring-buffer index. No initializer or executable statement
changes. This restores the original pointer and saved-register allocation and
makes the 940-byte function exact in US, Europe and Germany.

`RenderLightning` tests `i & 1` at each texture-coordinate choice instead of
keeping an extra `odd` local alive across the vertex writes. The index does not
change between these tests, and the original DWARF lists no `odd` local. The
compiler still shares the calculation where appropriate, with the original
register lifetimes. The complete 4256-byte function becomes exact in the same
three regions.

Complete source-unit comparisons use the SDK restoration commit `2c5b52301` as
their baseline. Each debug region gains two exact functions and 5196 bytes:

| Unit | Exact bytes before | Exact bytes after |
| --- | ---: | ---: |
| `xFX.cpp` | 11924 | 12864 |
| `zLightning.cpp` | 2952 | 7208 |

Every other function score remains unchanged, as do function counts, target
sizes and data measures. The available French `xFX` profile remains unchanged
at 1820 exact bytes; these two functions are not identified in that profile,
so no French matching gain is claimed.

The shared source changes also preserve every exact GameCube function:
`xFX` retains 148 exact functions / 23256 bytes and improves the streak renderer
from 95.118515% to 95.525925%; `zLightning` retains all 17 exact functions /
12448 bytes with every score unchanged. No platform conditional was needed.

Private evidence is in `build/renderers-final-comparison.json` (seven complete
before/after unit-region pairs), `build/renderers-final-changes.json`, and
`build/{streak,lightning}-gc-{before,after}.json`. The regional rows record retained
compiled objects and complete function scores. These are ordinary source-code
comparison results; unresolved RenderWare call identities and data references
retain their existing limits, and no full-link claim is made. Profiles, identities,
relocation rules and compiler patches remain unchanged.

The subsequent ribbon comparison fix restores an actual out-of-line call:
the original `compare_ribbons` calls `xFXRibbon::render_compare`, while the
reconstructed compiler settings inline it. A PS2-only `dont_inline` scope around
that definition preserves the original call structure. Both `compare_ribbons`
(84 bytes) and `xFXRibbonRender` (308 bytes) become exact in all three debug
regions, adding another 392 bytes per region. Complete `xFX` comparisons advance
from 12864 to 13256 exact bytes with every other score and all size/data controls
unchanged; the French profile remains at 1820 exact bytes.

The 84-byte comparator also reproduces each debug original byte for byte after
applying its actual JAL relocation to the independently identified callee.
The private proof is `build/ribbon-comparator-proof.py` and its JSON result;
complete unit evidence is `build/ribbon-call-final-comparison.json` and
`build/ribbon-call-final-changes.json`. The guarded pragma preserves every
GameCube function score (`build/ribbon-call-gc-{before,after}.json`). This does
not expand the existing identity claims for the other renderer's SDK calls.

The ribbon strip's local vertex helper has the opposite requirement: its two
calls must be expanded inside `xFXRibbon::render_strip`, as in the original.
Declaring that existing helper `inline` at both its forward declaration and
definition restores the full 1112-byte strip function, from 9.374% to exact.
All three debug `xFX` units advance from 13256 to 14368 exact bytes with every
other score unchanged. France remains unchanged, and the shared inline declaration
preserves every GameCube function score, including 148 exact functions / 23256
bytes. Evidence is `build/ribbon-strip-final-comparison.json`, its function-delta
JSON, and `build/ribbon-strip-gc-{before,after}.json`. No compiler-version patch or
platform conditional is needed for this helper.

The normal helper then improves from 67.01744% to 98.77907% in all four
PS2 regions without changing the exact-byte count. Original DWARF lists only `a`, `b`,
`ax`, `ay` and `az`; the extra cached vector components were needed for the
GameCube reconstruction. The PS2 branch reads the absolute values directly and
reloads components in the first arm, as its original does. Local `peephole off`
and `opt_common_subs off` scopes preserve the original separate multiplies/adds
and repeated loads. The GameCube branch and its rounding remain unchanged.

The three original 688-byte normal functions have the same instruction stream
after excluding only actual J/JAL destinations (addresses US `0x1e5da0`, Europe
`0x1e6280`, Germany `0x1e5560`). The remaining source differences are the first
trigonometric call's argument-save scheduling and one branch-alignment NOP.
Diagnostic compilers 2.4 and 3.0.1 do not solve them (74.169% and 59.773%
respectively), so this is not evidence for a new compiler patch. Production
comparison rules and the compiler stay unchanged.

Complete regional evidence is `build/ribbon-normal-final-comparison.json` and
its function-delta JSON; France receives the same normal-helper improvement,
with no other function-score or size/data changes in any region. GameCube preserves every
score (`build/ribbon-normal-gc-{before,after}.json`). Original instruction-shape
evidence is in `build/ribbon-normal-original-shape.json`; the existing unresolved
trigonometric call identities are not newly promoted by this comparison.

Initializing `zLightningFunc_Render`'s vertex count before its vertex-pointer
setup moves the zero store toward its original position and improves that
function from 95.46598% to 95.75052% in all three debug regions. Complete units
retain 7208 exact bytes with every other function score and all size/data controls
unchanged. The GameCube unit remains 17/17 exact (12448 bytes). Evidence is
`build/lightning-init-final-comparison.json`, its function-delta JSON, and
`build/lightning-init-gc-{before,after}.json`. The French source profile does not
currently contain this function, so this change makes no French score claim.

The ring update's minimum-lifetime choice now initializes its local with the
equivalent conditional expression. This restores the original branch shape and
makes all 192 bytes exact in each debug region (formerly 95.833336%). Complete
`xFX` units advance from 14368 to 14560 exact bytes without other score or
size/data changes. France and every GameCube score remain unchanged. Actual
DWARF-backed data relocations reproduce all three original functions byte for
byte (`build/ring-update-proof.py` and `ring-update-raw-proof.json`). Complete
regional and GameCube evidence is `build/ring-update-final-comparison.json`,
its function-delta JSON, and `build/ring-update-gc-{before,after}.json`.

The fireworks update initializes its local trail-emitter flags after updating
the firework position and immediately before copying that position into the
emitter settings. This is the original store order; no call occurs between
these local operations. It restores the 1472-byte function from 98.75% to exact
in all three debug regions, raising complete `xFX` units to 16032 exact bytes.
All other regional function scores, size/data controls, the French profile and
every GameCube score remain unchanged. Evidence is
`build/fireworks-update-final-comparison.json`, its function-delta JSON and
`build/fireworks-update-gc-{before,after}.json`. Existing unresolved SDK call
limitations remain in place.

PS2 scene entry omits the unused `num_fx_atomics` definition and reset retained
by the GameCube source. The static counter has no reader or escaped address,
none of the three PS2 debug streams names it, and the original function lacks
the reconstructed extra store. The bounded platform guard improves
`xFX_SceneEnter` from 94.21429% to 95.36466% in each debug region. Complete
regional comparisons retain all exact bytes and other function scores, with
unchanged target sizes/counts and data measures; France and every GameCube
score remain unchanged. Evidence is `build/fx-counter-dwarf-audit.json`,
`build/fx-scene-enter-final-comparison.json`, its function-delta JSON, and
`build/fx-scene-enter-gc-{before,after}.json`.

All three debug originals declare the environment-map lookup local as
`RwTexture*`, rather than the reconstructed `void*`. Restoring that pointer type
recovers the original branch scheduling when the helper is inlined into the
bubble and shiny renderers. The bubble pass flags are one-bit unsigned fields,
so their extra `char` casts are redundant and introduce unwanted PS2 masks.
Removing those casts and restoring the pointer type makes the 608-byte bubble
and 568-byte shiny functions exact in each debug region, adding 1176 bytes.
The standalone lookup helper stays exact. Complete `xFX` units reach 17208
exact bytes; every other regional score and target/data control is unchanged,
as are France and every GameCube score. No platform conditional is needed.

The original pointer-type DIE audit is `build/fx-env-pointer-dwarf.json`.
Complete comparisons are `build/fx-bubble-final-comparison.json`, its
function-delta JSON, and `build/fx-bubble-gc-{before,after}.json`. This is an
ordinary source comparison with the existing unresolved SDK identity limits.
