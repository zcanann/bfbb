# PS2 NPCSupport source

The complete `SB/Game/zNPCSupport.cpp` translation unit compiles with its original
function bodies and the established PS2 profile. PS2 selects the existing
`rwim3d.h` interface needed by its rendering functions, replacing an unused
`rtslerp.h` include. The GameCube include path stays unchanged.

All three original debug files define `Firework` as 36 bytes with direct fields:
`fwstate` is an 8-bit `en_fwstate` field at bit 0, `fwstyle` is an 8-bit
`en_fwstyle` field at bit 8, and signed `flg_firework` occupies 16 bits at bit 16.
The scalar and vector members follow at offsets 4, 8, 12 and 24. The PS2 header
now spells these fields directly; its compiler did not expose members of the
previous anonymous wrapper. GameCube retains that wrapper unchanged.

The proof compares every member displacement, bit offset and width. It follows
the enum type references and checks both complete enumerator blocks and their
4-byte storage types. The flag field retains DWARF signed fundamental type 8.
No padding, replacement enumeration or synthetic SDK layout is added.

Actual whole-unit results are identical for SLUS-20680, SLES-51968 and SLES-51970:

- All 54 original functions / 12,552 bytes are included.
- 19 normal matches / 1,272 bytes; fuzzy similarity is 55.116955%.
- 16 functions / 892 bytes independently reproduce original instructions after
  real source ELF relocations are applied to original named addresses.
- `NPCC_TmrCycle`, `NPCC_ang_toXZDir` and `zNPC_SNDInit` retain unresolved math or
  literal references; their normal matches are not claimed as raw/link matches.
- All 177 shared aggregate names and variants, including direct bitfields, agree
  with all three originals. Normal/debug allocated sections are identical.
- Actual GameCube production compilation preserves all eight allocated sections.

A complete source-token search finds `Firework` and its changed fields only in
`zNPCSupport.h` and `zNPCSupport.cpp`; no previously enabled unit allocates,
embeds, takes its size or accesses these fields. This is a static consumer audit,
not a claim that every previously enabled unit was rebuilt.

The profile includes partial functions and retains 31 unresolved original
references. It is SHA-gated to the three authenticated debug originals; France
is not enabled without reviewed ownership. No whole-unit completion or executable
link claim is made.

Private evidence under `build/support242` contains original field/type records,
compiled enum/bitfield proofs, normal reports, raw relocation proofs, aggregate
inventories, compiler commands, `gc/proof.json`, `firework-consumers.json`,
`profiles.json` and `summary.json`. Original and compiled bytes remain private.
