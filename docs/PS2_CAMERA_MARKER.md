# PS2 camera-marker source comparison

The complete genuine `zCamMarker.cpp` source now compiles without importing the camera's runtime layout. The existing `xCamAsset` definition, transition enum, and five asset variants move unchanged into `xCamAsset.h`; `xCamera.h` includes that shared definition. Two existing public functions, `zCameraSetConvers` and `zCameraDoTrans`, move into `zCameraControl.h`. The marker selects those narrower dependencies on PS2 while GameCube retains its original include path. No function body is changed.

All three debug-bearing PS2 versions compare four of four functions, 220 of 220 code bytes, at 100% under normal objdiff. Each real source R_MIPS_26 and callback HI16/LO16 relocation also reconstructs the entire original function body byte-for-byte using independently named original DWARF destinations. The callback's original adjacent pair at offsets 48/52 is recorded in the proposed unit profile. No unknown call identity is needed.

The actual compiler's layouts for `zCamMarker`, `xCamAsset`, and all five variant asset structs agree with all three original builds, including the 136-byte camera asset. A private debug compile preserves the normal object's allocated sections. All 224 GameCube source objects compile with unchanged allocated sections.

The complete unit remains in the progress comparison and is not marked source-linked or complete: this work verifies source objects, not a full reconstructed PS2 executable. No France identities are inferred from the new compilation.

Private reproducible evidence is under `build/ps2marker168`: the compiler command, original-derived profiles and regional reports, `raw_proof.py`/`raw-proof.json`, `type_proof.py`/`type-proof.json`, and `gc/inventory.json`.
