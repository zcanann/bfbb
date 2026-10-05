# France clump-collision and spline recovery

The original-only verifier now identifies both complete units: `xClumpColl.cpp`
(11 functions / 10,260 bytes) and `xSpline.cpp` (13 functions / 7,152 bytes).
Actual production compilation matches three clump functions / 1,280 bytes and
four spline functions / 1,432 bytes. That adds **2,712 bytes / seven functions**,
bringing France to **25,180 matched bytes / 146 functions**. Every holdout remains
in the whole-source comparison; these code scores do not establish a completed
TU or retail link.

## Original-only proof

All three debug originals independently supply matching ordered DWARF names,
canonical linkages and sizes. Each complete sequence maps uniquely to France:
clump collision at `0x3021c0`, splines at `0x20b080`. Their full sequences span
10,304 and 7,240 bytes including alignment, with unchanged search anchors of
6,572 and 1,600 bytes respectively. Every non-address instruction
bit is identical. All inter-function alignment gaps are shorter than 16 bytes
and zero in both originals. No compiled candidate supplies boundaries or names.

All 24 original functions pass the existing strict control-flow bounds checker;
no new tail, indirect-dispatch, instruction-effect or address-lifetime exception
is needed. Twelve entries additionally have rooted direct-call witnesses.
Internal calls must target corresponding named same-TU entries. External entry
prefixes corroborate placement but do not become new named relocation anchors.

Four clump-collision address operands point to complete same-TU callback bodies:
`LeafNodeBoxPolyIntersect`, `LeafNodeSpherePolyIntersect`,
`LeafNodeLinePolyIntersect` and `AddAtomicCB`. The original variable/function
pointer declarations independently identify these addresses.

Both units declare the same `ourGlobals[4096]` array in original DWARF. Its
zero-based bound and unsigned four-byte element type establish 16,384 bytes of
storage within authenticated startup-zeroed BSS. Only the actual observed slots
at offsets `0x134` and `0x138` are used as array-relative address evidence. The
units independently identify the same base in each original and France. This
does not infer a RenderWare struct layout or assign undocumented callback names.

The two spline coefficient tables, `sBasisBezier` and `sBasisHermite`, each have
nested original descriptors for a 4-by-4 array of four-byte floating-point values.
Each full 64-byte initialized payload is identical in all four originals.
Together these checks account for all five distinct clump addresses and all four
spline addresses, preserving the observed operand roles and one-to-one mapping.

## Validation scope

The six already known extents deduplicate. The French known inventory becomes
538 functions / 195,964 bytes. The original whole-sequence registry reaches
nine units / 246 functions / 127,096 bytes, retaining all prior 222 records.

Production reporting passes against the verified 551d59645 CI baseline. Only
these two units and the unresolved CPU range change; every other unit report is
unchanged. Full code/data denominators stay 2,979,968 / 2,384,384 bytes, with zero
completion. All previous profiles and effective debug-version profiles remain
unchanged.

The complete source profiles restore four clump calls and twenty spline calls
only when independently verified
original identities and inverse J/JAL reconstruction agree. No game source,
compiler flags, existing profiles or report math change. Unknown external
identities remain unresolved under the project's normal objdiff policy, and
complete-TU/source-link claims remain false.

```sh
python tools/platforms/france_tu_sequences.py --orig-dir orig --check
```
