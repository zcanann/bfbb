# Reading Gekko asm: what the target is telling you

Three references, three different questions:

| File | Answers |
|---|---|
| **this file** | "what does this asm *mean*, and what source produced it" |
| `codewarrior.md` | "what source shape should I *write* to move a number" |
| `measuring.md` | "which number means what, and how has this project fooled itself" |

Everything below was either **measured against our own compiler** (`GC/2.0p1a`,
the patched one, under the real cflags) or **read off a target object in this
repo**. Nothing here comes from the internet. CodeWarrior lore for other
consoles and other major versions is mostly wrong for us, and a claim you
cannot reproduce is worse than no claim — this project has twice built on a
confident premise that a control experiment then demolished.

**Reproduce or extend any entry with the recipe at the bottom.** If you add an
entry, add it the same way: probe, compile, read, then write.

## Reading a mangled name

Every row of a diff is labelled with one, and the name alone often settles a
question you were about to spend an hour on. All examples below are real
symbols from this repo.

Form is `name__` + scope + qualifiers + `F` + argument types, where `F` opens
the argument list and `v` means "no arguments".

| Fragment | Means | Real example |
|---|---|---|
| `__F` | free function | `xDrawLine__FPC5xVec3PC5xVec3` |
| `__8xtextboxF` | member of `xtextbox` (8 = name length) | `render__8xtextboxFb` |
| `__8xtextboxCF` | **`const`** member function — the `C` sits before the `F` | `yextent__8xtextboxCFb` |
| `P` / `PC` | pointer / pointer-to-const | `set_text__8xtextboxFPCc` is `(const char*)` |
| `R` / `RC` | reference / const reference | `get_grid_index__FRC5xGridff` is `(const xGrid&, F32, F32)` |
| `Pv` | `void*` | `nearestFloorCB__F…Pv` |
| `i` `f` `b` `c` | int, float, bool, char | `contract__13basic_rect<f>Ff` |
| `Ui` `Ul` `Us` `Uc` | unsigned int / long / short / char | `range_limit<Us>__FUsUsUs` |
| `10iColor_tag` | class **by value** (10 = name length) | `create__5xfontFUifff10iColor_tagRC13basic_rect<f>` |
| `<f>` `<i>` `<Us>` | template arguments | `xUtil_choose<i>__FPCiiPCf` |

**`Ul` vs `Ui` keeps costing this project time.** `u32` is `unsigned long`
(`Ul`); `U32` is `unsigned int` (`Ui`). They are the same width and produce
identical code, so nothing but the mangled name tells you which the original
used. Container index parameters are `u32`. Check the name before deciding a
body is wrong.

Compiler-generated names you will meet constantly:

| Symbol | Is |
|---|---|
| `__ct__4BaseFv` / `__dt__` | constructor / destructor |
| `__as__3BigFRC3Big` | implicit `operator=` — see `codewarrior.md` |
| `__nw__FUl` / `__dl__FPv` | `operator new` / `operator delete` |
| `__vt__4Base` | the vtable object |
| `__cvt_fp2unsigned` | runtime helper for `(unsigned)someFloat` |
| `cached$45`, `init$46` | function-scope static, and its init guard |
| `x$localstatic4$mangledfunc` | local static of an **`inline`** function |

The last two are different manglings for the same language feature and the
difference is diagnostic — see `codewarrior.md` on `$localstatic`. Both
counters are **file-global**, so their numbers cannot converge until every
earlier static in the TU exists.

## Loops: the immediate is not the trip count

Three source shapes, three distinct signatures. Measured on `sink()` bodies.

```
for (i = 0; i < N; i++)     li  r31, 0x0          <- counts UP, body first
                        L:  bl sink
                            addi r31, r31, 0x1
                            cmpwi r31, N
                            blt L

for (i = N; i > 0; i--)     li  r31, N            <- counts DOWN, body first
                        L:  bl sink                  note bgt
                            subic. r31, r31, 0x1
                            bgt L

i = N; while (--i)          li  r31, N            <- counts DOWN, TEST first
                            b   T                    note bne, and the `b`
                        L:  bl sink                  jumping over the body
                        T:  subic. r31, r31, 0x1
                            bne L
```

