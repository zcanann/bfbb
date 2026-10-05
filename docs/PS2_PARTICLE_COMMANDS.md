# PS2 particle-command source comparison

The unchanged complete `xParCmd.cpp` compiles with the existing pinned PS2
compiler and headers. All 30 original functions / 7,544 bytes are represented
in the comparison, including all unmatched functions. The initial standard
comparison matches 15 functions / 2,780 bytes, with 59.130436% fuzzy matching.
This is partial source coverage, not a completed TU or retail executable link.

Actual debug compilation confirms the sizes and every direct member offset
of all 22 concrete command/particle types present in this unit's original
DWARF, across all three debug-bearing releases. Debug information does not
change allocated source sections. The remaining source-only type names have
no corresponding concrete original declaration here and are not claimed to
have independently restored layouts.

Call and GP identities in the comparison profile derive from original DWARF.
No source functions, compiler settings, guessed API layouts, or original
boundaries were changed. All original-owned functions remain visible even
where compilation differs substantially. The normal code-match metric does
not establish complete relocation/data matching. France needs its own
independently verified identities before this unit can be reported there.

Private reproduction: `build/particles170/compile.py`, `compare.py`,
`type_proof.py`, and `type-proof.json`.
