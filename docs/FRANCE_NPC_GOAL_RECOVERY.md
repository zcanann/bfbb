# France Common and Standard NPC goal recovery

The original-only verifier identifies complete `zNPCGoalCommon.cpp` and
`zNPCGoalStd.cpp`: 59 functions / 18,884 bytes. Every holdout, including the
partially reconstructed Standard factory, remains in the source comparison.
Actual production compilation adds 29 code-matched functions / 4,320 bytes,
bringing France to 175 functions / 29,500 matched bytes. Common contributes
two / 320 bytes (65.13% fuzzy); Standard contributes 27 / 4,000 bytes (76.35%
fuzzy). No game source, compiler option, or reporting metric changes.

## Original identity and bounds

All three named debug originals independently provide the same ordered names,
canonical linkages and function sizes. The complete Common sequence maps
uniquely to France at `0x2cb190`: 984 bytes including alignment, with an unchanged
184-byte search anchor. Standard maps uniquely at `0x2c62c0`: 18,328 bytes with
an unchanged 600-byte anchor. Every non-address instruction bit is preserved.

These units have **no rooted direct-JAL witnesses**. Their identities rely on
unique complete three-original sequences, typed globals, and full vtable
relationships. No execution-reachability claim is made.

Fifty-five functions satisfy the unchanged strict local bounds checker. Three
Standard frame-free leaves tail-call complete, independently closed Common
`Enter` or `Resume` bodies. `MoveAutoSmooth` contains the same two unreachable
scalar load/store instructions at offsets `0x104` and `0x108` in all originals.
The original unconditional branch and its executed delay slot skip that island;
no local edge, named original entry, or direct transfer anywhere in the actual
CPU section enters it. All dead bytes and surrounding zero words remain intact.

Both sequences contain one authentic eight-byte header body: `Notice` from
`xBehaveMgr.h`, and `AnimPick` from `zNPCTypeCommon.h`. Their complete original
bytes, bounds, ownership and surrounding alignment are checked. Neither is
promoted as a new function or credited to these source units.

## Data and call evidence

Fifteen named original vtable declarations reference complete anonymous data
descriptors: fourteen 52-byte tables and the 44-byte `xGoal` table. Every null
slot and method slot is verified, including the original method name, linkage,
source, extent and consistent French entry. The 56 distinct method destinations
include 45 bodies in the two complete sequences and 11 external methods checked
in full. The external header `Clear` leaf has a real halfword-store delay slot
and tails to the independently closed, byte-identical eight-byte
`xBehaviour::Clear`. These external methods remain corroborating context.

The `zNPCGoalNoManLand` vtable is an authentic original data declaration. This
proof does not infer a missing class layout or manufacture an absent method.
The source factory remains a partial comparison.

All 22 distinct absolute data addresses are accounted for. In addition to the
15 tables, these are original `g_O3` vector components, `g_Z3`, the actual
single-precision `ds2_min` scalar, and two `globals` fields. Original DWARF
establishes the precise path through `zGlobals.player`, `zPlayerGlobals.ent`,
the zero-offset `zEnt` base `xEnt`, and its `model` pointer; the second field is
`zGlobals.sceneCur`. Initialized payloads are checked in full, aggregate storage
is bounded, and all pointer/component relationships are bijective.

The module-local address resolver recognizes the observed COP1 single-precision
`SUB.S` and `MOV.S` as FPR-only operations, alongside the previously decoded
math operations. Reaching-LUI, GPR clobber, control-flow and entry-bypass checks
remain mandatory. No shared decoder is relaxed. Thirty-seven external callee
entry prefixes corroborate structure but never become new named anchors.

## Validation

The module was independently reviewed and replayed against all four originals,
producing byte-identical proof metadata. The shared aggregate preserves all
246 prior whole-sequence records. The expanded registry contains 11 units,
305 functions and 145,980 bytes; the French known extent inventory is
597 functions / 214,848 bytes.

The normal production report passes against the verified platform206 CI
baseline. The two new units are the only added source comparisons; every old
unit is unchanged except the unresolved CPU range, reduced by the newly known
18,884 bytes. All prior profiles and effective debug-version profiles remain
unchanged. The new profiles restore one Common and 79 Standard calls only
through verified original identities and inverse J/JAL reconstruction. Full code/data denominators remain 2,979,968 / 2,384,384 bytes;
source completion and retail-link claims remain false.

```sh
python tools/platforms/france_tu_sequences.py --orig-dir orig --check
```
