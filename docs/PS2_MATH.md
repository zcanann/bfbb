# PS2 math and vector source comparison

The original debug releases name the shared xMath, xJaw and xVec3 functions through DWARF1. The complete translation units now compile with the registered PS2 compiler and flags; this does not establish whole-executable linking or completed units.

## Platform declarations and source separation

The PS2 math header supplies ordinary float sqrtf, powf, fabsf and floorf declarations. The standard-library header declares double atof(const char*) without supplying an implementation. Platform isin/icos/itan declarations use the existing shared API signatures. Compiler recognition of __s_abs was checked independently: its registration declares one four-byte floating argument and result, and actual full-TU output emits ABS.S without an undefined callee or a fabricated prototype.

The PS2 xsqrt body is the ordinary expression sqrtf(x). Its compiler output reproduces all seven instructions of the original 28-byte xsqrt header function. This diagnostic helper comparison is not counted as a separate progress translation unit. GameCube retains its existing implementation.

xMath chooses its existing GameCube intrinsic or the PS2 iabs implementation through a local source macro. GameCube preprocessing preserves the original calls. xJaw uses the appropriate platform math include. The streaming-service header now forward-declares its three opaque packer types; its implementation directly includes the defining packer header. No platform file or time structures are guessed.

xVec3 retains the two functions owned by its original PS2 translation unit. The paired-single Copy/Dot definitions and associated includes remain GameCube-only. PS2's unit-length branch reads the source fields in store order, matching the original y/z reloads; the GameCube cached-coordinate branch is unchanged.

## Measured results

The compiler output was compared against independently named original functions. Call and GP relocations are restored only when original DWARF function/data identities prove the destination, using the existing inverse-reconstruction checks. Unidentified runtime destinations remain unresolved in the target comparison; no source-derived target identity is introduced.

- xMath: all 18 original functions represented, 8 code-matched functions / 616 bytes under the standard objdiff report settings. Seven functions independently reproduce all 540 original bytes after relocation application: EndPoints (60), AccelMoveTime (120), SolveQuadratic (264), xrand (32), xsrand (8), MathExit (24), and MathInit (32).
- xJaw: both original functions represented; FindData matches all 232 bytes. EvalData remains nonmatching.
- xVec3: both original functions represented and nonmatching, each 66.60714% versus 65.41071% before the supported field-read change; 200 source bytes versus 224 original bytes. These fuzzy comparisons do not count as exact matched bytes.

Raw source relocation reapplication against all three original releases independently confirms 772 bytes across xMath and xJaw. Standard objdiff additionally counts the 76-byte AngleClamp code body: its external xfmod destination remains unidentified, so those bytes are not claimed as independently reconstructed. The project uses standard objdiff code scoring, which ignores function relocation identity; the report is not manually rewritten or switched to a stricter metric. Both the standard code score (848 bytes) and the independent reconstruction result (772 bytes) are recorded explicitly. Complete/link status remains false.

The four PS2 production configurations passed after the header changes with their prior matched-byte totals unchanged (5696 for each debug release, 1460 for France). New comparison profiles are restricted to the three debug executable hashes; no stripped France function identities are inferred. All 224 GameCube game translation units compile with identical allocated sections after direct packer/file header dependencies are supplied to their actual users. The normal GameCube all-source build, retail executable hash check, and full report comparison also pass without changes to the previous report.

One unresolved ABI observation is retained without changing the standard API: original xatof is an eight-byte tail jump, whereas the standard double atof declaration emits an additional conversion in the current configuration. Original DWARF does not identify atof or its return type. This is insufficient evidence to alter its declaration or choose a different double-width setting.
