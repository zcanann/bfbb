# Complete PS2 Tiki and Boss goal comparisons

Both `zNPCGoalTiki.cpp` and `zNPCGoalBoss.cpp` compile as complete shared source
translation units without source/header changes, stubs, or compiler-option changes.
The pinned MW PS2 3.0b38 compiler uses the existing production flags. Each profile
retains every function owned by its original DWARF translation unit and is scoped
to the three authenticated debug-region executables.

| Unit | Original inventory | Standard code matches | Fuzzy similarity |
| --- | --- | --- | --- |
| zNPCGoalTiki | 10 functions / 1,664 bytes | 7 / 424 bytes | 74.33654% |
| zNPCGoalBoss | 1 function / 3,576 bytes | 0 / 0 bytes | 63.868008% |

These results are identical in SLUS-20680, SLES-51968 and SLES-51970. The Boss goal
factory remains an honest partial comparison; its full original body is retained.
No France boundary or profile is inferred, and no full-unit completion is claimed.

All seven standard matches in Tiki goals independently reconstruct byte-for-byte
in every original after applying actual source ELF relocations to independently
named original function/data addresses. They are the Enter/Exit pair for Dead,
the Enter/Exit pair for Dying, Count::Enter, and Hide::Enter/Exit. Both complete
profiles have zero unresolved original J/JAL or GP references in the scanned
instruction forms. Unknown address-pair identities are not fabricated.

Normal and `-g` builds have identical allocated sections. The compiled layouts
agree with original DWARF in every region for all 142 shared concrete named
aggregates in Tiki goals and all 10 in the Boss factory. These are the aggregates
actually present in both scoped DWARF inventories; no opaque definitions or
unrecorded derived class layouts are claimed verified. Original/compiled xNPCBasic
bitfield storage is also checked where present. The corrected PS2 hazard header
is inherited from the already-integrated baseline.

No game source file changes in this batch, so the existing GameCube and Xbox
source inputs remain unchanged. No shared compiler flag or report-policy changes
are needed. The profiles add genuine complete-source comparisons and preserve
all nonmatching original members.

Private artifacts are in `build/goals218`: `compile.json`, `validation.json`, and
per-unit `profile.json`, `report.json`, `layout-inventory.json`,
`all-region-summary.json`, and `raw-proof.json`. The additive profile files are
handed to the parent integration checkout separately from this evidence document.
