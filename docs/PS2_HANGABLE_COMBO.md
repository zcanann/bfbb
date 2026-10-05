# PS2 hangable and combo source comparison

All three debug-bearing PS2 versions now compare the complete existing
zEntHangable.cpp (11 functions / 4,868 bytes) and zCombo.cpp (6 / 2,120).
Hangable matches 5 functions / 728 bytes at 66.05916% fuzzy; combo matches
4 / 408 at 91.47925%. All remaining functions remain partial comparisons.
The combined gain is 1,136 bytes / 9 functions per debug version.

Hangable needs no source changes. Combo required excluding the GC-specific
out-of-class color assignment definition from PS2: the existing PS2 iColor_tag
is a plain four-byte aggregate, exactly as original DWARF declares it, and
uses implicit assignment. No new fields or explicit assignment declarations
were added. Other platform preprocessing is unchanged.

Actual compiler debug objects and original DWARF agree on all direct member
names, offsets and sizes for fourteen hangable consumer types (including the
complete entity, hangable, frame, model, particle settings, player and globals
layouts) and nine combo/HUD types. All three original versions were checked.
Debug and production compilation produce identical ordered allocated sections.

Canonical original DWARF linkage names select every owned function. Direct
calls and GP references use independently named original anchors, and unknown
references remain explicit. The original combo update call at offset 276 has
no recovered callee identity and remains raw. No score policy changes.

Four hangable matches / 644 bytes reconstruct the original bytes after named
relocations, as do all four combo matches / 408 bytes. Hangable SetupFX's 84-byte
stock objdiff match still has four anonymous string-address pairs, so it is not
claimed as raw relocated equality. This is source comparison, without a retail
executable relink or whole-TU completion claim.

Local evidence is under build/hangable186/zEntHangable and build/combo186/zCombo:
all-region-summary.json, compiled-layouts.json, layout-comparison.json and
raw-proof.json. Parent normal GC source build, full report comparison and retail
checksum all pass unchanged. Normal platform CI regenerates the published scores.
