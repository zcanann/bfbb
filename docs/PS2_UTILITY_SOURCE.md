# PS2 utility and serializer comparisons

The registered PS2 compiler now builds the complete existing xutil.cpp and xserializer.cpp sources. The serializer needs no source edits after the earlier savegame/header separation. xutil only selects the PS2 standard header and receives the ordinary C declaration int isprint(int); no replacement implementation or compiler intrinsic is invented. GameCube keeps the original includes, and its rebuilt xutil allocated sections are identical.

Original DWARF owns 11 utility functions (2668 bytes) and 39 serializer functions (4212 bytes) in each of the three debug releases. All of these targets remain in each comparison. The existing utility source emits all eleven functions. The restored strtosjis adds 552 matched code bytes; BCDtoi remains nonmatching. See [text utility evidence](PS2_TEXT_UTILITIES.md). Both restored itoBCD overloads match all four PS2 originals byte-for-byte; see [BCD evidence](PS2_BCD.md). The serializer emits every original-owned function, plus ordinary compiler-emitted helpers.

Actual standard objdiff reports agree across all three debug releases:

| Unit | Matched code bytes | Matched functions | Original functions |
| --- | ---: | ---: | ---: |
| xutil | 1092 | 6 | 11 |
| xserializer | 3088 | 31 | 39 |

These are the project's normal code-match measures, which do not prove every external relocation or a complete executable link. Completed-unit status remains false. Target calls and GP references use only independently named original DWARF addresses, with the existing inverse relocation checks. Four serializer calls reuse the already reviewed runtime memset identity, including the original tail-jump opcode in WipeMainBuffer. No new runtime identity or structure layout is inferred from compiled output.

The source profiles cover whole original translation units and are scoped to the three debug executable hashes. No France ownership or progress is inferred by this change.

## Existing French bounds subset

The France-only xBound profile compiles the complete existing source against all three already confirmed original members: xRayHitsBound (208 bytes), xBoundOBBIsectRay (952), and xBoundGetBox (236). Their canonical linkage names agree across all three authenticated reference DWARF originals. The explicit-transfer registry records the corresponding French extents; this change adds no function identities or boundaries. Seven call relocations resolve to existing confirmed French callee symbols and pass the original-instruction inverse checks.

The two nonmatching members remain in the report, and complete-unit/source-link status stays false. The profile is restricted to the French executable SHA-1; every effective debug-region profile is unchanged. This is an extension of the existing partial French comparisons, not a claim that its entire bounds unit is recovered.
