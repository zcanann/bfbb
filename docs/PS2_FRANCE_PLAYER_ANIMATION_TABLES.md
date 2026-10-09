# France player animation builders

`platforms/france_player_animation_tables.py` proves three complete builders
owned by the original `SB/Game/zEntPlayerAnimationTables.h` compilation unit:

| Builder | France entry | Code bytes |
| --- | --- | ---: |
| TreeDome SpongeBob | `0x159f20` | 6,536 |
| Boulder vehicle | `0x15b8b0` | 352 |
| SpongeBob tongue | `0x15ba10` | 560 |

All three authenticated debug originals must agree on DWARF ownership,
complete extents, canonical linkages, every non-address instruction bit,
strict closed control flow, and unique complete-body placement. Each body
is a scoped `reviewed-complete-caller-callee-cluster`; no whole-unit claim
is made. The original unit has six functions and 56,516 code bytes.

The proof inventories every changed address operand: 106 complete string
operands and 99 independently proven `xAnimDefaultBeforeEnter` pointers.
All 104 direct calls retain independently proven full original identities:
three `xAnimTableNew`, 99 `xAnimTableNewState`, and two
`xAnimTableNewTransition` calls. Unchanged address words remain literal
instruction comparisons. No fuzzy score, generated object, guessed extent,
or inferred adjacency is proof input. No existing callee is promoted again.

The proof excludes all records owned by its source from context lookup, so
later builder discoveries cannot bootstrap its replay. The shared decoder
recognizes only the exact COP1 `MOV.S` format with zero reserved `ft` bits;
MFC1/CFC1 writes still invalidate a reaching GPR definition.

Validation: three original-backed and literal-reader tests pass, including
mutations of caller instructions, string bytes, callback address operands,
and the independently known complete callback body. Three decoder tests
cover all 1,024 valid MOV.S source/destination pairs, invalid formats and
reserved fields, and MFC1/CFC1 clobber negatives. Standalone proof replay
takes about seven seconds with authenticated local originals.

The larger SpongeBob, Patrick and Sandy builders remain outside this proof.
Their additional callbacks, insertion helpers and typed pointer tables need
complete independent evidence; the German original relocates player callback
pointers that remain literal unchanged words in USA/PAL.
