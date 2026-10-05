# PS2 thrown objects, boulders and shrapnel

The existing PS2 headers compile the complete `zThrown.cpp`, `xEntBoulder.cpp` and `zShrapnel.cpp` translation units without source changes. Their original DWARF inventories remain complete, including every unmatched function. The normal compiler settings, authenticated version define and standard objdiff comparison policy are unchanged.

| Unit | Original functions / bytes | Code-matched functions / bytes | Fuzzy code match |
| --- | ---: | ---: | ---: |
| zThrown | 23 / 14,140 | 11 / 1,460 | 81.90382% |
| xEntBoulder | 21 / 16,344 | 6 / 492 | 65.50367% |
| zShrapnel | 28 / 17,636 | 7 / 800 | 59.291904% |

These results hold independently for USA, PAL and German originals. All 24 code-matched functions, totaling 2,752 bytes per version, also reconstruct the authenticated original bytes after applying independently known source relocations. This is function evidence, not a whole-unit data or executable-link claim. Each unit remains partial.

The additive profiles cover all 72 original functions / 48,120 bytes. Original-backed call/GP relocation counts are 73/72 for zThrown, 138/17 for xEntBoulder and 162/35 for zShrapnel. Their four, three and eleven unmodeled references remain visible in the unmatched source comparisons; no replacement identities or implementations are introduced.

Actual normal and debug compiler outputs have identical ordered allocated sections. Broad comparison of every common named aggregate variant passes against all three original debug builds: 162 variants for zThrown, 214 for xEntBoulder and 162 for zShrapnel. The audit includes direct bitfield type/storage, bit offset and width. No layout repair, padding or compiler workaround is needed.

The source files are byte-identical to the existing integration checkout; this handoff changes no GameCube, Xbox or PS2 source/header contents. No redundant GC rebuild is claimed for this documentation/profile-only addition. Private evidence under `build/game231` includes the actual bounded compile inventory, complete regional reports, per-unit normal/debug commands, aggregate inventories, raw relocation proofs and source-identity hashes. `profiles.json` contains three additive profile groups, each restricted to the three independently verified original executable hashes. France coverage is not inferred.
