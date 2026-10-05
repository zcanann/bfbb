# France collision recovery

France now compares all 36 `xCollide.cpp` functions / 36,648 bytes from the
unchanged shared source. Eight functions / 2,236 bytes match, bringing France to
**18,888 matched bytes / 116 functions**. The whole unit has 53.6079463% fuzzy
matching. All holdouts remain in the normal comparison; these code scores do not
imply a completed TU or retail link.

## Original evidence

The three original debug builds independently identify the same ordered names,
canonical linkages and extents. Each complete sequence has exactly one matching
French location, starting at `0x1c5d50`, after accounting only for validated
address operands and direct transfers. Every non-address bit and zero alignment
gap is preserved. No compiled candidate participates in this proof.

Thirty-four bodies pass the strict control-flow bounds checker. Two frame-free
leaf tails preserve their incoming return address and have no calls, uncovered
words or other outgoing local edges:

- `sphereHitsModelCB` (28 bytes) tails to `iCollide.cpp::sphereHitsEnvCB`.
  That separate 472-byte original body is compared in full and independently
  closes in all four originals.
- `xCollideInit` (8 bytes) tails to the original eight-byte `iCollideInit`,
  whose complete body also independently closes.

These external callees provide boundary evidence, not new named relocation
anchors. Other external neighbors supply at most 32 bytes of prefix comparison
and are not promoted. Six entries have rooted direct-call witnesses; remaining
entries additionally rely on the unique whole-unit sequence, internal call
relationships and six verified callback pointers to complete same-TU bodies.

All fifteen changed data addresses are accounted for through actual original
DWARF declarations and operand roles:

- `g_O3` has three float component reads; its full 12-byte payload is identical.
- `g_I3`, `xqc_def_ctrl` and the three collision grids use their original typed
  bases. Recorded sizes are 64, 60 and 52 bytes respectively; alignment gaps
  are not invented as part of those objects.
- `anim_coll_old_mt.verts` is the recorded pointer-to-`RwV3d` member at offset
  20 of the 28-byte `RpMorphTarget`. Both original loads and stores use this
  same component.
- Six callback address pairs resolve to their fully verified function entries.

Mutable global extents lie within authenticated startup-zeroed storage. The
existing math verifier handles float loads while retaining reaching-definition,
register-clobber, branch-entry and delay-slot checks. No new instruction masks,
shared CFG exceptions, SDK layouts or global data extents are introduced.

## Production validation

The original registry adds 36 records, reaching six units / 174 functions /
90,648 bytes. All prior 138 records remain unchanged. Deduplicating the two
previously known collision extents produces 482 known French functions /
170,808 bytes. The full CPU/data denominators and zero completion claims stay
unchanged.

Actual production compilation and stock objdiff reporting pass. Only the
collision unit and unresolved CPU range change; every other unit report is
unchanged. The full code and data denominators remain 2,979,968 and 2,384,384
bytes. All previous profiles and all three debug-version effective profiles are
unchanged.

The source profile contains all 36 functions and 38 restored named call
relocations. Named call relocations are restored
only when an independently verified original callee and inverse instruction
check agree. Unknown external names remain unresolved under the project's normal
objdiff policy. No game source, compiler flags, debug-version profiles or report
math change.

```sh
python tools/platforms/france_tu_sequences.py --orig-dir orig --check
```
