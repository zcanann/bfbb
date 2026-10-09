# PS2 climate and decal source recovery

## Climate wind local

The three debug originals declare `_tagWind* w` inside `xClimateInitAsset`.
Restoring that local for the initial wind reset and direction setup recovers
the original saved register and stack frame. The no-climate branch continues
to write through `climate`, as the original instructions do.

The complete 340-byte initializer advances from 93.17647% to exact in USA,
Europe and Germany. Complete climate units advance from 168 to 508 exact bytes
and from four to five exact functions, with every other function score and
target/data control unchanged. GameCube retains all 12 exact functions / 1772
bytes. The current French source profile does not include this unit.

The source snapshot is based on staging `893cd6c8e`, with the already validated
lightning slot-bound change applied separately. Private evidence is
`build/climate-wind-final-comparison.json`, its function-delta JSON and
`build/climate-wind-gc-{before,after}.json`. The debug-local inspection is
reproducible with `build/climate-dwarf.py`. Existing unresolved runtime-call
limitations remain unchanged; this is not a fully linked runtime match claim.

## Decal culling

The reconstructed decal update omitted the PS2 culling path. Its original
`_loc` and `par_dist` locals belong to an inlined four-plane VU sphere test and
a scalar near-plane rejection, rather than unused declarations. Local PS2
helpers restore these operations before allocating a render-pool entry. A
culled decal still advances the queue iterator and remains alive; expiration
and pool exhaustion retain their original separate break paths.

The independent original-only audit in `build/decal-cull-proof.py` verifies all
three debug executables. It follows the declared `globals` type through the
zero-offset `zGlobals` base, `xGlobals::camera`, and `xCamera::frustplane` at
offset 0x270. The plane addresses are USA 0x52cb60, Europe 0x52c660 and Germany
0x52c060. All three update bodies are 1268 bytes and contain the same VU kernel
at offset 0x200. The audit checks all four preload offsets, the four-lane
arithmetic masks, the packed negative-lane test, and the `vmul.w` instruction
in the rejection branch's delay slot. These lanes and delay slots remain part
of ordinary matching; they are not masked from the source comparison.

The scalar test uses the independently addressed plane components at offsets
0x40, 0x50, 0x60 and 0x70 from that plane base. It computes the original dot
product minus plane distance plus half the stored radius, and renders only
when that result is less than -0.1. The same authenticated kernel is also
present in the independently investigated PS2 particle culler. Helpers remain
local to this translation unit and introduce no new call identities or shared
header changes.

The update improves from 48.630917% to 92.32808% in each debug region; complete
decal units improve from 87.45661% to 97.30868%, retaining 2540 exact bytes and
all other function scores and target/data controls. The French profile's
existing 468-byte member remains exact; it does not yet include this update.
All 46 GameCube functions / 4388 bytes remain exact. Evidence is
`build/decal-cull-proof.json`, `build/decal-cull-final-comparison.json`, its
function-delta JSON and `build/decal-cull-gc-{before,after}.json`. Remaining
differences include curve-index reloads and integer-register assignment; no
exact or complete-runtime match is claimed.

Caching the decal's promoted curve index before the two adjacent curve-node
lookups further raises its update from 92.32808% to 94.31546% in each debug
region. The update's fraction helper remains unchanged: caching its separate
index regressed and was discarded. Complete units reach 97.75676%, with every
other score, exact byte count and size/data control unchanged. France and all
46 exact GameCube functions remain unchanged. Evidence is
`build/decal-index-final-comparison.json`, its function-delta JSON,
`build/decal-index-gc-{before,after}.json`, and the repeated compiled VU
lane/delay-slot audit in `build/decal-index-compiled-proof.json`.

The climate declaration of `gPTankDisable` now agrees with its existing
`extern const U32` definition in `zParPTank.cpp`. This restores the original
HI/LO address load instead of an incorrect GP-relative access. Both original
translation units identify the same unsigned 32-bit object and zero initial
value in each debug version (`build/climate-ptank-type.json`); their DWARF
records do not separately establish a const qualifier. The correction is
bounded to this consumer, without changing the object or other declarations.

`UpdateRain` improves from 93.83189% to 94.44348% in each debug region. All other
scores, 508 exact bytes and target/data controls remain unchanged, as do all
12 exact GameCube functions. Evidence is
`build/climate-const-final-comparison.json`, its function-delta JSON and
`build/climate-const-gc-{before,after}.json`.

Keeping the curve-array pointer local across each adjacent-node lookup brings
the decal update to 94.67508% in all three debug regions, and complete units to
97.83784%. This removes another two redundant loads, reducing the compiled
update from 1300 to 1292 bytes against the 1268-byte original. Every other
function score, exact byte count and target/data control is unchanged, as are
France and all 46 exact GameCube functions. The complete comparisons and raw
VU/delay-slot recheck are `build/decal-curve-final-comparison.json`, its
function-delta JSON, `build/decal-curve-gc-{before,after}.json` and
`build/decal-curve-compiled-proof.json`.
