# Xbox matrix and quaternion comparison

The complete `xMath3.cpp` now compiles and links with five real dependency TUs
and the pinned compiler runtime. A diagnostic host retains all 39 matrix APIs;
it does not implement game behavior or enter the progress denominator. Thirty
independently reviewed original functions (5164 bytes) enter the standard
comparison, including every partial match. The other nine source APIs have no
newly assigned original extents.

Both original versions gain **2114 matched bytes**, increasing the Xbox total
from **3354 to 5468**. The matrix unit is **76.18060% fuzzy**, with **14/30 exact
functions** and no complete-TU or executable-relink claim.

| Exact function | Original entry | Bytes |
| --- | --- | ---: |
| xBoxUnion | 0x016c30 | 142 |
| xBoxFromCircle | 0x144f60 | 224 |
| xMath3Init | 0x145040 | 121 |
| xBoxInitBoundCapsule | 0x145340 | 143 |
| xBoxFromCone | 0x1453d0 | 223 |
| xMat3x3Normalize | 0x1454b0 | 38 |
| xMat3x3Euler(vector) | 0x1457d0 | 24 |
| xMat3x3RMulRotY | 0x145a40 | 263 |
| xMat3x3Transpose | 0x145b50 | 101 |
| xQuatFromMat | 0x145e80 | 326 |
| xQuatFromAxisAngle | 0x145fd0 | 79 |
| xQuatToMat | 0x146020 | 205 |
| xQuatMul | 0x1462f0 | 135 |
| xQuatDiff | 0x146380 | 90 |

## Original evidence

The original particle routines independently anchor GetEuler, LookVec and
scalar Euler. Their decoded call graph and the original component-level math
identify the wider cluster: basis construction, axis rotations, matrix products,
quaternion trace/diagonal cases and oriented circle bounds. Cone calls Circle
twice and Union once. Circle is at `0x144f60` and Union at `0x016c30`; the next
two functions after the quaternion cluster are unrelated and were rejected.
Names were not assigned merely by source declaration order or adjacency.

Twenty-six matrix entries already had original-only anonymous closed-CFG and
caller proofs. RMulRotY has a further direct caller. Normalize, vector Euler and
pivot rotation have no direct E8 entry witness in the original candidate
inventory: their evidence is the original aligned entry, preceding RET/CC gap,
closed CFG, return/following CC gap and distinctive math/callee semantics. This
limitation is recorded rather than inventing callers. `xMat3x3Mul` has a final
alias-path backward jump; its reachable return is earlier at `0x145ccc`.
Optional `closed_cfg` evidence replays the actual original flow and requires the
exact reviewed extent, instruction coverage and internal gaps. Earlier reviewed
records keep their terminal-RET rule.

The independently named vector normalizer at `0x15cf00` (163 bytes) supports
matrix call relocations. Its original length-squared/FSQRT/scaling branches and
callers establish its identity; it is not added as a source comparison in this
batch. Existing anonymous extents are replaced without double counting. Known
coverage grows by only four functions / 471 bytes to 2547 functions / 633159
bytes; the full 1798760-byte code denominator and data denominator are unchanged.

## Source and relocation evidence

Xbox sine/cosine/tangent wrappers now use ordinary inline calls to the standard
float APIs. The original scalar Euler uses six hardware FSIN/FCOS operations;
the former out-of-line wrappers introduced six calls. This header ownership
change improves that partial comparison without changing any previous report
unit. `gc/iMath.cpp` retains its full former body outside the Xbox branch.
No compiler flags, assembly, source arithmetic tricks or score policy change.

The new address-field case accepts only decoded `C7 05 disp32 imm32` stores.
The displacement must have an actual source PE HIGHLOW relocation at that exact
field; its offset/size and both operands are checked. The trailing immediate is
never normalized. All twelve original identity-matrix stored values remain
unchanged through comparison and inverse reconstruction.

`xQuatFromMat` uses the real local static `nxt[3] = {1,2,0}`. Original indexed
loads at `0x145efe` and `0x145f05` establish its address `0x2870bc`; the actual
12 initializer bytes are verified. Source location is independently discovered
from the unique HIGHLOW-backed indexed load in the named compiled function,
with `xMath3.obj` ownership and writable mapped range checked. The source's own
12 initializer bytes are also verified. No target address or target instruction
offset is used to discover the source table.

The fourteen exact functions contain **32 DIR32 fields and nine REL32 fields**.
Each normalized source body reconstructs the actual linked PE, and those exact
functions independently reconstruct both authenticated original bodies. Unknown
original callees remain raw in partial functions. Every one of the thirty
source comparisons round-trips its actual PE. Existing xString, xPar,
xParGroup and xParCmd report-unit JSON and normalized source bytes/relocations
are identical; the genuine 117-byte CRT conversion helper remains unchanged.

## Reproduction

```sh
python tools/platforms/verify_xbox_reviewed.py --orig-dir /private/orig
python tools/platform_progress.py report --version XBOX-US \
  --orig-dir /private/orig --build-dir build/platforms \
  --xbox-compilers /xbox-compilers --objdiff /tools/objdiff-cli --wine /usr/bin/wine
```

Repeat the report for `XBOX-EU`. Actual local production reports are under
`build/xbox178/final`; independent all-source/original inverse and unchanged
prior payload evidence is `build/xbox178/validation.json`. No binaries are
committed. GC/PS2 source branches are preserved; the parent integration performs
the final normal GC build and retail checksum check.
