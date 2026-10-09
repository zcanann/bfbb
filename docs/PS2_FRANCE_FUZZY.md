# France fuzzy candidate discovery

`tools/platforms/france_fuzzy.py` searches the stripped French PS2 executable
for counterparts of named functions in the three debug-bearing originals.
It reads authenticated original executables and their actual DWARF ranges;
compiled source objects are not inputs.

```sh
python tools/platforms/france_fuzzy.py --orig-dir /path/to/orig \
  --source xGrid.cpp --name xGridUpdate --output build/france-grid-candidates.json
python tools/platforms/france_fuzzy.py --orig-dir /path/to/orig \
  --reference SLES-51968 --source zEntTrigger.cpp --top 5
```

Name and source filters are case-insensitive substrings. `--address 0x...`
selects an exact **reference** entry address. The default reference is USA;
the target is always France. `--limit` defaults to 20 functions and the output
reports any omitted references. No filter is required, but broad runs should
set an intentional function budget.

The tool uses the existing reviewed `cpu_text` region associated with the
authenticated French executable. It excludes VU packets and initialized data.
It normalizes ordinary GPR/FPR assignments, branch and call destinations,
LUI values, GP/stack offsets and large address-like offsets. Zero, GP, SP and
RA retain distinct roles. Opcode selectors, shift amounts, comparison and
logical literals, and small non-stack offsets remain useful discriminants.
Unmodeled encodings remain literal. This deliberately lossy normalization is
for discovery; it does not establish equivalent behavior or relocation rules.

Rare five-instruction seeds rank nearby candidate windows. Only a bounded
number of windows receive exact semiglobal edit-distance alignment: the whole
reference must be consumed, while target prefix/suffix words are free. The
JSON includes inferred target spans, substitutions, insertions, deletions,
matching runs, coverage, edit similarity, exact longest-common-subsequence
length and its Dice score. The LCS implementation is bit-parallel and does not
mean longest common substring. Body hashes and a separate raw-body-equality
flag distinguish normalized matches from actual byte equality. Executable
payloads are not written into the candidate JSON.

`best_runner_up_gap_percent` is the score difference between the two best
refined candidates. Zero preserves ambiguity; null means fewer than two were
refined. It is not an identity confidence or an exhaustive uniqueness proof.
The seed occurrence limit, retained seed count, skipped windows and other
search budgets are recorded explicitly. Sparse seeds or changed instruction
order can hide the right candidate.

The default limits are 12 windows, 24 selected seeds, 64 occurrences per seed,
32 words of window slack, and 1,000,000 alignment cells per window. Functions
over 4,096 words are skipped before search. The cell cap is a separate limit:
it normally skips refinement beyond roughly 970 words even below that size
limit. `cell_budget_skipped_windows` records this case. Increase `--max-cells`
for an intentional larger query; quadratic refinement is never silently run
beyond the requested budget. `--slack`, `--max-windows`, `--max-seeds`,
`--max-occurrences` and `--seed-words` expose the other tradeoffs.

Hash-keyed caches under `build/france-fuzzy-cache` retain ELF/DWARF metadata,
normalized target tokens and a SQLite seed index. Original hashes are checked
on every invocation. Cache keys include normalization version, executable
identity, CPU region and seed width. A changed algorithm must bump
`CACHE_VERSION`. Temporary files and database replacement keep incomplete
indexes out of subsequent queries. Use `--cache-dir` for isolated runs.

Candidate output defaults to `build/france-fuzzy-candidates.json`; originals
and configuration directories cannot be output/cache destinations. Every
document and function explicitly says `eligible_for_progress: false` and
`boundary_confirmation: false`. It never changes a registry, source profile,
symbol table, denominator or matching report. A candidate still needs the
existing independent original-only ownership, entry and boundary review.

Validation on 2026-10-08 uses three independently known exact string functions
at `0x20f190`, `0x20f1f0` and `0x20f260`, plus the independently verified,
raw-byte-different `xGridUpdate` at `0x305730` (204 bytes). Each ranks first.
A private cold run for the three string functions took 6.97 seconds, including
DWARF parsing and index construction; the same warm query took 0.10 seconds.
The warm Grid query took 0.08 seconds. These are observed local timings, not
runtime guarantees. Artifacts are `build/france-fuzzy-benchmark-*.json` in the
regional worker checkout.

```sh
python -m unittest discover -s tools/tests -p test_france_fuzzy.py -v
# Also run the private original checks:
# PowerShell: $env:BFBB_FRANCE_TEST_ORIG='C:/path/to/orig'
# POSIX:     export BFBB_FRANCE_TEST_ORIG=/path/to/orig
python -m unittest discover -s tools/tests -p test_france_fuzzy.py -v
```

Six tests pass with originals supplied. Synthetic tests preserve register-role
distinctions, reconstruct mixed insert/delete/substitution alignments, retain
ambiguous duplicate candidates, enforce the cell budget and reproduce cached
rankings. Three hundred randomized short sequences compare the bit-parallel
LCS result with an independent quadratic implementation. No matching-score
or source-link increase is claimed by adding this discovery tool.
