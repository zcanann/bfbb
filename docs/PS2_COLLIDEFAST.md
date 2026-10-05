# PS2 fast collision and vector inlines

The complete shared `xCollideFast.cpp` builds with a minimal PS2
`iCollideFastInit(xScene*)` declaration, a pointer-only scene dependency, and an
explicit platform math include. The original canonical linkage is
`iCollideFastInit__FP6xScene`; the older source dump omits its unused parameter.
The original `xCollideFastInit` is an eight-byte tail `J` plus delay slot to that
function, not an empty stub. No platform implementation is invented.

The PS2 headers restore ordinary scalar `xVec3Sub` and `xVec3Dot` inlines.
The original DWARF explicitly attributes the standalone 52-byte `xVec3Sub`
function to `SB/Core/x/xVec3Inlines.h`. The original fast sphere consumer has
scalar subtraction and dot arithmetic in its own body, while the previous
header declarations caused external calls and a stack frame. The natural dot
expression and component-wise subtraction recover that exact arithmetic tree.
The original spelling/location of a separate Dot definition is not proven;
this header definition is the source reconstruction supported by the consumer.
GameCube keeps its existing platform declarations and implementation.

The sphere function's final ternary is expressed as an ordinary early return.
This preserves the original guarded square calculation and branch boundary;
with the ternary, the PS2 compiler moved the square into a branch delay slot.
The explicit early return is also instruction-identical on GameCube.

All three original functions / 348 bytes are code-exact in SLUS-20680,
SLES-51968, and SLES-51970. Independently applying the actual object relocations
for the `iBoxIsectRay` call and `iCollideFastInit` tail jump to the original named
addresses reproduces all 348 bytes in each version. This is full function-code
proof, not a source-linked executable or data-layout completion claim.

All existing compiled PS2 profile consumers were rebuilt. Relative to staging
47ed9725e, each debug version gains the 348 bytes and has no function-score loss.
The restored vector inlines also improve `xQuatSlerp` from 53.409523 to 69.98095
and `xLine3VecDist2` from 7.6842103 to 8.460526. Existing France profiles are
unchanged; this candidate does not extend France ownership. All 224 GameCube
game/engine objects retain identical allocated sections.

Private reproducible evidence is in `build/ps2fast158`: actual compiler command,
whole source object, candidate `profile.json`, `raw-proof.json`, initial and
inline diffs, `whole/verify` outputs through `verify_whole.py` and
`whole/comparison.json`, and `gc/comparison.json`. The whole-profile comparison
uses the normal objdiff policy and the authenticated original registries.
