# PS2 main startup recovery

## Memory-card startup query (2026-10-09)

`zMainMemCardSpaceQuery` now has its PS2 controller/card retry loop instead of
being excluded from the source object. The original function is 1,908 bytes in
all three debug executables: USA `0x185a70`, Europe `0x185a60`, Germany
`0x185b60`. Normal `tools/ps2solo.py zMain.cpp --version VERSION --keep` builds
improve it from absent (0%) to **96.43606%** in all three regions.

Original DWARF identifies `fullCard` as the card-check result and `status` as
the controller-state result. The original references to `globals`, `mPad`,
`gTrcPad`, and `bad_card_needed`, the error switch, and the seven text strings
were checked against the original executable data. Retry tests PS2 Triangle
bit 18, consistent with the independently checked original `iPadUpdate`
routing; this does not change the shared platform button definitions.

All nine inlined camera creations in the original query use 640 by 448 for
USA and 512 by 512 for Europe/Germany. Their pad-update intervals are 1/60 and
1/50 second respectively. The PAL controller message is
`{i:text_no_controller_pal}`; USA uses `{i:text_no_controller}`. The screen
constants and message selection are limited to the verified debug versions.
France has no enabled zMain source profile; this change makes no French
matching claim or inference about its display settings.

The existing `scePadGetState` source interface is reused. The original calls
`0x10dd50`, also called by original `iPadEnableGuts`; its runtime symbol name
has not been independently authenticated. Existing RenderWare interfaces
likewise remain source reconstructions, with no new runtime aliases or
identity evidence. Named game calls were corroborated by original DWARF.

### Instruction and regional checks

Each compiled query is 1,968 bytes versus 1,908 original bytes. Removing NOPs
leaves **430 instructions in both**. All opcode/register words agree after
masking 110 source relocation fields and relative branch displacements;
every relative branch reaches the same non-NOP instruction ordinal. Those
masked relocations are not proof of linked runtime/data identities. The
remaining 15 extra source NOPs occur around inlined render-helper setup and
control flow. Explicit aggregate zero initializers produced the same result;
an explicit memset form regressed. No padding, forced registers, compiler
patch, or compiler-version diagnosis is introduced.

Whole-unit comparisons show no regressions. Additional PAL constant fixes:

| Region | Function | Before | After |
| --- | --- | ---: | ---: |
| Europe | main | 97.17249% | 97.669% |
| Europe | zMainLoop | 97.78956% | 98.368675% |
| Germany | zMainShowProgressBar | 76.30189% | 76.971695% |

Exact byte/function totals are unchanged. Fresh GameCube
`tools/solo.py Game/zMain --top 3` remains 16/16 exact. Full source builds and
the other platform/region gates belong to integration; this local check is
not a fresh whole-project report.

Private investigation artifacts are under `build/maincard-oct09`: original
ELF/DWARF data audit `original-proof.json`, normal-unit comparisons
`after-proof.json`, and per-region `*-alignment-proof.json`. Original ELF
SHA-1 values are USA `32e3b7dda09fd8d7fcba4eb769fa17c08cea05df`, Europe
`0c3e685edc13a362d0d27a10dccb0b9698eb5597`, and Germany
`83bf81a139ea24fdb015f1419377de85b643828a`.
