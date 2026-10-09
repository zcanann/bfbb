# PS2 hazard call boundaries and configuration

The October 9 pass on `zNPCHazard.cpp` restores five call boundaries visible
in the PS2 originals: `HAZ_Acquire`, `PickFunFrag`, `DeathStar`,
`FodBombBubbles` and `ColTestCyl`. PS2-only `dont_inline` directives preserve
those calls. Inlining the helpers had expanded several small update routines
to include acquisition loops, particle emission, or cylinder collision code.

`ConfigHelper` also directly converts the five locally constructed spin
vectors to rotation matrices before setting the rotation flag. The previous
nullable rotation helper left a null-address test and an unreachable random
rotation path in each of these five cases. The original PS2 instructions
contain only the matrix call and flag update. The two cases requesting a
random rotation still use the nullable helper. Non-PS2 call sites retain
their previous source.

Original DWARF records the union pointers `shroom`, `patriot`, `cloud`,
`tartar` and `hazcol` alongside `ball`. Restoring those locals allows the
compiler to use their common union address for all member accesses. This
shared source change also preserves the complete GC USA report.

## Validation

All 75 functions were compiled before and after for USA `SLUS-20680`, Europe
`SLES-51968` and Germany `SLES-51970`. Nine functions improve and the other
66 retain their scores. `KickSteamyStinky` becomes exact, adding **332 exact
bytes and one function per region**. The USA unit rises from 91.87636% to
97.72971% fuzzy and from 25,184 to 25,516 exact bytes.

| Function | Bytes | USA before | USA after |
| --- | ---: | ---: | ---: |
| ConfigHelper | 5068 | 68.698% | 99.970% |
| Upd_Explode | 432 | 3.157% | 91.759% |
| Upd_PuppyNuke | 588 | 45.714% | 89.497% |
| Upd_FodBomb | 572 | 42.126% | 87.552% |
| Upd_ChuckBlast | 368 | 38.489% | 97.609% |
| KickSteamyStinky | 332 | 46.241% | 100% |
| KickOilBurst | 468 | 58.265% | 96.581% |
| KickOilGlobby | 752 | 74.803% | 97.872% |
| KickBlooshBlob | 1432 | 85.218% | 96.768% |

Private evidence lives under `build/hazard-oct09/`: three before/after unit
reports, retained-project paths, `proof.json`, and original/source diffs.
The full GC USA report remains JSON-identical to
`build/string-oct09/GC-before-report.json`. Its checked retail DOL SHA-1 is
`306526d90b48e99894c3138f5fc8f2716d9fecf6`.

## Remaining configuration mismatch

The USA `ConfigHelper` instructions now differ only in stack frame size and
stack offsets. Its original frame is `0x120` bytes; the candidate uses `0xd0`.
The difference equals the five removed random-rotation temporaries. This is
consistent with the original having eliminated their code after assigning
stack slots, but does not establish the exact original source or compiler
pass. No dummy stack objects or forced register assignments were added.

Adding `inline` to the nullable helper and moving its definition before the
caller left the unwanted branches unchanged. A reduced nullable-helper
example reproduces them in MW 2.4, the current 3.0b38 and later 3.0.1b74.
Expanding the reduced helper as a macro also retains the branches. The later
compiler therefore supplies no evidence for a corrective compiler patch.
These probes are private and no compiler binaries, flags, source profiles or
proof registries change.

Other residuals still need investigation. In particular, some target calls
remain anonymous in the extracted object; their formatting alone is not
sufficient evidence to rename or authenticate them.
