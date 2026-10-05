# PS2 move-point source comparison

The existing scene resolver and name-lookup declarations now live in a small
xSceneLookup.h header that forward-declares xScene and xBase. xScene.h includes
that same API header. xMovePoint uses the lightweight header on PS2 because it
only calls xSceneResolvID; it never accesses scene or entity fields. No scene,
entity, RenderWare layout, or replacement function implementation is introduced.
GameCube retains the original xScene.h include context because removing it
changes that compiler's output even though the unused layouts are unnecessary.

The complete unchanged xMovePoint.cpp function bodies compile with the registered
PS2 toolchain. All nine original-owned functions (1500 bytes) remain in the
profile. The six code-matched functions total 320 bytes in each debug original:
Setup 208, Reset 64, SplineDestroy 24, Save 8, Load 8, and GetPos 8. The three
remaining functions remain visible and unmatched; overall code similarity is
90.57333 percent. This is the normal objdiff code score, not a complete unit or
executable link claim.

Direct calls and GP references use named original DWARF identities with the
existing inverse checks. Original xMovePoint and xMovePointAsset layouts were
read independently from all three debug releases and agree. No French function
extent is inferred or promoted by this profile.

Private evidence in build/ps2move160 includes actual whole-source compilation,
per-region original target objects and normal reports, original type layouts,
and all-224-object GameCube allocated-section comparisons.
