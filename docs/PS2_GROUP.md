# PS2 group source

The actual complete `xGroup.cpp` now compiles using the verified entity data
header and existing scene-lookup/event declarations. It does not require the
full renderer, collision implementation, or scene/global layouts. Original and
compiled xGroup (32 bytes) and xGroupAsset (12 bytes) member layouts agree in
all three debug PS2 versions.

Original caller instructions establish three PS2 inline helper boundaries:
the typed initializer is embedded in the public void-pointer initializer,
FindItemPtr's ID lookup is embedded in setup, and Reset's base reset and index
store are embedded in the event callback. Their existing bodies are preserved.
The public GetCount and Setup functions remain emitted for other consumers.

At the PS2 initialization/setup sites, reading asset->itemCount directly restores
the original embedded accessor operations while preserving the separately
exported GetCount function. Setup uses the single loop index recorded in original
DWARF, replacing two equivalent counters on that platform. GameCube retains its
existing accessor calls and loop spelling. The zero-count guard in get_any is
written <=0, preserving behavior for its U16 count while reproducing the original
integer-promotion branch; this produces unchanged GameCube instructions.

All nine original functions / 1,352 bytes remain represented. Seven functions /
440 bytes match in SLUS-20680, SLES-51968 and SLES-51970, with whole-unit fuzzy
matching 91.81065%. Independently applying actual source J/JAL, GP and callback
HI16/LO16 relocations to original addresses reproduces every one of those 440
bytes in each original. GetItemPtr and EventCB remain unmatched. No complete TU
or source-linked PS2 executable claim is made; France is excluded pending its
own reviewed identities.

The final actual GameCube xGroup object retains the identical ordered list of
allocated sections. The entity header dependency was separately checked across
all 224 game/engine objects. Final PS2 compilation after restoring GameCube's
accessor spelling reproduced the complete previously verified PS2 object hash.

Private evidence is in `build/ps2group168`: original call inventory, full-source
command/object, `profile.json`, three normal reports, `raw-proof.json`, original
and compiled group layouts, and the GC object comparison. No reporting/backend
extension or compiler flag change is required.

## Sequential event dispatch source recovery

Reading `asset->itemCount` into a local before the sequential index calculation
restores the original operand order for unsigned remainder. The complete source
object now matches all nine functions and 1,352 function bytes in USA, PAL, and
Germany. The 712-byte event callback is newly exact, increasing the immediate
baseline from 640 to 1,352 exact bytes per version.

Raw comparison against each original ELF reproduces every callback byte after
applying its five actual `R_MIPS_26` relocations: one base reset, one random
selection, and three event-dispatch calls. Every GU4Y78 GameCube function score
remains unchanged, with 12 exact functions / 1,232 bytes. The source expression
and arithmetic behavior remain equivalent; no compiler or scoring options change.

France currently profiles only the independently identified item accessor, so
this source improvement makes no additional French coverage claim. Function
matches also do not establish a fully linked retail translation unit. Private
validation artifacts are `build/group-raw-proof.json`, `build/groupproof.py`,
`build/group-gc-{before,after}.json`, and retained complete `ps2solo` objects in
the PS2 agent worktree.
