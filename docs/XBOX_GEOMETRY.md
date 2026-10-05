# Xbox sphere, ray and box geometry

The complete existing `gc/iMath3.cpp` implementation is now compiled for Xbox,
with four real math dependency TUs and the pinned compiler/runtime. No game
source, headers, flags or backend behavior changes are needed. The original
platform unit is labelled `SB/Core/xbox/iMath3.cpp`; the profile explicitly
points to the shared implementation's actual file.

Instruction-sequence fingerprints locate a coherent original geometry cluster
at `0x16b360..0x16c201`. These fingerprints propose identities only. Every new
entry is then checked against the original's decoded closed control flow,
actual direct callers, return boundaries, scalar operations and component
layout. All eleven identified functions enter the normal comparison, including
four partial matches. Other source functions without independently reviewed
original locations remain unassigned rather than borrowing similar tiny bodies.

Seven routines reproduce **2485 original bytes** after verified named call and
literal relocations:

| Exact function | Original entry | Bytes |
| --- | --- | ---: |
| iSphereIsectVec | 0x16b360 | 73 |
| iSphereIsectRay | 0x16b3b0 | 332 |
| iBoxVecDist | 0x16b670 | 1313 |
| iBoxIsectVec | 0x16bba0 | 91 |
| ClipBox | 0x16bca0 | 201 |
| iBoxIsectRay | 0x16bd70 | 330 |
| iBoxBoundVec | 0x16c170 | 145 |

Both Xbox reports increase **5468 -> 7953 matched bytes**. The geometry unit
is **93.55293% fuzzy**, with **7/11 exact functions**. All five prior compared
unit JSON records and normalized source bytes/relocations remain identical.
All eleven new source comparisons round-trip their actual linked PE; the seven
exact routines reconstruct both original versions byte for byte.

The original inventory increases by only two functions / 157 bytes to 2549
functions / 633316 known bytes, as the other newly named extents replace prior
anonymous records. Full code and data denominators are unchanged, including
the 1798760-byte code denominator.

The eleven original functions total **3668 bytes**. Sphere expansion, cylinder
intersection and box/sphere intersection retain their real source differences.
`ClipPlane` remains 99.9%: the final conditional branch selects a different,
identical `return 1` block. Its branch displacement is not masked or credited as
byte-exact.

The call graph provides further independent identity evidence: ray/sphere
intersection constructs the quadratic coefficients from direction length,
twice direction/origin dot, and origin distance minus radius squared. Its
callee at `0x144560` (217 bytes) handles degenerate coefficients, discriminant
cases, direct roots `-b/(2a) +/- sqrt(discriminant)/(2a)`, and root ordering by
the sign of `a`. This callee is registered to
validate its actual call relocation, but does not receive source-match credit
in this batch. Box/ray intersection calls the six-plane clipping helper, whose
six calls resolve to the independently reviewed sign-sensitive interval update
routine. Box/sphere intersection calls the reviewed point-in-box and point-box
distance routines. The latter's face/edge/corner cases account for its full
1313-byte body.

Only actual PE HIGHLOW operands and MAP-resolved direct calls are normalized.
All source comparison bodies reconstruct their linked PE bytes, and the seven
exact functions also reconstruct both authenticated originals. The diagnostic
entry invokes the full public geometry API; it supplies no fake game function,
original-address import or runtime implementation. Host and runtime bytes do
not enter source progress.

Reproduce using the existing original verifier and normal Xbox report command:

```sh
python tools/platforms/verify_xbox_reviewed.py --orig-dir /private/orig
python tools/platform_progress.py report --version XBOX-US \
  --orig-dir /private/orig --build-dir build/platforms \
  --xbox-compilers /xbox-compilers --objdiff /tools/objdiff-cli --wine /usr/bin/wine
```

Repeat for `XBOX-EU`. Local proof and production artifacts are under
`build/xbox180/final` and `build/xbox180/validation.json`; no binaries are committed. This remains partial function
comparison, without a whole-TU completion or original-executable relink claim.


## Sphere and cylinder helper boundaries

