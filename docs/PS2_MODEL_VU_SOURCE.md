# PS2 model VU source restoration

Restore the missing `iModelAnimMatrices` and `iModelCull` bodies in the PS2-only
model source. Across SLUS-20680, SLES-51968, and SLES-51970, the complete
30-function, 9,920-byte unit improves from 70.40685% to 78.12218%. Every other
function and data measure remains unchanged, including the existing 3,196
exact bytes / 16 functions. These two restorations make no new exact claim:

| Function | Original and compiled size | Before | After |
| --- | ---: | ---: | ---: |
| `iModelAnimMatrices` | 540 bytes | missing | 93.62963% |
| `iModelCull` | 288 bytes | missing | 90.19444% |

Animation matrices use the original 32-entry stack of 64-byte `RwMatrixTag`
objects, independently typed by all three original DWARF records. This differs
from the 33-entry GameCube implementation. The current parent matrix remains
in VU registers; node flag 2 pushes it, each quaternion and translation forms a
local matrix and composes it with the parent, and node flag 1 pops the parent.
Other nodes advance the parent to their newly written matrix. The MMI quaternion
shuffle preserves the original a4-a6 scratch registers and shift-amount setup.
The stack push and increment are separate operations, matching their actual
original lifetime instead of passing a post-incremented pointer to the helper.

Culling transforms the local sphere and scales its radius by the square root of
the largest matrix-axis squared length. It writes the resulting world sphere
before either rejection, tests four packed side planes, then two depth planes,
and joins both rejection paths at the same return. A Boolean-expression return
introduced a byte-normalized result and changed the control flow; the explicit
original return paths retain the integer result and reach 90.19444%.

All three original layouts prove `RpAtomic` size 112, local/world sphere offsets
28/44, `RwMatrixTag` size 64, `xCamera` size 816 with frustum planes at 624, and
`xGlobals::camera` at zero. The culling routine's sole original LUI/ADDIU address
agrees with its original named `globals` declaration plus `0x270` in every region.
No new data or runtime alias is introduced.

The scalar hierarchy traversal and branches remain C++. VU math and packed
shuffles use explicit inline assembly because lane masks, broadcasts, accumulator
ordering, and packed comparison semantics are part of the original implementation.
The emitted ordered sequences of 39 and 26 VU arithmetic words agree completely
with each original, including masks and operands. Animation's MMI operation
words and their order also agree exactly. CPU register allocation, scheduling,
and unresolved SDK call identities remain ordinary report residuals. The two
hierarchy SDK calls use existing declarations; this change supplies no new
identity evidence for their stripped runtime destinations.

GameCube and Xbox select separate model sources. No shared header, compiler
flag, target profile, function boundary, relocation rule, or registry changes.
French matching is not claimed: this worktree's existing French target cache
does not contain the model unit. The nearby missing light-kit body remains
unimplemented because its final SDK call's name has not been independently
established.

Private evidence in `C:/Projects/bfbb-agent-ps2-oct08/build`:

- `model-originals.py` / `.txt`: full original instructions and locals in all
  three debug regions.
- `model-layout-proof.py` / `.json`: exact array and structure descriptor checks.
- `model-vu-decode.py` / `.txt` and `model-vu-iModel*.json`: decoded kernels,
  with masks independently recovered from original words.
- `model-vu-final-comparison.py`, `model-vu-final-after.py`,
  `model-vu-final-comparison.json`, and `model-vu-final-changes.json`: complete
  before/after source reports against `39d4ec169`, with no regressions.
- `model-vu-raw-proof.py` / `.json`: ordered raw VU/MMI checks, original typed
  frustum address proof, and retained compiled-object/relocation inventories.
