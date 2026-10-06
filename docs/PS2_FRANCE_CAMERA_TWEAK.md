# French PS2 camera-tweak coverage

The stripped French original now has the complete twelve-function `SB/Game/zCameraTweak.cpp` comparison. Eleven newly confirmed functions add 1,632 known code bytes; the previously confirmed 52-byte Reset function retains its complete original record. The unchanged complete source TU reports **12/12 functions and 1,684/1,684 code bytes at 100%**. This is a code-comparison result, not a complete data match or source-linked executable claim; completion metadata remains false.

`france_camera_tweak_sequence.py` authenticates all four executables and checks the complete 1,776-byte original sequence, including alignment, independently against each of the three named debug originals. All twelve members and their original linkage names agree, the complete sequence occurs uniquely, and every original alignment gap remains zero. Ordinary functions pass the existing strict CFG verifier.

The two eight-byte Load/Save wrappers are specifically verified as a single J followed by NOP, targeting the previously established complete xBaseLoad/xBaseSave entries. Both original and French destination bodies must pass strict CFG checks. These exact tail wrappers preserve SP and RA; the generic verifier is unchanged.

Changed state operands require the original DWARF types, not just inferred sizes:

- `zCamTweak` is 20 bytes: U32 owner at offset 0, and F32 priority/time/pitch/distMult at offsets 4/8/12/16. Its original row-major array descriptor proves exactly eight elements.
- Each `zCamTweakLook` is 12 bytes, with F32 h/dist/pitch at offsets 0/4/8. The two complete look aggregates and complete tweak array remain inside authentic runtime BSS.
- Every observed floating-point memory operand addresses one of those actual F32 fields. The locally accepted SUB.S, SQRT.S, MULA.S and MADD.S instructions write only FPR/ACC state and cannot clobber the checked GPR address lifetime.
- The callback address belongs to the independently declared, complete EventCB member. Internal and external direct transfers retain proved entry identities and bijective address relationships.

Three opaque runtime calls keep their actual JAL words unmasked. Their addresses and complete 64-byte entry contexts agree literally across all four originals. They receive no name, function extent, or progress credit. No state-data extent is promoted either.

The French profile expands the existing Reset-only comparison to all twelve functions. It restores five independently identified direct transfers and the EventCB HI/LO callback pair. Applying those actual original destinations to the genuine compiled source relocations reproduces all original bytes of EventCB, Load, Save and Init: **292 bytes/four functions**. Remaining GP/runtime relocations are unresolved and are not claimed as raw matches.

Validation artifacts in the isolated checkout are `build/camera276/original-proof.json`, `registry-preservation.json`, `report.json`, `raw-scope.json`, and `production/SLES-53623/report.json`. The actual complete source was recompiled after both accepted PS2 atan and xrmod header updates. The full-report gate compares with the published platform276 CI report, preserving all prior functions and the full CPU/data denominators. Reused objects come from completed actual whole-source compilations with checked hashes: the utility gate, the seven accepted atan consumers, and the accepted xrmod camera control. No compiler flags, game sources, or generic CFG rules change in this metadata commit.

The completed combined gate (`full-report-proof.json`) reports **47,520 matched bytes / 252 functions**, with **660 known function extents**, against the actual published platform276 baseline of 45,888 / 241 and 649 extents. The only gain is the eleven newly visible camera-tweak members. Every previous function record and every unrelated integer measure is preserved; seven unrelated aggregate fuzzy percentages differ only by rounding below 1e-12. Full CPU and data denominators are unchanged. The gate uses 53 checked objects from the completed utility gate, seven accepted header-control objects, and one freshly compiled complete camera-tweak TU.
