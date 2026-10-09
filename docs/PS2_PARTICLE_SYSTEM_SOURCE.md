# PS2 particle-system source recovery

## Recursive update and sprite culling

Keep `xParGroupUpdateR` out of line on PS2. Its original 220-byte body calls
itself once; automatic recursive expansion had triplicated the command loop.
The explicit particle-tank Boolean predicate uses the flag mask directly, which
also recovers the original 52-byte `render_par_sprite` body.

Restore the original PS2 packed side-plane and scalar near-plane culling in
`par_sprite_update`. The GC `RwCameraFrustumTestSphere` path remains present for
GC. Independent original DWARF proves `xPar` is 96 bytes, with position at 16,
size at 28, and the next pointer at 0. The four-float position/size load therefore
matches the original vector input. The globals/camera plane chain, all vector
lane masks, packed sign tests, half-radius and `-0.1f` scalar constants, and actual
`vmul.w` branch delay slot are checked in all three debug originals and compiled
objects. Culled particles advance to the next particle without consuming a pool
slot; pool exhaustion still ends rendering.

All three debug versions have the same results:

| Function | Original bytes | Before | After |
| --- | ---: | ---: | ---: |
| `xParGroupUpdateR` | 220 | 0% | 99.90909% |
| `render_par_sprite` | 52 | 84.53846% | 100% |
| `par_sprite_update` | 1,580 | 68.38481% | 89.68355% |

The complete unit improves from 79.59359% to 94.184074%, with exact code rising
from 952 to 1,004 bytes (five to six functions). Every other function score and
data control is unchanged. GC retains every score and 22 exact functions /
3,492 bytes. France is outside the current source profile.

The recursive updater and render wrapper reproduce all 272 raw original bytes
after applying independently identified original call targets. The current
report still has a call-normalization residue in the recursive updater; no
target identity or comparison rule was changed to remove it. The sprite body
is 1,588 compiled bytes versus 1,580 original bytes; remaining sign-negation,
register, scheduling, and conversion differences are still counted.

Private evidence under `C:/Projects/bfbb-agent-ps2-oct08/build`:

- `particle-sprite-original.py` / `.txt`: original instruction streams.
- `particle-sprite-proof.py` / `.json`: independent original layout, camera
  address, kernel and scalar-test audits.
- `particle-sprite-compiled-proof.py` / `.json`: actual compiled vector masks,
  packed tests, and branch delay slots, with GPR differences recorded.
- `particle-recursion-raw-proof.py` / `.json`: actual call relocations and full
  byte equality for the 220-byte updater and 52-byte render wrapper.
- `particle-sprite-final-comparison.json`, `particle-sprite-final-changes.json`,
  and `particle-sprite-gc-{before,after}.json`: complete comparison controls.

## Original flag predicates

Use explicit equality-to-zero tests for aging/back-to-life flags and a Boolean
nonzero expression for visibility. All values remain 0 or 1, while the PS2
compiler recovers the original mask/XOR/SLTIU instructions. The unsigned-byte
link count likewise uses its original nonzero test.

In all three debug versions, the event callback improves from 92.62069% to 100%
(348 bytes), and init improves from 94.96089% to 98.88268%. Complete-unit fuzzy
matching rises from 94.184074% to 95.57394%, exact code rises from 1,004 to 1,352
bytes, and exact functions rise from six to seven. Every other score and data
control is unchanged. The shared source preserves all GC scores and its 22
exact functions / 3,492 bytes.

Evidence: `particle-flags-final-comparison.json`,
`particle-flags-final-changes.json`, `particle-flags-gc-{before,after}.json`, and
`particle-flags-raw-proof.py` / `.json`. The raw proof reproduces all 348 callback
bytes after independently mapped actual call relocations in all three originals.
Local sign-bit-negation helper probes lowered sprite matching and were discarded.
