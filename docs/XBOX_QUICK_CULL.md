# Xbox quick-cull source comparison

The complete `xQuickCull.cpp` API now compiles and links for Xbox with five real
math dependency TUs and the pinned runtime. The only source change extends the
existing PS2 `xCollis` forward declaration in `xBound.h` to Xbox. This header
uses collision pointers only; no structure layout, implementation, compiler
flag or GC/PS2 preprocessing path changes.

Instruction fingerprints locate the original quick-cull cluster, then decoded
callers, control flow, field accesses and algorithms independently establish
eight original identities and extents. All eight (1167 bytes) enter comparison,
including the partially matching bound and oriented-box dispatchers. The host
retains the complete public API, but four further source APIs without verified
original entries are not given guessed targets. Source function ordering alone
is not evidence: `0x14ff20` is the oriented-box routine, established by its call
to the already reviewed `xBoxInitBoundOBB` and subsequent corner conversion;
the following original function is unrelated.

| Exact function | Original entry | Bytes |
| --- | --- | ---: |
| xQuickCullInit(scalar bounds) | 0x14faa0 | 275 |
| xQuickCullIsects | 0x14fbc0 | 55 |
| xQuickCullCellForVec | 0x14fc80 | 167 |
| xQuickCullCellMerge | 0x14fd30 | 99 |
| xQuickCullForRay | 0x14fda0 | 240 |
| xQuickCullForSphere | 0x14fe90 | 138 |

Both normal Xbox reports increase **7953 -> 8927 matched bytes (+974)**.
The quick-cull unit is **90.91291% fuzzy**, with **6/8 exact functions**. All six
previously compared unit JSON records and normalized source bytes/relocations
remain identical. All eight new source comparisons reconstruct their actual PE;
the exact six also reconstruct both originals byte for byte.

Known inventory gains only one function / 275 bytes, reaching 2550 functions /
633591 bytes, because the other named extents replace existing anonymous ones.
The full 1798760-byte code denominator and data denominator are unchanged.

These six routines reproduce **974 original bytes**. Initialization stores the
six world bounds, dimensions, `127/dimension` scales and half-cell-adjusted
centers. Cell conversion uses the independently reviewed genuine `__ftol2`
callee three times, clamps coordinates to [-127,127], and duplicates signed
Z interval bytes. The merge/overlap routines establish the eight-byte quantized
layout directly. Original bound dispatch reads type at `+0x20`, sphere at
`+0x24`, box at `+0x30`, and matrix pointer at `+0x48`, confirming the existing
shared `xBound` declaration rather than inventing a platform structure.

Every entry has actual original direct-call witnesses and closed-CFG replay.
Only genuine PE HIGHLOW fields and named MAP calls are restored; all comparison
bodies invert to the actual linked source bytes. The exact six also invert to
both authenticated original bodies. The host and compiler runtime remain
excluded from source credit, and no complete-TU or retail-relink claim is made.

Reproduce using:

```sh
python tools/platforms/verify_xbox_reviewed.py --orig-dir /private/orig
python tools/platform_progress.py report --version XBOX-US \
  --orig-dir /private/orig --build-dir build/platforms \
  --xbox-compilers /xbox-compilers --objdiff /tools/objdiff-cli --wine /usr/bin/wine
```

Repeat for `XBOX-EU`. Actual local reports and independent reconstruction proof
are under `build/xbox182/final` and `build/xbox182/validation.json`. No original,
compiler or generated object binaries are committed.
