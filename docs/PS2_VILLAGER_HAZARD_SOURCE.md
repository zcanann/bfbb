# PS2 Villager and Hazard source coverage

The complete `zNPCTypeVillager`, `zNPCGoalVillager`, and `zNPCHazard` translation units compile with the established PS2 b38 flags. Their owned source fixes are limited to existing PS2 API access:

- Villager includes the existing `rwim3d.h` declarations for Bubble Buddy's render-state operations.
- Hazard includes existing streaming and render-state declarations. Its TU-local PS2 `FABS` spelling uses `xabs`, which already maps to the compiler-proven single-precision `__s_abs` intrinsic.
- Villager goals use that same float absolute operation for the PS2 `zNPCGoalTalk::Process` expression; the GameCube spelling remains unchanged.

The authenticated originals contain `abs.s` at Talk::Process offset 256, ReconChuck offset 400, and ReconSlickOil offset 372 in all three debug versions. No shared header, layout, compiler flag or implementation stub is introduced.

| Complete original unit | Original functions | Standard code matches / bytes | Raw reconstructed functions / bytes |
| --- | ---: | ---: | ---: |
| zNPCTypeVillager | 81 | 35 / 5,688 | 30 / 2,824 |
| zNPCGoalVillager | 35 | 20 / 2,408 | 20 / 2,408 |
| zNPCHazard | 75 | 10 / 1,192 | 8 / 396 |
| Combined | 191 | 65 / 9,288 | 58 / 5,628 |

These match counts agree across SLUS-20680, SLES-51968 and SLES-51970. Villager owns 20,796 bytes in each version. Villager goals own 10,972 bytes in USA/PAL and 11,048 in German; the independently generated German call records preserve that real difference. Hazard owns 58,072 bytes in every version. The fuzzy scores are Villager 76.94018%; goals 84.14145% in USA/PAL and 84.599205% in German; Hazard 48.031822% in USA and 48.032375% in PAL/German.

Profiles retain every original member, including unmatched factories and large partial functions. They restore only independently named original calls and GP operands. Unmodeled references remain: 18 for Villager, four for goals, and 81 for Hazard. Five version-scoped profile groups cover the three units.

Raw source-relocation application exactly reconstructs the functions listed above. Five additional standard-only Villager matches (BalloonBoy Reset, BubbleBuddy Setup and three animation tables) retain unresolved references. Hazard's animation table and RenderAll are also standard-only matches. No raw mismatch with fully resolved references was found. Standard code matching is not asserted to prove final data layout or executable linkage.

All 234, 222 and 186 respective shared aggregate variants match every debug original, including direct bitfield type/storage, bit offset and width. Existing independently justified wrapper normalization is retained. Normal/debug PS2 builds have identical ordered allocated sections. Actual GameCube recompiles preserve all 27, 22 and 21 respective allocated sections; only nonallocated debugging information may differ.

Private reproducible evidence lives in `build/npc227`: `profiles.json`, `validation.json`, original ABS.S witnesses, and each unit's full regional reports, original relocation records, raw inverse proofs, layout inventories, actual compile commands and GC allocated-section comparisons. All three units remain partial; no executable-link or data-completion claim is made.