**The third shape runs the body `N - 1` times.** The `b` lands on the
decrement, so the first thing that happens is `N -> N-1`, and the body executes
for counter values `N-1 … 1`. The immediate is the source constant, *not* the
iteration count.

This is live in `zMainFirstScreen`, whose delay loop reads `li r28, 0xb4`
(180) and therefore spins **179** times, calling
`iTRCDisk::CheckDVDAndResetState` and `iVSync`. Writing `for (i = 0; i < 180;
i++)` there would produce the wrong shape *and* the wrong count.

`bgt` vs `bne` is the tell that separates shapes two and three. Read it before
you write the loop.

## Conversions

| Target asm | Source |
|---|---|
| `fctiwz f0,f1` / `stfd f0,0x8(r1)` / `lwz r3,0xc(r1)` | `(S32)someFloat` — note the load is at **+4**, the low word of the stored double |
| `bl __cvt_fp2unsigned` | `(U32)someFloat`. A **function call**, not inline. Seeing it means the destination is unsigned |
| `xoris r3,r3,0x8000` / `lis r0,0x4330` / `stw` / `lfd @N@sda21` / `stw` / `lfd` / `fsubs` | `(F32)someInt` — the standard magic-number int→float |

**The int→float sequence interns a `.sdata2` double** (`0x4330000080000000`).
That is a pool-seeding side effect: an integer-to-float cast anywhere in a
function puts a constant in the literal pool. If your pool has one slot too
few or too many against the target, count the int→float conversions before you
go looking for anything more exotic. See `codewarrior.md` on pool ordering.

**That constant is also a tooling trap.** `tools/tasm.py --pool` prints slot
values interpreted as floats, and `0x4330000080000000` read that way renders as
a plausible-looking **176**. An agent chasing `InvertRaster` spent time hunting
for where the source produced 176 before realising the slot is not a float at
all — it is the conversion magic, and our object interns exactly the same
value. **If a pool slot shows a suspiciously round number you cannot find in
the source, check whether it is an 8-byte double before assuming it is a
literal you failed to write.**

## Division and modulo: the constants name the divisor

CW turns division by a compile-time constant into a multiply-high. **The magic
number identifies the divisor**, so you never have to guess.

| Target asm | Source |
|---|---|
| `lis 0x6666` / `addi 0x6667` / `mulhw` / `srawi r0,r0,2` / `srwi r3,r0,31` / `add` | `v / 10` where `v` is **signed** |
| `lis 0xcccd` / `subi 0x3333` / `mulhwu` / `srwi r3,r0,3` | `v / 10` where `v` is **unsigned** |
| …either of the above, then `mulli r0,r0,0xa` / `subf r3,r0,r3` | `v % 10` — **the `mulli` literally spells the modulus** |
| `srawi r0,r3,3` / `addze r3,r0` | `v / 8`, signed |
| `srawi r0,r3,3` with **no** `addze` | `v >> 3` — the source wrote a shift, not a divide |

Two things fall out of this that are worth internalising:

- **`mulhw` vs `mulhwu` recovers the signedness of the dividend**, the same way
  `cmplwi` vs `cmpwi` recovers it at a comparison. Fix the declaration, don't
  cast at the operation.
- **`addze` is the whole difference between `/ 8` and `>> 3`.** It is the
  round-toward-zero correction that a signed divide needs and a shift does not.
  A bare `srawi` means retail wrote `>>`.

The modulo entry is not theoretical: last session `get_next_quadrant` needed
`%` where our source had `- row * count`, and the `mulli`/`subf` pair was
sitting in the target the whole time.

## Struct copies and by-value arguments

- **`*dst = *src` on a struct emits `bl __as__<T>F RC<T>`** — a call to a
  compiler-generated `operator=`, not an inline copy. If the target has that
  call and you do not, you hand-wrote the field copies (or vice versa). See the
  implicit-`operator=` rule in `codewarrior.md`.
