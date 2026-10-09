# France Plankton parameter body

`platforms/france_plankton_tweaks.py` proves the complete 12,572-byte Plankton
`register_tweaks` body at `0x36a0f0`. All three authenticated debug originals
must agree on its full DWARF ownership, canonical linkage, size and strict
closed control-flow/frame checks. Every non-address instruction bit is
preserved, and the complete masked body must occur uniquely in France.
This does not claim the complete Plankton translation unit.

All 101 literal operands preserve complete byte-identical strings. They use
96 distinct addresses, including six references to an empty-string default.
Every direct transfer preserves an independently proven complete callee:
89 float-parameter, five vector-parameter, one integer-parameter and six
xStrHash calls. The parameter helpers are independently regenerated from
their original-backed Dutchman proof before use. There are no optional entry
contexts whose output can change when later identities enter the registry.

Original DWARF proves two distinct sound arrays: `sound_asset_ids` is
`uint32[6][10]`, occupying 240 bytes; `sound_data` contains six 16-byte
`sound_data_type` records, occupying 96 bytes. Its unsigned id field is at
offset zero. Six loads preserve the precise original ID-array indices
`[0][4]`, `[1][3]`, `[2][0]`, `[3][3]`, `[4][3]`, `[5][3]`; six stores preserve
the id field in each sound record. All accesses must imply consistent French
array bases, preserve their original 272-byte separation, and keep the full
typed arrays within zero-fill storage. No BSS extent or data value is promoted.

The record uses the scoped `reviewed-complete-caller-callee-cluster` kind.
Neither fuzzy search output nor compiled-source objects are proof inputs.
Standalone validation passed all three originals in 29.36 seconds on
2026-10-09, including independent regeneration of the parameter helpers.

Authenticated-original tests passed in 70.17 seconds, rejecting changed
instruction bits, strings and typed-array mappings. The France-only source
profile restores all 101 verified calls and compares the newly selected body
at 97.76456% fuzzy matching, with no additional exact bytes. This expands
selected-function coverage by 12,572 bytes; the full CPU-code denominator is
unchanged. Existing source and other version profiles are unchanged. Private
validation: `build/plankton-france-pilot/report.json` in the regional checkout.
