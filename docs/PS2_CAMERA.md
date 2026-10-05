# PS2 whole-camera source comparison

The complete `xCamera.cpp` now compiles with the published PS2 profile. The
platform header additions are standard float math declarations, opaque RenderWare
raster/texture names, and eleven public screen-effect APIs whose original linkage
signatures are independently present in all three debug executables. No renderer
storage layout is invented. `xCamera.h` directly forward-declares its `xEnt` pointer.

On PS2, the source uses the standard math/string headers and the already-restored
shared `xVec3Sub` inline. The GameCube float-to-double math wrappers are excluded
on PS2, leaving normal float runtime API calls. Their GameCube definitions and
all existing Xbox ownership guards remain intact. Declaring the standard math
APIs does not establish identities for unnamed original runtime callees.

All 33 original functions remain in the comparison:

| Version | Original code bytes | Code matched bytes | Fuzzy score |
| --- | ---: | ---: | ---: |
| SLUS-20680 | 15,376 | 2,188 | 73.42508% |
| SLES-51968 | 15,384 | 2,188 | 73.32917% |
| SLES-51970 | 15,384 | 2,188 | 73.32917% |

Each has the same 18 code-matched functions. `_xCameraUpdate` is 5,448 bytes in
USA and 5,456 in PAL/German, shifting several later direct-call instructions.
The shared-USA-offset trial correctly failed original instruction validation.
Separate executable-SHA-gated USA and PAL profiles now use independently decoded
call sites and original callee identities. No backend checks or comparison
settings were weakened. Remaining camera functions retain their honest partial
scores; no whole-TU completion or executable link is claimed.

Actual normal/debug whole-source builds have identical ordered allocated sections.
Their DWARF reproduces every direct member offset and complete size from all
three originals for `xCamera` (816), `xBinaryCamera` (112), `cameraFX` (76),
`cameraFXShake` (60), `cameraFXZoom` (32), and `xSweptSphere` (336).

Independent application of actual source relocations proves 15 functions /
1,880 bytes exactly in each original. The remaining normal code matches are
`xCameraFXBegin` (76) and `xCameraFXEnd` (68), with unresolved `memcpy` identities,
and `xCameraUpdate` (164), with an unresolved `ceilf` identity. These 308 bytes
are not claimed as raw linked-byte matches. France has no new profile until its
function identities are independently reviewed.

All 224 actual GameCube source objects preserve every allocated section's bytes
and size. The existing 51-unit PS2 production run retains 40,628 matched bytes.
Compared with the newer root CI report, every function record outside `xScene`
is identical; that sole expected difference comes from the private configuration
predating the already-published scene profile, not a source regression.

Private evidence is under `build/ps2camera182`: compiler inventory and commands,
all three authenticated API/type records, GameCube section comparison, and the
full old-profile production regression. `xCamera/` contains `profile-us.json`,
`profile-pal.json`, every actual regional report, original/compiled layouts,
`all-region-summary.json`, and `raw-proof.json`. `profile.json` is a USA-only
compatibility copy; integration must use both gated entries.
