# PS2 file debug display source matching

The status guard in `iFileDebugMode` now compares its enum bitfield explicitly
with `HIS_STATUS_INVALID_ID`. This preserves the original valid/invalid branch
and removes a redundant sign-extension sequence before indexing the status
strings. The 328-byte routine becomes exact in USA, PAL and Germany.

The actual complete fifteen-function / 3,504-byte source unit improves from
2,732 exact bytes / twelve functions to 3,060 / thirteen, and from 97.53425%
to 99.23516% fuzzy matching. Every other function and prior data result stays
unchanged. The remaining `iFileLoad` and `iFileFullPath` residuals are retained.
France has no source profile for this unit; no identity or profile is added.
The separate GameCube/Xbox implementations and all shared headers are untouched.

Original DWARF in each debug executable independently confirms the 28-byte
`HISRequestDebug` layout, its four-byte packed prefix with eight-bit next-request,
status and destination-type fields at bit offsets 0/8/16, and the four-byte
`HISStatus` enum with values 0 through 7. The existing local declarations already
agree; no type was changed to affect code generation. The 60-byte file-index
layout and all twelve entries of the original `STATUSES` and `TYPES` string
pointer tables also match the actual compiled object and its data relocations.

Raw replay resolves calls and data only through original DWARF identities and
checks the original diagnostic string bytes. All 328 bytes agree in each
version; this function has no remaining unresolved relocation in that replay.
The production target metadata and exporter rules are unchanged.

The isolated source baseline is `5cb49a1a2`, with read-only target inputs from
`C:/Projects/bfbb/build/oct09-obb-smooth/<version>/`. Private evidence is frozen
in `build/ifile-before/` and `build/ifile-debug/`, including whole-source objects,
compiler logs and reports. Comparison/changes are in
`ifile-before-comparison.json`, `ifile-debug-comparison.json` and
`ifile-debug-changes.json`; original-only layout/data and actual-object raw
replays are `ifile-debug-layout-data-proof.py` / `.json` and
`ifile-debug-raw-proof.py` / `.json`.

## Loader index lifetime

Declaring the once-assigned file index as a constant local restores the original
register lifetime across the failure guard: the successful path reuses the name
register only after the diagnostic-return path has ended. `iFileLoad` becomes
exact at 264 bytes in all three debug versions, with unchanged signed alignment
arithmetic, allocation, block-loading arguments and returned size.

The full unit advances from 3,060 to 3,324 exact bytes and thirteen to fourteen
exact functions, with 99.77169% fuzzy matching. Every other function/data result
is unchanged; the sole remaining body is the 180-byte `iFileFullPath` at
95.55556%. Moving declarations, reversing or spelling out the guard, and using
an assignment inside the condition were tested separately before retaining the
constant-local form. The existing source is otherwise unchanged.

Raw replay agrees for 256 / 264 bytes. The two remaining original transfer
identities at offsets 36 and 60 are not established by this proof (the compiler
spells them `HISGetFileIndex` and `printf`); no target alias is inferred. The
other original calls, allocator-data relocation, diagnostic bytes and every
arithmetic/control-flow instruction replay exactly. Normal production scoring
and exporter rules stay unchanged.

Private evidence: `build/ifile-load/<version>/` frozen objects/reports,
`ifile-load-comparison.json`, `ifile-load-changes.json`, and
`ifile-load-raw-proof.py` / `.json`. The source parent is `fbca61fba`.
