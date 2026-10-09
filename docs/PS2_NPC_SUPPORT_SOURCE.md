# PS2 NPC support vector lifetimes

Three PS2-only source changes improve the complete NPCSupport unit in all three
debug regions and the existing French selection. LineHitsBound now matches all
240 bytes exactly. Other platforms retain their existing expressions.

* LineHitsBound retains the origin X coordinate for both the displacement and
  ray-origin assignments. Direct component expressions avoid the extra address
  temporaries introduced by the inlined vector helpers. Reusing the retained
  coordinate reproduces its original live floating-point register and restores
  the exact load order, length register, stores and call sequence.
* aimVary computes its initial displacement directly in the caller. This
  removes three excess saved GPR/component-address lifetimes and substantially
  improves the original frame and register assignments.
* MakeArbPlane writes its cross-product components directly, with the original
  multiplication operand order for the Y component. This restores the original
  fused multiply/subtract instead of separate multiplies and subtraction.

The per-function results are identical in SLUS-20680, SLES-51968, SLES-51970 and
SLES-53623:

| Function | Original bytes | Before | After |
| --- | ---: | ---: | ---: |
| NPCC_LineHitsBound | 240 | 83.11667% | 100% |
| NPCC_aimVary | 1,248 | 91.926285% | 97.301285% |
| NPCC_MakeArbPlane | 196 | 95.10204% | 99.755104% |

Full 54-function debug-unit checks against verified `8831f2efa` preserve every
other function record. Exact coverage increases from 4,072 bytes / 39 functions
to 4,312 bytes / 40 functions, out of 12,552 bytes / 54 functions. Overall unit
similarity increases from 91.13257% to 92.06246% in all three regions.

The current sixteen-function French selection preserves its other thirteen
records, gains the same 240 exact bytes / one function, and improves from
94.83901% to 98.26792%. It now has 1,300 exact bytes / eleven functions out of
3,404 bytes / sixteen functions. No identities, bounds or relocations change.

All 73 function records and all data measures remain unchanged in each of
GQPE78, GQPP78 and GU4Y78: 7,716 exact code bytes / seventy exact functions,
5,024 exact data bytes, and 99.40031% code similarity. No compiler patch is
introduced, and the remaining source differences are not attributed to a
compiler-version mismatch.

Original DWARF declarations and instruction differences were inspected before
the source probes. Moving the origin copy earlier, using ray.dir as the
intermediate, replacing the cross product with a vector member call, and
several operand/copy variants regressed or failed to improve and were discarded.

Private evidence: `build/support-original-locals.txt`,
`build/support-source-region-summary.json`,
`build/support-source-france-summary.json`,
`build/support-source-gc-verify/<version>/report.json`, the
`build/support-source-probes*.txt` results, and saved candidate object files.
