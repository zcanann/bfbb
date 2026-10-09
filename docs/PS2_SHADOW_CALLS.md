# PS2 shadow call boundaries

The original shadow initializer calls `SetupShadow` through the inlined camera
creation wrapper, and the eight-byte list wrapper tail-calls
`xShadowManager_Add`. PS2-only `dont_inline` scopes preserve those complete
callee bodies instead of expanding them into the callers. The initializer
also clears `shadow_ent_count` before calling `ShadowMapCreatePipelines`, as
shown by the original call's delay slot.

The 96-byte initializer and eight-byte list wrapper become exact in all three
debug releases: **104 bytes and two functions** each. Units rise from 18/34
functions and 4,760 bytes to 20/34 and 4,864 bytes. Every other function score
is unchanged. The full GameCube USA report remains identical and its retail
DOL SHA-1 passes.

France has no enabled source profile for this unit, so no France gain is
claimed. Compiler flags, profiles, target boundaries and registries remain
unchanged. Private evidence is `build/shadow-oct09/*-{before,calls}.json` and
separate instruction diffs for the original caller bodies.
