# PS2 streak renderer restoration

Restore the two missing PS2 `iParMgrRenderParSys_Streak` and `InvStreak`
bodies. Their originals each contain 728 bytes. Both use the system's cached
particle texture, build a three-vertex streak with VU instructions, and flush
the immediate render buffer when another triangle would exceed its limit.
The ordinary streak uses length +5; the inverted streak uses -5.

All three debug originals independently establish the following:

- `globals.camera.frustplane` is at `globals + 0x270`; four packed side planes
  load into vf14 through vf17. Camera right loads from `gRenderBuffer + 0x60`.
  Both sets of registers reload after an intermediate RenderWare flush.
- A 96-byte `xPar` stores position/size at offsets 16/28, velocity/size velocity
  at 32/44, packed color at 12, and its next pointer at 0. The cached texture
  is at offset 40 in the `xParSys` base of `zParSys`.
- The VU kernel constructs the tail and its two width offsets. Head and tail
  plane results are ANDed before the packed sign test, rejecting a streak only
  if both endpoints are outside the same side plane. There is no scalar near
  plane test in these two renderers.
- The vertex order is head, tail-minus-width, tail-plus-width, with UVs
  `(0.5, 1)`, `(0, 0)`, `(1, 0)`. Each 36-byte vertex gets the particle's
  packed color at offset 12. Integer stores preserve the original UV words.

The complete 13-function unit improves from 38.355755% to 51.78363% in
SLUS-20680, SLES-51968, and SLES-51970. Each restored function improves from
missing to 91.04395%. Every other function score is unchanged; all five exact
functions / 844 bytes and data controls are preserved. No new exact bytes are
claimed. Ground and Sprite remain missing and are separate work.

Compiled functions are 736 bytes rather than 728. Remaining differences include
GPR allocation, a transfer hazard NOP, an unfilled culling branch delay slot,
constant vertex-count materialization, and flush scheduling/reset order. The
arithmetic opcodes and all VU lane masks agree exactly with each original.
Compiled packed vertex transfers and stores were symbolically checked against
the original component/color/UV order. These checks do not relax any comparison
mask or promote unresolved call identities. No compiler patch is proposed.

Only the PS2 implementation file changes. GC and Xbox use their own unchanged
renderer files. This source unit is not selected by the current France profile;
no France identity or coverage claim is made, and no profile/registry is changed.

Private reproducible evidence in `build/`:

- `par-streak-original.py` / `.txt`: complete original instruction streams.
- `par-streak-proof.py` / `.json`: original DWARF layouts, actual global
  addresses, exact kernel words, vertex stores, signed length and UV constants.
- `par-streak-compiled-proof.py` / `.json`: actual VU opcodes/masks, packed
  stores, branch delay slot and remaining size/NOP differences in all regions.
- `par-streak-final-comparison.py` / `.json` and
  `par-streak-final-changes.json`: full before/after unit comparisons.
