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
