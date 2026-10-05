# PS2 Robo goal source comparison

The complete `zNPCGoalRobo.cpp` source now compiles and compares all 229 original
functions / 96,804 bytes in each debug PS2 version. All three match 77 functions /
9,112 bytes. Fuzzy matching is 64.161980% in USA and 64.161896% in Europe/Germany.
Every partial function remains included. Debug-version totals become 69,392
matched bytes / 557 functions.

## Source change

Six direct uses of the GC-specific `__fabs` intrinsic now use the established
`xabs` macro already used elsewhere in this same source. All operands are floats.
The existing PS2 `iabs` uses `__s_abs`; GC uses its existing `__fabs` implementation.
There is no compiler patch, new intrinsic shim, handwritten assembly or flag change.

Original `PlayerInSpot` and `ZoomMove` each contain two ABS.S instructions in all
three versions, as does each corresponding compiled function. The Tubelet helper
is inlined into original `Process`, which contains two ABS.S instructions. The
current compiler emits a separate `PeteAttackParSys` helper with one shared ABS.S
result. That caller remains an honest partial comparison; its different inlining
is not represented as an exact match.

The normal GameCube build passes the retail executable SHA1 gate, and its entire
objdiff report remains identical to the previously verified CI report.

## Original boundaries and layout

Each debug original independently supplies all named function boundaries and
canonical linkages. Actual compiler debug information agrees with 77 original
aggregate sizes and direct member offsets in every region, covering Robo goals,
base goal classes, robot/entity, model, message and player/global dependencies.
Normal and debug builds have identical ordered allocated sections.

Original-backed named calls and GP addresses use the existing inverse checks.
The 31 unidentified references in the initial profile inventory remain explicit;
no guessed callees or data addresses are introduced to improve the score.

## Raw-byte verification

Independent application of actual source relocations reconstructs 69 functions /
7,908 bytes exactly against each original. Eight further normal objdiff matches /
1,204 bytes retain unresolved anonymous strings, static data or class-static
laser references. These are recorded in the raw proof and are not claimed as
raw linked equality. The private inverse checker handles the actual LHU signed
displacement relocation encountered here using the same HI16/LO16 address rule.

The standard objdiff relocation policy and full-game denominators are unchanged.
No data-match, complete-TU or executable-link claim is made. France and Xbox
profiles remain unchanged by this addition.

Local evidence is in `build/robo198/`: actual compile inventory, GC gate/report
verification, original ABS.S locations, and `zNPCGoalRobo/` containing all regional
reports, original/compiled layouts, profile, unknown references and raw proof.
The large whole-source inventory compile uses its supported 600-second timeout.
