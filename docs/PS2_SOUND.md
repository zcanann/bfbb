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

## Counter lifetimes and stream-ready type (2026-10-09)

Five more complete bodies now match in all three debug regions: Init (264
bytes), ParentDied (64), IDIsPlaying (88), AddDelayed (128), and StreamReady
(88). This adds 632 exact bytes and five functions, reaching 36/37 functions
and 6148/6764 bytes. Fuzzy matching rises from 98.3974% to 99.09048%. Only
ProcessSoundPos remains unmatched. Every other function score is unchanged.

The three voice loops use unsigned counters, matching the original unsigned
comparisons. Init and ParentDied initialize their counters before forming the
voice pointer; unsigned types alone improved the bodies but did not reproduce
their register lifetimes. These changes preserve the exact GameCube bodies.
AddDelayed uses the original signed ascending counter on PS2 and retains the
existing countdown loop on GameCube.

StreamReady's PS2 definition now returns U8. Authenticated DWARF1 records in
all three debug executables give fundamental type 3 (unsigned char), and its
sole source caller in zTalkBox already declares that type. The old U32
definition introduced an extra unsigned-byte conversion and padding. No shared
header or caller declaration changes. GameCube retains its existing definition.

All four PS2 versions were compiled before and after. France's established
three-function subset stays exactly 132/132 bytes. The full GameCube USA build
preserves its entire progress report and passes the retail DOL SHA-1 check.
Private evidence is `build/sound-oct09`, including the original DWARF type
records, per-region reports and rejected source probes. No profile, registry,
original target or compiler binary changed.

ProcessSoundPos's switch cases now break to the existing function end instead
of returning separately. This preserves behavior and the exact GameCube body
while improving the remaining 616-byte PS2 function from 90.012985% to
91.31169% in USA, Europe and Germany. Unit fuzzy matching reaches 99.208755%;
the 36 exact functions and 6148 exact bytes are unchanged. France's established
subset remains identical. The full GameCube USA report is unchanged and its
retail DOL SHA-1 check passes. Four-region reports use the `-break.json` suffix
under `build/sound-oct09`. Several vector-temporary and half-multiply variants
scored lower and were discarded.

## Complete position processing (2026-10-09)

ProcessSoundPos now matches its complete 616-byte body in USA, Europe and
Germany. The original retains the three listener coordinates in floating-point
registers across both distance calculations and the final position addition.
The previous expression form reloaded those coordinates near the end, changing
register lifetimes throughout the body. Two PS2-only inline value-returning
helpers explicitly retain the scalar coordinates while preserving the original
vector temporary copies. The inward shift uses the original multiply by 0.5.

This changes only ProcessSoundPos: 91.31169% becomes 100%. All three debug
unit reports reach 37/37 exact functions and 6764/6764 exact bytes, up from
36/37 and 6148/6764. Unit fuzzy matching rises from 99.208755% to 100%.
France's current three-function subset remains exactly 132/132 bytes. All
three GameCube unit reports retain identical function records and measures.

Private regional-checkout evidence: `build/sound-position-source-summary.json`,
`build/sound-position-source-after`, and `build/sound-position-gc-verify`.
The discarded probes in `build/sound-position-source-probe` include cached
vectors and pointers, direct memberwise operations, and changed compound
assignments. No comparison settings, original targets, headers or compiler
binaries changed.

## PS2 voice selection source recovery (2026-10-09)

The platform-specific iSndFindFreeVoice now reports 100% for its 900-byte
body in USA, Europe, Germany and France, previously 85.28%. The locked-voice
range end derives from its begin pointer, selection sentinels initialize after
the diagnostic state write, and function-scoped `inline_intrinsics off` retains
the original two out-of-line signed absolute-value operations. The pragma resets
before the next function; no shared headers or other platform sources change.

Full debug-region iSnd reports rise from 4888/6908 exact bytes (23/25 functions)
to 5788/6908 (24/25), with fuzzy matching 94.23335% to 96.15113%. The existing
four-function French subset rises from 872/2892 exact bytes to 1772/2892,
86.22545% to 90.806366%. All other function and data records stay unchanged.
The French target is copied unchanged from the independently verified terminal
`oct09-france-math-particle-laser` cache; no new identities are registered.

Independent raw replay in each debug original agrees on 892/900 bytes after
applying original-DWARF-backed function/global relocations. The two remaining
JAL words at offsets 504 and 516 call the same unnamed runtime entry (USA
0x114b50). The source calls `abs`; that does not establish the original symbol's
identity, so both destinations remain unresolved in target metadata. This is
complete comparison matching, not a claim of a fully linked byte-identical body.

Private evidence: `build/isnd-free-final-comparison.json`,
`build/isnd-free-final-changes.json`, `build/isnd-free-raw-proof.json`, and
`build/isnd-locals.py`. GameCube and Xbox use separate platform sound sources.

## PS2 playback stream lifetimes (2026-10-09)

The remaining iSndPlay body improves from 76.26071% to 87.63571% in all
four versions. Original DWARF records contain two branch-specific file locals;
the source now scopes those pointers and the stream-only scalar locals to their
uses. Interleaved playback retains the block size across the diagnostic write,
then uses it for the track LSN, block size and skip count. Non-interleaved
playback selects its loop flag before that write, matching the retail order.
The selected flags and the arguments passed to each playback operation retain
their existing values.

Full debug-region unit fuzzy matching rises from 96.15113% to 97.99537%;
the four-function French subset rises from 90.806366% to 95.21162%. Exact
counts remain 5788/6908 and 1772/2892 respectively. Every other function and
data record stays unchanged, including the newly exact voice selector. No
comparison settings, original metadata, headers or compiler binaries change.

The original and compiled body still differ substantially in volume-copy
lifetimes: the compiler preserves a pointer to nvol.volR across playback calls,
adding a saved register and stack space. Separate assignments, field copies,
reference/value copy helpers, temporary objects, and alternate pitch/stream
lifetimes did not resolve this. The later 3.0.1b74 compiler also preserves this
pointer and regresses the full function; it does not provide evidence for a
compiler patch. A 2.4 comparison used its supported auto,deferred inlining mode
because bottomup is unavailable there. None of these diagnostic compilers or
copy-helper probes were retained.

Private evidence: `build/isnd-play-final-comparison.json`,
`build/isnd-play-final-changes.json`, `build/isnd-locals.py`, and
`build/isnd-compiler-{deferred-,}mwcps2-*`. This remains a source improvement,
not an exact or fully linked-body claim. Other platforms select different files.
