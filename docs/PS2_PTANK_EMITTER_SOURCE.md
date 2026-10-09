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

## Emitter event callback

Cache the receiving object as a typed `xParEmitter*` and use it throughout
`xParEmitterEventCB`. This removes the compiler's extra saved member-address
register and recovers the original stack layout and member accesses. Event
ordering, reset behavior, flags, and custom emission arguments are unchanged.

The 196-byte callback improves from 83.87755% to 100% in all three debug
versions. Complete-unit exact matching rises from 772 to 968 bytes (six to seven
functions), and fuzzy matching rises from 96.549774% to 97.06376%. Every other
function score and data control remains unchanged. GC retains every score and
16 exact functions / 2,816 bytes. France is outside this unit's source profile.

Private evidence: `emitter-event-final-comparison.json`,
`emitter-event-final-changes.json`, and `emitter-event-gc-{before,after}.json`.
`emitter-event-raw-proof.py` / `.json` independently reproduces all 196 original
bytes in each debug executable after applying actual call addresses from
original linkage identities and the existing reviewed `memset` runtime anchor.
No relocation is inferred from the compiled call's position.

## Interpolation result lifetime

The PS2 original has a distinct result local `val` and uses the `time` parameter
for interpolation arithmetic. Remove the reconstructed `val = time` initializer
on PS2. All three originals' unsigned mode guard and eight-entry jump tables
show that modes 0, 1, 2, 3, 4, 5, and 7 assign the result on every path. Mode 6
and out-of-range inputs reach the epilogue without assigning the return register.
This restores that original undefined return behavior instead of inventing a
fallback. The mode conversion helper produces only the handled modes; unknown
names convert to mode 0. GC retains its existing initializer.

The 404-byte helper improves from 98.36633% to 100% in the existing source report
for all three debug versions. Unit exact matching rises from 968 to 1,372 bytes
(seven to eight functions), with every other score and data control unchanged.
GC retains all scores and its 16 exact functions / 2,816 bytes.

Private evidence: `emitter-interp-original-proof.py` / `.json` validates original
dispatch, result assignments, and the local's DWARF type;
`emitter-interp-final-comparison.json`, `emitter-interp-final-changes.json`, and
`emitter-interp-gc-{before,after}.json` preserve complete comparisons. The raw
audit `emitter-interp-raw-proof.py` / `.json` also checks all eight compiled jump
table entries against the original targets and reproduces the function bytes
except the two explicitly unresolved math-call addresses. It does not establish
new `sinf`/`cosf` identities or change call normalization or coverage.

## Signed positive particle limits

All three original `xParEmitterEmit` bodies load the unsigned 16-bit particle
limit and test it with signed `blez` branches. Express the three enabled-limit
checks as `(S32)maxPar > 0` on PS2. This is equivalent to the prior nonzero check
over the complete unsigned 16-bit domain, while avoiding extra compiler masks.
The duplicate capacity check is present in the original and remains intact.

Emit improves from 94.56892% to 95.37297% in all three debug versions; complete
unit fuzzy matching rises from 97.17111% to 97.55823%, with all exact bytes,
other function scores, and data controls unchanged. An initial shared spelling
regressed GC Emit from 98.86503% to 98.466255%, so the final condition is scoped
to PS2. GC then preserves every score and all 16 exact functions / 2,816 bytes.
Evidence: `emitter-limit-original.py` / `.txt`,
`emitter-limit-final-comparison.json`, `emitter-limit-final-changes.json`, and
`emitter-limit-gc-{before,after}.json` in the private build directory.
