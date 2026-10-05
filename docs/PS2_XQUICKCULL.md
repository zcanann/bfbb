# PS2 xQuickCull comparison

Ten of eleven retail xQuickCull functions reproduce their complete original
bytes in all three debug releases: 2,196 of 2,236 bytes. The scalar shared-source
intersection function remains unmatched against its 40-byte PS2 packed-MMI body.
All eleven authenticated DWARF extents enter the profile; this is not a complete
translation-unit or executable-link claim.

The sole source change marks the existing CellMerge definition inline on PS2.
Retail has no standalone CellMerge body and incorporates its ordinary signed-byte
minimum/maximum operations in the Line, Ray, Sphere and Box consumers. CellForVec
remains a static out-of-line helper. No arithmetic, calls, stores, linkage or source
function order was changed. GameCube allocated sections and their relocation references remain identical.

Inlining CellMerge alone substantially improves the four callers but initially
keeps their live pointers in a4-a6, using knowledge of CellForVec's exact clobbers.
Retail instead saves s0-s2 according to the ordinary call ABI. Actual original
.line records place CellForVec at line 202, before its consumers at lines 540,
552, 567 and 593; moving the helper later would contradict that source evidence.
The compiler's documented `-sym on` control produced real DWARF/line sections but
changed no comparison, so it was rejected.

The documented `-inline deferred` setting delays inlining until the end of the
translation unit. With the existing inline CellMerge source, this reproduces all
four caller bodies and their register saves exactly, also recovering retail's
reverse source-order emission. Before adding that shared setting, all 49 functions
in the eight existing source profiles were compiled with and without it: their
instruction bytes, relocation references and objdiff measures were identical,
including the existing xSurfaceInit and XOrdSort misses. No compiler binary,
undocumented pragma or instruction injection is involved.

All constants (including 0.05, 0.5, 1 and 127) are independently visible immediate
LUI/ORI/MTC1 patterns or stores; there are no guessed literal-pool relocations.
The fourteen direct transfers resolve through original DWARF canonical names,
including the two Init overloads. Applying those known destinations to the real
compiled source reproduces each complete exact body in all three originals.

The remaining Isects body uses four LW instructions, two PEXTLW instructions,
PCGTB and a zero test. No genuine typed SDK intrinsic interface was established;
inline-assembly macro substitutes are not used. The actual 144-byte scalar source
is still compiled and compared honestly against the 40-byte retail extent.

The complete profile is restricted to the three authenticated debug SHA-1s.
France's known-function registry and matching baseline remain unchanged. Private
commands, original .line evidence, raw relocation proofs and compiler-setting
controls are preserved under `build/math151/`.

The combined production prepare/compile/objdiff/section-export checks pass for
all four originals: 5,696 exact bytes in each debug region, with France unchanged
at 1,460. Existing symbol registries are identical. xQuickCull contributes exactly
2,196 matched code bytes and retains zero completed units.
