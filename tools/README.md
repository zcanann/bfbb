# tools

Build plumbing plus the analysis tools used to work the decompilation.

## Build plumbing

| | |
|---|---|
| `project.py`, `ninja_syntax.py` | decomp-toolkit build generation, driven by `configure.py` |
| `patch_compiler.py` | derives the scheduler-patched CodeWarrior at build time |
| `decompctx.py` | flattens a translation unit and its includes into one file |
| `download_tool.py`, `transform_dep.py` | fetch pinned tools, rewrite depfiles |
| `cwexec.py` | reads the CodeWarrior launch command out of `build.ninja`, so the analysis tools compile the way ninja does |
| `report.py`, `upload_progress.py` | progress reporting |

## Analysis

These read `objdiff.json` and `build.ninja`, so run `configure.py` first.
They compile through `cwexec.py`, which takes the launcher, the sjiswrap
wrapper and the compiler path from the `mwcc`/`mwcc_sjis` rules rather than
assuming them - so they work on Linux (where the `.exe` files run under
`wibo`) and with `configure.py --compilers <dir>`.

### `solo.py` — compile and diff one unit, without touching the build

```
solo.py <unit-fragment>                 non-matching functions and percents
solo.py <unit-fragment> <symbol-frag>   side-by-side instruction diff
solo.py <unit-fragment> --missing       target functions absent from our object
```

Compiles a single source file into a temp directory with the exact compiler and
flags `build.ninja` would use, then diffs it against the original object. About
two seconds, and it writes no shared state — so any number of them can run
concurrently in one checkout while `ninja` would have them clobbering each
other's objects and `report.json`.

`--missing` is where the volume is: it lists what has not been written at all.

### `symdump.py` — read a static table out of the target object

```
symdump.py <unit-fragment>                      data symbols, largest first
symdump.py <unit-fragment> <sym> [u32|f32|raw]  dump the bytes
```

The original objects still carry all their static data — goal sequence tables,
sound asset lists, hook name arrays, tweak defaults, jump tables. When a
function you are reconstructing indexes into one of those, read the contents
instead of guessing at them. Guessed data produces source that looks
authoritative and is wrong, and it also settles enum values that would
otherwise be invented.

### `classify.py` — why the remaining functions differ

"3000 functions left" is not actionable. This buckets them:

- `MISSING` — not written yet
- `POOL` — byte-identical apart from an anonymous `@NNN` literal-pool index, so
  blocked on translation-unit completeness rather than on the function itself
- `SCHED` — same instructions, different order
- `REGS` — same mnemonics, different registers; usually responds to reshaping
  the source expression
- `SIZE` / `OTHER` — a real source difference

Also prints the units closest to complete, which is the cheapest way to raise
`complete_units`.

### `smoke.py` — does everything still compile?

```
smoke.py                        every unit
smoke.py <frag> [<frag>...]     only matching units
smoke.py --skip <frag>,<frag>   everything except these
```

Compiles every unit into a throwaway object and reports only failures. Useful
for checking a shared-header change against the whole tree without writing
build state or waiting for a link. It proves the tree compiles; it says nothing
about whether the DOL still matches — run a real build for that.

### `flagsweep.py` — find a library's real compiler flags

```
flagsweep.py <object-path-fragment> [flag[,flag...]]
```

Appends a candidate flag and counts how many more functions reach 100%. This is
how `MSL_C`'s `-opt level=4 -inline on` and rwsdk's `-fp_contract on` were
found. The reconstructed command matches ninja's exactly for `.c` units but not
always for C++ ones, so treat a hit as a hypothesis and confirm it with a full
build before changing `configure.py`.

### `playtest_iso.py` — run the decomp on real hardware or an emulator

```
playtest_iso.py <retail.iso> <output.iso> [--dol build/GQPE78/main.dol]
```

Turns a playtest build into a bootable disc. Build it first with

```
python configure.py --non-matching && ninja
```

which links every `src/SB` unit from our source and takes the libraries from the
retail objects. Our DOL is smaller than retail's, so this writes it over the
original in place and zeroes the slack up to the FST; the filesystem is not
rebuilt and every other byte of the image is untouched. The 1.4 GB copy happens
once, so re-run after each build to refresh just the DOL.

It refuses to patch anything but a verified GQPE78 image whose embedded DOL
hashes to `306526d9…`, since assets and code have to come from the same build.

### `solo.py --shadow` and `--mw` — two flags worth knowing

```
solo.py <unit-frag> --shadow <dir>       prepend a private include directory
solo.py <unit-frag> --mw GC/2.0p1        compile with a different CodeWarrior
```

`--shadow` is how a shared-header change gets measured. Copy the header into a
scratch directory, edit it there, and pass the directory: the real tree is never
touched, so another agent compiling the same unit at the same moment does not
silently get numbers from the wrong tree. That failure mode is invisible — the
numbers look perfectly ordinary — which is why it is worth the extra step.

