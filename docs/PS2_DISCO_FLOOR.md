# PS2 disco-floor bounds

`z_disco_floor::refresh_bound` (1,948 bytes) improves from 65.172485% to
91.72279% in USA, Europe, and Germany when its two box operations are inlined.
The original function contains no calls. Previously the source called
`xBoxFromSphere` and `xBoxUnion`, forcing a return-address save and keeping
the reciprocal tile count in a saved floating-point register.

`zDiscoFloor.cpp` now opts into the existing sphere-to-box header body and a
new, PS2-only `XMATH3_BOX_UNION_INLINE` guard. The union body is the existing
six-component implementation from `xMath3.cpp`: maxima of the upper bounds
and minima of the lower bounds. Its inlined loads, comparisons, and stores
correspond to the original bounds loop. The default header branch retains the
existing declaration, and `xMath3.cpp` retains its out-of-line definition.

The normal compiled disco object emits neither box-helper symbol; both bodies
are completely inlined. Raw instruction inspection confirms `refresh_bound`
has no direct or indirect calls. Thus this visibility change does not introduce
another retained helper owner. No compiler flag, runtime alias, or target
profile changes are involved.

The remaining differences include a 16-byte frame-size discrepancy, temporary
placement, register allocation, and scheduling. Original DWARF lists the expected
centres, boxes, loop variables, and radii; it does not identify an additional
unused vector like the one found in the separate goo callback. Removing the
source's `mid_center` const qualifier did not improve the result. No synthetic
padding or compiler-version explanation is introduced.

## Validation

Normal `tools/ps2solo.py zDiscoFloor.cpp --version <version> --keep` builds
preserve the other 26 profiled functions in each of `SLUS-20680`, `SLES-51968`,
and `SLES-51970`. Exact byte/function totals do not change. France currently
has no disco-floor source profile, so no French match increase is claimed.

`tools/solo.py Game/zDiscoFloor --top 3` retains all 50 GameCube functions
exact. The opt-in is PS2-only. Because the change touches `xMath3.h`, fresh
whole-source regional builds remain part of the integration gate rather than
being inferred from the unit checks.

Private evidence is under `build/disco-oct09`, including regional before/after
reports, `after-proof.json`, `symbol-proof.txt`, and instruction diffs. The
sphere-only inline pilot reached 71.67762%; making both bodies visible reached
the retained 91.72279% result.
