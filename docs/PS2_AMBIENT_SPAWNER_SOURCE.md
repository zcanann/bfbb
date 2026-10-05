# PS2 ambient NPC and spawner sources

Three complete shared translation units compile with the established PS2 profile:
`zNPCTypeAmbient`, `zNPCGoalAmbient` and `zNPCSpawner`. The only source edit replaces
one PowerPC-specific `(F32)__fabs(isin(x))` expression with the existing portable
`xabs(isin(x))` interface. The original PS2 `zNPCJelly::Process` contains the
inlined alpha helper and uses `ABS.S` in all three debug originals. No new
interface, synthetic type, compiler flag or replacement SDK body is added.
GoalAmbient and Spawner function bodies are unchanged.

All three debug versions have the following actual whole-unit results:

| Translation unit | Original functions / bytes | Normal exact | Independently raw exact | Fuzzy |
| --- | --- | --- | --- | --- |
| zNPCTypeAmbient | 35 / 8,080 | 20 / 3,368 | 17 / 2,000 | 76.854454% |
| zNPCGoalAmbient | 14 / 5,736 | 4 / 456 | 4 / 456 | 55.95816% |
| zNPCSpawner | 22 / 6,276 | 8 / 552 | 8 / 552 | 71.32887% |

The combined comparison includes all 71 functions / 20,092 bytes, with 32 normal
matches / 4,376 bytes. Independent application of actual source ELF relocations
to named original addresses reproduces 29 functions / 3,008 bytes. The three
additional normal matches are animation-table constructors with unresolved data
references; these are not claimed as raw or linked matches. Partial functions
remain present, and no unknown original callees are assigned invented names.
The units retain respectively five, two and one unresolved original references.

All 190, 176 and 172 shared aggregate names and their variants, including direct
bitfields, agree with each original debug file. Normal and debug source objects
have identical ordered allocated sections. Actual GameCube production compilation
of the changed Ambient source preserves all 15 allocated sections exactly; the
other two source files are unchanged.

The three profiles are restricted to the authenticated US, EU and German debug
originals. Their complete profile contents agree across those regions. France
is not enabled without reviewed ownership. No whole-unit completion or executable
link claim is made.

Private `build/ambient240` evidence contains all complete compiler commands and
objects, normal reports, independent raw proofs, original inlined `ABS.S`
instructions, all-region aggregate inventories, the Ambient GameCube comparison,
`profiles.json` and `summary.json`. Original and compiled bytes remain private.
