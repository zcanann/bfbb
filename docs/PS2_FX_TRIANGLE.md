# PS2 triangle skinning source comparison

The 2264-byte `eval_tri` in `zFX.cpp` improves from 55.86926% to
99.22085% in the US, European, and German PS2 profiles. All other 37
profiled functions retain their scores. The complete GameCube report is
unchanged, including its separately emitted 532-byte skinning helper.

The original embeds `SkinXformVertAndNormal` in `eval_tri`; an ordinary
`inline` declaration restores that boundary. Its bone-cache loop masks
each packed index to eight bits before deriving the cache word and bit.
The vertex pass consumes a copy of the packed indices, while the normal
pass consumes the original local. The original also establishes the loop
state before clearing the vector accumulator and addresses the model
bone palette after the root matrix. The PS2-specific local lifetimes
preserve those sequences without changing the GameCube helper.

Original DWARF identifies `eval_tri` at `0x177090` in US/EU and
`0x177190` in Germany, with size `0x8d8` in all three. At offsets
`0x110..0x130`, all three contain the same load, variable shift, `0xff`
mask, and cache word/bit computation. The palette multiplication call is
at `+0x15c`; the two weighted transform loops and final root transforms
remain inside this function. No call to a separate skinning helper is
present. The retained candidate preserves the original called operations.

The candidate still has four additional NOPs and a different stack
layout (`0x160` versus the original `0x170`). This is a partial source
gain, with no new exact-function, runtime-identity, or compiler-patch
claim. Mutable input-pointer declarations and hoisted skin API locals
did not resolve those differences. No padding or register bindings were
introduced to force the remaining layout.

Validation used normal full-unit `ps2solo.py` builds for all three debug
regions and a complete GameCube `ninja -j 8` report comparison. Private
evidence is in `build/zfx-oct09/after-proof.json` and
`build/zfx-oct09/original-proof.json`. France has no profile for this
function in the source baseline used for this change.

The same unit's 520-byte `zFX_SpawnBubbleWall` also reaches 100% from
69.63077% in all three debug regions. Its original loop reloads the
position and velocity scale components after the random calls. The
existing manual load-hoisting workaround is now limited to non-PS2
builds, preserving GameCube's distinct original ordering. Normal
full-unit checks confirm this adds 520 exact bytes and one exact
function per debug region without other changes. The French unit's
existing `update_popper` profile and the complete GameCube report remain
unchanged. Private evidence is in `build/zfx-oct09/wall-proof.json`.

`zFXGooUpdateInstance` (1124 bytes) improves from 71.626335% to 99.99288%
in the three debug regions. The PS2 original omits the CPU geometry lock,
sine deformation loop, and unlock used on GameCube. PS2's existing
`zFXGooRenderAtomic` instead supplies the warping parameters to its goo
pipeline. The update now retains CPU vertex deformation only on non-PS2
builds. All 281 instructions align; excluding existing relocation-name
differences, only the prologue and epilogue stack adjustments differ
(`0x40` original versus `0x30` compiled). Existing runtime call and data
identities are not promoted by this source comparison. No artificial
stack allocation was added. Full-unit regional comparisons, the French
`update_popper` control, and the complete GameCube report show no
regressions; private evidence is in `build/zfx-oct09/goo-proof.json`.

The remaining goo frame difference has a concrete source-lifetime lead:
original DWARF includes a 12-byte `xVec3 pos` at `sp+0x30`, which is
absent from the reconstructed function. The original has no instruction
accessing that slot. A plain unused declaration is eliminated by the
current compiler; center-copy, component, assignment, and zero
initialization probes introduce instructions absent from retail and
regress the match. None is retained. The frame difference must not be
treated as evidence of a compiler defect or filled with synthetic padding.
The original local records are saved privately in
`build/zfx-oct09/goo-dwarf.txt`.
