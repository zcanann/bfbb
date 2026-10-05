# PS2 entity source comparison

The complete existing xEnt.cpp now compiles for all three debug-bearing PS2
versions. Every original-owned function is compared: 48 functions / 19,036
bytes, 74.32444% fuzzy, with 23 exact functions / 3,580 bytes. The other 25
functions remain partial; no game function body was rewritten.

Missing platform declarations were the compile blockers. The PS2 headers now
provide the complete original RpMaterial (28 bytes), RwSurfaceProperties (12)
and iLight (60) declarations. All three original debug executables independently
confirm every direct field offset and aggregate size. Existing source SDK
rpworld.h and rwplcore.h corroborate the material and matrix API declarations.
RxObjSpace3DVertex is pointer-only here and stays opaque.

Five iModel declarations and iDrawSetFBMSK use the original p2 translation-unit
linkage names and return types. The SDK RwMatrixUpdate/Invert signatures are
reused unchanged. No platform implementation, runtime stub or invented padding
is added, and GC/Xbox include paths remain unchanged.

Seventeen concrete consumer layouts in the actual compiler debug object match
all three original xEnt debug declarations, including entity/frame/collision,
model, surface, material/geometry/atomic and animation-collision state. Unused
original globals types are retained separately and are not claimed as emitted
compiler evidence. Debug and normal objects have identical ordered allocated
sections, including repeated weak-section names.

All 23 exact functions independently reconstruct the actual original bytes
through named call and data relocations in each region. Unknown references in
partial functions remain explicit; the initial profile records 21 such operands.
No score policy changes or whole-TU completion/relink claims are made.

Local evidence is build/entity188: original-layouts.json, api-records.json,
and xEnt/{all-region-summary,compiled-layouts,raw-proof}.json. The standard
platform CI regenerates the comparison with the pinned compiler and stock objdiff.
