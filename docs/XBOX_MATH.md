# Xbox shared math comparison

The complete existing `src/SB/Core/x/xMath.cpp` now compiles and links with the
pinned Microsoft compiler and genuine private CRT. The diagnostic entry retains
all eighteen public APIs. No game source, headers or compiler flags change.

Original instruction fingerprints locate the math cluster, followed by checks
of actual direct callers, closed control flow, return boundaries and algorithm
identity. Eleven independently reviewed original functions, totaling 2511 bytes,
enter the normal comparison. Seven other public APIs without reviewed original
locations remain unassigned. The quadratic solver was already reviewed as a
geometry callee; its existing extent is reused, not counted twice.

Five functions reproduce **702 original bytes** after verified relocations:

| Function | Original entry | Bytes |
| --- | --- | ---: |
| xMathSolveQuadratic | 0x144560 | 217 |
| xAngleClamp | 0x1448d0 | 35 |
| xDangleClamp | 0x144900 | 55 |
| xFuncPiece_Eval | 0x144dc0 | 154 |
| xFuncPiece_ShiftPiece | 0x144e60 | 241 |

Both Xbox reports increase **8927 -> 9629 matched bytes**. The new unit is
**88.32875% fuzzy**, with **5/11 exact functions**. Random generation, the cubic
solver and four acceleration routines retain their real partial scores. The
cubic solver's unresolved original call destinations remain unnormalized.

All seven prior compared unit records and their normalized source bytes and
relocations are identical. All eleven new source comparisons reconstruct their
actual linked PE; the five exact functions additionally reconstruct both
original versions. Their normalization covers seventeen actual PE HIGHLOW
fields and two independently named direct calls. Full code and data denominators
remain unchanged, including the 1798760-byte original code denominator. The
recovered inventory adds only the newly verified ten-byte runtime entry, reaching
2551 functions / 633601 known bytes; the math bodies replace existing anonymous
extents or the previously named quadratic solver.

## Independent remainder-runtime identity

The original angle routines call a ten-byte entry at `0x1c708a`: it loads the
descriptor at `0x287108`, then jumps to the dispatch context at `0x1c4644`.
The authenticated descriptor literally names `fmod` and points to the fourteen-byte
implementation at `0x1c7094`. That implementation exchanges the x87 inputs,
repeats FPREM while the C2 condition is set, removes the second stack operand,
and returns. Both angle call sites and these original bytes are checked.

`tools/platforms/xbox_runtime.py` also verifies the pinned private `libcmt.lib`
archive and its `..\build\intel\mt_obj\87fmod.obj` member. The actual vendor
COFF declares `__CIfmod` with a ten-byte auxiliary function extent and two named
relocations: DIR32 to `__OP_FMODjmptab` and REL32 to `__cintrindisp2`. Its dispatch
instructions corroborate the original entry's identity and boundary. The vendor
implementation includes additional handling and is not claimed to equal the
original implementation.

This restricted verification accepts only this reviewed runtime-dispatch record;
all other original functions retain the existing return/closed-CFG rules. The
runtime entry receives no source-match credit. No original-address import,
runtime stub, arbitrary operand masking or proprietary binary is committed.
Production source compilation rechecks the actual pinned vendor archive/member.

## Reproduction

```sh
python tools/platforms/verify_xbox_reviewed.py --orig-dir /private/orig
python tools/platform_progress.py report --version XBOX-US \
  --orig-dir /private/orig --build-dir build/platforms \
  --xbox-compilers /xbox-compilers --objdiff /tools/objdiff-cli --wine /usr/bin/wine
```

Repeat for `XBOX-EU`. Local production artifacts and source/original inverse
checks are in `build/xbox184/final` and `build/xbox184/validation.json`.
This is partial function comparison, with no complete-TU or retail-relink claim.


## Acceleration timing lifetime

The original 83-byte `xAccelMoveTime` starts by halving the requested distance,
then computes reciprocal acceleration and acceleration time/distance. Keeping
that half-distance live lets the generated code reuse dead argument slots and
avoid a local stack frame. The former source calculated acceleration time
first, producing 96 bytes and an 8-byte frame.

For Xbox only, the existing `dx *= 0.5f` statement now precedes the existing
`atime = maxv / time` calculation. Every arithmetic tree, branch predicate,
parameter and return expression is unchanged. This was one control grounded in
the original first operation, with no additional locals, compiler flags or
permutation search. Non-Xbox source order remains unchanged.

Both complete reports improve the function from 36.076923% to 100%. Its actual
83-byte source extent is independently decoded and owned by `xMath.obj`.
All original bytes reconstruct in both authenticated executables after only
three actual PE HIGHLOW fields at +6, +12 and +40 are assigned the reviewed
constant addresses. Actual source/original 0.5 and 1.0 payloads are identical;
there are no calls or other ignored fields.

The totals become 13,935 exact bytes / 76 functions. Every other function record,
all denominator/completion fields and all 33 ordered GameCube consumer sections
remain unchanged apart from generated COFF offsets. Full code-weighted fuzzy
progress rises from 1.1555546631813025% to 1.1585042592313592%. No profile,
original metadata, source ownership or backend changes were needed.

Ignored reproducible evidence: `build/xbox262/{residual-disasm.txt,compile.py,
compare.py,deltas.json,gc/proof.json,production,verify_final.py,
final-verification.json}`. Full executable relinking remains pending.
