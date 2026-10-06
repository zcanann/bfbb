# French bounds, particle initialization, and update-culling coverage

This metadata-only change extends three existing whole-source comparisons. It
recovers ten function bounds (3,548 bytes) from authenticated originals, without
changing game source, compiler settings, or the full section denominators.

| Complete source TU | Newly identified functions / bytes | New standard matched bytes |
| --- | ---: | ---: |
| `xUpdateCull.cpp` | 3 / 1,620 | 1,620 |
| `xBound.cpp` | 6 / 1,852 | 0 |
| `xPar.cpp` | 1 / 76 | 0 |

All three named debug originals independently establish each unique complete
French sequence. Every member passes the unchanged strict control-flow/frame
validator; intervening alignment is zero. The modules check all prior neighboring
identities, actual addresses, original hashes, and canonical linkage names.
Existing public function records are preserved.

Update culling uses four prior neighbors to identify all seven original members.
The 1,484-byte initializer, 128-byte distance callback, and eight-byte constant
callback compare at 100% under the ordinary report settings. Its complete TU is
still partial: four of seven functions, 1,732 of 3,040 bytes.

The initializer's group-count call is corroborated by the complete 220-byte
sequence comprising the already identified 200-byte item accessor, eight zero
alignment bytes, and 12-byte count getter. Its sole JAL at offset 88 independently
resolves to the existing 128-byte `zSceneFindObject` identity. German is explicitly
re-proved through that whole context because the earlier exact-body accessor
registry had only USA/PAL provenance. Neither the count getter nor any other
context is added as a new progress symbol or relocation identity.

Allocator pointer slots are correlated against the independently reviewed
360-byte original allocator neighborhood. Actual slot addresses must remain
inside authenticated runtime BSS. Two runtime destinations retain their literal
unchanged transfer words and identical 64-byte entry context; they receive no
new names or extents. The profile normalizes only already proven calls, the
existing `gActiveHeap` address, and the two real local callback address pairs.
These code scores do not establish fully resolved relocations or a retail link.

Bounds uses three prior neighbors and replays the complete, previously verified
platform-geometry sequence for its external calls. Original DWARF proves the
single changed `xqc_def_ctrl` address is a 60-byte `xQCControl` inside runtime BSS.
The six new comparisons remain partial; the earlier 208-byte exact function is
unchanged. The original eight-byte drawing stub remains an unmatched original
member where the compiled source does not emit that symbol.

Particle initialization uses three prior neighbors. The original row-major array
descriptor proves `gParPool` contains 2,000 96-byte particles, entirely within
runtime BSS. No data extent is promoted. The initializer remains 96.052635%:
source materializes the pool through `v1` and copies it to `a0`, while retail
materializes it directly into `a0` with an intervening NOP. No source workaround
was introduced.

Validation uses actual complete source objects and the standard full French
report. Against published `140f17204`, matched code rises from 42,352 / 227
functions to 43,972 / 230. Known function count rises from 622 to 632. Every old
function record keeps its score and all unrelated function records and integer
measures remain unchanged. Seven unrelated aggregate fuzzy percentages differ
only by less than 1e-12 from floating-point summation. Total
CPU code stays 2,979,968 bytes with 2,384,384 bytes of data and zero matched data.
No complete-unit or source-link claim is made.

Private evidence is retained under `build/par256`: individual original proofs,
actual compiler commands and whole objects, unit reports, input comparison,
and `full-report-proof.json`. Fifty-four authenticated whole objects are reused
from a completed earlier gate after verifying unchanged PS2 source inputs; seven
whole objects are freshly compiled. The only changed shared headers reduce to
identical tokens with Xbox-only branches excluded. All changed PS2 source units
are recompiled. The final report is compared to the actual published CI artifact,
not merely to the older object cache.
