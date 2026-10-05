# Original PS2 render arrays and complete Sandy source

The PS2 `iParMgr.h` was missing an actual original declaration. Sandy's
`SpringRender` uses `gRenderArr.m_vertex`; it does not obtain vertices through
`gRenderBuffer`. Restoring the real type/global declaration and including the
existing PS2 `rwim3d.h` allows the complete Sandy translation unit to compile.
No rendering body or other function implementation changes.

All three authenticated originals record `tagiRenderArrays` as 0x5280 bytes:

| Member | Offset | Original array |
| --- | --- | --- |
| m_index | 0 | U16[960] |
| m_vertex | 0x780 | RxObjSpace3DVertex[480] |
| m_vertexTZ | 0x4b00 | F32[480] |

The DWARF array subscript records independently prove each element count and
fundamental/aggregate element type. RxObjSpace3DVertex is 36 bytes. Original
`gRenderArr` addresses are 0x533950, 0x533450 and 0x532e50 in USA, Europe and Germany;
the separately named `gRenderBuffer` follows each array at +0x5280. The global is
recorded in Core/p2/iParMgr.cpp and its consumers, including xCutscene, zLasso,
zLightning, BossPatrick and Sandy. The PS2 header now declares this genuine object;
it creates no replacement storage or alias.

In each original SpringRender, instructions at function offsets 0x34 and 0x4c are
LUI/ADDIU targeting s4. Decoding their opcodes, register fields and signed low
immediate reconstructs exactly `gRenderArr + 0x780`. The original loop stores
36-byte vertices in 72-byte pairs. This establishes the buffer use from the original
instructions independently of the compiled source or a guessed renderer API.

The complete original-derived profile covers all 70 functions / 44,456 bytes in each
of SLUS-20680, SLES-51968 and SLES-51970. All three actual reports give 23 standard
code matches / 3,852 bytes and 78.112114% fuzzy similarity. All partial functions and
30 unresolved scanned original references remain represented. Independently
regenerated regional symbolic call/GP records agree exactly and combine under a
three-SHA allowlist; production preparation reproduces each verified target.

Independent application of real source relocations reconstructs 22 functions /
3,396 bytes exactly in all three originals. AnimPick's 456 bytes remain a standard
code match with unresolved local addresses, not a raw-reconstructed or linked claim.
The duplicate file-local BoundEventCB name is resolved using the actual source
STB_LOCAL binding and the original Sandy compilation-unit ownership. No external
identity is inferred from a desired source destination.

Normal and `-g` objects have identical allocated sections. All 245 shared concrete
named aggregate variants, including repeated nested types and direct bitfield
storage metadata, agree with original DWARF in all three regions. Opaque and
original-only declarations outside that shared set are not claimed verified.

An actual isolated GameCube compile preserves all 21 allocated sections in order.
Its debug-bearing object file differs in nonallocated information; no machine-code
or allocated-data change is claimed. Removing the PS2-only include block reproduces
the original Sandy source exactly. The array declaration resides only in the PS2
platform header. Combined production reports will check existing consumers of that
header; this worker's proof covers the new full Sandy comparison and actual GC
allocated-section identity.

Private evidence: `build/sandy220/buffer-validation.json`, `array-layout-proof.json`,
`validation.json`, `gc/verification.json`, and `fixed/` containing the complete
profile, each regional report, layout-variant inventories and `raw-proof.json`.
The profile is handed separately to parent integration. No whole-unit completion,
France identity, full PS2 link, compiler option, or scoring-policy change is claimed.