Both originals inline the sphere/point distance work inside `iSphereBoundVec`
(271 bytes at `0x16b500`) and the two-dimensional distance work inside
`iCylinderIsectVec` (84 bytes at `0x16b610`): both complete original functions
have no calls. The previous complete source TU instead called `iSphereIsectVec`
and `xVec2Dist`, respectively.

The existing definitions in the actual `iMath3.cpp` are now ordinary inline
functions for Xbox only. Their bodies, declarations, arithmetic and source
ownership remain unchanged. No new helper, forced emission, compiler option,
SDK layout or original identity was introduced. Only the obsolete source call
expectation for the no-longer-emitted `xVec2Dist` was removed from the profile.
The actual existing 73-byte `iSphereIsectVec` remains emitted by `iMath3.obj` and
is byte-identical to both originals; no old function or credit is discarded.

| Function | Original bytes | Before | After |
| --- | ---: | ---: | ---: |
| iSphereBoundVec | 271 | 61.84694% | 90.591835% |
| iCylinderIsectVec | 84 | 70.5625% | 99.9375% |

The sphere source remains 269 bytes and partial. The cylinder source is 84
bytes, with only the two real operand bytes at +18/+21 differing: the FLD/FADD
operands for center.y and height exchange places. Those differences remain in
the normal comparison; no operand permutation was attempted to force a match.

Both complete production reports pass. Full code-weighted fuzzy progress rises
from 1.1352485942393649% to 1.1409510595738175%; exact totals remain
13,501 bytes / 70 functions. Every other function record, denominator and
completion field is unchanged except generated COFF offsets. Seven actual
consumer builds have no other score changes, and all four ordered GameCube
iMath3 allocated sections remain byte-identical. No full executable link or
original TU-completion claim is added.

Ignored evidence: `build/xbox254/{compile.py,compare.py,sphere-deltas.json,
compile-pair.py,compare-pair.py,pair-deltas.json,consumers,pair-consumers,
gc/proof.json,production,verify_final.py,final-verification.json}`.


## Quick-cull and matrix composition boundaries

The original `xQuickCullForOBB` and the box branch of `xQuickCullForBound`
contain the two cell calculations and merge directly. The prior source called
`xQuickCullForBox` instead. Likewise, original `xMat4x3Rot` calls RotC, Toworld
and Mat3Mul, while the prior source called RotC and Mat4x3Mul.

The existing definitions of `xQuickCullForBox` and `xMat4x3Mul` are now ordinary
inline functions for Xbox only, within their existing complete source TUs.
Their bodies and all non-Xbox code are unchanged. The source profile records
the actual newly exposed calls to existing reviewed helpers. Original symbols,
bounds, target relocations, compiler flags and backend rules are unchanged.
Standalone source definitions retain their real owners and prior scores; no
artificial caller or forced emission was introduced.

| Function | Original bytes | Before | After |
| --- | ---: | ---: | ---: |
| xQuickCullForBound | 125 | 45.792454% | 100% |
| xQuickCullForOBB | 68 | 52.260868% | 100% |
| xMat4x3Rot | 146 | 72.48077% | 79.15385% |

All 193 newly matched bytes independently reconstruct both authenticated
originals after only the nine actual source E8 operands are assigned their
already reviewed original callee addresses. Source CFG extents are decoded
independently and both functions are owned by actual `xQuickCull.obj`. No
constant or instruction bytes are excluded. Matrix rotation remains partial.

Both full reports contain 13,694 exact bytes / 72 exact functions. All eight
reviewed QuickCull functions now match, totaling 1,167 bytes; original TU
coverage and full executable relinking remain unproven, so completion stays
false. Every prior function record, denominator and completion field is
unchanged except the three listed scores and generated COFF offsets. Full
code-weighted fuzzy progress increases from 1.1409510595738175% to
1.147064423172074%. Seven actual consumer builds and 33 ordered GameCube
allocated sections pass without regressions.

Ignored evidence: `build/xbox256/{compile.py,compare.py,matrix-deltas.json,
compile-pair.py,compare-pair.py,pair-deltas.json,gc/proof.json,production,
verify_final.py,final-verification.json}`. The final verifier independently
replays the actual named call destinations and all complete report invariants.
