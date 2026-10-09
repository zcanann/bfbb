# French particle builders and Hazard kernels

`france_npc_particle_kernels.py` identifies nine independent complete bodies,
3,504 code bytes across `zNPCSupplement.cpp` and `zNPCHazard.cpp`.

| Complete function | French entry | Bytes |
| --- | --- | ---: |
| NPARParmVisSplash::ConfigPar | 0x3b80f0 | 284 |
| NPARParmChuckSplash::ConfigPar | 0x3b89d0 | 540 |
| NPARParmTubeConfetti::ConfigPar | 0x3baf00 | 1,144 |
| NPARParmOilBub::ConfigPar | 0x3bc8a0 | 304 |
| NPCHazard::StreakUpdate | 0x3c05d0 | 544 |
| NPCHazard::ColResp_Default | 0x3c8f00 | 280 |
| NPCHazard::ColTestCyl | 0x3c9840 | 240 |
| NPCHazard::PosSet | 0x3cb840 | 80 |
| HAZ_ord_sorttest | 0x3cd690 | 88 |

Each original USA, PAL and German body must have its authenticated source owner,
canonical linkage, full DWARF extent, normal entry/frame/return control flow and
zero terminal alignment padding. Every full masked template must independently
match exactly one address across all loaded French file spans. The large fuzzy
Supplement/Hazard blocks supplied discovery hints only. This proof makes no
whole-unit claim. Five other ConfigPar bodies failed individual uniqueness and
are deliberately absent: complete duplicate bodies cannot identify their owners.

The fixed external dependency set is xurand (88 bytes at 0x1ee4e0) and
xFXStreakUpdate (208 bytes at 0x1e8170). All eight direct calls are independently
checked against complete original callee identities. New registry records cannot
expand this set or change emitted evidence. The only data masks are the three
OilBub operands taking the address of g_O3 at 0x4f89a0. Its original declaration
in the Supplement source unit must be an xVec3 with three float components at
0, 4 and 8, total size 12. Both complete initialized-data payloads are checked
as twelve zero bytes. No data extent is promoted.

Seven original-backed tests passed in 126.0 seconds; standalone full replay took
16.8 seconds. Rejection controls include changed complete body/call/data words,
alignment padding, duplicate complete templates, original source/extent/type,
full callee bytes, missing or altered dependency identities, and the complete
zero-vector payload. Extra unrelated registry entries must produce identical
proof output, and JSON round-trip identity is checked.

Private evidence: `build/npc-particle-kernels-proof.json`,
`build/npc-particle-kernels-tests.txt` and
`build/npc-particle-kernels-france-pilot/report.json`. The production canonical
registry replay and full French source report remain integration gates.

With the current staging source, eight new functions add 2,360 exact bytes.
The five Hazard additions (1,232 bytes) match exactly, and its previous two
records remain exact, giving 2,060 exact bytes across all seven selected
functions. The four Supplement builders score 97.34155% fuzzy: three are exact
(1,128 bytes), while TubeConfetti retains the same 94.72028% score as all three
debug-reference versions. No source modification is included.
