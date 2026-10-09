# PS2 particle-pool source recovery

The existing nine-function `xPtankPool.cpp` source unit improves from
81.39526% to 99.37032% in SLUS-20680, SLES-51968, and SLES-51970. Exact report
coverage increases from 984 bytes / four functions to 1,968 bytes / seven
functions. Existing exact functions, other scores, and data controls do not
regress. The France profile currently excludes this unit.

| Function | Bytes | Before | After |
| --- | ---: | ---: | ---: |
| `xPTankPoolSceneEnter` | 388 | 43.618557% | 100% |
| `sort_buckets` | 336 | 97.916664% | 100% |
| `ptank_pool::flush` | 260 | 92.69231% | 100% |
| `ptank_pool::grab_block` | 992 | 65.314514% | 98.770164% |

The PS2 initializer converts the unsigned maximum count directly to a float,
multiplies by one quarter, adds one half, and converts to the allocation count.
The previous intermediate `F64` produced software float/double conversion calls
absent from every original. Other platforms retain that existing expression.

Explicit PS2 inline declarations recover the original `create_ptanks` body in
both callers and the original `create_ptank` body in the replacement path of
`grab_block`. The compiler still emits the independently existing out-of-line
`create_ptank` function and uses it from the allocation loop, as retail does.
Declare that loop's iterator before its endpoint on PS2 to recover the original
register lifetimes. A measured shared version changed the GameCube
`create_ptanks` object, so that declaration order is platform-scoped.

The sorter now declares the texture before the bucket pointer, matching the
original DWARF local order and registers. Flush computes its existing ten-item
padding in the same initializer as its conditional unused count. This retains
the original signed comparison, unsigned subtraction, and clamping semantics
while recovering the original conditional-result lifetime and branch sequence.
These two shared expression/declaration changes preserve the complete GU4Y78
unit: all 12 functions / 2,648 bytes remain exact, with every score unchanged.

## Original and relocation evidence

`pool-originals.py` / `.txt` in the worktree's private `build` directory records
all three originals' entire instructions and DWARF locals. `pool-raw-proof.py`
replays actual source object relocations against the original named call targets,
existing GP relocation evidence, and the TU's original static `groups` address.
The complete 388-byte initializer and 336-byte sorter reproduce each original.

Flush reproduces 256/260 raw bytes after those proven relocations. Its one
remaining operand is the pre-existing stripped RenderWare call at offset
`0x24`, emitted by the source as `RpPTankAtomicUnlock`; its destinations are
USA `0x3799d8`, Europe `0x379ec8`, and Germany `0x379218`. Its identity remains
unresolved, and no alias or relocation is introduced from the matched source.
Thus the report's 100% result does not constitute a new proof of that SDK name.

`grab_block` still differs in the initial-count reload, register assignment,
and associated scheduling. `init_groups` remains at 96.77419% with its two
additional loop/alignment NOPs. Both deficits remain in the full-unit score.
No compiler mismatch is asserted from these remaining differences.

Private reproduction artifacts:

- `pool-final-comparison.py` / `.json`, `pool-final-changes.json`: all three
  complete before/after source comparisons against baseline `a64b247e3`.
- `pool-raw-proof.py` / `.json`: actual instruction and relocation checks.
- `checkpool-gc.py`, `pool-gc-before.json`, `pool-gc-after.json`: complete
  GameCube controls.

No build profile, original evidence registry, target object, shared header, or
compiler was changed.
