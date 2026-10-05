# Additional France source comparisons

Eight more complete shared translation units now compile against every currently
verified French member belonging to them. These nine original extents, totaling
2,104 bytes, were already in the authenticated French function inventory:

| Unit | Verified member(s) | Original bytes |
| --- | --- | ---: |
| zVolume | zVolumeSetup | 100 |
| xDebug | xprintf | 40 |
| xMorph | xMorphRender | 508 |
| xMovePoint | xMovePointGetNext | 560 |
| xParMgr | xParMgrUpdate | 116 |
| xPartition | xPartitionSpaceMove, xPartitionUpdate | 480 |
| xBehaveMgr | xBehaveMgr_SceneReset | 248 |
| zCameraTweak | zCameraTweakGlobal_Reset | 52 |

The profiles are enabled only for the authenticated France executable SHA-1.
Each linkage name was read from the original reference executable's DWARF at the
address already recorded in the corroborated registry. Reference names, source
ownership, extents and byte hashes were checked against those records. The
profiles record this provenance and include every currently verified member of
each selected unit; missing original functions are not invented or counted.
No original symbols, splits, boundary registry, shared source or compiler flags
change in this batch.

Three original direct JALs have independently named existing French destinations:
`zVolumeSetup` calls `xQuickCullForEverything`, `xMovePointGetNext` calls `xrand`,
and `xPartitionUpdate` calls `xPartitionSpaceMove`. Their source linkage names
come from the named reference DWARF, and the existing backend verifies the actual
French instruction target and inverse reconstruction. Unknown calls and globals
are left unresolved. No address or GP identity is inferred from a source object.

Actual whole-TU production compilation and the standard report increase France
matched code **6,144 -> 6,336 bytes** (+192): `xprintf` (40),
`zCameraTweakGlobal_Reset` (52), and `zVolumeSetup` (100). The other newly compared
members remain partial or unmatched. All eight units remain incomplete.
`xprintf` also has direct 40-byte original/source equality. The other two are
standard code matches, not claims that their unresolved globals have been
reconstructed or that the executable links exactly.

Every previously compared French unit outside this selected set remains
JSON-identical. The 331-function inventory and full 2,979,968-byte code denominator
are unchanged. Regenerating the target reproduces the committed symbols exactly.
All three debug-region full production reports remain JSON-identical, at 27,720
matched code bytes each. The ordinary original proof validators run during
report generation; no additional scoring policy or proof relaxation is used.
