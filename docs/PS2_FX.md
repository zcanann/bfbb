# PS2 full xFX source comparison

The entire unchanged `xFX.cpp` function bodies now compile with the published
PS2 compiler profile. Its 72 original functions cover 30,512 bytes. All three
debug originals report 54.51167% fuzzy matching and 32 functions / 3,596 normal
matched code bytes. Every nonmatching original function remains represented;
France is not newly enabled and no whole-TU or executable link is claimed.

The source selects existing RenderWare material-effects and skin headers on PS2.
Their common SDK API declarations remain intact; their dependency includes select
the actual PS2 geometry foundation. The GameCube-specific material pipeline
interface is excluded from PS2; existing non-PS2 behavior stays unchanged. Skin only needs an opaque `RpHAnimHierarchy*` in its public
prototype, so no unsupported hierarchy layout is introduced. All GameCube source
includes retain their original order.

Additional headers restore established SDK callbacks/accessors for atomics,
materials, worlds, lights, textures and frames, cull/lighting enums, render-state
get, and standard `memmove`. The atomic render-callback setter retains the SDK's
null fallback to `AtomicDefaultRenderCallBack`. `iFXanimUVCreatePipe` and
`iModelNormalEval` have their original parameter signatures in all three debug
versions; NormalEval is `xVec3*, const RpAtomic&, const RwMatrixTag*, unsigned int,
int, const xVec3*`. These are declarations, not substitute implementations.

The newly consumed SDK string callback is independently checked in retail.
`MaterialSetEnvMap2` loads and calls a pointer at original `ourGlobals + 0x114`:
USA 0x5bba3c, Europe 0x5bb53c, Germany 0x5baf3c. This is precisely the complete
SDK `RwGlobals.stringFuncs` offset 240 plus `RwStringFunctions.vecStrcmp` offset
36. The proof checks the actual LUI/LW/JALR words and intervening register writes;
it does not invent an allocator/string prefix layout or infer one from a score.

Twenty-four consumed aggregate records, including SDK structures, the render
input buffer, rings, ribbon/streak/shine state, aura/firework state, and triangle
vertices, match original full sizes and direct member offsets in every debug
region. There are two distinct `tri_data` declarations in the included types;
the anonymous xFX declaration is selected by its sole `vert` member, distinct
from the shadow-cache type that also has a normal, before comparing layouts.
The actual ordinary and debug whole-source objects have identical allocated
sections. All 224 GC source objects were rebuilt with identical allocated bytes.
The complete frozen `2bd13b322` PS2 profile regression also preserves every one
of the 5,391 function records and every overall measure. After integrating
`5adf9e717`, the newly published complete `zNPCTypeCommon.cpp` was rebuilt too:
all 273 ordered allocated sections are identical to its saved production object,
preserving the independently verified regional results.

Independent application of real source relocations to original named addresses
proves 21 functions / 1,516 bytes exactly in each region. The other normal code
matches retain unresolved SDK/runtime/trigonometric callees or anonymous data.
They remain ordinary objdiff matches, not claims of raw linked-byte equality.

Private evidence is under `build/ps2fx204/`: original API signatures and callback
slot windows, actual full-source/debug compiler logs, original/compiler layout
records, regional reports, independent relocation proofs, the candidate profile,
and GC comparisons. No inline assembly, compiler change, ABI placeholder, or
function-body workaround is introduced.
