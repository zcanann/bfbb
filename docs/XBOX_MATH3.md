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


## World-transform call boundary

Three already reviewed original functions call the same 98-byte leaf at
0x19700: BoxInitBoundOBB at 0x1452ed, Mat4x3Rot at 0x145e02 and Mat4x3Mul at 0x145e52.
The prior source expanded xMat4x3Toworld inline. Xbox now declares the existing
API in its header and supplies its unchanged body as an ordinary definition in
xMath3.cpp. All other platforms retain the former inline body. No noinline,
calling-convention override, compiler flag or artificial caller was added.

Independent original-only review proves the helper identity and boundaries.
Both authenticated originals contain exactly one copy of the complete 98 bytes;
the existing anonymous registry had already bounded this same 38-instruction leaf.
EAX points to the matrix and ECX to the input vector; the output pointer is loaded
from entry ESP+4 into EDX. All input-vector and basis products are read before
the first output store. The rotated x/y/z results are then stored, followed by
position addition and x/y/z stores. The output-x float store/reload and actual
x87 operation order are preserved; expression descriptions are provenance, not
a claim of rounding equivalence. The same input/output vector may alias without
clobbering unread components. Matrix/output alias effects are not rewritten.

The original helper returns at+97, with preceding RET/two INT3 and fourteen INT3
following. Its three semantic callers, full dataflow, unique payload and prior
anonymous proof establish identity independently of compiled source. The original
definition/header TU remains unknown. xMath3.cpp is the reconstructed reporting
group because the actual emitted body belongs to xMath3.obj.

Original call restoration includes all six real E8 operands across those three
callers: the three new helper edges plus their already reviewed RotC/Mat3Mul
callees. The existing verifier requires complete call lists and inverse equality;
no unknown target or partial-list exemption is added. The helper takes over its
same anonymous 98-byte extent without adding bytes/functions to the denominator.

The compiled helper is 94 bytes: its complete arithmetic and read/write sequence
is byte-identical apart from the original four-byte MOV EDX,[ESP+4] that the
source ABI does not need. That real mismatch is retained in ordinary objdiff;
no bytes are removed from comparison and the helper is not reported exact.
Rot also still calls the real Mat4x3Mul where the original expanded composition,
so that caller remains partial.

Both final production reports measure the same gains:

| Function | Original bytes | Before | After |
| --- | ---: | ---: | ---: |
| xBoxInitBoundOBB | 263 | 41.287357% | 82.63219% |
| xMat4x3Rot | 146 | 70.26923% | 72.48077% |
| xMat4x3Mul | 63 | 0% | 72.545456% |
| xQuickCullForOBB | 68 | 43.695652% | 52.260868% |
| xMat4x3Toworld | 98 | target-only anonymous | 97.36842% |

The full code-weighted fuzzy measure rises from 1.1121375440798102% to
1.1265316152705196%. The exact totals remain 13,501 bytes / 70 functions.
Known coverage remains 2,556 functions / 635,415 bytes, with 115 reviewed and
2,441 anonymous functions. Every other previous function record is unchanged
apart from regenerated COFF offsets. No original address or coverage denominator
changes. Seven actual source-consumer builds had no prior function regressions. Thirty-three
ordered GameCube allocated sections across those consumers remain byte-identical.
All prior exact matches remain; no TU or executable relink claim is added.

Ignored evidence: build/xbox242/{original-helper-proof.json,proposed-helper.json,
original-complete-calls.json,helper-abi-difference.json,consumer-deltas.json,
gc/proof.json,production,final-verification.json}. The normal reviewed-original
verifier and full report commands reproduce the production checks.


## Left-multiply helper visibility

The complete original `xMat3x3Tolocal` at `0x145ce0` is a closed 191-byte
function with no calls in either authenticated Xbox executable. It includes
all vector-left-multiply arithmetic between the squared basis lengths and the
three divisions. The prior complete source build instead emitted a 142-byte
caller with an out-of-line `xMat3x3LMulVec` call at +106, spilling all three
lengths before that call.

For Xbox, the existing unchanged helper body is now an ordinary inline header
definition. The non-Xbox definition remains unchanged. This is a source boundary
reconstruction supported by the original caller; it does not establish the
original definition/header TU. No calling convention, compiler flags, forced
emission, target metadata or backend rules change. The source profile removes
only the two obsolete direct-call expectations for the helper, which no longer
appears in those actual linked source call paths.

Both complete production reports retain the real partial differences:

| Function | Original bytes | Before | After |
| --- | ---: | ---: | ---: |
| xMat3x3Tolocal | 191 | 60.075% | 94.775% |
| xParCmdRandomVelocityPar_Update | 335 | 34.189472% | 61.210526% |

The new actual Tolocal body is 191 bytes with no calls; it is not byte-exact.
The full code-weighted fuzzy measure rises from 1.1265316152705196% to
1.1352485942393649%. All 13,501 exact code bytes / 70 exact functions remain.
Every other function record, all denominator fields and all completion fields
are unchanged, apart from generated COFF offsets. Seven actual consumer builds
and 33 ordered GameCube allocated sections also pass without regressions.

Ignored reproducible evidence: `build/xbox250/{compile.py,compare.py,
prove_boundary.py,boundary-proof.json,consumer-deltas.json,gc/proof.json,
production,verify_final.py,final-verification.json}`. The collision dependency
investigation in the same directory remains negative: the available complete
RenderWare headers select GameCube big-endian definitions and do not establish
Xbox world/geometry layouts. No substitute SDK structures or link stubs were
introduced to bypass that limitation.


## Matrix product component lifetimes

Original Xbox `xMat3x3Mul` at `0x145bc0` computes and stores each result component
before starting the next, then clears matrix flags. Its alias checks select a
48-byte temporary when output equals either input. The shared source's nine
phased scalar temporaries delayed all component stores and produced a 333-byte
function with a 72-byte frame, versus the original 283 bytes / 48-byte frame.

The Xbox branch now writes the nine components directly. Each expression keeps
the existing tree `third + (first + middle)` from the phased temporaries; this
was one lifetime correction, not an arithmetic reassociation search. The same
alias temporary/pointer choice and final matrix copy remain. Flags are cleared
after the nine assignments in source. All non-Xbox source remains unchanged.

Both complete reports improve this one function from 50.271843% to 76.05825%.
Actual source size/frame now equal the original. The first 27 bytes of alias
checks and the copy/alternate-temporary tail at +254..+283 are byte-identical.
Both paths write component offsets 0,4,8,16,20,24,32,36,40 in that order. The
compiler still schedules the flags store before the last component store,
whereas original flags follow it; operand/arithmetic scheduling also differs.
Those differences remain compared, with no claim of exact reconstruction.

All prior 13,694 exact bytes / 72 exact functions, every other function record,
all denominator/completion fields and all 33 ordered GameCube consumer sections
are unchanged (apart from generated COFF offsets). Full code-weighted fuzzy
progress increases from 1.147064423172074% to 1.151121414199782% in both regions.
No profile, original metadata, backend or compiler flag changes were needed.

Ignored evidence: `build/xbox258/{matrix-disasm.txt,compile.py,compare.py,
deltas.json,gc/proof.json,production,prove_alias.py,alias-proof.json,
verify_final.py,final-verification.json}`. The alias proof reads both final
production PEs and both authenticated originals.