- **The generated `operator=` is field-by-field `lfs`/`stfs`, software-pipelined
  (load next, store previous), with no loop** — even for a 13-float struct.
  A long alternating `lfs`/`stfs` run is a struct assignment, not vector code.
- **Passing a large struct by value** copies it into a stack temp and passes
  *the address of the temp* in the argument register. A block of `lwz` from one
  base followed by `stw` to `r1+K`, then `addi r3, r1, K` before the `bl`, is
  `f(*p)` where the parameter is by value — not `f(p)`.
- **A hidden stack temp also appears for small by-value aggregates.** In
  `zMainFirstScreen`, `xfont::create`'s `iColor_tag` argument is materialised by
  `lwz r0, @1171@sda21` / `stw r0, 0x8(r1)` and passed as `addi r5, r1, 0x8`.

## Returning an aggregate by value: the destination is a hidden `r3`

```
Vec make_vec(F32 a, F32 b, F32 c)     ->   make_vec__Ffff
                                           r3  = address of the caller's slot
                                           f1,f2,f3 = a, b, c
```

The struct is written **through `r3`**, and the real arguments shift right. So
a function whose mangled name says three floats but whose body stores to
`0x0(r3)`, `0x4(r3)`, `0x8(r3)` is not taking a pointer — it returns by value.

At the call site the caller passes `addi r3, r1, K` and then usually copies the
result out of that slot again. Two consecutive copies around a call is normal
for this shape, not a sign you got something wrong.

This matters here because `xVec3`, `basic_rect` and friends are returned by
value all over the codebase.

## Virtual calls, `new`, `delete`

```
b->v0()          lwz r12, 0x4(r3)      <- load vptr (offset 4 here, not 0)
                 lwz r12, 0x8(r12)     <- slot
                 mtctr r12
                 bctrl
```

**The first virtual sits at vtable offset `0x8`, so the declaration index is
`(slot - 8) / 4`.** In the probe `v0` is at `+0x8`, `v1` at `+0xc`, and the
destructor at `+0x10`. Counting slots in the target tells you how many virtuals
the class declares and in what order — and the order is declaration order.

**The vptr is not necessarily at offset 0.** In the probe it is at `+0x4`,
because `Base` declares a data member before its first `virtual`. That is the
same rule recorded in `codewarrior.md`: CW puts the vptr where the first
`virtual` is declared. The constructor's `stw r0, 0x4(r3)` of `__vt__4Base` is
the clearest place to read the offset off the target.

Real game code confirms both halves. `zNPCGoalRobo` dispatches through
`lwz r12, 0x1b8(r3)` / `lwz r12, 0x94(r12)` — a vptr sitting 440 bytes into a
deep NPC class, and slot `0x94` meaning declaration index `(0x94 - 8) / 4 = 55`.
If you are staring at a large offset wondering what it indexes, it is the vptr.

A **qualified** call (`d->Derived::v0()`) devirtualises to a plain
`bl v0__7DerivedFv`. A direct `bl` to a function you know is virtual means the
source qualified it.

```
new Base()       li r3, 0x8            <- sizeof
                 bl __nw__FUl
                 mr. r0, r3            <- null check is COMPILER-generated
                 beq skip
                 bl __ct__4BaseFv

delete b         cmplwi r3, 0x0        <- null check is COMPILER-generated
                 beq skip
                 lwz r12, 0x4(r3)
                 li r4, 0x1            <- the "deleting" flag
                 lwz r12, 0x10(r12)    <- destructor slot
                 bctrl
```

Two things to take from this:

- **Do not write the null checks.** `if (p) delete p;` in the source would emit
  a *second* compare. A single `cmplwi`/`beq` around a delete is what plain
  `delete p;` already produces.
- **`delete` calls the destructor through the vtable with `r4 = 1`.** That
  argument is the deleting-destructor flag: 1 means "run the destructor and
  free the storage", 0 means "destroy only" (as for a subobject or a stack
  object). If you see `li r4, 0x0` before a destructor call, the object was not
  being freed. Both values occur in the target objects currently cached in this
  tree, so the distinction is live here, not a textbook curiosity — read the
  flag rather than assuming `delete`.

