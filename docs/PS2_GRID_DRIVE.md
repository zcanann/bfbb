# PS2 grid and entity-drive source comparisons

The complete unchanged `xGrid.cpp` now compiles using the restored entity,
collision and matrix declarations. `xEntDrive.cpp` additionally needs the SDK's
portable statement wrapper macros. The PS2 RenderWare header now supplies the
same guarded `MACRO_START do` / `MACRO_STOP while (0)` definitions already present
in the repository's RenderWare SDK header. No source bodies, data layouts,
compiler flags, or GameCube headers change.

All original functions remain in the comparisons:

| Source | Original functions | Compared bytes | Code matched bytes | Fuzzy score |
| --- | ---: | ---: | ---: | ---: |
| xGrid | 12 | 5,732 | 820 | 70.3873% |
| xEntDrive | 4 | 5,680 | 56 | 61.51197% |

Results agree across SLUS-20680, SLES-51968, and SLES-51970. All six code-matched
functions (876 bytes total) independently reproduce original bytes after applying
the actual source relocations to authenticated original symbols. The other
functions remain partial; neither whole-TU completion nor a linked executable
is claimed. France is not enabled without independently reviewed identities.

Original-versus-compiled DWARF checks verify every direct member offset and size
of `xGrid`, `xGridBound`, `xGridIterator`, `xEnt`, `xBound`, `xEntDrive`, `xCollis`,
`xEntFrame`, and `xModelInstance` in the applicable complete translation units.
Normal and debug compilations have identical ordered allocated sections.

The existing 45-source-unit PS2 production report retains all 5,391 complete
function records exactly, including 36,292 matched bytes. The three separately
validated matrix consumers from the preceding batch are not part of that older
private profile snapshot. GameCube does not include the modified PS2 header;
its source and preprocessor input remain unchanged.

Reproduction artifacts are under `build/ps2core176`: authenticated whole-source
compile inventories and commands, per-unit `profile.json`, `verify_regions.py`,
`all-region-summary.json`, original/compiled layouts, and `raw-proof.json`.
`regression-diff.json` is empty. Profile integration and the normal multi-version
CI run are performed separately.

## Larger collision follow-up

`xCollide.cpp` covers another 36,648 original bytes but needs additional genuine
RenderWare collision declarations. Its original code loads the collision-plugin
pointer through `_rpCollisionGeometryDataOffset`, then its BSP tree at offset 4
and triangle map at offset 12. Those used offsets agree with the existing SDK
header. The absence of a concrete `RpCollisionData` DWARF name is not evidence
of a different layout, nor sufficient proof of its unused members. No speculative
plugin layout was introduced for this batch.

## Entity-cell accessor call boundary

A PS2-only `dont_inline` scope around the entity-based `xGridGetCell` definition
restores the original call from `xGridUpdate`. The coordinate-based overload
keeps its existing inlining behavior. The actual complete source object now
matches all twelve functions / 5,732 function bytes in all four PS2 versions,
adding the 204-byte updater. The GU4Y78 GameCube object retains every function
score, including 14 exact functions / 3,576 bytes.

Independent raw comparisons reproduce all 204 updater bytes in each debug
original after applying its three actual calls and the real `gGridIterActive`
GP relocation. France's existing comparison profile still leaves the overloaded
accessor call and iterator-active global unresolved. Its ordinary exact code
score is therefore not a complete relocation or retail-link claim. No scoring
settings or original identities were changed to obtain this source result.

Private PS2-worktree evidence includes `build/grid-raw-proof.json`, its replay
script, retained whole `ps2solo` objects, `build/grid-gc-{before,after}.json`, and
the fresh limited French target in `build/grid-france-diagnostic`.
