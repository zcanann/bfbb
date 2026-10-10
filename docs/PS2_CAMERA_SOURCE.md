# PS2 camera source recovery

The complete existing iCamera translation unit now matches all 14 functions and
3984 bytes in SLUS-20680, SLES-51968 and SLES-51970. The previous unit matched
9 functions / 1580 bytes at 97.400604%; this adds five functions and 2404 bytes.
Every previously matched function and data record remains unchanged. France has
no camera source profile yet; no French identity or coverage is claimed here.

The changes restore five original source details:

* Begin calls iDrawSetFBA1(1) after RwCameraBeginUpdate. All three originals have
  that call at byte 92 and the argument assignment at 96. The callee has original
  canonical linkage iDrawSetFBA1__Fi and a signed-int parameter; the declaration
  already used by xShadow is repeated locally for this PS2-only call.
* FrustumPlanes reads its final four planes from the PS2 camera extension,
  rather than repeating the ordinary camera planes. Original DWARF names the
  signed integer skyCameraExt at USA 0x50fd2c, Europe 0x50f82c and Germany 0x50f22c.
  Original instructions independently load it, add it to the camera pointer,
  and copy all sixteen float components from offsets 96 + index * 20, with indices
  2, 4, 5, 3. The RwFrustumPlane 20-byte stride and RwPlane normal/distance offsets
  0/12 are independently recorded in each original. A bounded byte-address
  accessor expresses these actual operations; no speculative SDK extension
  structure or API is introduced. GameCube retains the ordinary planes.
* GetViewMatrix retains the derived matrix pointer before clearing the output.
* UpdateFog retains separate source and destination fog-parameter pointers.
  Declaring the source pointers before the destination, then initializing the
  destination first, recovers their original register lifetimes and ordering.
* SetNearFarClip assigns each conditional result directly to its global. The
  <= 0.0f test is preserved, including the existing handling of NaN inputs.

Independent source-relocation replay against each debug original proves every
byte of UpdateFog 1672, FrustumPlanes 396 and SetNearFarClip 64. ViewMatrix agrees
on 152/156 bytes; its unnamed memset-like runtime destination remains unresolved.
Begin agrees on 100/116 bytes; four SDK call operands remain unnamed. Its game
call and globals are replayed only from original named DWARF evidence. Thus the
five improved bodies have 2384/2404 independently equal raw bytes, and complete
ordinary comparison matching. This is not a complete executable-link claim.

Full actual GameCube USA, Europe and Germany compiles preserve every allocated
section byte-for-byte, including all 15 functions / 3048 bytes. The current Xbox
production profile does not include this unit. No compiler binary, profile,
comparison setting, shared header or original metadata changed.

Private evidence in the PS2 agent checkout:

* build/camera-final-comparison.json and camera-final-changes.json
* build/camera-iCameraUpdateFog-raw-proof.json
* build/camera-iCameraFrustumPlanes-raw-proof.json
* build/camera-iCamGetViewMatrix-raw-proof.json
* build/camera-iCameraBegin-raw-proof.json
* build/camera-iCameraSetNearFarClip-raw-proof.json
* build/camera-extension-original-proof.json and camera-layout-call-proof.json
* build/camera-gc-allocations.json and camera-gc-REGION-before/after.json

Discarded clip experiments included explicit if/else global stores and double
literals. The retained conditional assignments recover the original body without
compiler scheduling changes or artificial instructions.
