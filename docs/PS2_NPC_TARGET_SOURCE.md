# PS2 NPC target search

`NPCTarget::FindNearest` now preserves the original vertical-magnitude
evaluation, best-candidate initialization order and `found` guard. The changes
are restricted to PS2. The 808-byte function improves from 62.519802% to
98.935646% in all three debug releases.

Each complete original body contains `abs.s f2, f2` (`0x46001085`) at byte
offset 620, between the vector subtraction and optional Y flattening. The
earlier source omitted this calculation. Restoring `fv = iabs(vec.y)` uses the
existing authenticated PS2 absolute-value primitive; it introduces no new
assembly, register constraint or compiler patch. Although the magnitude is
subsequently overwritten, the original instruction remains observable in the
instruction stream. The platform primitive also inhibits the loop motion and
constant propagation that had displaced most of this function's instructions.

The original DWARF local order and registers agree with the source variables.
Initializing `npc_best` before the zero-initialized vector restores its early
saved-register assignment. Retaining the original `!found` guard restores the
branch preceding the source-pointer and category checks.

Full 54-function unit comparisons preserve the other 53 function records and
the existing 4,312 exact bytes / forty exact functions out of 12,552 bytes.
Overall similarity improves from 92.06246% to 94.40663% in SLUS-20680,
SLES-51968 and SLES-51970. These figures use the previous NPCSupport vector
improvements as the baseline.

The existing sixteen-function French selection remains unchanged at 1,300
exact bytes / eleven functions and 98.26792% similarity. FindNearest has no
selected French identity. All 73 function records and all data measures remain
unchanged in GQPE78, GQPP78 and GU4Y78.

Positive distance comparisons, four separate early category rejections and a
direct caller-side squared-length expression did not further improve the
result. The remaining differences are two extra padding instructions and the
floating-point destination used for the final squared distance and its two
consumers. These residuals are not attributed to a compiler-version mismatch.

Private evidence: `build/support-find-original-proof.json`,
`build/support-find-locals.txt`, `build/support-find-*-probes.txt`,
`build/support-find-init-diff.json`, `build/support-find-region-summary.json`
and `build/support-find-gc.txt`.
