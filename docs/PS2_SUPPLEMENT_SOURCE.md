# PS2 Supplement source and particle layout

The PS2 compiler cannot expose the members of `NPARData`'s existing anonymous bitfield wrapper. Whole-source compilation first fails on `nparmode`; flattening these fields only for PS2 restores the layout independently recorded in all three debug originals:

- `sizeof(NPARData)` is 80 bytes.
- Signed 32-bit bitfield storage begins at byte 64: `flg_popts` has bit offset 0, width 24; `nparmode` has bit offset 24, width 8.
- `unused[3]` begins at byte 68.

The GameCube/Xbox declaration remains unchanged. This is real particle storage used by `NPARMgmt::NextAvail` and `PromoteTail`, not padding or a matching-only qualifier. The complete compiled `NPARMgmt::Init` reconstructs its 228 original bytes exactly.

Two remaining compilation boundaries are restored without implementations: the GameCube-only `std::floorf` redeclaration is excluded on PS2, which already has the standard API and namespace alias; PS2 `rwcore.h` gains the exact `RwCameraFrustumTestSphere(const RwCamera*, const RwSphere*)` declaration from the supplied vendor SDK `include/rwsdk/rwcore.h`. The existing PS2 camera/sphere/result types are retained. That prototype is vendor API evidence, not a recovered original callee identity. No frustum function, stub, or named original call relocation is fabricated.

All three versions retain the complete 65-function / 41,380-byte original Supplement inventory. Standard comparisons report 13 functions / 3,808 bytes in USA (52.153988% fuzzy), and 12 / 3,216 in PAL/German (52.153408%). Independent application of source relocations reconstructs ten functions / 1,020 bytes exactly in every version. The remaining standard-only matches retain unresolved switch-table/static references. Profiles restore 187 independently known calls and 38 GP operands; 32 unmodeled references remain unchanged.

The 592-byte streak-info function has a real regional timing difference. Original instruction pairs construct `0x3c888889` and `0x3cccccce` in USA, versus `0x3ca3d70a` and `0x3cf5c28f` in PAL/German: 60 Hz versus 50 Hz frame-time expressions. Existing PS2 profiles do not provide regional preprocessing definitions, so this change leaves the PAL/German miss visible. No compiler flags or scoring rules are changed to hide it.

All 215 common named aggregate variants agree with every original, including direct bitfield type/storage, offset and width. Normal/debug Supplement builds have identical ordered allocated sections. With the final shared headers, actual Robot, King Jelly and Villager recompiles preserve all 594, 409 and 331 allocated sections respectively. The source inventory finds particle-storage operations only in Supplement; other named NPARMgmt uses are pointer declarations and the cinematic `KillAll` count reset. An actual GC Supplement rebuild also preserves its 19 allocated sections.

Private evidence is under `build/npc229`: exact final-header consumer comparisons, source type-use inventory, complete regional reports/profiles, layout inventories, raw reconstruction, compile commands and the GC comparison. `build/npc228/npar-originals.json` records original field attributes; `zNPCSupplement/original-streak-timing.json` records the timing operands. The additive profile remains partial, with no data-layout completion or executable-link claim.
