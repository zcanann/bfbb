# PS2 conditional game logic

The complete shared `zConditional.cpp` now compiles against the existing scene lookup API. `zSceneLookup.h` factors the three existing lookup/name declarations out of `zScene.h`; its opaque xBase declaration introduces no scene or entity layout. GameCube retains its original full scene include.

The original PS2 unit owns five functions / 632 bytes. The typed initialization and reset helpers have no separate original-owned bodies and appear inside their callers. PS2-only inline declarations restore that boundary. The event callback also explicitly handles eEventFalse as a no-op, matching the original branch for event 0x3e. GameCube's case list and helper boundary stay unchanged.

Actual complete-TU compilation and standard objdiff compare all five functions at 100%. Original DWARF supplies the comparison identities and all direct-call destinations; no source function or target bytes are trimmed. This is code matching, not a complete linked PS2 executable claim. The normal GameCube source build and retail hash pass, its complete report is unchanged, and the conditional object's allocated sections remain identical.
