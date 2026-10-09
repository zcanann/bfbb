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

## Original stream-voice limit

The PS2 stream lock, ready and unlock loops use four voices. All three
original debug executables calculate their end pointer with `+0x190`; original
DWARF gives each voice a 100-byte extent. The previous shared six-voice bound
compiled as `+0x258`. A platform constant now selects four for PS2 and preserves
six for GameCube, changing exactly these three instruction immediates.

Actual complete-source comparisons in USA, Europe and Germany each improve
from 15 / 1,388 to 17 / 1,876 code-matched functions / bytes: StreamLock (416)
and StreamUnlock (72) become standard objdiff matches. StreamReady improves
81.77273% to 81.818184% and remains unmatched. All other function records are
unchanged, and all 37 functions / 6,764 bytes remain in the comparison. France's
existing three-function / 132-byte subset is unchanged (one / 68 matched).
All four independently compiled PS2 source objects are byte-identical.

Raw relocation reconstruction reproduces both StreamLock (416 bytes) and
StreamUnlock (72 bytes) in all three debug originals. The original StreamLock
calls at offsets 264 and 348 directly target `iSndStop__FUi`, owned by the PS2
platform sound unit, rather than the shared `xSndStop` wrapper. PS2 now selects
those original callees; GameCube retains its wrapper calls. This changes exactly
two source call relocation identities, with every allocated section byte and
all regional standard function records unchanged. The comparison policy is
unchanged, and no whole sound-unit or executable reconstruction is claimed.

An actual GameCube compile preserves all seven ordered allocated sections.
Private evidence is in build/sound243: original-stream-limits.json,
source-delta-proof.json, all-region-proof.json, raw/raw-proof.json and
GC proof gc/proof.json. No additional layout, flags, profile or backend change
is involved.

The callee follow-up is independently recorded in build/sound245/proof.json,
raw/raw-proof.json and gc/proof.json; authenticated original callee identities
are in build/sound243/lock-call-identities.json. All four actual whole-source
objects remain byte-identical across PS2 versions, and the GC control preserves
its seven allocated sections.

## Stop-fade loop exit

When deleting the last matching fader, the original PS2 StopFade branches to the
loop's existing return block. Replacing the inner return with break preserves
behavior because the loop is immediately followed by return. Actual source
compilation changes only the branch displacement at function offset 176, from
an epilogue jump to that existing exit; all relocation records remain identical.

Each debug-region comparison gains StopFade's 396 bytes, reaching 18 / 2,272
matched functions / bytes for the unchanged complete 37-function / 6,764-byte
sound inventory. All other function records and France's existing subset remain
unchanged. All four compiled whole-source objects are byte-identical, and actual
GC compilation preserves all seven allocated sections.

This is a standard code match. Raw reconstruction still distinguishes the two
stop callees and the source xSndGetVol call from the original platform targets;
no raw396 claim is made. Private evidence: build/near246/proof.json,
raw/raw-proof.json and gc/proof.json. No profile, header or backend changed.

## Remaining voice bounds and PS2 wrappers (2026-10-08)

PauseAll, PauseCategory and StopAll still used a literal 64-voice loop after
the earlier capacity correction. They now use `XSND_VOICE_COUNT`, as the
other voice loops already do. Original PS2 instructions compare against 48.
This promotes those three functions and Resume, which inlines PauseAll, for
648 additional exact source bytes. GameCube keeps its existing 64-voice bound.

The original Suspend calls the complete Update routine. The selected compiler's
bottom-up inlining expanded Update into Suspend, producing a much larger body.
A PS2-only `dont_inline` pragma around Update's definition preserves the original
call while keeping its own body unchanged. Suspend's 160 bytes then match.
The pragma resets immediately after the definition. Disabling bottom-up inlining
for the entire unit regressed Init; that broader change was rejected.

Two missing shared wrappers are also restored only for PS2. `xSndIsReady`
returns the platform `iSndIsReady` byte result as U32, reproducing the original
32-byte call and unsigned-byte conversion. `xSndLoadExternalData` tailcalls its
platform counterpart, reproducing all eight bytes. Original DWARF gives the
platform return type and parameter types; existing independently captured
target relocations identify both callees. The declarations are in the PS2
platform header, with the shared wrapper declarations guarded for PS2.

Fresh complete-source reports agree in USA, Europe and Germany: 24/37 exact
functions and 4668/6764 exact bytes become 31/37 and 5516/6764, a gain of seven
functions and 848 bytes per release. Unit fuzzy matching rises from 95.61502%
to 98.3974%. Every previously matched function stays matched. France's existing
three-function profile gains IsReady's 32 bytes and reaches 3/3, 132/132.
This does not establish complete French TU ownership or whole-unit linking.

The full GameCube USA build preserves its complete progress report and passes
the retail DOL SHA-1 check. Private original/after/final unit reports and rejected
inlining probes are under `build/sound-oct08`. No comparison profile, registry,
original target, or compiler binary was changed.
