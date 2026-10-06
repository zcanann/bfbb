# Xbox particle command initializer

Ordinary Xbox-only inline visibility for the unchanged `xParCmdRegister` body
restores the complete `xParCmdInit()` registration program. The production
compiler and linker flags, complete source TU, entry context, and other platforms
are unchanged. The original 721-byte function at `0x149a40` contains 81 constant
stores registering 27 command IDs, asset sizes and callbacks in a 35-slot,
420-byte table. The previous generated body called Register 27 times; the new
ordinary `/GL` build emits the same complete store program as the original.

The new reviewed identity follows the original program and the 21 previously
reviewed callback registrations. The initializer previously existed as the
anonymous extent `anon_00149a40`; that same extent is promoted once. No original
TU ownership or complete source-linked executable is claimed.

## Original and source address proof

`tools/platforms/xbox_particle_initializer.py` checks the full constant-store
program against all 27 registration tuples. Original callback addresses must
refer to the pre-existing reviewed callback records, with their complete hashes,
bounds, command IDs, sizes and original initializer provenance. The table must
occupy a writable original data allocation. All original bytes are preserved.

The five empty callback registrations are explicit source aliases, not arbitrary
RET acceptance: IDs 11 (Scale3rdPolyReg), 12 (Tex), 19 (Alpha3rdPolyReg), 24
(SmokeAlpha), and 25 (Scale). Each actual named source function must belong to
`xParCmd.obj`, have a closed one-byte RET body, and fold to the same address. The
original registrations must select the same reviewed one-byte RET at `0x0bcd00`.
These aliases provide function-address identity only and receive no new credit.

There are exactly **107 address operands**: 81 table store destinations and 26
callback values. Production source addresses must be genuine PE HIGHLOW fields
at decoded four-byte instruction operands. The complete source registration
program independently establishes one consistent table base and all ID, size,
and callback associations; it does not use the original table address or a
minimum-store heuristic. Source callback owners and closed extents are checked
against the actual linked MAP and code. The source table must lie wholly in a
writable, non-executable allocation.

The proprietary production LTCG object does not expose an ordinary static COFF
symbol table. A separately recorded evidence object compiles the **same complete
TU with the same pinned compiler and options, omitting only `/GL`**. It is never
linked or scored. It proves the actual `_sCmdInfo` static symbol owns the sole
420-byte BSS allocation, the named initializer owns its complete 721-byte code
section, and all 107 named DIR32 records occupy decoded fields with their correct
roles: table references are absolute store displacements; callback references
are immediates. Command and object SHA256 are recorded in the build artifacts.
This sidecar establishes source storage/relocation ownership, not production
addresses; the independent PE program proof supplies those.

Function-address anchors remain distinct from generic data globals. The COFF
writer accepts defined-function DIR32 references only from explicitly typed
function anchors with zero addends. The exporter reader recognizes the resulting
entry references, then retains exact reviewed-relocation equality and original
inverse reconstruction. Generic data-symbol lookup and scoring policy are
unchanged. Actual named function entries are never treated as interior data.

## Validation

Both authenticated originals and final normal production builds pass:

- All 721 initializer bytes reconstruct exactly from each actual production PE
  after its 107 genuine address fixups; every other instruction byte is literal.
- Both reports gain **721 matched bytes / one function**, reaching **14869 / 79**.
  All prior function records remain unchanged apart from generated COFF offsets.
- The original `.text` denominator stays **1798760**, with **2556** known
  functions and **635415** known function bytes. The old anonymous 721-byte
  extent is removed exactly once. Data and completion measures are unchanged.
- All seven real source consumer controls preserve their prior scores; all
  **33 ordered GameCube allocated sections** remain byte-identical.
- The actual standalone `python tools/platforms/verify_xbox_reviewed.py` CLI
  passes both originals. No full-TU or retail-relink completion is asserted.

Private reproducible evidence is under `build/xbox292/`: `propose.py`,
`initializer_proof.py`, `source-symbol-proof-command.json`, `compile.py`,
`compare.py`, `gc/proof.json`, `verify_final.py`, `final-verification.json`, and
both complete `production/<version>/` outputs. Final source command manifests,
sidecar hashes, PE/MAP files and function extents are retained there. Original
bytes and compiler binaries stay in ignored build/original directories.
