# PS2 SKB translation source recovery

The PS2-only `iAnimSKB.cpp` source unit improves from 43.771347% to 64.62998%
in all three debug versions. `_iAnimSKBAdjustTranslate` (1,240 original bytes)
improves from 28.290323% to 99.17742%, and `_iAnimSKBExtractTranslate` (1,464
bytes) improves from 99.90437% to 99.931694%. The existing 32-byte duration
function remains exact. No new exact-function claim is made; the currently
missing 1,480-byte evaluator remains in the report denominator.

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
Their source and headers are unchanged. Although the French original sequence
has been independently recovered, the existing source profile currently selects
only the three debug versions. This change does not alter that profile.
