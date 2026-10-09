# PS2 memory-card space calculation

`iSG_isSpaceForFile` now reproduces its 408-byte original body in USA, Europe,
and Germany, rising from 22.40196% to 100%. The complete 53-function unit gains
408 exact bytes and one exact function in each version: 10,956 to 11,364 bytes
out of 13,984, and 42 to 43 functions. Whole-unit fuzzy matching rises from
94.35183% to 96.615845% (Germany: 94.35155% to 96.61556%). Every other function
score and the data result are unchanged.

The originals call `iSG_mc_availclust` and `iSG_mc_availDirEnt` separately.
A scoped `dont_inline` pragma around those two definitions preserves their
boundaries. Setting the local path-reset flag before copying the path restores
its delay-slot lifetime. Testing the existing-file-size `>=` case first restores
the original branch order and inlined file-size result lifetime. The arithmetic,
outputs, and path-restoration behavior remain unchanged.

Independent relocation replay reproduces 404 / 408 bytes in each debug original.
The one remaining JAL operand at offset 92 uses the compiled spelling `strcpy`;
its original runtime destination remains unnamed and unnormalized. All five
other calls use independently established original DWARF callee identities.
Original declarations agree on the 256-byte memory-card record: port at 0, slot
at 4, path at 16, and its 64-byte directory record at 128. The file size is at
offset 16 within that directory record, making the observed load offset 144.

Only the PS2 implementation file changes; GameCube and Xbox build their separate
platform implementations. No shared header, compiler setting, source profile,
or target metadata changes in this source correction. France's existing known
memory-card bodies are not yet source-profiled at this checkpoint.

Private evidence under `build`: `save-space-originals.py` / `.txt`,
`save-space-final-comparison.py` / `.json`, `save-space-final-changes.json`,
`save-space-raw-proof.py`, `save-space-iSG_isSpaceForFile-raw-proof.json`, and
`save-space-layout-proof.py` / `.json`. The before/after source parent is
`5a9192699`. A global file-size helper rewrite was tested and rejected because
it regressed already matching callers; that helper remains unchanged.

## French source coverage

A separate French profile selects all 24 already proven memory-card functions,
7,044 bytes. Their canonical linkages agree independently in all three debug
originals. Actual compilation of the complete source with `VERSION_SLES_53623`
and the existing `auto,bottomup` inline setting produces 21 code-matched
functions / 6,540 bytes, with 95.30949% whole-unit fuzzy matching. The two
format/unformat helpers remain unmatched and `iSG_mc_isformatted` remains at
96.44444%; none is excluded from the profile. Existing profile records are
unchanged. This adds no original identities, bounds, relocations, or target
bytes, and does not increase the known-code denominator.

The source previously selected the USA save-directory prefix for France.
The French original contains `BESLES-53623`, including the complete save-name
literal. Four independently decoded address pairs in its 500-byte name-builder
context reach that exact null-terminated string at `0x4f75cc`. Comparing the
entire context at `0x1b6c80` with all three named debug originals preserves
every opcode, register, arithmetic immediate, and branch displacement except
actual address/call operands. The existing `iSGStartup` anchor locates the
context consistently from all three references. This supports a bounded
`VERSION_SLES_53623` product-code branch; the context is not registered as a new
French function or used to grant additional matching credit. The 24 selected
function reports are identical before and after this regional string fix.

The French space-check body reports 100%, while independent raw replay confirms
396 / 408 bytes. Three JAL operands remain unresolved: one compiled as `strcpy`
and two compiled as `iSG_get_finfo`. The remaining three callees use the existing
French registry. These are code-comparison results, not complete runtime-link
closure. The production full-target/source gate remains the integration check.

Private evidence: `build/save-france-pilot/SLES-53623/` contains the actual
compiler result, complete report, and three-reference linkage proof;
`save-france-product-proof.json`, `save-france-product-context.json`,
`save-france-space-raw-proof.json`, and `save-france-profile-preservation.json`
record original strings/uses, raw comparison limits, and preservation checks.
