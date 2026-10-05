# PS2 move-point source comparison

The existing scene resolver and name-lookup declarations now live in a small
xSceneLookup.h header that forward-declares xScene and xBase. xScene.h includes
that same API header. xMovePoint uses the lightweight header on PS2 because it
only calls xSceneResolvID; it never accesses scene or entity fields. No scene,
entity, RenderWare layout, or replacement function implementation is introduced.
GameCube retains the original xScene.h include context because removing it
changes that compiler's output even though the unused layouts are unnecessary.

The initial complete xMovePoint.cpp function bodies compile with the registered
PS2 toolchain. All nine original-owned functions (1500 bytes) remain in the
profile. The initial six code-matched functions totaled 320 bytes in each debug original:
Setup 208, Reset 64, SplineDestroy 24, Save 8, Load 8, and GetPos 8. The initial three
remaining functions stayed visible and unmatched; overall code similarity was
90.57333 percent. This is the normal objdiff code score, not a complete unit or
executable link claim.

Direct calls and GP references use named original DWARF identities with the
existing inverse checks. Original xMovePoint and xMovePointAsset layouts were
read independently from all three debug releases and agree. No French function
extent is inferred or promoted by this profile.

Private evidence in build/ps2move160 includes actual whole-source compilation,
per-region original target objects and normal reports, original type layouts,
and all-224-object GameCube allocated-section comparisons.


## Original unsigned zero tests

Both original member definitions and actual instructions establish that
numPoints is unsigned16 at asset+26 and bezIndex is unsigned8 at asset+23.
Init uses LHU followed by BLEZ at +68; SplineSetup uses LBU followed by BEQZ
at +96. The source now spells those equivalent nonzero tests as numPoints > 0
and bezIndex != 0 respectively. Both values promote to nonnegative integers,
so these source expressions preserve all input cases.

Actual compilation changes exactly those two branch words and no other
allocated bytes or relocation records. Init132 and SplineSetup488 both become
code-matched and independently reconstruct byte-exact against all three debug
originals using named original call/data addresses. The full nine-function
unit now matches eight functions /940 of1,500 bytes, a gain of620 bytes/two
functions; GetNext560 remains partial and visible. No completion, source-link,
or denominator claim changes.

All four complete source objects are byte-identical. France's existing reviewed
subset and function records remain unchanged. GameCube's four ordered allocated
sections are identical under the actual compiler. Private reproduction:
build/near253/{original-proof.json,proof.json,raw/raw-proof.json,gc/proof.json,
candidate/<version>/report.json}. No platform guard, compiler flag, type, header,
profile, or scoring-backend changes are required.
