# PS2 particle tank and emitter source recovery

## Snow frustum culling

The original `zParPTankSnowUpdate` uses the same packed PS2 frustum
representation as the decal and NPAR renderers. The source had retained `_loc`
and `par_dist` but omitted the culling operations. Restore the original vector
unit side-plane test and scalar near-plane test before allocating a render slot.
Alive particles outside the frustum still advance and remain in the simulation;
expired particles are replaced from the end, and pool exhaustion still truncates
at the current particle.

All three debug originals independently establish the `globals` -> `xCamera` ->
`frustplane` address path at offset `0x270`, and the 48-byte particle layout with
position at 0, size at 12, velocity at 16, and life at 28. The original kernel
starts at function offset `0x1dc`. Its four side-plane preload offsets, vector
arithmetic lane masks, packed sign test, and `vmul.w` branch-delay instruction
are checked as raw words. The scalar test uses planes 4–7, adds half the size,
and accepts only distances below `-0.1f`, preserving the original NaN behavior.

Private reproducible evidence in `C:/Projects/bfbb-agent-ps2-oct08/build`:

- `snow-original.py` / `.txt`: original instructions and particle-layout DIEs.
- `snow-cull-proof.py` / `.json`: independent per-original layout, addresses,
  kernel, scalar plane references, and constants.
- `snow-cull-compiled-proof.py` / `.json`: compiled lane and opcode audit,
  including the actual branch delay slot. Output GPR differences remain counted.
- `snow-cull-final-comparison.json` / `snow-cull-final-changes.json`: complete
  before/after reports for all three debug versions.
- `snow-cull-gc-before.json` / `snow-cull-gc-after.json`: complete GC controls.

SnowUpdate (1,400 original bytes) improves from 76.06857% to 95.42857% in all
three debug versions. Complete-unit fuzzy matching improves from 95.37636% to
98.32117%, preserving all 17 exact functions / 5,980 exact bytes and every other
function score. The compiled function is 1,424 bytes, so this is not an exact
match. GC preserves all scores and 26 exact functions / 7,096 bytes. The private
France profile has no supported source compile for this unit; no France match
claim is made. No targets, profiles, comparison rules, or compiler were changed.
