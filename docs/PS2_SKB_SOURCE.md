# PS2 SKB source recovery

The PS2-only `iAnimSKB.cpp` source unit improves from 43.771347% to 64.62998%
in all three debug versions. `_iAnimSKBAdjustTranslate` (1,240 original bytes)
improves from 28.290323% to 99.17742%, and `_iAnimSKBExtractTranslate` (1,464
bytes) improves from 99.90437% to 99.931694%. The existing 32-byte duration
function remains exact. No new exact-function claim is made. This first change
leaves the missing 1,480-byte evaluator in the report denominator.

The original adjustment routine copies its position into a temporary register
and performs separate `abs.s` instructions for each maximum comparison and
update. A local inline helper using the actual volatile assembly operation
restores these operations and the original index-based loop body. The intrinsic
`FABS` form allowed commoning and pointer induction that changed the body and
expanded the saved-register frame. An MW assembly-block spelling also retained
those optimizer differences; the GNU-style volatile operand form reproduces the
retail operations. This is a source correction confined to the existing PS2
translation unit, with no global math-header or compiler change.

Load the key/bone/time counts in their original order before initializing the
two maxima arrays. The original explicitly clears those arrays with byte loops;
the empty aggregate initializers retain that behavior. Restore the original
integer multiplication operand order in the two key-endpoint expressions.
All arithmetic, interpolation bounds, scale updates, and retail extraction
behavior remain intact, including the existing unused `maxTran` parameter.

The adjustment's compiled body is still eight bytes larger: each zero-array
loop has one additional NOP. Nine instructions retain floating-register
assignment/lifetime differences. Extraction retains four register differences
in its initial header loads and time-count calculations. These residuals remain
fully counted; no compiler-version explanation or comparison relaxation is
asserted.

Private evidence in `C:/Projects/bfbb-agent-ps2-oct08/build`:

- `skb-originals.py` / `.txt`: all three originals' complete instruction words
  and DWARF locals for all four functions.
- `skb-adjust-final-comparison.py` / `.json` and
  `skb-adjust-final-changes.json`: complete before/after reports against
  `8988fbeb0`, with no function or data regression.
- Retained pilot objects document the intrinsic, assembly-block, volatile
  helper, and initializer-lifetime steps separately. A diagnostic `-O2` build
  regressed the routine and duration function and was discarded; no flag change
  is part of the result.

GameCube selects `SB/Core/gc/iAnimSKB.cpp`; Xbox does not select this PS2 source.
Their source and headers are unchanged. The first two source commits select
only the three debug versions; the separate French extension below adds the
already recovered French sequence to source comparison.

## VU evaluator restoration

The follow-up restores `iAnimEvalSKB`, improving the complete 4,216-byte unit
from 64.62998% to 92.31973% in each debug version. The evaluator itself improves
from missing to 78.87838%. All three previously compiled functions, their exact
status, and data measures remain unchanged. The evaluator compiles to 1,512
bytes against 1,480 original bytes, so it makes no new exact-function claim.

The scalar C++ code restores the original time clamp, four-step interval
search, root-bone flags, and single-key decoding. In the single-key path the
three translation scales are loaded at their original uses. The multi-key path
unpacks signed 16-bit quaternion and translation components in VU0, scales them,
forms the quaternion dot product, flips the second quaternion for the shorter
arc, and interpolates translation. The near-parallel threshold is the original
`0x3f7fff58` float bit pattern. The other path evaluates the original piecewise
acos and vector sine polynomials before combining the quaternion endpoints.

Only the packed-key and VU math kernels use inline assembly; time selection,
loops, and all approximation interval branches remain ordinary C++. The scale
packing kernel retains the original a0-a2 scratch convention. The original VU
operations use partial destination masks and broadcasts that ordinary scalar
math would lose, so these are explicit in the source. An independent raw-word
audit verifies all 122 VU arithmetic instructions, including every lane mask,
broadcast, VU register operand, and Q operation, in each compiled body against
its original. No instruction mask or target metadata is changed for comparison.

The evaluator's only external reference is `slerpPolynomial`. Original DWARF
proves a 24-element float array; the original load pair targets that declaration,
and all 96 table bytes agree across the three debug originals. The existing
[French SKB proof](PS2_FRANCE_SKB.md) independently proves the same table and
the complete evaluator body in France. This source declares the original table
without copying it into this translation unit or inventing an SDK alias.

The residuals include scalar register allocation, branch-block placement, and
scheduling around assembly boundaries. These remain fully counted. No compiler
patch, altered function bounds, new relocation identity, or profile change is
part of the restoration. GameCube's separate source is unchanged.

Additional private evidence:

- `skb-vu-decode.py`, `.json`, and `.txt`: original evaluator decoding; VU masks
  are independently taken from the actual original instruction words.
- `skb-eval-final-comparison.py`, `.json`, and `skb-eval-final-changes.json`:
  all three complete before/after unit reports against `9606eef07`.
- `skb-eval-raw-proof.py` / `.json`: full-word VU arithmetic checks, original
  typed polynomial identity/content hashes, and the compiled object's sole
  HI/LO relocation pair in each version.

## French source profile

A separate profile entry selects only the authenticated French executable and
the four existing SKB functions. Every address-qualified target name maps to the
same canonical linker spelling in all three original DWARF records. The profile
records those source addresses, executable identities, and body hashes. It adds
no functions, boundaries, data identities, relocation rules, or calls.

Compiling all four functions with `VERSION_SLES_53623=1` yields 92.31973% for
the unchanged 4,216-byte unit. Duration adds 32 raw-exact bytes; Eval is
78.87838%, Adjust is 99.17742%, and Extract is 99.931694%. The emitted bytes of
each of the four functions agree with each of the three debug-version builds.
The 122 full VU words also agree directly with the French original evaluator.

The private target rebuild preserves each of the previous four `.text` sections
byte for byte, changing only their authenticated symbol spellings. Its function
count and all 4,216 original code bytes remain unchanged. The cached report's
coverage document is preserved, as are every unrelated profile and target unit.
Root's combined regeneration gate remains responsible for the final full report.

Private reproduction and evidence: `build/skb-france-profile.py`,
`build/skb-france-source-proof.py`, and
`build/skb-france-diagnostic/{profile-proof,source-proof}.json`. The compiled
French object and report are in `build/ps2solo-tvrkkkoi`.
