# CodeWarrior source shapes

The compiler is `mwcceppc` 2.0p1a targeting Gekko. These are shapes that produce
different code from an equivalent-looking alternative. When instructions match
but registers or scheduling do not, the source form is usually the reason.

## Expression and control flow

| Shape | Effect |
|---|---|
| `x *= k` vs `x = x * k` | the compound form survives where the other gets folded |
| `a <= b` on floats | emits `cror eq,lt,eq` + `beq` |
| `!(a > b)` on floats | emits `ble`. Pick the one matching the target |
| small `switch` | becomes a **binary search**, not a jump table |
| sequential `cmplwi` on one CSE'd load | the source was an **if/else-if chain**, not a switch — worth ~6% on a recent function |
| `if (a < b) {X} else {Y}` vs `if (a >= b) {Y} else {X}` | different branch emitted. Swapping arms has been worth 9–12 points |
| `arr[i].member` vs `ptr->member` | different register allocation |
| `S32& n = arr[i];` vs `S32 n = arr[i];` | binding as a reference changes register pressure; often the difference between ~97% and 100% |
| `T* last = &begin[N];` vs `T* last = arr + N;` | `arr + N` re-derives the end from `this`; `&begin[N]` derives it from the begin pointer, which is what the target usually does |
| `bool` vs `S32` return | `clrlwi. r0,r3,24` at the call site means `bool`; `cmpwi r3,0` means `S32`. The mangled name does not encode return type, so the call site is your only evidence |

## Copies and inlining

- **Copy-initialisation** (`T v = expr;`) inlines a struct copy. **Copy-assignment** (`v = expr;`) routes through an out-of-line `__as__`. These are not interchangeable.
- CW inlines a same-TU callee **only if it is defined earlier in the file**. If the target inlines something you call, move that definition above the caller.
- CW in this build will **not** inline a function that returns a struct by value. Proven with a local static defined immediately above its caller; it still emitted a `bl`. Do not fight it.
- `INLINE` is never defined in this build, so MSL `cmath` only *declares* the `std::` functions; each TU defines its own under `#ifndef INLINE`.

## Layout

- **Local declaration order controls stack slot assignment.** Take the order from `dwarf/`. Reordering three locals to the original order took one function from 99.016% to 100%. It is not a general fix though — see the bfbb-recovering-source skill for when it works and when it does not.
- Local declaration order can also assign stack slots **descending**: on one function four `xVec3` locals landed at 0x30/0x24/0x18/0x0c in declaration order. If your stack offsets are permuted, permute the declarations.
- A raster or similar value chosen by a condition usually needs to be a **hoisted local with an explicit `if/else`**, not a ternary written inline in the call.
- Vertex or struct field stores sometimes must go through explicit scalar temps loaded in a specific order; a direct member-to-member store interleaves load/store differently, and a whole-struct copy routes through an out-of-line `__as__`.

## Literal pool

Anonymous literals (`@1234`) are numbered by a **file-global counter**, but numbering is cosmetic — objdiff compares relocations by **target offset**. What matters is `.sdata2` *layout*.

- Filling in a function interns new literals and shifts every later offset, which can knock neighbours off 100%.
- Conversely, writing a large function at the target's position can seed the pool correctly and lift unrelated neighbours to exact for free.
- Constant folding can silently prevent a literal from being created at all. `2.0f * (PI * i) / 3.0f` with a locally-assigned `i = 0` folds to `0.0f` and never interns `2.0f`/`PI`/`3.0f`, shifting the whole file's pool by 12 bytes. Assigning the index in *both* arms of a branch blocks the front-end fold while the value stays 0.
- Retail sometimes has **ghost literals** with no surviving reference — constants from code that was optimised away. You cannot reproduce those without fabricating dead code. Do not; report the ceiling instead.

**Diagnosing a pool permutation.** Disassemble both objects, list `.sdata2` in slot order, and for each slot record the *first function that references it* in emission order. Compare the two lists side by side. If the values are the same multiset in a different order, it is ordering, not content, and the first row where they disagree names the culprit.

