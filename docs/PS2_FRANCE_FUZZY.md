# France fuzzy candidate discovery

`tools/platforms/france_fuzzy.py` searches the stripped French PS2 executable
using authenticated debug-region originals. It prioritizes large functions and,
with `--blocks`, physically contiguous translation-unit blocks. Compiled source
objects are never inputs.

```sh
# Find large regions first and save a reusable diagnostic map.
python tools/platforms/france_fuzzy.py --orig-dir /path/to/orig \
  --blocks --limit 10 --map-output build/france-large-map.json
# Use those tentative regions when ranking smaller candidates.
python tools/platforms/france_fuzzy.py --orig-dir /path/to/orig \
  --source zNPCGoal --hypotheses build/france-large-map.json
# Inspect an independently verified anchor, including its occupied range.
python tools/platforms/france_fuzzy.py --orig-dir /path/to/orig \
  --source xGrid.cpp --name xGridUpdate --include-verified
```

Name/source filters are case-insensitive substrings; `--address` selects an exact
reference entry. USA is the default reference; `--reference` also accepts the
European and German debug releases. `--order source` restores source/address
ordering. The default largest-first order uses reference byte size, before the
query limit is applied. Too-small/too-large references do not consume that limit.

A TU block contains consecutive original functions from one source, separated
by at most `--block-gap` bytes (default 16). It never joins separated functions
merely because they share a source name. Already verified members break unknown
blocks. Member names and original boundaries remain in the diagnostic output.

## Large bodies and bounded refinement

Rare five-instruction seeds propose windows. Exact bit-parallel LCS scores every
eligible window without allocating a quadratic traceback matrix. This works for
large functions and TU blocks beyond the edit-alignment cell budget. LCS means
longest common subsequence, including gaps, rather than common substring.

Up to four LCS-ranked windows receive exact semiglobal edit alignment when within
`--max-cells` (default 1,000,000). All reference words are consumed; target flanks
are free. Refined candidates include matching runs, substitutions, insertions,
deletions, inferred spans and edit scores. `--max-refinements 0` selects coarse
search only. Quadratic refinement never exceeds its explicit cell cap.

Unrefined candidates remain in the ranking with
`alignment_kind: coarse_seed_window`, an empty alignment, and null edit scores.
Their candidate edges are search-window edges, **not inferred function bounds**.
Each result records its search window, seed start estimate and nominal flank
slack. Verified exclusions or region edges may clip that window. Normalized LCS
scores, body hashes and raw-byte equality are separate fields.

Defaults are 20 queries, 16,384 words per query, 12 windows, 24 selected seeds,
64 occurrences per seed and 32 words of flank slack. `--max-words`, `--slack`,
`--max-windows`, `--max-seeds`, `--max-occurrences` and `--seed-words` expose these
budgets. Cell-budget diagnostics count windows left coarse, not discarded
candidates. Sparse seeds or changed instruction order can still hide a match.

Normalization removes ordinary register allocation and address-like differences
while retaining zero/GP/SP/RA roles, operation selectors, shifts, comparisons,
logical literals and small non-stack field offsets. Unknown encodings remain
literal. This intentionally weak comparison is not a relocation or behavior proof.

## Verified occupancy and tentative overlays

`france_layout.py` reads only the established reviewed, CFG-corroborated,
transfer-corroborated and TU-corroborated function registries. It requires their
explicit boundary confirmations, expected provenance kind, executable identity
and complete original body hashes. Conflicting boundaries fail closed. These
checks authenticate existing independently reviewed proofs; this search does
not replace or regenerate those proofs. Address-only anchors and candidate files
never become hard exclusions.

By default, occupied instruction seeds are ignored and candidate windows are
split at independently verified boundaries. Reference provenance identifies known
functions by version, executable identity, source/name and original entry address,
including overloads. Skipped references have an explicit `already_verified_reference`
status and a `--include-verified` hint; the CLI prints their count.

Earlier larger hypotheses also help rank later small queries. If a candidate
substantially overlaps a larger incompatible hypothesis, its ranking loses five
points by default. The unmodified LCS score, penalty and conflicting tentative
identities remain visible. Only the earlier query's best candidate influences
this penalty; a block is compatible with members from its own source. No candidate
is removed on this evidence. Set `--soft-overlap-penalty 0` to disable the penalty.
`--hypotheses` accepts earlier candidate outputs or maps, including ambiguous
alternatives; all imported hypotheses remain explicitly unconfirmed.

`ranking_score_percent` is the normalized LCS Dice score minus this optional soft
penalty. `best_runner_up_gap_percent` compares the two best retained rankings,
including coarse candidates. A zero gap preserves ambiguity; a null gap means
fewer than two candidates. Neither is an identity confidence or uniqueness proof.

Every output includes a reusable `layout`; `--map-output` writes it separately.
It lists authenticated occupied functions, registry hashes, unclaimed regions
ranked largest first, and distinct soft overlays. Unclaimed regions can include
padding. Tentative overlays never reduce verified-free space. Outputs and caches
cannot overwrite originals or configuration directories.

## Caches and validation

Hash-keyed metadata, normalized tokens and SQLite seeds remain under
`build/france-fuzzy-cache`. Originals are authenticated on every invocation.
Cache keys cover normalization/index version, executable identity, CPU region
and seed width; bump `CACHE_VERSION` if those cached representations change.
Temporary replacements prevent incomplete caches. Use `--cache-dir` to isolate runs.

The initial large-block check found candidate windows for Dutchman (59,688
reference bytes), Hazard (57,056), BossSB2 (56,124) and KingJelly (49,460), at
99.68–99.82% normalized LCS. A five-query cold run took 19.27 seconds, including
5.58 seconds preparing caches. These are unconfirmed coarse windows and observed
local timings. None was promoted into a registry. Private evidence is
`build/france-large-oct08.json` and `build/france-large-map-oct08.json`.

```sh
python -m unittest discover -s tools/tests -p test_france_fuzzy.py -v
python -m unittest discover -s tools/tests -p test_france_layout.py -v
# Also authenticate the private xStrHash/xGridUpdate regression fixtures:
# PowerShell: $env:BFBB_FRANCE_TEST_ORIG='C:/path/to/orig'
# POSIX:     export BFBB_FRANCE_TEST_ORIG=/path/to/orig
python -m unittest discover -s tools/tests -p test_france_fuzzy.py -v
```

Tests cover independent LCS cross-checks, reconstructed gapped alignment, large
bodies above the cell cap, ambiguous copies, verified exclusions, overload
provenance, TU adjacency, body/identity failures and tentative-only penalties.
The existing independently known string and Grid candidates remain top-ranked
with `--include-verified`. No matching-progress increase or source-link claim is
made by this discovery tool.
