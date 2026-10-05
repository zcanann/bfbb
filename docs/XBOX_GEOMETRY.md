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
