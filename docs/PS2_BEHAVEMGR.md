# PS2 behavior-manager comparison

The existing complete xBehaveMgr.cpp needs direct xMemMgr.h and string.h includes
for gActiveHeap, xMemAlloc, and memset. Earlier PS2 header separation removed the
incidental scene/entity include chain that supplied those declarations. Adding
the actual API owners compiles the unchanged function bodies without inventing
layout shims, implementations, or compiler flags.

The profile retains all 31 original-owned functions (6196 bytes) in each of the
three debug releases. Normal objdiff reports agree at nine code-matched functions
and 712 bytes: TimerGet 52, ParseTranRequest 300, GIDOfPending 32, GIDOfActive 44,
GIDInStack 80, GetCurGoal 40, IndexInStack 80, GetSelf 8, and Shutdown 76. Other
members remain visible and unmatched; no complete-unit or retail-link claim is
made. These are the standard code-match measures, not a separate strict relocation
metric.

Call destinations and GP-relative data anchors come from the authenticated
original DWARF. Startup reuses the previously reviewed runtime memset identity;
it does not infer a new runtime name from source output. The duplicate static
g_modinit name uses the existing source-owner filter to select xBehaveMgr.cpp's
original data anchor. Original behavior-manager and psyche layouts were checked
in all three debug releases. The one currently confirmed French member,
SceneReset, remains outside this debug-only source profile.

All 224 actual GameCube game/engine objects recompiled with identical allocated
sections. Private evidence in build/ps2core162 includes the unchanged-body source
object, per-region target objects/profiles/reports, original layouts, and the
GameCube comparison. No function identity or extent is added by this work.
