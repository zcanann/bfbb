# PS2 emitter angle and circle helpers

The original angle-variation helper clears its angle array, then assigns each
random angle separately. Restoring that source form recovers its saved pointer
lifetimes and also improves the many emitters that inline it. The circle and
circle-edge emitters inline `xVec2Init` and `xVec2Dot`; their existing definitions
from `iMath3.cpp` are now supplied locally under PS2.

All three debug releases improve identically. Fifteen of nineteen function
scores rise and four remain unchanged. These are fuzzy gains: exact matching
remains zero of nineteen functions over 13,116 measured bytes. Representative
complete-body scores are:

| Function | Before | After |
| --- | ---: | ---: |
| AngleVariation | 78.14085% | 97.1831% |
| EmitPoint | 81.74117% | 97.64706% |
| EmitCircle | 73.83851% | 98.75777% |
| EmitCircleEdge | 74.89809% | 98.407646% |

Several remaining differences involve NOP placement in compiler-generated
zero-initialization loops; no compiler-version cause is established by this
change. France has no enabled source profile for this unit, so no France gain
is claimed. No target boundaries, scoring rules, registries or compiler flags
change.

The full GameCube USA report is identical and its retail DOL SHA-1 passes.
Private evidence is `build/emitter-oct09/*-{before,after}.json`, plus separate
angle and circle source snapshots and instruction diffs.
