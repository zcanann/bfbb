# PS2 INI newline-count ordering

Incrementing each CR/LF counter before recording its index restores more of the
original parsing instruction order. The two local assignments are independent,
so the resulting counts and final newline index are unchanged.

Actual complete source objects for USA, PAL and Germany improve from 94.46125%
to 95.42533% whole-unit fuzzy matching. The 1,396-byte parser improves from
91.60458% to 93.0659% under the ordinary report. All four other functions keep
their exact scores; exact code remains 720 of 2,116 bytes. The interactive diff
uses a different percentage calculation (91.44412% to 92.90544%); these values
are not substituted for the normal report. Every GU4Y78 GameCube function score
is unchanged, including all eight exact functions and 1,476 exact bytes.

France currently profiles only the integer and string getters, so no additional
French coverage is claimed. The parser retains register-allocation and repeated
inlined-whitespace-loop scheduling differences. These are unresolved differences,
not evidence that a compiler patch is justified. Several declaration and case
order probes are retained privately and were not applied to production source.
No compiler flags, profile, relocation rules or scoring settings change.

Private reproducible evidence in the PS2 worktree is
`build/ini-full-comparison.json` and its replay script, retained whole source
objects, `build/ini-gc-before.json`, `build/ini-gc-after.json`, and the exploratory
`build/ini-variants.json`. This is a fuzzy-score improvement with no new exact
function or retail-link claim.
