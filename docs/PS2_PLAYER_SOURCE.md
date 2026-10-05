# PS2 complete player source comparison

The complete zEntPlayer.cpp compares all 281 original functions in each debug
PS2 version: 160,892 bytes in USA and Europe, and 161,156 bytes in Germany.
Each version has 133 standard code matches / 12,736 bytes. Fuzzy matching is
72.44676% in USA/Europe and 72.33453% in Germany. All partial functions remain
represented; these results do not establish full-TU or executable linking.

The source selects the actual PS2 immediate-mode declarations. Its sound-loading
loop omits the GameCube disk polling call on PS2: all three originals have 17
direct calls and no indirect calls in player_sound_hop_load, with no disk poll.
The original zEntPlayer_UnloadSounds has six xSTUnLoadScene calls and no indirect
calls; its three GameCube iSndSceneExit calls are therefore excluded on PS2.
The resulting complete 228-byte unloading function matches normally.

PS2 iAnimSKB.h reuses the existing platform-neutral animation stream declarations.
The original header is 28 bytes (Magic 0, Flags 4, BoneCount 8, TimeCount 10,
KeyCount 12, Scale 16), with 16-byte key records. The actual PS2 TRC pad record
contains only the four-byte pad_init enum. No GameCube TRC API or sound shutdown
stub is introduced. The string header adds the standard strncmp and stricmp
declarations already present in the shipped MSL string/extras headers.

Ordinary and debug compiler objects have identical ordered allocated sections.
An independent audit checks all 257 shared concrete aggregate names, retaining
all distinct variants, across all three originals. Direct bitfield storage,
type, bit offset and width attributes agree, with the existing explicit
xNPCBasic wrapper normalization. Original-only/opaque types are not claimed
verified.

Each version supplies its own original function extents, canonical symbols,
call targets and GP references. The 183 initially unknown references remain
unassigned. Applying named source relocations reconstructs 126 functions /
10,884 bytes exactly in each original. Seven standard matches retain unresolved
literals, statics or calls and are not claimed raw equality: DefeatedCB,
JumpApexCheck, MeleeCheck, PlayerDepenQuery, PlayerHackFixBbashMiss,
PlayerLedgeInit and zEntPlayer_SNDStopStream. No fully resolved reconstruction
mismatches were found.

The actual GameCube all-source build and retail executable SHA1 check pass;
the complete progress report is unchanged. Private evidence is under
build/player224, including per-version objects/profiles/reports, original
platform-call/header proofs and gc-verification.json. Independent layout and
raw proofs are under the reviewer worktree build/player224-review.
