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

## PS2 splash screen

`zMainFirstScreen` previously compiled the GameCube legal-text renderer for
PS2, scoring 0% against the 576-byte original. The PS2 path now decodes the
original palette-indexed RLE splash image, converts it to a raster, draws it,
and releases its temporary image/raster. The two-frame presentation and the
five-second wait follow the original: 300 vblanks in USA, 250 in PAL.
Normal whole-unit builds score **95.173615% in all three debug regions**,
with every other reported function unchanged and no exact-count increase.
Fresh GC solo remains 16/16 exact.

DWARF directly identifies the data names, array element types, and bounds:

| Region | Function address | RlePalette (U32) | RleData (U8) | Decoded pixels |
| --- | --- | --- | --- | ---: |
| USA | 0x1861f0 | 101 at 0x408bb0 | 52247 at 0x408d50 | 640 x 448 |
| Europe | 0x1861e0 | 104 at 0x409030 | 49821 at 0x4091d0 | 512 x 512 |
| Germany | 0x1862e0 | 104 at 0x4083b0 | 51445 at 0x408550 | 512 x 512 |

The declarations refer to original data; this change does not reconstruct or
embed the asset definitions and does not establish a linked PS2 executable.
An independent original-data decoder consumed each complete stream, checked
every palette index, and produced exactly the pixel counts above. A high-bit
entry repeats its palette color `next_byte + 2` times; other entries emit one
pixel. The row-padding mask and byte counter follow the original instructions.

The original raster-render callee dispatches through standard-function slot
17 (globals offset 0x8c, table offset 0x48), consistent with `RwRasterRender`.
Push/pop context use slot 11. This rules out the initially considered
`RwRasterRenderFast` spelling; no runtime alias or proof-registry identity is
added. Remaining differences are principally decoder register allocation
and NOP placement. Signed division for row padding added extra branches;
typed-pixel and declaration-order alternatives did not recover the complete
original allocation. No compiler defect is asserted.

Private original data bounds, hashes, and decode counts are recorded in
`build/maincard-oct09/first-original-proof.json`; the normal regional
comparisons are in `first-proof.json`. France still has no enabled source
profile for this unit, and full integration gates remain separate.
