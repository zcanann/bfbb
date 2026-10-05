# PS2 common and standard NPC goals

The complete unchanged `zNPCGoalCommon.cpp` and `zNPCGoalStd.cpp` source files
now compile with the existing PS2 declarations and pinned compiler flags.
All original functions remain in their comparisons, including the holdouts.

| Source | Original functions | Original bytes | Exact functions | Matched bytes | Fuzzy |
| --- | ---: | ---: | ---: | ---: | ---: |
| zNPCGoalCommon.cpp | 5 | 940 | 2 | 320 | 65.13191% |
| zNPCGoalStd.cpp | 54 | 17,944 | 27 | 4,000 | 76.34842% |

All three debug regions independently produce these results. Together they add
29 matched functions / 4,320 bytes across 18,884 compared bytes, bringing each
debug-version total to 73,712 bytes / 586 matched functions.

## Evidence and limitations

Original DWARF records establish complete ordered membership, canonical linkage,
and function boundaries independently in each region. Ten concrete aggregate
records for the common goal unit and 25 for the standard goal unit agree with
the actual compiler on full size and direct member offsets. Normal and debug
compilations preserve identical ordered allocated sections.

The common goal original contains no concrete NPCMsg, zGlobals or zPlayerGlobals
record, so those optional dependency types are not included in its layout claim.
The source standard-goal class `zNPCGoalNoManLand` has no same-named original
concrete record; its layout is not claimed verified. The complete factory that
constructs it remains included as a 63.736584% partial comparison, not an exact
match or a proven source identity for every allocation case.

Every one of the 29 normal exact functions also reconstructs byte for byte at
its independently known original address in all three regions after applying
actual source relocations. The standard unit's nine unresolved initial references
remain explicit in partial functions; no guessed identities are added.

There are no source, header, compiler, linker or report-policy changes in this
PS2 profile addition. Full-game code/data denominators and completion remain
unchanged. No complete-TU or retail-link claim is made.

Private evidence is in `build/npc202/`: complete compile inventory and per-unit
normal/debug objects, original/compiled layouts, all-region reports, profiles,
unknown relocation inventories and `raw-proof.json` files.
