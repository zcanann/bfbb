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

## Stopping velocity capture

The original 227-byte `xAccelStop` reads the referenced velocity directly for
its initial epsilon and sign tests. It captures the old velocity only after
choosing acceleration's sign, immediately before writing the new velocity.
The former source captured it at entry, introducing a four-byte stack slot
and repeated reloads in its 234-byte output.

For Xbox only, the initial tests now read `v` and the existing `oldv = v`
assignment moves immediately before the velocity update. There are no writes
to either referenced argument between these reads, including when `x` aliases
`v`. Predicates, arithmetic trees, subsequent store order and non-Xbox code
remain unchanged. This single lifetime correction produces 225 bytes owned
by the actual complete `xMath.obj`; its initial code through the new-velocity
store has the original instruction structure. Post-update sign-test order and
division association still differ and remain visible in the normal comparison.

Both complete reports improve only this function, from 75.52564% to 92.69231%.
Exact totals remain 13,935 bytes / 76 functions; full code-weighted fuzzy
progress rises from 1.1585042592313592% to 1.160670659468189%. Every other
function record and every denominator/completion field is unchanged apart
from generated COFF offsets. All 33 ordered GameCube consumer sections are
byte-identical. The profile updates only the actual source literal-operand
expectations: each field is a real PE HIGHLOW relocation to the same verified
constant payload. Original metadata, ownership and report rules are unchanged.

Ignored reproducible evidence: `build/xbox264/{accel-disasm.txt,compile.py,
compare.py,consumers,production,verify_final.py,final-verification.json,
gc/proof.json}`. This is a partial match, not a byte-exact or retail-link claim.

## Speed-limited acceleration expressions

The original speed-limited `xAccelMove` uses average velocity for its unclamped
step: `(0.5f * dv + v) * dt`, reusing the velocity change already computed for
the limit test. The prior shared source separately formed acceleration distance
and `v * dt`. The original clamped step computes `diff / a`, multiplies that by
`diff`, then multiplies by 0.5; the prior shared expression multiplied first
and divided last. These are actual decoded floating-point operation sequences,
not a search through algebraic variants.

Two Xbox-only expressions now follow those original sequences. Branch tests,
velocity/position store order and non-Xbox expressions are unchanged. The
average-velocity correction alone improves 92.23529% to 95.05882%; restoring
the independently visible division order raises the result to 98.117645%.
The actual complete source emits 223 bytes versus the original 225. It
coalesces a duplicate velocity-change spill present in the original; that
remaining difference is retained rather than adding an artificial temporary.

Both complete reports retain 13,935 exact bytes / 76 functions. All other
function records, denominator/completion fields and 33 ordered GameCube
consumer sections are unchanged apart from generated COFF offsets. No
profile, original metadata, source ownership or backend change is required.
Actual source constant fields remain genuine PE HIGHLOW relocations to the
same authenticated original zero/half payloads. This remains a partial match.

Ignored evidence: `build/xbox266/{compile.py,compare.py,average-only-consumers,
average-only-production,consumers,production,verify_final.py,
final-verification.json,gc/proof.json}`. No complete-TU or retail-relink claim.

## End-position acceleration entry condition

The original end-position `xAccelMove` branches directly on the initial
small-velocity/sign-disagreement condition. The shared reconstruction first
materialized that boolean into an integer and then tested its low eight bits.
The condition is already exactly zero or one, so the mask adds no semantics.
Xbox now uses the unchanged short-circuit condition directly in the `if`;
non-Xbox source, arithmetic expressions and all downstream stores are unchanged.

This single source control improves 83.72396% to 89.083336%, removing the
unnecessary boolean-materialization block. The actual complete `xMath.obj`
body decreases from 581 to 568 bytes versus the original 574. Remaining
arithmetic, temporary and sign-test-order differences stay visible. No
expression or register permutation trial was performed.

Both regional production reports preserve every other function record and
all exact counts, denominator and completion fields apart from generated
COFF offsets. The actual source literal fields remain the same genuine
HIGHLOW references with matching original payloads; no profile, metadata or
backend changes are required. All 33 ordered GameCube consumer sections are
identical. This is a partial comparison, with no exact-function or relink claim.

Ignored evidence: `build/xbox274/{compile.py,compare.py,consumers,production,
verify_final.py,final-verification.json,gc/proof.json}`; the original/source
entry comparison is also preserved in `build/xbox268/inventory-disasm.txt`.

## End-position old-velocity acquisition

A separate original instruction boundary loads the old velocity before
computing and spilling `dv = a * dt`. The prior source did those two assignments
in reverse order, forcing a spill/reload before forming the new velocity.
For Xbox only, the existing `oldv = v` now precedes `dv = a * dt`. There are no
intervening calls or writes to the referenced velocity, and all expressions,
branch predicates, output stores and non-Xbox ordering remain unchanged.

This single acquisition correction improves the endpoint routine from
89.083336% to 91.265625%; actual source size falls from 568 to 565 bytes versus
the original 574. It does not resolve the remaining arithmetic and temporary
choices, which stay visible in the normal comparison. Every other function
record, exact count and denominator/completion field is unchanged apart from
generated COFF offsets. Both regional production reports pass, and all 33
ordered GameCube consumer sections remain byte-identical. Existing genuine
literal relocation expectations remain valid without a profile change.

Ignored evidence: `build/xbox276/{compile.py,compare.py,consumers,production,
verify_final.py,final-verification.json,gc/proof.json}`. This remains a partial
source comparison with no original TU completion or retail-link claim.
