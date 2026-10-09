# French boss animation builders

`france_npc_boss_animation.py` proves five complete builders independently
against the USA, PAL and German originals:

| Builder | French entry | Code bytes |
| --- | --- | ---: |
| KingJelly | 0x349810 | 2296 |
| BossSB2 | 0x358b20 | 3096 |
| Prawn | 0x362d40 | 992 |
| BossPlankton | 0x36dc80 | 2440 |
| Dutchman | 0x3aac70 | 1648 |

Each full original body must have the exact reviewed DWARF owner, linkage
and extent, pass the existing control-flow/frame/return checks, preserve its
zero alignment padding, and have exactly one full masked-template match
across all loaded French file spans. No whole-TU or contiguous cross-TU
claim is made. The generic uniqueness threshold is unchanged.

The proof inventories 279 changed data operands and 143 JALs per reference.
Only five fixed, independently proved xAnim/Common identities are accepted
as callees or callbacks. Later registry additions cannot expand that fence.
The unchanged instructions and all unreviewed operands remain literal.

Each owner's original declaration separately establishes its shared table:
`g_strz_bossanim` is char*[78] and `g_strz_subbanim` is char*[23]. Every full
string and table pointer alias is checked. KingJelly, Prawn and Dutchman also
copy typed local `ourAnims` arrays of 11, 10 and 13 integers. Their complete
136-byte initializers, original stack locations and exact load/store coverage
are checked through the first call delay slot. SB2 and Plankton construct
local arrays with literal stores already preserved by the complete body.
No data range is promoted, and compiled source supplies no identity evidence.

The shared initializer checker now reads the already authenticated function's
DWARF source owner. Replaying the preceding Robot proof produces exactly the
same complete JSON as before that change. Nine original-backed tests pass (112.7 seconds). Negative checks reject mismatched
owners, types, counts, stack locations, copy coverage or copy-prefix control
flow, as well as changed bodies, calls, callbacks, padding, tables, strings,
initializer payloads, original extents, dependencies and duplicate bodies.

Private original evidence is `build/npc-boss-animation-proof.json`; tests are
`tools.tests.test_france_npc_boss_animation` with `BFBB_FRANCE_TEST_ORIG` and
`BFBB_FRANCE_TEST_REGISTRY` set to authenticated originals/current registries.

All five newly selected French source functions match exactly: +10,472
exact bytes and +5 exact functions. Across the five selected source subsets,
exact bytes rise from 2,660 to 13,132 and exact functions from one to six.
The five previously selected function records retain their exact scores,
sizes and metadata relative to the verified `oct09-france-render-save`
baseline. All prior profile entries and every non-French profile are unchanged.
The source pilot is `build/npc-boss-animation-france-pilot/report.json`.
