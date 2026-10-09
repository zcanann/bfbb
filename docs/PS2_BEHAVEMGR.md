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

## Transition call and goal-list access recovery

A PS2-only `dont_inline` scope preserves the original call to `GoalPopToBase`
from `ParseTranRequest`. Reading the public list link directly in `FindGoal`
restores the original inlined accessor load. Both changes are scoped to PS2:
the shared direct-field spelling was measured and regressed the existing
GameCube `FindGoal` match, so GameCube retains its accessor call.

Complete source objects in USA, PAL and Germany add two exact functions:
`ParseTranRequest` (300 bytes) and `FindGoal` (164). Exact code rises from
2,280 / 18 functions to 2,744 / 20 functions out of 6,196 bytes / 31 functions.
All other function scores are unchanged. Raw original-byte comparisons pass
for all 464 bytes in each region after applying the ten real JAL relocations.
Every GU4Y78 GameCube score remains unchanged, including 59 exact functions
and 7,280 bytes. France currently profiles only `xBehaveMgr_SceneReset`, whose
83.54839% source score is unchanged; no new French coverage is claimed.

Private PS2-worktree artifacts include `build/behave-raw-proof.json`,
`build/behaveproof.py`, `build/behave-gc-{before,after}.json`, retained whole
source objects, and the limited fresh French diagnostic target. Common
`ForceTran` and loop-source probes did not improve the remaining differences
and were restored. No compiler, profile, relocation or scoring settings change.
These code results do not establish a retail executable link.

## Scene-reset list traversal

The same PS2 direct-link access in `Amnesia` restores the original traversal
in the inlined scene reset. `xBehaveMgr_SceneReset` becomes exact at 248 bytes
in all four PS2 versions. Debug totals rise to 2,992 bytes / 21 functions;
France's currently profiled single function becomes 248 / 248 bytes exact.
All other function scores remain unchanged. GameCube retains its accessor
spelling and every GU4Y78 score, including 59 exact functions / 7,280 bytes.

Raw comparisons reproduce every scene-reset byte in all three debug originals
after applying the single genuine `g_behavmgr` GP relocation. The French
profile still leaves this global operand unresolved, so its ordinary code
score is not a complete relocation or link result. Private evidence is
`build/behave-scene-raw-proof.json`, `build/behave-scene-proof.py`, and
`build/behave-scene-gc-{before,after}.json`, with retained whole source objects.

## Timer-stack loop shape

The five timer entries in `FreshWipe` use a PS2 do/while loop, retaining the
original loop instead of expanding it into five stores. Its initialized index
is zero and its fixed bound is five, so the first iteration and all five writes
are guaranteed in both forms. The pointer-stack clearing loop remains unchanged.

The 108-byte function improves from 58.88889% to 96.296295% in all three debug
versions, with only one extra scheduled NOP remaining. Complete-unit fuzzy
matching rises from 95.06714% to 95.71918%; exact code remains 2,992 bytes / 21
functions. France's existing exact scene-reset comparison remains unchanged.
The shared loop spelling was measured and regressed GameCube, so the guarded
change preserves all GU4Y78 scores, including 59 exact functions / 7,280 bytes.

Whole-unit reports and retained source-object paths are recorded in
`build/behave-fresh-full-comparison.json`, with a replay script and
`build/behave-fresh-gc-{before,after}.json`. Earlier/later compiler and inlining
probes did not reproduce the complete original function without regressions;
production compiler settings remain unchanged. No exact-function or link claim
is added by this fuzzy improvement.
