# Xbox inline angle helper boundary

The Xbox source now exposes the existing `xAngleClampFast` body to the existing
inline `xatan2` definition. The arithmetic and ordinary `/O2 /Ob1 /GL` + `/LTCG`
recipe are unchanged. GameCube and PS2 retain their previous declarations and
out-of-line bodies.

The original cubic solver calls a 50-byte helper at `0x12710`: two float stack
arguments feed FPATAN, followed by the zero/2pi range adjustment. The original
Euler extractor instead contains four inline FPATAN/adjustment paths. Making
`xatan2` globally out of line reproduced the helper but regressed Euler; that
candidate was rejected. Exposing both ordinary inline bodies lets the compiler
retain the helper in Cubic and inline it in Euler, as the original does.

The actual whole-source builds improve `xMat3x3GetEuler` from 26.981482% to 100%
(338 bytes), and Cubic from 90.70051% to 93.5533%. All other 85 functions in the
seven affected source profiles keep their prior scores. The emitted helper also
matches all 50 bytes. The actual MSVC MAP marks this emitted body `f i`; the
parser now accepts that exact optional marker while retaining unique symbol,
object ownership, decoded CFG, and real relocation checks.

The helper's original definition/header TU is unknown. Its reconstructed
`xMath.cpp` reporting group follows the actual emitted `xMath.obj` body from the
complete shared source build. This is not a claim of recovered original TU
ownership or a complete executable relink.

Independent validation reproduces both authenticated originals byte for byte:
Euler uses 22 actual PE HIGHLOW constant fields and its sole real relative call;
the helper uses four actual HIGHLOW fields. Literal values remain +0, +/-pi/2,
and 2pi. The original Cubic call independently witnesses the helper identity;
Cubic's other runtime callees remain unnormalized, and its partial score is
retained honestly.

Euler's runtime destination `0x1c70a4` is independently identified by the pinned
MSVC7.1 `libcmt.lib` member `asin.obj`. The named `__CIasin` vendor container is
203 bytes and includes a separate `_asin` stack entry at +20. All 203 original
bytes agree outside exactly the 14 genuine COFF relocation fields, including
consistent repeated-symbol targets. Its embedded `asin` string ends at the
original .data raw boundary; the NUL is actual loader zero-fill. Only the closed,
RET-terminated 20-byte x87 wrapper enters target coverage. The remaining vendor
container and alternate entry are identity context, with zero CRT source credit.
No runtime implementation is synthesized.

The 50-byte helper and 20-byte runtime wrapper replace equal-size anonymous
extents. Original .text bytes, known-function totals, and initialized-data
coverage do not change. Existing exact-byte/named-relocation checks remain in
force. `tools/platforms/xbox_asin.py` rechecks the actual pinned vendor member and
original proof; `verify_xbox_reviewed.py` retains the ordinary closed-CFG check.

Both full production builds and strict reports pass: 11,936 matched bytes in
62 functions per release, up 388 bytes and two functions. All previous source
functions retain their scores apart from the two improvements above. Independent
GameCube compilation of actual xMath.cpp and iMath.cpp preserves every ordered
allocated section; the shared header changes are Xbox-only.

## Box containment return ownership

A follow-up original comparison found that `xPointInBox` returns integer 1 from
the successful innermost bound check and integer 0 from the fallback block.
The Xbox source now uses those ordinary early returns, preserving comparison
order and the previous GameCube/PS2 body. Merely removing the source's final
byte cast was insufficient: its early zero value remained live across FNSTSW
and required a saved register, unlike the original. No compiler flag changed.

The complete shared xMath3 build now reproduces all 84 original function bytes
literally, with no relocations. Both full production reports pass at 12,020
matched bytes / 63 functions. Every other report unit and function is unchanged
from the inline-helper result; code/data denominators and completion claims
are unchanged. The actual GameCube xMath3 object retains all five ordered
allocated sections byte for byte. Local evidence is
`build/xbox222/final-verification.json` and `build/xbox222/gc/proof.json`.

## Quaternion helper visibility

The original quaternion normalizer contains the complete length-squared and
scale operations. Slerp likewise contains its component scaling and addition
before calling Normalize. Xbox now sees the existing, unchanged `xQuatLength2`,
`xQuatSMul`, and `xQuatAdd` bodies as ordinary header inlines. Other platforms
retain the existing out-of-line definitions; no compiler flags changed.

The seven affected complete source builds have only two score changes:
Normalize improves from 55.20548% to 97.78082%, and Slerp from 47.26% to 96.64%.
Both full Xbox production reports pass with every other function and unit
unchanged. These are partial gains: exact code remains 12,020 bytes / 63
functions; the standard overall fuzzy score rises from 1.071856459597167% to
1.0847653571599323%. All integer measures and completion claims are unchanged.
Actual GameCube compilation preserves all five ordered xMath3 allocated
sections. Evidence is `build/xbox226/final-verification.json` and
`build/xbox226/gc/proof.json`; remaining operand differences are left unresolved.

## Vector normalization input ownership

The reviewed Xbox `xVec3Normalize` reloads each input field after the preceding
output store in its unit-length branch. The shared source already expresses
this behavior for PS2. Extending that branch to Xbox removes the unnecessary
captured y/z lifetimes, including a stack spill; the actual linked function
shrinks from 181 to the original 163 bytes. Arithmetic and branch comparisons
are unchanged, and the GameCube body is preserved.

A new profile compiles complete xVec3.cpp with the established real matrix
caller/dependency context and unchanged diagnostic matrix entry. Its only
reviewed original function is compared; NormalizeFast remains unassigned.
The six original constant operands refer to existing verified 0, 1 and epsilon
anchors. Real PE HIGHLOW records identify the corresponding source operands;
reapplying those original addresses reproduces all 163 original bytes in both
releases. No function boundary, original data value, denominator, compiler flag,
or backend rule changes.

Both full production reports pass at 12,183 exact bytes / 64 functions, up
163 bytes / one function. Every previous report unit is unchanged. Seven real
consumer builds retain all 88 prior function scores, and actual GameCube xVec3
compilation preserves all four ordered allocated sections. This is partial
source comparison, with no TU or executable relink claim. Evidence is
`build/xbox228/final-verification.json`, `original-literal-proof.json`, and
`gc/proof.json`.
