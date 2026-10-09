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
