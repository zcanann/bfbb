# France animation recovery

France now compares the entire `xAnim.cpp` TU: 34 functions / 17,656 bytes.
The unchanged shared source matches 17 functions / 3,032 bytes with 77.96624%
fuzzy matching. The French total rises from 11,936 / 78 to **14,968 matched
bytes / 95 matched functions**. All holdouts remain in the report. This is
normal code matching, not a complete-TU or retail-link claim.

## Original-only identity and bounds

All three debug originals identify the same ordered 34 functions and canonical
linkages through DWARF. Their full instruction sequences each locate exactly
once in France, starting at `0x210df0`. Every opcode, register, arithmetic
immediate, relative branch and zero alignment gap agrees. Only actual direct
transfer operands and independently validated LUI-derived addresses vary.
No compiled source object participates in this identity proof.

Thirty-one bodies pass the unchanged strict CFG checker directly. Three require
narrow animation-specific checks, repeated against all three debug originals:

- `xAnimPlaySetup` (212 bytes) and `xAnimTableAddTransition` (8 bytes) end in
  direct tails to the proven same-TU entries `xAnimPlaySetState` and
  `_xAnimTableAddTransition`. Both are completely covered frame-free leaves,
  have valid delay slots, contain no calls or return-address writes, and have
  no other outgoing local edges. Their callees independently have closed bounds.
- `xAnimTableNewTransition` (824 bytes) has three unreachable nonzero words at
  offsets `0x20c`, `0x250`, and `0x2bc`. Each is an exact copy of an executed
  conditional delay-slot instruction. A preceding unconditional branch skips
  it, while the matching conditional targets the following instruction. Only
  the observed SRL/LUI forms qualify. All other frame, return and local-edge
  checks pass; every original byte, including these dead copies, is retained.

The shared CFG implementation and its strict defaults are unchanged. Its legacy
`unreachable_zero_words` field lists all uncovered words when strict checking
fails; the animation exception separately verifies the three nonzero entries
and records them in `duplicated_dead_instructions`.

Twenty-nine animation entries have rooted direct JAL witnesses. The other five
use the independently unique complete-TU sequence and the checked callback/tail
relationships; they are not described as rooted direct-call entries.

## Address and neighbor evidence

All 37 distinct direct-callee relationships form consistent one-to-one maps.
Internal destinations must be corresponding complete same-TU entries. Named
external neighbors have their complete original bodies compared structurally.
Seven unnamed runtime neighbors have only their first 32 bytes corroborated:
this is **prefix-only evidence**, not a recovered function boundary or identity.
Neither kind of neighbor check independently promotes relocation anchors.

The seven distinct changed data references are the complete `xAnimPoolCB`
callback entry, four identical NUL-terminated literals, the DWARF-named
`sxAnimTempTranPool` zero-fill address, and `_impure_ptr`. Initialized-data and
zero-fill classifications come from the authenticated original layout. The
runtime pointer's stored destination remains at offset `-0x2f0` from its global
in every original and lies in initialized data. No runtime-object name, size or
invented data extent is inferred from that relationship.

## Actual compilation and reporting

The France profile compiles the same full TU as the debug profiles. It compares
all 34 original bodies and restores 54 calls only where independent French
callee identities are unique. Ambiguous overloads, unproved external calls and
GP data references stay unrestored. Standard `functionRelocDiffs=none` reporting
is unchanged; the matched-byte count is not a claim of full raw relocation or
retail-link equality.

Eight animation extents were already known and are deduplicated. The French
known-function inventory increases from 380 / 103,168 bytes to 406 / 118,352
bytes. The combined original-only TU registry now contains 83 records / 30,784
bytes, including those eight overlaps with older independently recovered bounds.

Actual `platform_progress.py report --version SLES-53623` compilation and stock
objdiff generation pass. Against published `020350a13`, only animation and the
unresolved CPU remainder change. Every other unit report is unchanged, including
streaming and particle commands. Full code remains 2,979,968 bytes, data remains
2,384,384 bytes, and completion remains zero. All prior 49 TU proof records are
unchanged. There are no game-source, compiler-flag or report-math changes, and
this patch leaves all three effective debug profiles unchanged.

Reproduce original-only checks with:

```sh
python tools/platforms/france_tu_sequences.py --orig-dir orig --check
```
