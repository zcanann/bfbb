# PS2 King Jelly source comparison

The complete existing `src/SB/Game/zNPCTypeKingJelly.cpp` compiles with the established PS2 b38 profile, including deferred inlining and disabled C++ exceptions. No source, header, compiler flag, or layout change is required. The target inventory retains all 69 functions independently owned by this translation unit in each authenticated debug executable.

| Original | Owned code bytes | Standard code-matched functions / bytes | Fuzzy code score |
| --- | ---: | ---: | ---: |
| SLUS-20680 | 56,828 | 17 / 4,976 | 70.16302% |
| SLES-51968 | 56,812 | 17 / 4,976 | 70.1273% |
| SLES-51970 | 56,812 | 17 / 4,976 | 70.1273% |

Profiles restore 429 direct call/tail-jump operands and 11 GP operands per executable using original DWARF function identities and original data anchors. Twelve unmodeled call/GP references remain visible; no identity is inferred from the compiled source. The USA and PAL relocation records require two profile groups; PAL and German records agree after their executable hashes are removed.

Independent application of actual source relocations reconstructs 15 functions / 2,380 bytes exactly in all three originals. These include the 664-byte tentacle-lightning update, 324-byte lightning creation helper, and the 248-byte expanding-ring update. The additional standard code matches are `AnimPick` (300 bytes) and `ZNPC_AnimTable_KingJelly` (2,296 bytes), whose literal/static references remain unresolved in the raw proof. They are not claimed as byte-reconstructed or linked matches.

Normal and debug builds have identical ordered allocated sections. All 200 shared named aggregate variants agree with every original, including direct bitfield storage/type, bit offset and width. Repeated names retain all distinct layouts. The existing independently justified `xNPCBasic` flags and `effect_data` wrapper normalization is preserved. No GC or Xbox source preprocessing changes, and no original function is excluded to improve the score.

Private reproducible evidence is under `build/kingjelly226`: complete regional reports and profiles, `profiles.json`, `compile-commands.json`, `validation.json`, `raw-proof.json`, and the three layout inventories. The unit remains partial; executable link completion and data matching are not established by this comparison.
