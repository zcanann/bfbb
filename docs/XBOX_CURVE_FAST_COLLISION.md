# Xbox curve and fast-collision source coverage

Two complete shared TUs now compile and link with the pinned normal Xbox
compiler settings and genuine existing source/runtime dependencies. The target
inventory contains three independently reviewed functions / 501 bytes. It
remains partial: no unlocated initializer identity or complete executable is
claimed, and every compared partial function stays visible.

| Function | Original entry | Extent | Actual source comparison |
| --- | --- | ---: | --- |
| `xRayHitsSphereFast` | `0x1295e0` | 189 | Exact after two literal relocations |
| `xRayHitsBoxFast` | `0x1296a0` | 71 | Exact after known call/literal relocations |
| `xCurveAssetEvaluate` | `0x129970` | 241 | Partial, 93.765434% |

## Source and dependencies

Fast collision needs only an opaque `xScene` pointer declaration and the
existing platform math API. Xbox guards remove unnecessary scene-layout
includes. The existing shared `gc/iCollideFast.cpp` implementation is compiled
as a dependency, together with complete iMath3/xMath/xMath3/xVec3/iMath sources.
No platform implementation or SDK layout is fabricated. The initializer's
original entry is unassigned, and this dependency earns no source credit.

The curve uses the existing genuine `__ftol2` compiler runtime. A single bounded
source trial reuses its existing `t` variable for the MIN and MAX operations
instead of introducing `curve_length`. This preserves the same comparisons,
including their unordered behavior, and recovers the original kept x87 value
and FCOM clamp sequence. Its normal score improves 90.37037% to 93.765434%
(source 233 to 229 bytes); the original remains 241 bytes. Original maximum-time
reload/store and earlier load placement remain unresolved. No additional
source-shape or flag sweep was attempted. GC and PS2 retain the previous branch.

## Independent original evidence

The sphere function's complete original arithmetic subtracts center from ray
origin, tests squared distance minus radius squared, applies the ray's 0x800
maximum-distance flag, rejects a nonnegative direction dot, and tests its
square. Box intersection calls the already reviewed `iBoxIsectRay`, then tests
its real xIsect penned/contained fields. Both extents close through decoded
branches/returns and trailing alignment.

The original caller block at `0x11e844` dispatches on bound type. Type one
branches to `0x11e98f`, expands a sphere radius and calls the sphere function at
`0x11e9b6` with ECX=ray and EDX=sphere. Type two branches to `0x11e92a`, builds
expanded box corners on the stack, and calls the box function at `0x11e985`
with EAX=ray and a stack box pointer. The complete manually decoded dispatch
block is hashed in both reviewed registries; individual E8 targets are checked.
This is original-only identity/entry evidence, not a compiler-fingerprint claim.

Curve identity follows clamp modes, fields at 4/8/12/16, oscillation parity,
adjacent sample interpolation, and two independently named `__ftol2` calls.
Original callers supply ESI=asset and stack time and use the x87 result as a
scale. Existing literal and callee anchors are reused; no new data identities
or relocation backend rules are introduced.

## Validation

Both authenticated original verifiers check 110 reviewed functions, 22,016
extent bytes and 139 direct-call witnesses. Actual whole-source PE output and
MAP owners provide source boundaries, literal HIGHLOW fields and named calls.
Source and target inverses are checked separately; the two exact comparisons
reconstruct all 260 original bytes. The curve remains a genuine partial.

Both normal production reports pass at 11,445 matched bytes / 59 functions,
up 260 bytes / two functions. All nine older compared-unit reports and normalized
source bodies/relocations are identical. The full code denominator remains
1,798,760 bytes. These new fast
functions were not previously in the conservative anonymous registry, so named
boundary coverage legitimately gains two functions / 260 bytes. Curve replaces
an already known anonymous range. No whole-section or runtime credit is added.

Private evidence and reproduction commands: `build/xbox190/`, including complete
compile/link logs, original/source CFGs, the sole clamp candidate/baseline
reports, actual caller disassembly, relocation proofs and final validation.
