# PS2 zUI source comparison

The complete `SB/Game/zUI.cpp` translation unit now selects the existing PS2
standard library, immediate-mode 2D and render-state headers. Its function bodies
are unchanged. `RwIm2DVertex` is the established SDK Sky vertex type; no GameCube
vertex alias, synthetic layout or SDK implementation is introduced.

Actual complete-unit results for SLUS-20680, SLES-51968 and SLES-51970 are:

- All 28 original functions / 14,620 bytes are included.
- 11 normal code matches / 2,144 bytes, with 66.93269% fuzzy similarity.
- 9 functions / 960 bytes independently reproduce the original instructions after
  real ELF relocations are applied to original named addresses. The additional
  normal matches `zUI_PortalToKrabs` and `zUI_PreUpdate` retain unresolved literal
  or `strcpy` references and are not claimed as raw/link matches.
- All 131 shared aggregate names and their variants, including direct bitfields,
  agree with all three original debug files. Normal and debug source objects have
  identical ordered allocated sections.
- The actual GameCube production compilation preserves all 13 allocated sections.

All original functions remain in the report, including partial matches. There are
29 unresolved original references per region; no identities are guessed. Two
SHA-gated profiles preserve the actual regional helper-address aliases. France
is not enabled without reviewed source ownership. No complete-unit or executable
link claim is made.

Private evidence in `build/game236` contains complete compiler commands and
objects, all-region normal reports, raw relocation proofs, aggregate inventories,
`gc/proof.json` and `profiles.json`. Original and compiled bytes remain private.
