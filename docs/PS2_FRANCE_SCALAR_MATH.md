# Complete French scalar-math comparison

The French `xMath.cpp` profile now compares the actual complete source TU against
all 18 independently identified original members. Nine newly recovered bounds
add 1,896 bytes of classified code. Four new standard code matches add 212 bytes:
`xFuncPiece_EndPoints` (60), `xAccelMoveTime` (120), `xsrand` (8), and `xMathExit`
(24). The whole TU remains partial at nine matched functions / 748 of 3,700 bytes.

Each of the three named debug originals independently proves the unique complete
3,808-byte sequence, including alignment, using nine already reviewed neighbors.
All non-address instructions remain unchanged. Only three internal transfer
fields are masked; literal calls into unnamed runtime code remain unmasked and
must also retain identical 64-byte entry context. No runtime name, function
extent, or progress is credited from those contexts.

Two narrowly scoped boundaries preserve actual original behavior without
relaxing the generic CFG checker:

- The 60-byte `xFuncPiece_EndPoints` wrapper has fourteen exact non-transfer
  words and one terminal J at offset 52. Its instructions leave SP and RA
  unchanged. It targets the adjacent original 284-byte `ShiftPiece`, which
  independently passes strict CFG/frame checks in both images.
- The original eight-byte `xatof` is exactly an unchanged J/NOP tail. Its literal
  runtime destination and context agree across all four originals. The compiled
  source is present as a 36-byte body calling `atof` and converting double to
  float; it remains unmatched. No guessed return-type or SDK change is made.

The two `xAccelMove` overloads use address-qualified selectors with their actual,
distinct canonical linkage names from original DWARF. All existing function
records and prior scores are retained. Only the two proven inter-function
transfers receive relocations; no unknown callee is assigned an identity.

Raw verification separately establishes 180 newly matched bytes: all 60 bytes of
EndPoints after applying its actual R_MIPS_26 relocation to the independently
identified ShiftPiece entry, and all 120 AccelMoveTime bytes with no relocations.
The eight-byte seed setter and 24-byte exit body still have unresolved GP data
relocations; their standard code matches are not raw/link claims. The TU is not
marked complete, and no full executable link is claimed.

Full-report validation uses the actual published `0880fe1bf` French CI artifact
and the already accepted bounds/culling metadata. The combined report is 44,316
matched bytes / 235 functions and 641 known functions. Scalar math contributes
212 bytes / four matches beyond the accepted 44,104 / 231 baseline. Every prior
function score remains unchanged; unrelated integer measures and full code/data
denominators remain unchanged. Minor aggregate float summation differences are
bounded below 1e-12. Total CPU code is 2,979,968 bytes and total data is 2,384,384
bytes, with zero matched data.

Private evidence is in `build/math266`: all-original replay, reproducible module
output, actual whole-source compile command/object, normal report, independent
raw relocation audit, and final full-report proof. The final gate reuses 58
hash-authenticated whole objects from the completed prior gate after verifying
unchanged PS2 input tokens, and freshly compiles Math, String, and Volume to
include all source changes in the frozen `cc35f3d7e` baseline. No game source,
compiler flags, generic decoder, or scoring settings change in this metadata
handoff.
