# PS2 camera-tweak source comparison

The genuine complete `zCameraTweak.cpp` source now compiles for PS2 using seven existing public camera-tuning variables, factored into `zCameraTuning.h`. No camera or entity runtime layouts are reconstructed. PS2 uses its existing authenticated `size_t` declaration; other platforms retain their standard header. GameCube retains the original heavy include path.

The original twelve-function unit contains neither the typed initialization helper nor `zCameraTweak_LookPreCalc` as standalone bodies. PS2 inline declarations recover those original ownership boundaries. The typed initializer tests an unsigned-byte link count with `> 0`, reproducing the original BLEZ instruction and preserving its existing behavior.

The initial comparison of all three debug-bearing versions reported 11 of 12 functions and 1,444 of 1,684 bytes code-matched under the normal objdiff policy, with a 96.04513 fuzzy score. Every original-owned function remains in the profile. The 240-byte global initializer was partially matched: retail incorporates Reset's state writes while the source retains its call. Marking Reset inline recovered the initializer but removed the externally required Reset body, so that control was rejected. No forced symbol was retained in that initial control.

The profile restores calls and GP accesses only where original DWARF independently supplies identities. The initializer callback HI/LO pair at offsets 28/36 additionally passes the existing intervening-register and inverse-byte checks. Array component GP accesses and stripped math-call identities remain unresolved. Standard code-match counts do not imply all address operands or data have been reconstructed, and the TU remains incomplete for source linking. France gains no inferred identities in this change.

The original and compiled layouts of `zCameraTweak`, `CameraTweak_asset`, `zCamTweak`, and `zCamTweakLook` agree across all three debug versions. A private debug compile preserves all allocated sections. All 224 GameCube source objects compile with allocated sections identical to the verified baseline.

Private reproduction and evidence live under `build/ps2camera166`: actual whole-TU compiler commands, original-derived profiles and region reports, `type-proof.json`, and `gc/inventory.json`.


## Shared reset-state implementation

Original line records place Init at lines 88-92 and Reset at lines 186-195,
so moving Reset before Init is not supported by the original ordering. An
ordinary PS2-only private inline state-reset implementation now serves both
Init and the public Reset function. It performs the existing stores in their
existing order. This reconstructs useful shared implementation ownership;
it does not claim that the helper's name or spelling existed in retail.
The GameCube body and call path remain unchanged.

This preserves all twelve emitted original functions, including standalone
Reset's exact 52 bytes. Only Init changes, from 200 to the original 240 bytes;
every other allocated section is identical to the previous source object.
All three debug versions now report 12/12 functions and 1,684/1,684 code bytes
matched, a gain of 240 bytes and one function under normal objdiff policy.
France's already-reviewed Reset remains 1/1 and 52/52; no new French extent
is inferred. All four complete source objects are byte-identical. Actual
GameCube compilation preserves all six ordered allocated sections.

Independent original-address reconstruction proves ten functions /1,200 bytes
raw exact in each debug version. Init still has unresolved itan calls at
+40 and +108; Update retains unresolved icos/isin calls at +140/+176. Those
four operands are the only raw differences. Full code matching therefore
does not assert a reconstructed source link, complete data, or full binary
identity. No profile, flags, compiler, backend, or completion metadata changes.

Private evidence: build/near249/{proof.json,original-lines.json,raw/raw-proof.json,
gc/proof.json,helper/<version>/report.json}. The rejected inline-only Reset
control is preserved under candidate/SLUS-20680: it recovered Init but lost the
standalone 52-byte Reset, unlike the retained shared implementation.
