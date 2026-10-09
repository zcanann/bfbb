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

## Ground renderer

Restore the missing 2,384-byte Ground renderer with its original packed sphere
culling, VU Euler polynomial, temporary vectors, four-vertex geometry, atlas UVs,
six-index copy loop, and buffer flush behavior. The cached texture follows the
same independently proved layout as the streak renderers. The original static
index array is `{0, 1, 2, 3, 0, 1}` in all three regions.

Ground's original matrix path wraps each particle angle above pi by subtracting
two pi, then evaluates the VU polynomial and writes three matrix rows. The
separate scalar `xMat3x3Euler` call was a poor reconstruction of this path and
is not used by the restored function. The local assembly helper reproduces the
actual original arithmetic and lane masks; it does not change shared headers.

Original DWARF independently identifies `cosSinPolynomial` as an external F32
array in this unit and a 16-element F32 definition in `SB/Core/p2/iMath.cpp`.
Its addresses are 0x416de0, 0x4168e0, and 0x4162c0 in USA, Europe, and Germany.
All 64 original coefficient bytes agree. The compiled helper references that
same named table through its actual HI/LO relocations; no new constant table,
symbol alias, or normalization rule is introduced.

Ground improves from missing to 89.23322% in every debug region. The complete
unit rises from 51.78363% to 73.33266%; every other function score and all exact
code/data controls remain unchanged. The compiled function is 2,520 bytes
versus 2,384 original bytes. Register allocation, stack layout, scalar scheduling,
and three transfer hazard NOPs remain different. The VU arithmetic and lane
masks agree exactly, and the culling `vmul.w` occupies the actual branch delay
slot as in the original. No compiler-version explanation is assumed.

Evidence: `par-quad-original.py` / `.txt`, `par-ground-proof.py` / `.json`,
`par-ground-compiled-proof.py` / `.json`, `par-ground-final-comparison.py` /
`.json`, and `par-ground-final-changes.json`. GC and Xbox renderer sources remain
unchanged; this PS2 unit still has no France profile selection. Sprite remains
the sole missing function in the measured unit.

## Sprite renderer

Restore the final missing 2,032-byte Sprite renderer. Its pivot offset is a
zero-initialized vector, shifted by half of camera right/up according to the
asset flags. For each particle, retail adds the scaled pivot offset to the
particle position, performs packed side-plane culling and quad construction,
then subtracts that offset even when culled. The restoration preserves those
floating-point operations and their side effects; it does not replace them
with a separate render-only center.

The original preloads camera right/up into vf10/vf11 and sets vf10.w to 0.5.
The side-plane test prefetches the next particle when present. Four corners and
their packed colors use the original VU arithmetic and 64/32-bit store pattern.
Atlas UVs and the `{0, 1, 2, 3, 0, 1}` index pattern retain the original vertex
ordering. All VU state reloads after an intermediate RenderWare flush.

Sprite improves from missing to 94.82874% in USA, Europe, and Germany. The
complete unit improves from 73.33266% to 92.8517%, with all other scores and
all five exact functions / 844 bytes unchanged. Relative to the initial unit
before these four restorations, fuzzy matching rises from 38.355755% to
92.8517%. All 13 measured functions now have source bodies.

The compiled Sprite body is 2,016 bytes versus 2,032 original bytes. GPR choices,
initialization-loop NOPs and scheduling remain counted. The actual prefetch
branch's `vmul.w` delay slot and culling branch's `vadda.xyz` delay slot agree
with the originals, as do every arithmetic opcode and lane mask. The compiler
moves the initial half-size setup into the loop-entry branch delay slot; that
remaining difference is recorded. Symbolic inspection verifies every packed
position/color store and that the culling branch still reaches pivot restoration.

Evidence: `par-sprite-proof.py` / `.json`,
`par-sprite-render-compiled-proof.py` / `.json`,
`par-sprite-render-final-comparison.py` / `.json`, and
`par-sprite-render-final-changes.json`. These audits cover all three debug
originals and actual compiled objects. No shared renderer source, comparison
mask, compiler, France profile, or identity registry changes are included.

## Flush flags and reset order

Restore the U32 `transformFlags` local used for immediate rendering. The
original Streak debug scope names this local in all three versions. Reset the
vertex count before the index count, matching every original inlined flush in
all eight callers. Actual HI/LO relocation audits reproduce that field order
for all 24 compiled function/region pairs; its raw improvement alone does not
change the current report score and is not claimed as extra matched bytes.

The flags local improves Ground from 89.23322% to 89.6359%, both streaks from
91.04395% to 92.36264%, and Sprite from 94.82874% to 95.30118% in every debug
region. Complete-unit matching rises from 92.8517% to 93.240685%, preserving
all other scores, five exact functions / 844 bytes, and data controls. All
four restored functions' raw VU, delay-slot and packed-store audits were replayed
against these final compiled objects and still pass.

Evidence: `par-render-flush-proof.py` / `.json`,
`par-render-flush-compiled-proof.py` / `.json`,
`par-render-flush-final-comparison.py` / `.json`,
`par-render-flush-final-changes.json`, and the three
`par-render-flush-par-*-compiled-proof.json` replay reports.
