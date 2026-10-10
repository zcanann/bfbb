# PS2 laser-bolt source comparison

Marking the existing `reset_fx` and texture-setting overloads inline
restores helper bodies embedded in the original callers. In all three
debug regions:

| Function | Bytes | Before | After |
| --- | ---: | ---: | ---: |
| `attach_effects` | 128 | 8.59375% | 100% |
| `set_texture(const char*)` | 80 | 64.5% | 100% |
| `reset` | 676 | 80.73965% | 98.30177% |

The attachment path contains the original effect-rate reciprocal loop.
The name-based texture setter contains the hash lookup, asset lookup,
null test, and raster assignment rather than additional calls through
its overloads. This adds 208 exact bytes and two functions per region.
All 14 profiled functions were checked with normal `ps2solo.py` builds;
no other scores change. The complete GameCube report remains identical,
its laser-bolt unit remains 50/50 exact, and the rebuilt DOL passes its
SHA-1 check. France has no laser-bolt profile in this source baseline.
Private comparisons are in `build/laser-oct09/helpers-proof.json`.

The larger decal and rendering mismatches remain under investigation.
The original decal callers embed the body of `xMat3x3LookVec3`, but retain
calls to several nested vector helpers. Merely making the current body
visible expands too much and regresses the comparison. Inline-depth
experiments also affect other functions under deferred compilation;
those experiments and shared-header changes are not part of this fix.
