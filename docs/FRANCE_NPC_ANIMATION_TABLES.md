# French Robot animation builders

`tools/platforms/france_npc_animation_tables.py` proves the contiguous block
from `ZNPC_AnimTable_Slick` at 0x2e3980 through `ZNPC_AnimTable_RobotBase`,
ending at 0x2e601c. Its 17 complete functions contain 9,808 code bytes inside
a 9,884-byte span. The remaining 76 bytes are individually checked zero
alignment gaps. Four trailing alignment bytes are also checked separately.
This is a complete cluster proof, not a whole-translation-unit claim.

USA, PAL and German original DWARF must independently identify the same
ordered members, sizes and relative entry offsets, with no additional member
inside the span. Each body must close under the existing control-flow and
frame/return checks. Exhaustive uniqueness is checked for the entire original
span against all loaded French file spans. Small members inherit identity
from their exact position in that unique complete cluster; they are not
claimed to have independently unique short instruction seeds. The generic
32-byte minimum anchor rule is unchanged.

Each reference supplies exactly 257 changed address operands and 149 JALs.
Five fixed, independently proved complete functions supply the external
callee and callback identities. Internal calls may target only the complete
RobotBase member, which does not call back into the cluster. Operand masks
cover only the inventoried addresses and calls. All other instruction and
alignment bytes remain literal.

The data evidence includes the original char-pointer array types and all 44
complete strings in g_strz_roboanim[41] and g_strz_cloudanim[3], sixteen
additional complete literals, and twelve local integer-array initializers
totaling 376 bytes. Every local array must have its original DWARF element
type, count and stack location. A scoped decoder follows every source load
and destination store through the first call delay slot, proving that the
full initializer reaches precisely that typed stack object without a branch,
gap, duplicate byte or excess copy. No data extent is promoted.

The original-backed tests reject changed bodies, calls, callbacks, padding,
complete string tables and literals, initializer payloads, original function
extents, array element types, stack locations, unexpected copy-prefix control
flow, missing or altered dependencies, and a duplicate complete cluster.
JSON round-trip identity is required. Set `BFBB_FRANCE_TEST_ORIG` and
`BFBB_FRANCE_TEST_REGISTRY` before running
`tools.tests.test_france_npc_animation_tables`.

The private French source pilot matches all 17 new functions exactly, adding
9,808 exact bytes. Its 38 selected Robot functions contain 13,864 exact bytes
and 34 exact functions, with 99.03137% fuzzy matching. No source edit is part
of this recovery. Private evidence is `build/npc-animation-proof.json`,
`build/npc-animation-tests.log` and `build/npc-animation-france-pilot/report.json`.