## Function-scope statics

```
static S32 cached = <dynamic>;    lbz r0, init$46@sda21(r0)
                                  extsb. r0, r0
                                  bne already
                                  ...compute...
                                  stw r3, cached$45@sda21(r0)
                                  li r0, 0x1
                                  stb r0, init$46@sda21(r0)
```

A **one-byte guard** in `.sbss` next to the value, tested with `lbz`/`extsb.`.
Two consecutive anonymous counters get consumed (`$45` for the value, `$46` for
the guard), which is worth knowing when you are trying to make a later
`$NNNN` suffix line up.

**`static const S32 k = 7;` produces no storage at all** — just `li r3, 0x7`.
So a static in the target that *has* storage and a guard had a dynamic
initialiser; one that folds to an immediate was a constant.

## Comparisons, narrow types, bitfields, indexing

| Target asm | Source |
|---|---|
| `fcmpo` then `cror eq, lt, eq` | `a <= b` — the `cror` is the giveaway |
| `fcmpo` then `mfcr` / `extrwi` / `cntlzw` / `srwi r3,r0,5` | a float comparison being **materialised as a 0/1 value**, not branched on |
| `extsb r3, r3` | value is `S8` |
| `extsh r3, r3` | value is `S16` |
| `clrlwi r3, r3, 24` | value is `U8` |
| `clrlwi rD, rS, 27` after a `lbz` | reading a **5-bit bitfield** (`32 - 27`) |
| `rlwimi` | **writing a bitfield.** Almost nothing else generates it |
| `mulli r0, r3, 0xc` before an array base | scaled index — **the multiplier is `sizeof(element)`**, here 12 = `Vec` |

Two notable **absences**, both measured:

- `a < b ? a : b` compiles to `fcmpo` / `bltlr` / `fmr` — **not `fsel`**. An
  `fsel` in a target did not come from a plain ternary.
- `a < 0.0f ? -a : a` compiles to a compare against a pooled zero, `bgelr`,
  `fneg` — **not `fabs`**. An `fabs`/`fnabs` in a target came from an explicit
  abs helper or intrinsic, so go find which one.

## Bulk copies: unrolled below ~64 bytes, `mtctr` loop above

Measured on literal-initialised `char` arrays:

| Size | Form |
|---|---|
| 32 B | unrolled `lwz`/`stw` run |
| 64 B | unrolled `lwz`/`stw` run |
| 128 B | `mtctr` + `lwzu r0,0x8(r4)` / `stwu r0,0x8(r5)` / `bdnz` |

A 104-byte copy in `zMainFirstScreen` is also a loop, so the changeover sits
somewhere in 64–104 bytes. It was not worth pinning down more precisely; what
matters is reading the loop.

**To size an `lwzu`/`stwu` copy loop: trips × 8, plus any tail.** The classic
setup biases the source pointer back by 4 (`subi r4, r3, 0x4`) so the first
`lwz r3, 0x4(r4)` / `lwzu r0, 0x8(r4)` pair reads at offset 0 and 4.

Worked example, `zMainFirstScreen`: `mtctr 0x4d` (77 trips) × 8 = 616 bytes,
plus a trailing `lbz`/`stb` = **617**. That matched `@1170`'s recorded
`size:0x269` exactly, and 617 with a NUL at index 616 is how we knew it was a
`char[617]` initialised from a string literal rather than a struct.

## Local array from a string literal

`char buf[N] = "...";` copies N bytes out of `.rodata` into the frame — **the
declared array size sets the copy length, not `strlen`**. `const char* p =
"...";` produces a pointer load (`lis`/`addi` of the symbol) and no copy at all.
If the target copies, the source declared an array.

## `switch`: the jump-table threshold is exactly 7 arms

Measured directly, cases `0..N-1`:

