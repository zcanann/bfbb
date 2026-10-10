# PS2 BSP line/triangle intersection

`LeafNodeLinePolyIntersect` in `xClumpColl.cpp` now restores the PS2 VU
intersection kernel. The 920-byte original improves from 3.1086957% to
78.30869% in USA, Europe, Germany, and France. This is a fuzzy source gain;
exact byte/function counts do not increase.

The original packs each 12-byte vertex with scalar loads, `pextlw`, and
`pcpyld`, then transfers it to the VU. It loads the aligned ray start/delta
separately. The VU computes the two edges, the ray/edge cross product, and
the determinant. A negative determinant reverses the edges and repeats the
cross/dot calculation. Scalar rejection tests use the original negative
epsilon and determinant-scaled tolerance. Subsequent VU cross/dot products
produce the barycentric coordinates and segment distance.

The new PS2 block retains that division between vector arithmetic and scalar
control flow. The GameCube/Xbox scalar path remains under the other preprocessor
branch. Scalar GPR/FPR operands are compiler allocated; the VU registers are
the registers used by the original kernel. No compiler patch, shared header,
runtime alias, or target-profile change is involved.

## Raw-word checks

The USA kernel occupies function offsets `0x60..0x1ec` (function entry
`0x302860`). That complete 396-byte original window is identical across all
four PS2 regions, SHA-1 `e3acbb7000f433ffcdda5582d72d62adc7f1564c`.

Raw source/target inspection confirms all 28 VU arithmetic words in order,
including every lane mask: cross/vector operations use `xyz`, while the
horizontal dot-product reductions use `x`. The eight coprocessor transfers
preserve the original VU registers and non-interlocked bits, with only scalar
GPR allocation differing. Both quadword loads preserve their VU destinations
and offsets, with only their base GPRs differing. Objdiff's formatted display
does not show these lane masks, so formatted similarity alone was not used.

The remaining differences include scalar register allocation, scalar/assembly
scheduling and hazard slots, the result check, and the loop tail. A helper-return
pilot, direct scalar-literal setup, and merged rejection tests were also tried.
Merged tests produced inverted comparisons; the retained separate tests preserve
the original rejection directions. These residuals are not attributed to a
compiler-version defect.

## Validation

Normal `tools/ps2solo.py xClumpColl.cpp --version <version> --keep` compilation
of the complete unit preserves the other ten profiled functions in each of
`SLUS-20680`, `SLES-51968`, `SLES-51970`, and `SLES-53623`. The existing French
profile supplies independently recovered targets; no new French identity is
claimed here.

`tools/solo.py Core/x/xClumpColl --top 3` retains all nine GameCube functions
exact. Whole-build GameCube validation is performed by the integration gate.
Private before/after reports and raw-word checks are in `build/clump-oct09`,
particularly `after-proof.json`, `raw-proof.json`, and `rawproof.py`.

## Sphere broad phase

The neighboring 512-byte `LeafNodeSpherePolyIntersect` also lacked the PS2
hardware minimum/maximum operations. Replacing comparison-and-select macros
with a bounded `SphereMinMax` inline helper, and retaining the loaded vertex,
centre, and radius values across each axis test, improves this function from
0% to 65.359375% in all four PS2 versions. It does not change the detailed
`FastIntersectSphereTriangle` test or its call boundary.

For each axis, the original computes `min(a,b)`, `max(a,b)`, `min(lo,c)`, and
`max(hi,c)`, rejecting when `centre + radius <= lo` or
`hi <= centre - radius`. It then records the vertex coordinates relative to
the sphere centre. Raw-word inspection verifies all twelve hardware min/max
instructions and their operand dependencies in the source and each original.
The remaining register allocation, instruction scheduling, and temporary-store
ordering differences are retained as source work, not a compiler diagnosis.

The normal four-region unit checks preserve the other ten functions, including
the restored line kernel. The GameCube unit remains 9/9 exact. This is another fuzzy gain with no change to exact byte/function counts.
Private evidence is `sphere-proof.json`, `sphere-raw-proof.json`, and
`sphererawproof.py` under `build/clump-oct09`.

## Triangle-list advancement

All three original leaf callbacks advance the triangle pointer before loading
the preceding triangle's continuation flag. The PS2 condition now spells this
as `(++triangles)[-1].flags`, preserving the same flag and advancement on every
iteration, including early `continue` paths. Other platforms retain their
existing postincrement spelling.

This source change makes `LeafNodeBoxPolyIntersect` (560 bytes) report 100%,
and lifts line/sphere to 79.40435% and 66.27344%, respectively, in all four PS2
regions. It adds 560 code-matched bytes and one function per region, with all
other unit functions unchanged. The box callback still has an existing unresolved
SDK call identity (`RtIntersectionBBoxTriangle` in the source); report equality
does not authenticate that runtime name or claim complete linked raw bytes.
The GameCube unit stays 9/9 exact. Regional before/after
evidence is `build/clump-oct09/loops-proof.json`.
