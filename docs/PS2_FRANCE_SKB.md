# French SKB animation boundaries

The stripped French executable contains the complete original four-function
`SB/Core/p2/iAnimSKB.cpp` sequence. Two functions were already corroborated;
`iAnimDurationSKB` at `0x1a51d0` (32 bytes) and `iAnimEvalSKB` at `0x1a51f0`
(1,480 bytes) are independently recoverable from all three named originals.

The scoped proof preserves every instruction bit except the explicitly decoded
LUI/ADDIU address at Eval offsets 504/508. All four functions pass the unmodified
strict CFG/frame checks. The 4,216 function bytes occupy a unique 4,232-byte
sequence with zero-only alignment; both existing neighbors retain their exact
addresses, sizes, and bytes. Actual following alignment words are supplied to
the checker rather than fabricated zero padding.

Original DWARF identifies the changed address as `slerpPolynomial`, a zero-based
row-major array of 24 four-byte floats. Its complete 96-byte contents match
between all three debug originals and the French target, inside independently
established initialized-data regions. This is original-only name/extent
recovery, not compiled-source evidence or a new data-matching claim.

No game source, compiler flags, or generic instruction/CFG rules change.
The report's CPU-text and data denominators remain fixed. The two new bounds
reclassify 1,512 previously unresolved CPU bytes without adding any match merely
because those bounds became known. Whole-source comparison is a separate step.

A whole-source compile pilot of the shared `gc/iAnimSKB.cpp` remains blocked by
missing authentic `rtslerp.h` and `limits.h` interfaces and its GameCube math
macros. No guessed SDK declarations or platform implementation were added.
The boundary recovery is therefore published without an SKB source profile.

The standard target preparation and section export passed with all 60 unchanged
source objects authenticated against the preceding complete QuickCull build.
Every prior function record is unchanged: 34,384 bytes / 209 functions remain
matched, while known function count increases from 600 to 602. CPU text stays
2,979,968 bytes and data stays 2,384,384 bytes. Private evidence is in
`build/skb248/original-proof.json`, `full-report-proof.json`, and
`production/SLES-53623/reused-actual-source-build.json`.