| Arms | Form |
|---|---|
| 6 | binary search — a tree of `cmpwi` / `beq` / `bge` |
| 7 | jump table — `cmplwi` range check / `slwi r0,r3,2` / `lwzx` / `mtctr` / `bctr` |
| 8 | jump table |
| 7, with a hole in the range (0,1,2,4,5,6,7) | **still a jump table** |

So it is the **arm count**, not perfect density, that flips it — at least over
a range this tight. This sharpens the existing `codewarrior.md` entry, which
correctly said a dense `switch` tables but left "dense enough" undefined: if
the target has a `bctr` and you have a `cmpwi` tree, **you are short an arm**,
and the fix is usually a `case` you left out (possibly an empty one).

Cross-references in `codewarrior.md`: arms are emitted in **source order**, so
the target's physical block order gives you the order to write them in; and a
run of `cmplwi` against one CSE'd load is an if/else-if chain rather than a
`switch` at all.

## Carries no source information — do not chase it

- **`psq_st f31, 0x38(r1), 0, qr0` / `psq_l` in the prologue and epilogue.**
  Paired-single save/restore of a callee-saved FPR under `-proc gekko`. It is
  not vector code and it says nothing about the source.
- **`lmw` / `stmw` register blocks.** That is `-use_lmw_stmw on`, a build flag.
- **`@sda21(r0)`.** Small-data-area addressing. `r0` in a base position reads as
  literal zero, so this is an absolute reference to a pool object, not a
  computation on `r0`.
- **`lis rX, sym@ha` followed by `addi rX, rX, sym@l`.** One 32-bit address
  being built in two halves.
- **`clrlwi rD, rS, 16`.** Zero-extend the low halfword — a `U16` being used,
  not a mask the source wrote. (`clrlwi` with other shift amounts *is*
  informative; see the narrow-types table above.)

See `codewarrior.md`'s "Things that are not your fault" for the *scheduling*
equivalents of this list.

## Which section a symbol lands in is source information

Relocations are compared by target **offset**, so a symbol in the wrong section
mismatches every reference to it — and one misplaced table can make a dozen
unrelated functions look broken at once.

- **`.bss` on our side where the target has `.rodata` or `.data` means our
  table has no initialiser.** Writing the real initial contents fixes every
  dependent function in one edit. `tools/symdump.py` gets you the target's
  bytes.
- **`.sbss2` holds anonymous 4-byte zero objects**, including ones created at
  *parse* time by an aggregate initialiser inside an `inline` function. That
  mechanism, and the one case in this tree where it misled everyone, is written
  up under "Settled" in `docs/DUPLOTRON.md` — read it before concluding a pool
  ordering is unreachable.
- **`scope:weak` in `config/GQPE78/symbols.txt` means the source said
  `inline`**, and dtk extracts the single surviving copy into whichever object
  won at link time. "Our object defines a weak symbol the target's does not" is
  the expected state, not a bug. See `measuring.md`.

## Recipe: adding a measured entry

Compile a probe outside the tree and disassemble it. This touches no build
state and takes a couple of seconds.

```sh
SP=<your scratchpad>            # unique prefix per agent
./build/tools/sjiswrap.exe "./build/compilers/GC/2.0p1a/mwcceppc.exe" \
  -nodefaults -proc gekko -align powerpc -enum int -fp hardware \
  -Cpp_exceptions off -W err -O4,p -inline auto -maxerrors 1 -nosyspath \
  -RTTI off -fp_contract on -lang=c++ -common on -char unsigned \
  -str reuse,pool,readonly -use_lmw_stmw on -inline off \
  -c "$SP/probe.cpp" -o "$SP/probe.o"
./build/tools/dtk.exe elf disasm "$SP/probe.o" "$SP/probe.txt"
```

Take the real flags from `build.ninja` rather than copying the line above if
the unit you care about has `extra_cflags` — `zLightning` builds with `-sym on`,
for instance. `mwcceppc` has no "stop after assembly" flag here; it will try to
invoke the linker and fail, so always compile to an object with `-c` and
disassemble that.

Probes live in the scratchpad. **Never commit one into `src/`** — a dead probe
function in the tree is exactly the speculative filler the skill forbids.
