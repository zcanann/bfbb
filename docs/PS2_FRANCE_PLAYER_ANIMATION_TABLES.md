# France player animation builders

`platforms/france_player_animation_tables.py` proves five complete builders
owned by the original `SB/Game/zEntPlayerAnimationTables.h` compilation unit:

| Builder | France entry | Code bytes |
| --- | --- | ---: |
| TreeDome SpongeBob | `0x159f20` | 6,536 |
| Boulder vehicle | `0x15b8b0` | 352 |
| SpongeBob tongue | `0x15ba10` | 560 |
| Patrick | `0x161880` | 11,056 |
| Sandy | `0x1643b0` | 14,400 |

All three authenticated debug originals must agree on DWARF ownership,
complete extents, canonical linkages, every non-address instruction bit,
strict closed control flow, and unique complete-body placement. Each body
is a scoped `reviewed-complete-caller-callee-cluster`; no whole-unit claim
is made. The original unit has six functions and 56,516 code bytes.

The first three builders inventory 106 complete changed string operands and
99 independently proven `xAnimDefaultBeforeEnter` pointers, plus 104 known
direct calls. Patrick and Sandy add 558 changed string operands, including
complete lists longer than 256 bytes, 117 default-before-enter pointers,
and 345 known direct calls. Unchanged address words remain literal
instruction comparisons. No fuzzy score, generated object, guessed extent,
or inferred adjacency is proof input. No existing callee is promoted again.

The proof excludes all records owned by its source from context lookup, so
later builder discoveries cannot bootstrap its replay. The shared decoder
recognizes only the exact COP1 `MOV.S` format with zero reserved `ft` bits;
MFC1/CFC1 writes still invalidate a reaching GPR definition.

Validation includes original-backed and literal-reader tests, including
mutations of caller instructions, string bytes, callback address operands,
and the independently known complete callback body. Three decoder tests
cover all 1,024 valid MOV.S source/destination pairs, invalid formats and
reserved fields, and MFC1/CFC1 clobber negatives. Standalone proof replay
takes about seven seconds for the first three builders and 38 seconds for
all five with authenticated local originals.

The private France source comparison restores all 449 calls and matches all
five builders exactly: 32,904 of 32,904 bytes and five of five functions.
The France-only profile adds those bytes to the selected-function coverage;
the full CPU-code denominator is unchanged. Source and existing version
profiles are unchanged. Private evidence and report are
`build/player-animation-tables-proof.json` and
`build/player-animation-tables-france-pilot/report.json` in the regional
checkout.

`france_player_animation_context.py` proves the complete 34 player callback
contexts referenced by Patrick and Sandy. German relocates 67 and 83 of
those pointer operands respectively; USA/PAL keep the same address words
literal. Each complete callback is compared against all three original
identities and must have strict closed bounds. Its identity is rooted by
the complete unique builder, and none of these callback extents are promoted.

Original DWARF proves globals inheritance, player member offsets and types,
and the two `uint32[3][47]` sound arrays used by `IdleCB`. Each relocated
operand must preserve its exact typed field/index, base, storage and array
separation. The sound-stop wrapper is exactly J/NOP to the complete 184-byte
`iSndStop`, whose typed 48-voice array and sole call to the independently
known `HISStopVoiceAsync` are rechecked. Its complete body is unique.

The 52-byte OOB timer's three scalar operands must preserve original `shared`
and `fixed` float fields and initialized values. Its complete masked body
must be unique; a scoped exhaustive search uses its 20-byte unchanged run
and exactly three low-half masks. The generic whole-unit uniqueness helper
is unchanged. Mutation tests reject modified callback fields, long string
tails and a duplicate complete timer body. Future registry discoveries
cannot substitute weaker optional contexts for these explicit dependencies.

Only the main SpongeBob builder remains outside this proof. Its insertion
helpers and typed string-pointer tables still need complete evidence.
