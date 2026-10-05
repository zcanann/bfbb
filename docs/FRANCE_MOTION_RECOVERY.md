# France entity-motion recovery

France now compares the complete `xEntMotion.cpp` TU: 20 functions / 14,616
bytes. The same whole source used by the debug builds matches five functions /
956 bytes, with 81.82923% fuzzy matching. All holdouts remain in the report.
France rises from 14,968 / 95 to **15,924 matched bytes / 100 matched functions**.
This is standard code matching, not a complete-TU or retail-link claim.

## Independent original identity

Three original debug builds identify the same ordered 20 functions and canonical
linkages in DWARF. Each complete original sequence locates uniquely at French
`0x1d73c0`. Every non-address instruction bit and zero alignment gap agrees.
Only actual direct-transfer operands and explicitly proven address operands vary.
No source object or proposed source match establishes an original identity.

Eighteen functions pass the existing strict CFG checker without dispatch help.
The two debug functions use four ordinary indexed jump tables:

| Function | JR offset | Entries |
| --- | ---: | ---: |
| xEntMotionDebugDraw | 0x48 | 6 |
| xEntMotionDebugWrite | 0x3c | 6 |
| xEntMotionDebugWrite | 0x17c | 6 |
| xEntMotionDebugWrite | 0x59c | 8 |

For each table, the verifier traces the actual unsigned SLTIU range test and BEQ
out-of-range path, unchanged index register, SLL by two, LUI/ADDIU table address,
ADDU index/base combination, LW target and JR with NOP delay slot. It rejects
incoming direct or table edges that bypass these checks. All 26 original table
entries must be aligned inside the same independently bounded function, at the
same relative destinations in all four originals.

Only these proven successor sets are supplied to the local CFG walker. Every
path then passes its existing frame-balance, return, delay-slot, call-save and
padding checks. Default callers still stop at unresolved indirect jumps. Five
motion entries have rooted direct-call witnesses; the remainder use the unique
whole-TU sequence and checked internal/callback relationships.

## Original address ownership

Five string-selection diamonds in `xEntMotionDebugWrite` need control-flow-aware
address provenance, at offsets `0xdc`, `0x4b8`, `0x530`, `0x6b0`, and `0x85c`.
Each conditional executes a LUI in its delay slot. Its taken path reaches the
final ADDIU directly. The other path repeats the identical LUI and branches
past that ADDIU while executing its own alternate ADDIU in the delay slot.
The verifier checks this exact instruction shape, both reaching definitions,
branch destinations and absence of other direct/table entries into the window.
It does not treat the linear preceding ADDIU as a producer on the taken path.
All other operands retain the existing strict address-lifetime checker.

The 89 distinct changed address relationships are completely accounted for:
84 identical NUL-terminated strings in initialized data, four original dispatch
tables, and the complete same-TU `xEntMotionDebugCB` callback entry. No guessed
storage layout or data-symbol extent is introduced.

All 38 distinct direct-callee relationships form consistent bijections across
references. Internal transfers reach corresponding complete motion entries.
External neighbors provide at most 32 original bytes of entry-prefix evidence
(shorter named bodies use their complete actual length). These are explicitly
**prefix-only corroborations**, not additional recovered boundaries or promoted
named relocation anchors. Named reference identities agree across all three
originals; unnamed runtime neighbors remain unnamed.

## Actual source comparison

The France-only profile retains all 20 original targets and compiles the same
complete shared TU. Eighty-seven calls use unambiguous independently established
French targets; unproved calls and GP/data identities stay unrestored. Normal
`functionRelocDiffs=none` scoring and report aggregation are unchanged.

The previously known 744-byte initialization function is deduplicated. France's
known inventory rises from 406 functions / 118,352 bytes to 425 / 132,224 bytes.
The TU proof registry now contains 103 records / 45,400 bytes; its prior 83
records remain identical. Every existing source profile is unchanged.

Actual production compilation and objdiff reporting pass. Only motion and the
unresolved CPU remainder change relative to the prior French report. Total code
remains 2,979,968 bytes, data 2,384,384 bytes, and completion zero. Game sources,
compiler flags, scoring settings and previously reported unit results are
unchanged by this recovery.

```sh
python tools/platforms/france_tu_sequences.py --orig-dir orig --check
```
