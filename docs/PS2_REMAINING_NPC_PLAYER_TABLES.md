# PS2 remaining NPC and Player table comparisons

Three additional complete NPC source files compile with the established PS2
profile: `zNPCTypeBossSB1`, `zNPCTypeDuplotron` and `zNPCMessenger`. The only source
edit replaces four PowerPC `FABS` uses in Messenger with the existing portable
`xabs` interface. All three original Messenger bodies contain the corresponding
four `ABS.S` instructions. Actual GameCube compilation preserves all 13 allocated
sections. SB1, Duplotron and Player source bodies are unchanged.

The original debug information also assigns six animation-table functions to
`SB/Game/zEntPlayerAnimationTables.h`. Their real definitions are already included
by the complete `zEntPlayer.cpp` through `zEntPlayerAnimationTables.inl`. A fresh
compilation of the entire Player TU preserves all 1,118 allocated sections of the
previously verified Player object. No extracted functions, wrapper source or
alternate compiler flags are used.

A separate profile compares those six original header-owned functions using that
complete source compilation. It does not replace the existing 281-function Player
profile. The symbol sets are disjoint. Original addresses, sizes and canonical
linkage identities agree with the existing public report's six-function / 56,516
byte header group. The denominator and original ownership remain unchanged.

Actual results are identical across SLUS-20680, SLES-51968 and SLES-51970:

| Original source group | Functions / bytes | Normal exact | Raw exact | Fuzzy |
| --- | --- | --- | --- | --- |
| zNPCTypeBossSB1.cpp | 33 / 8,856 | 19 / 3,912 | 14 / 2,284 | 83.52394% |
| zNPCTypeDuplotron.cpp | 26 / 5,260 | 16 / 1,512 | 14 / 904 | 84.51787% |
| zNPCMessenger.cpp | 15 / 5,796 | 6 / 448 | 4 / 236 | 40.182194% |
| zEntPlayerAnimationTables.h | 6 / 56,516 | 1 / 560 | 0 / 0 | 92.902756% |

All 80 functions / 76,428 bytes remain in the comparisons. There are 42 normal
matches / 6,432 bytes and 32 independently raw-exact functions / 3,424 bytes after
real source relocations are applied to original named addresses. Normal matches
with unresolved data or runtime references are not claimed as raw/link matches.
The header group was previously entirely unpaired; its one normal match retains
unresolved literal/callback references.

All 195, 187, 181 and 257 shared aggregate names and their variants in the four
complete source TUs agree with every original, including direct bitfields. The
Player audit retains the previously documented node/curve member-name aliases
while checking their actual types and offsets. Normal/debug allocated sections
are identical. The units retain respectively 16, 1, 13 and 1 unresolved original
references. No SDK declarations or shared header bodies change.

Four SHA-gated profiles preserve all original members in the three debug regions.
France is not enabled without reviewed source ownership. No complete-unit or
executable-link claim is made.

Private `build/npc244` evidence contains complete compiler commands and objects,
normal reports, independent raw proofs, aggregate inventories, original ABS.S
records, `gc/proof.json`, `player-whole-object-identity.json`,
`header-denominator-proof.json`, `profiles.json` and `summary.json`. Original and
compiled bytes remain private.