The worked case is `zNPCGoalRobo`, and it is a ghost-literal ceiling, not a fixable defect. Slots 0–13 agree exactly. At slot 14 the target has `3.0f` and at 15 `1e-05f`, both interned *before* `FlankPlayer__19zNPCGoalAlertFodderFf` interns `6.0f` — yet their first use is `OrbitPlayer__20zNPCGoalAlertFodBzztFf` and `MoveChase__19zNPCGoalAlertHammerFf`, thirteen and twenty-eight functions later. Nothing between `CheckSpot` and `FlankPlayer` emits a load of either. Ours interns the same two values, correctly, at the point they are first used. The two-slot head start offsets every later slot, which is why ~100 functions in that unit sit at 99.x% each off by exactly one relocation — and why the unit reads well in `report.json` (99.x counts as matched) while being unable to reach complete coverage.

Function **definition order** is worth checking at the same time, since it drives interning order: extract `.fn` order from both objects and take the longest increasing subsequence. `zNPCGoalRobo` scores 320 of 361 common symbols with 10 descents, so its source order is also genuinely wrong in places — but fixing that cannot realign a pool whose head is already displaced by ghosts.

## Source shapes worth trying before you give up

Each of these was measured on a real function, and the gain is the measured one.

- **Brace-initialised aggregates emit a template, not stores.** `xVec2 v = { a, b };` with non-constant elements makes CW emit an 8-byte **`.sbss2`** template and copy it to the stack before storing the fields; `xVec3 v = { a, b, c };` does the same with a 12-byte **`.rodata`** template. Using `xVec2::create`/`xVec3::create` instead costs a `bl` and misplaces the template. Switching six functions in `zNPCTypeBossPlankton` to brace-init moved five of them from 47–64% to 99.3–99.7%.
- **`xabs()` and `std::fabsf()` are not interchangeable.** `xabs` inlines to `fabs`+`frsp`; `std::fabsf` emits `bl fabsf__3stdFf`. Retail used both, in different functions in the same file. MSL's `cmath` in this tree does not declare `fabsf`, so a file-local `namespace std { float fabsf(float); }` is the byte-neutral way to reach it.
- **Collapse early returns into one condition.** `if (A) { return X; } else if (B && C)` and `if (A || (B && C))` produce visibly different exit structure. `update_follow_player` went 49% → **100%** purely from that collapse.
- **A `switch` is not an if/else-if chain.** CW compiles `switch` over small integers as a binary search. `stun`'s dialogue selection went 84% → 96% on that change alone; `impart_velocity` dispatches on `flag.move` the same way.
- **…but a *dense* `switch` becomes a jump table, and the arm that decides it is often one you left out.** Binary search in your object where the target has a jump table means you are short an arm. Adding a `case 0:` arm took `zEntPlayerSurfDamageUpdate` from 78.0% → 85.4%, and adding an empty `case 6:` made `zNPCGoalBossPatSpin::Process` emit the target's 7-entry table `@2886` instead of a search. Count the target's arms before you write the `switch`. **The threshold is exactly 7**, measured directly: six arms compile to a `cmpwi` tree, seven to a `bctr` table, and seven arms with a hole in the range still table — so it is the arm *count*, not perfect density. See `gekko-asm.md`.
- **`switch` arms are emitted in source order, so the target's physical block order gives you the source order — which is often not enum order.** `AnimPick`'s blocks run IDLE, TAUNT, HUNT, DIZZY, HIT in the target; writing the cases in that order (plus dropping a pre-`switch` initialiser, making the loop counter unsigned, and writing the tail as `if (index > -1)`) took it 45.4% → 99.3%. Read the block order out of the disassembly before you write the `switch`.
- **A pointer walked with an explicit byte stride keeps an indexed load.** Typing an iterator `U8*` and advancing it with `it += stride` preserves the target's `lfsx`; computing the same address as a typed-pointer sum lets CW CSE it into a plain offset. Worth 87.7% → 100% on `find_active_node`.
- **Look for the file's own idioms before inventing one.** The `&begin[N]` fix above was already visible in a neighbouring function in the same file that happened to be matching. When several functions in one unit are all stuck in the same band, the author's habit is a better hypothesis than a per-function shape — that one substitution finished six functions at once.
- **Extra range checks inside a binary search are empty case arms.** When the target's search tests bounds your version does not, the missing arms are cases that fall straight through to `break`. `case ZENTTRIGGER_TYPE_4: case ZENTTRIGGER_TYPE_5: break;` plus DWARF's declaration order took `zEntPlayerCollTrigger` from 98.5% to **100%**.
- **`if (floatvar)` and `floatvar != 0.0f` compile differently.** The implicit-bool form emits `fcmpu (var, 0)`; the explicit comparison emits `fcmpu (0, var)` — the operands are swapped. Retail overwhelmingly used the implicit form. Worth 97.9% → 99.9% on `zEntPlayerVelUpdate` on its own, and it cleaned three more sites in `zEntPlayerJumpUpdate`.
- **Read the branch to recover the operator.** A plain `bgt` / `ble` / `blt` / `bge` in the target means the source used `<` or `>`, possibly negated — so `!(x > 0.0f)` is right and `x <= 0.0f` is not. `<=` and `>=` on floats *always* route through the `cror eq,lt,eq` pair. The branch mnemonic tells you which one the author wrote.
- **`cond ? x : default` and `(x == 0) ? default : x` are not the same shape.** Only the first reproduces the `beq`/`b` ternary structure.
- **`MAX(MAX(a,b),c)`** from `xMath.h` reproduces retail's double-evaluated max; a three-way hand-rolled comparison does not.
- **Splitting a fused expression into a named local blocks `fmadds`.** `apply_yaw` went 97.25% → **99.92%** from hoisting one multiply.
- **`&= 0xfffd` and `&= ~0x2` are different instructions.** The 16-bit literal gives `andi.`; the complement gives `rlwinm`.
- **`xfeq0(x)`** (`xMath.h`, `((x) >= -1e-5f) && ((x) <= 1e-5f)`) is the source of the `fcmpo` / `cror eq,gt,eq` / `bne` triple. If you see that triple, do not hand-roll the comparison — use the macro, and note the epsilon is `1e-5f`, not `1e-6f`.
- **A small constant-trip loop filling a local array is unrolled, but the array is not promoted — and writing the fills out by hand deletes the array entirely.** Six constant-index assignments (`pdx[0]=…; pdz[0]=…; …`) let CW scalarise `F32 pdx[3], pdz[3]` away completely: no stack slots, no stores. Writing the same fills as `for (i = 0; i < 3; i++) { pdx[i] = …; pdz[i] = …; }` emits the target's fully-unrolled straight line *and* keeps the dead stores and the two 12-byte slots. Worth **92.108% → 99.975%** on `nearestTrackCB`. **If a function is stuck in the low 90s with a frame that is short by exactly one or two array-sized chunks, this is the first thing to check.** The same defect is live in `xScene.cpp`'s `nearestFloorCB` (92.559%, frame short by 0x10, identical source shape).
- **`volatile` is evidence only when it produces a byte-exact function.** Retail's object reloads a global right after storing it, where this branch's compiler forwards the store; marking the variable `volatile` reproduces the reload and always raises the number. That does *not* make it retail's source. The same reload happens on pooled *literals* — `1.0f` in `xFXRingCreate`, `PI` and `180` in `zMainParseINIGlobals` — which cannot be `volatile` under any source, so the behaviour is compiler-wide (see the reload-after-aliasing-store lead in `docs/DUPLOTRON.md`). The line that has held up: if `volatile` takes a function to **100%**, retail almost certainly wrote it, and `zNPCTypeBossSandy`'s `sElbowDropTimer` is the worked example (`Exit__…ElbowDrop` 85.7 → 100). If it merely narrows a gap with nothing reaching exact, it is papering over the compiler — five such qualifiers were removed from `iSnd` for a cost of 9.5 fuzzy points across three functions and **zero** matched functions. Joey has rejected this shape before on `zNPCHazard::Discard`, where the `volatile` variant scored highest of three and emitted a load the target does not have.
- **`(j + 1) % 3` and `(j == 2) ? 0 : j + 1` are not the same code.** The modulo makes CW hoist the divide-by-3 magic multiplier into a callee-saved register, which enlarges the frame and moves the `stmw` base; the ternary compiles to a branchless equality select and does neither. Worth **78.986% → 82.300%** on `shadowCacheLeafCB`, and it fixed the frame size at the same time. Note this cuts the *opposite* way from `get_next_quadrant`, where the target wanted a real `%` — so read the target before choosing. A magic multiplier (`0x55555556` for 3, `0x66666667` for 10) in the target means `%`; its absence means the ternary. `gekko-asm.md` lists the constants.
- **A local static mangled `name$localstaticN$mangledfunc`, with WEAK scope, means the enclosing function was declared `inline`.** Every ordinary local static in a TU is `name$NNNN` and LOCAL. Retail's `offsetChuck$localstatic4$get_reticle_bound__FR5xVec3Rf` is WEAK in `.rodata` and `get_reticle_bound` is `scope:weak` in `.text`. Marking the function `inline` — not `static` — with the static `const` reproduces both the mangling form and the section. Note the counter is file-global: ours read `$localstatic3` where retail read `$localstatic4`, because retail emits one more inline-function local static earlier in that TU, so the relocation cannot converge until the earlier one exists.
- **`if (a == k) { X } else { Y }` and `if (a != k) { Y } else { X }` swap physical block order.** Worth 94.662% → 96.894% on `zEntPlayer_FindGrabEnt`. Same principle as the early-out bullet below, in its simplest form.
- **The stack layout rule, stated precisely: CW orders locals by *size descending*, ties broken by declaration order, at *descending* addresses.** Treat this as a strong default rather than a law — `SlideTrackUpdate`'s five large aggregates do **not** follow it, and its exact target frame came from permuting declaration order by hand (`xCollis` in the outer scope before `TrackPolyData`, then `qcd, isx, sph`). If size-descending does not reproduce the frame, permute. This supersedes the older "reverse the declaration list" heuristic, which is just the special case where every local is the same size. Confirmed on three functions in one session; it predicted `CollidePyramidBoxTop`'s frame exactly. Reversing the declaration list still fixed Sandy's 3852-byte `Process`, but only because its locals were uniform. If your `r1+offset`s are permuted, sort by size first.
- **A struct whose address is never taken is scalarised into FPRs, and its fields are numbered by first *write*, not by field order.** An `xVec3 quaddir` assigned `.z` before `.x` put `.z` in f31 and `.x` in f30. Where the target's registers disagree with your field order, replacing the struct with plain scalars declared in *write* order recovers it — worth 93.7% → 96.2% on `CollidePyramidBoxTop`, and it also explains a local that has no stack slot at all.
- **An early-out reorders the blocks physically.** `if (!A || !B) { …; continue; }` puts the else-work first in memory; `if (A && B) { … } else { … }` does not. If the target's else-block is physically first, write the early-out. Worth 96.6% → 99.8% on `PlayerCollsSelectDepen`.
- **A load that cannot be hoisted above an intervening call has a recoverable source position.** If the target loads something *after* a `bl` where you load it before, move the expression below the call in the source — the compiler was not free to reorder it, so its position is real information. `PlayerSwingUpdate` went 91.2% → 97.8% that way.
- **`cmplwi` vs `cmpwi` recovers a variable's signedness.** An unsigned compare against a file-scope flag means the flag is `U32`, not `S32`. Fix the declaration rather than casting at the comparison.
- **Splitting a fused expression also frees a register.** Beyond blocking `fmadds`/`fmsubs`, writing `s = s * rang; s -= 0.001f;` let `s` and `dang` coalesce into one callee-saved FPR. With two separate guard returns instead of one `if (A || B) return;`, that was 90.3% → 96.9% on `PlayerRotMatchUpdateEnt`.
- **`inline` in the `.cpp` reproduces a weak symbol without touching a header.** When the target has a member as `scope:weak` in its own `.text` section, retail declared it `inline`. Marking it `inline` at the definition in the `.cpp` gets the weak scope and the `.sbss2`/`.rodata` template ordering that comes with it. Moving the body into the class in the *header* also works but adds the template object to every including TU — which broke a `Matching` unit when tried.

## Things that are not your fault

- **Scheduling residue.** Identical instruction multiset in a different order, often just two epilogue instructions swapped. Not reachable from source.
- **Float scheduling.** Our build hoists a `lfs` of a constant above an intervening store where the target does not.
- **FPR/GPR colouring.** Same instructions, rotated register assignment, usually because one extra value stays live. Sometimes fixable by changing where a variable is initialised, often not.
