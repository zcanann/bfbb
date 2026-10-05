# PS2 camera-tweak source comparison

The genuine complete `zCameraTweak.cpp` source now compiles for PS2 using seven existing public camera-tuning variables, factored into `zCameraTuning.h`. No camera or entity runtime layouts are reconstructed. PS2 uses its existing authenticated `size_t` declaration; other platforms retain their standard header. GameCube retains the original heavy include path.

The original twelve-function unit contains neither the typed initialization helper nor `zCameraTweak_LookPreCalc` as standalone bodies. PS2 inline declarations recover those original ownership boundaries. The typed initializer tests an unsigned-byte link count with `> 0`, reproducing the original BLEZ instruction and preserving its existing behavior.

All three debug-bearing versions report 11 of 12 functions and 1,444 of 1,684 bytes code-matched under the normal objdiff policy, with a 96.04513 fuzzy score. Every original-owned function remains in the profile. The 240-byte global initializer remains partially matched: retail incorporates Reset's state writes while the source retains its call. Marking Reset inline recovered the initializer but removed the externally required Reset body, so that control was rejected. No forced symbol or duplicate algorithm is retained.

The profile restores calls and GP accesses only where original DWARF independently supplies identities. The initializer callback HI/LO pair at offsets 28/36 additionally passes the existing intervening-register and inverse-byte checks. Array component GP accesses and stripped math-call identities remain unresolved. Standard code-match counts do not imply all address operands or data have been reconstructed, and the TU remains incomplete for source linking. France gains no inferred identities in this change.

The original and compiled layouts of `zCameraTweak`, `CameraTweak_asset`, `zCamTweak`, and `zCamTweakLook` agree across all three debug versions. A private debug compile preserves all allocated sections. All 224 GameCube source objects compile with allocated sections identical to the verified baseline.

Private reproduction and evidence live under `build/ps2camera166`: actual whole-TU compiler commands, original-derived profiles and region reports, `type-proof.json`, and `gc/inventory.json`.
