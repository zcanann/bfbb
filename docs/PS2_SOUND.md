# PS2 sound source

The complete xSnd.cpp now compiles against genuine PS2 pad/sound declarations
and the verified entity/model data headers. Original DWARF establishes the
48-voice capacity, 5,024-byte xSndGlobals, aligned listener matrices, 100-byte
voice record, 16-byte platform sound info, and 28-byte lookup record. Actual
compiled sizes and every direct member offset of eight consumed aggregate
types agree with all three debug-bearing originals. Debug compilation leaves
all allocated source sections unchanged.

PS2 selects its original lookup record instead of GameCube's DSP lookup view.
Original PlayInternal instructions load the sample rate as U16 at offset 8 and
the internal ID at offset 0; the existing ID threshold and playback decisions
are preserved. Voice loops use the platform capacity. The PS2 external sound
callback includes a boolean second parameter, as recorded by both original
caller and callee signatures; the shared API uses the platform callback typedef.
GameCube retains its capacity, lookup layout, callback signature and helper
placement. The shared lightweight entity-position accessor preserves the real
model-matrix field access without importing collision implementations.

All 37 original functions / 6,764 bytes remain in the source comparison.
The initial normal comparison matches 15 functions / 1,388 bytes at 79.53755%
fuzzy matching. Unmatched functions remain visible. No original function or
runtime implementation is replaced by a stub. No complete data, TU-link or
retail executable reconstruction claim is made.

The four-byte PS2 pad context and public pad API are recovered from original
DWARF; no controller implementation is added. The normal GameCube source build,
retail executable hash and complete progress report remain unchanged.

Private evidence: build/sound172/compile.py, compare.py, type_proof.py,
type-proof.json and pad-original-types.json.
