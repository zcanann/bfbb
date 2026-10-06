# French PS2 utility coverage

The stripped French original now has the complete eleven-function `SB/Core/x/xutil.cpp` comparison. Eight newly recovered original functions account for 2,356 code bytes; three existing neighbors remain unchanged. No source, compiler flags, generic CFG rules, or code/data denominators change.

`france_utility_sequence.py` authenticates all four executables, then checks the complete 2,736-byte sequence (including original alignment) against each named original. All eleven extents pass strict control-flow checks. The complete sequence occurs uniquely; every prior neighbor retains its original identity and full-body hashes.

The one indirect jump belongs to the original 784-byte `xUtil_idtag2string`. Its local rule requires the exact seven-entry bounds check, register dataflow, table load, JR/NOP, internal destinations, and absence of entries bypassing the guard. The observed SLL moves source register 5 into destination 4. This rule is restricted to this function and dispatch offset; it does not weaken the shared recognizer.

Changed data operands require independent original evidence:

- DWARF proves the complete `g_crc32_table[256]` U32, `ascii_table[3][2]` U16, `ascii_k_table[33]` U16, and `buf[6][10]` char types. Complete initialized payloads agree, or the full array fits the authentic BSS region.
- Twelve byte-table reads have unsigned LBU indices, non-clobbered address/index lifetimes, and LBU accesses bounded to 256 identical initialized bytes. They establish observed context only, with no library-name or data-extent promotion.
- Complete string literals agree including their terminators.
- Two opaque runtime entry prefixes differ only in a proved pointer-load operand. Original animation DWARF explicitly declares `_impure_ptr` as a pointer to `_reent` of size 752. Existing independent animation records are checked by full original body hashes and by decoding their actual address pairs again. The pointee remains 752 bytes before the pointer slot and entirely inside initialized data. These runtime entry contexts receive no new name, function extent, or progress credit; their caller JAL words remain unmasked.

The actual unchanged complete source TU, built with the pinned production compiler and French flags, reports 1,092/2,668 code bytes and six of eleven functions at 100%. Newly visible standard code matches total **900 bytes/four functions**: weight adjustment (280), Shift-JIS conversion (552), shutdown (20), and startup (48). Weight adjustment additionally matches all 280 original bytes with no relocations. Other standard matches retain unresolved data/runtime relocations; this is not a whole-TU raw match or source-linked executable claim. All partial functions remain visible.

Validation artifacts in the isolated checkout are `build/util268/original-proof.json`, `report.json`, `raw-scope.json`, and `production/SLES-53623/report.json`. The full report is checked against the actual published platform268 CI report, preserving every prior function and the complete CPU/data denominators. Cache reuse is limited to previously completed actual whole-source objects with verified hashes and unchanged source/header/compiler inputs; current utility and changed volume source are rebuilt as complete TUs.

The completed full gate (`full-report-proof.json`) reports **45,216 matched bytes / 239 functions**, with **649 known function extents**, against the published platform268 baseline of 44,316 / 235 and 641 extents. Every old function record is unchanged; seven unrelated aggregate fuzzy percentages differ only by floating-point rounding below 1e-12. All integer measures outside the intended utility/unknown-coverage transfer are preserved. The gate reuses 59 authenticated complete objects and rebuilds two complete TUs. This frozen source baseline is `915d60b14`; later independently verified source gains are composed separately during integration.
