# PS2 Patrick parabola source follow-up

Expanding the ordinary three-statement parabola position evaluation at the PS2
conveyor collision call site improves ParabolaHitsConveyors (584 target bytes)
from 87.03425% to 87.35616%. The expressions and helper operations are the same
as xParabolaEvalPos; removing one inline layer changes the field-address and
floating-temporary handling. The non-PS2 path retains its existing call.

The full 62-function source units for USA, PAL and German change only this
function record. Unit fuzzy rises from 99.70952% to 99.71409%, with 27,624 exact
bytes unchanged. The newly proved French body has the same improvement. All
three GC full-unit reports retain identical function records and measures.
Evidence is `build/patrick-parabola-region-summary.json`, before/after reports
under `build/patrick-parabola-regions/`, `build/patrick-parabola-gc.txt` and the
French `build/patrick-parabola-eval-expand.json` pilot.

The remaining mismatch includes hoisted component-address temporaries and extra
saved integer registers. Replacing the full inner vector arithmetic with scalar
expressions removed the extra integer saves but reduced overall similarity to
85.10274% by changing floating register allocation and scheduling. Expanding
only the vector-length expressions scored 61.77397%; combining that with the
position expansion scored 67.45206%. Those candidates were discarded. No
compiler-version cause or compiler patch is claimed.
