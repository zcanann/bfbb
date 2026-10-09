# PS2 utility and serializer comparisons

The registered PS2 compiler builds the complete existing xutil.cpp and xserializer.cpp sources. The initial utility profile selected the PS2 standard header with an ordinary C declaration of `isprint`; the original table-based header form has since been restored, as documented below. See [serializer source matching](PS2_SERIALIZER_SOURCE.md) for the later inline-boundary fixes. GameCube keeps its original includes.

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

## Original PS2 character-classification macro

The original `xUtil_idtag2string` has twelve inline character-classification lookups, each loading a byte from `_ctype_ + 1 + character` and masking it with `0x97`. The incomplete PS2 header previously emitted twelve calls to `isprint`. Restoring this ordinary table macro makes the complete 784-byte function exact in SLUS-20680, SLES-51968 and SLES-51970, with no compiler or comparison changes.

Each authenticated original independently declares `_ctype_` in the `xutil.cpp` DWARF compilation unit. Its array type has an unspecified upper bound and fundamental `char` elements; the header therefore declares `extern char _ctype_[]`. The established data anchors give addresses `0x4eba38`, `0x4eb538` and `0x4eaf38`, respectively. All 257 table bytes agree across the originals: element zero is zero for EOF, and `table[character + 1] & 0x97` is nonzero exactly for character values 32 through 126. The macro evaluates its argument once, keeps the signed `int` index so EOF maps to element zero, and does not introduce an unsigned cast. The production compiler's existing unsigned-char option makes the tag's byte inputs range from 0 through 255.

The complete source/include scan found only `xutil.cpp` using `isprint` or including `ctype.h` among SB sources and headers. Other repository `ctype.h` consumers belong to GameCube MSL C sources, none of which appear in the PS2 source profile. The explicit intersection with every PS2 profiled source is therefore just `src/SB/Core/x/xutil.cpp`; no other profiled PS2 unit consumes this header. GameCube and Xbox use their existing separate headers.

Whole-unit before/after reports agree across all four PS2 releases: 8 exact functions / 1280 bytes become 9 / 2064 out of 11 / 2668. Every other function score is unchanged; aggregate similarity rises from 85.16642% to 99.04048%. A complete GU4Y78 comparison keeps all seven functions / 1704 bytes exact and every function score unchanged. The existing French utility profile includes all eleven independently identified members and receives the same ordinary 784-byte exact gain. French data references remain subject to their existing unresolved-reference limits; no new data identity or full-link claim is introduced.

Raw verification uses the independently named original buffer and `_ctype_` data addresses, applies the source object's real HI16/LO16 relocations including the table's +1 addend, and reproduces all 784 bytes in each debug original. It also decodes the original seven-entry switch table from its guarded original address load, checks every entry against its original case block, and verifies the source table's seven real R_MIPS_32 relocations. No unresolved external reference is silently ignored.

Private reproducible artifacts are `build/util-full-comparison.py` and `.json`, the corresponding `util-france-comparison` files and scoped `util-france-diagnostic.py`, `build/util-ctype-proof.py` and `build/util-ctype-raw-proof.json`, plus `build/checkutilgc.py` and `build/util-gc-{before,after}.json` in the agent worktree. Existing ownership, progress denominators, compiler flags and completed-unit status remain unchanged.