**It only shadows headers, and that is a trap for a return-type change.** The
declaration moves into the shadow; the DEFINITION stays in the real `.cpp`,
which the shadow build still compiles unchanged. So the unit that defines the
function is never measured under the change. Declaring
`iSndIsPlayingByHandle` as `U8` in a shadowed `iSnd.h` measured a clean +1 in
`zEntPlayer`; applying it for real also cost `iSndIsPlayingByHandle` itself
100.000 -> 85.588, because the conversion moved inside. Measure the defining
unit explicitly, with the real edit, before believing a signature change.

`--mw` picks any compiler under `build/compilers`. `GC/2.0p1` is the stock
compiler this branch's patched `GC/2.0p1a` is derived from, and it answers the
cheapest question in the project: *is this function my problem or the patch's?*
A function that is exact under stock is source-correct by construction and has
nothing left to recover.

### `patchcost.py` — what the compiler patch buys, and what it costs

```
patchcost.py [unit-frag ...]             functions the patch BREAKS
patchcost.py [unit-frag ...] --gains     functions the patch FIXES
```

Compiles every unit twice, once with each compiler, and diffs the two
non-matching sets. The default direction is the cost — exact under stock and not
under ours — and tree-wide that is seven functions. `--gains` is the other
direction, and together they are the only honest price for a change to
`patch_compiler.py`: a narrowing that does not recover one of the seven is
buying nothing, and one that drops a name from the `--gains` list is paying for
it.

### `rodatalayout.py` — the two `.rodata` object layouts, side by side

```
rodatalayout.py <unit-frag> [section]
```

Target address, size and symbol beside ours, with a `|` on every row whose sizes
disagree. This is the tool for the `__deadstripped_<unit>` work: the sizes are
easy and the ORDER is the whole job, because CodeWarrior interns anonymous
templates in first-use order, so a never-called function's locals land at that
function's position in the source. Six units were brought to a byte-identical
`.rodata` with it in one pass.

### `stridediff.py` — element strides ours emits and the target never does

```
stridediff.py [unit-frag ...]
```

For the "raw byte offset applied to a typed pointer" class, which scales twice
and which objdiff cannot see. Compares the per-function multiset of `mulli`
element strides and self-incrementing `addi` loop steps.

**Read the `mulli` rows; ignore the `addi` rows.** The `addi` half produces ~770
hits tree-wide, dominated by frame adjustments and pool bases. Even in the
`mulli` half, most rows are the documented mis-attribution — one body under two
names on the two sides — so check that both symbols disassemble to the same code
before believing a row.

### `promotable.py` — units at 100% that are still marked NonMatching

```
promotable.py
```

Cross-references `report.json` against `configure.py`. A unit only reaches
`complete_units`, and only links from OUR object, when it is marked `Matching`;
objdiff reaching 100% is necessary and not sufficient, because it pairs symbols
by name and is blind to definition order. Run `symorder.py` on anything this
lists to get the specific blocker, and remember that the DOL sha1 is the only
thing that settles a promotion.

### `unitrank.py` and `nmlist.py` — where the remaining work is

```
unitrank.py [N]                  units by non-matching count, with byte totals
nmlist.py <unit-frag> ...        each unit's non-matching functions
```

`nmlist.py` prints the `classify.py` bucket beside each function, so `SIZE` and
`OTHER` (a real source difference) can be told from `SCHED` and `REGS`
(compiler-track) before any time is spent.

### `metrics.py` — the headline figures out of report.json

```
metrics.py [path/to/report.json]
```

Per-category exact and fuzzy percentages with function and unit counts. The two
have decoupled, so quote both.

### `badges.py` — regenerate the README's progress badges

```
badges.py                    print the six values
badges.py --check            exit 1 if the README is out of date
badges.py --write            rewrite the badge lines in place
badges.py --summary          markdown table with deltas, for a CI job summary
```

The badge URLs are the only place this fork publishes its numbers, and they
were hand-maintained, so they drifted -- before this existed they read 7250
functions where `report.json` said 7251, and the percentages were truncated
rather than rounded.

**The README is the snapshot.** `--summary`'s deltas come from parsing the
values already in the README before rewriting them, so there is no second build
and no extra state file that can fall out of sync. That is also why `--check`
is a useful PR gate on its own.

Encoding order matters and is easy to get wrong: shields.io wants a literal `-`
doubled, and the percent-encoding has to run `%` first, then space, then `/`.
Encoding the space before the percent turns `%20` into `%2520`.

### `objsnap.py` — byte-identity snapshot, for compiler-patch work

```
objsnap.py save <out.json>     hash every built object, the DOL and the compiler
objsnap.py cmp  <before.json>  compare the tree against a snapshot; exit 1 on any change
```

A change to `patch_compiler.py` changes the derived compiler's sha1, and then
every measurement either side of it is incomparable. So the acceptance test for
a refactor meant to change **no** behaviour is not the match percentage -- it is
that all 542 objects come out byte-for-byte as they did, and the DOL still
hashes to retail.

`report.json` cannot do this job: it scores symbols by name and normalises pool
ordinals, so an object can shift, gain a symbol or reorder its `.text` and still
report identical percentages. Only the bytes settle it. This is `gcgate.py`'s
counterpart for compiler changes rather than source changes.
