# France King Jelly parameter cluster

`platforms/france_kingjelly_tweaks.py` proves the 23,444-byte King Jelly
`register_tweaks` at `0x342fb0` and its 376-byte `zParamGetFloatList` helper
at `0x1331f0`. These two newly proven bodies cover 23,820 bytes. The proof
does not claim the remaining King Jelly translation unit.

All three authenticated debug originals must agree on complete DWARF owners,
sizes, canonical linkage identities and strict closed control-flow/frame
checks. Comparison preserves every non-address instruction bit. The full
23,444-byte masked caller must occur uniquely in the French loaded image.
Its 160 data operands are complete byte-identical strings: 159 distinct
addresses with an injective mapping, including one repeated reference.

All 168 calls are accounted for. The verifier independently regenerates the
Dutchman parameter cluster before checking the 132 float-helper and 20
integer-helper calls, so this proof cannot bootstrap from its own registry
entries. Eight calls identify the new complete float-list helper, whose two
calls preserve independently verified xStrHash and xStrParseFloatList bodies.

The remaining eight calls preserve a complete, byte-identical 64-byte
`xColorFromRGBA` body owned by the original `xColor.h`. This body has closed
control flow and follows the large caller after exactly twelve zero alignment
bytes in every original and France. Its canonical linkage agrees across all
three references. It remains corroborating context, with no named function
extent, relocation or progress promoted through this module.

Records use `reviewed-complete-caller-callee-cluster`, with an explicit limited
scope. The existing canonical registry regeneration and strict ingestion
checks apply. Standalone validation passed all three originals on 2026-10-08
in 31.13 seconds, including independent regeneration of the Dutchman helpers.
This original-only evidence does not establish a linked executable or
compiled-source matching. Authenticated-original tests passed in 50.72 seconds,
including rejection of changed instruction bits, complete strings and the
complete color-helper context.

The France source profiles add these two members while preserving all earlier
zEnt selections and every other version profile. Authenticated source
compilation produced a 100% exact 376-byte float-list helper and 99.72701%
fuzzy matching for the 23,444-byte King Jelly body. Thus this addition supplies
23,820 newly covered bytes and 376 exact bytes. This expands the selected-function
comparison; the full production CPU-code denominator remains unchanged.
Existing zEnt function comparisons are unchanged.
Private validation is `build/kingjelly-france-pilot/report.json` in the regional
worker checkout. No source or compiler changes were required.
