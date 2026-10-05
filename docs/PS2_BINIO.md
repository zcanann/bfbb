# PS2 binary-file loading

The complete shared `xbinio.cpp` now compiles against genuine PS2 file types and
API declarations. Original DWARF in all three debug-bearing executables records
`tag_iFile` as 144 bytes: flags at 0, path[128] at 4, fd at 132, offset at 136,
and length at 140. Its containing `tag_xFile` is 180 bytes with user_data at 176.
`st_FILELOADINFO` and `st_BINIO_XTRADATA` remain the existing shared 104-byte and
60-byte structures. Actual whole-TU debug output agrees on all direct member
names, offsets, and sizes; enabling debug output preserves allocated sections.

The PS2 API declarations follow original canonical linkage names and return
attributes, including the const-qualified `iFileSetPath` argument. Standard
memcpy, strcmp, and strncpy declarations use the existing authenticated PS2
size_t. No file-system implementation or unknown SDK structure is supplied.

Three platform differences have direct original-code evidence:

* PS2 creation leaves the initialized base sector at zero. Its original call
  sequence has no GameCube sector lookup; `BFD_startSector` stays on other targets.
* Async reads pass priority 0x100, visible in the original fifth argument setup.
* The original async-status switch has six explicit entries: NOOP, FAIL and
  EXPIRED return failure; INPROG and QUEUED remain in progress; DONE succeeds.
  Making NOOP explicit preserves the shared default behavior and restores this
  switch structure.

Seven file-local helper declarations are inline on PS2: Swap2, Swap4, BFD_close,
BFD_seek, BFD_getLength, SetBuffer, and LoadDestroy. Their bodies are present in
original callers. Address-taken SetBuffer and LoadDestroy retain real out-of-line
bodies; no calls or stores are discarded to make the source compile.

The normal objdiff report covers all 25 original functions / 3,308 bytes, including
three remaining holdouts (BFD_read, BFD_open, xBinioLoadCreate). Each debug version
reports 22 exact functions / 1,844 matched code bytes and 93.41475% fuzzy match.
Independently applying genuine source relocations to original destinations
reproduces 21 functions / 1,636 bytes exactly in each original. The additional
208-byte BFD_AsyncReadStatus has an unresolved local jump-table relocation, so
normal code matching does not establish its raw linked identity. This is not a
complete source-linked PS2 executable claim. France is excluded from this new
profile pending independent function and relocation evidence.

The target profile uses named original direct calls, source-qualified static GP
anchors, the previously reviewed memset entry, and the existing strict callback
HI/LO validator. It requires no backend extension or altered report policy.
All 224 actual GameCube game/engine objects retain identical allocated sections.

Private evidence is in `build/ps2binio162`: full-source command/object,
`profile.json`, `all-region-summary.json`, `raw-proof.json`, original and compiled
layout inventories, and the 224-object GC comparison. Reproduce original layouts:

```sh
PYTHONPATH=tools python -m platforms.ps2_type_layouts \
  --version SLUS-20680 --version SLES-51968 --version SLES-51970 \
  --orig-root orig --source SB/Core/x/xbinio.cpp \
  --type tag_iFile --type tag_xFile \
  --type st_FILELOADINFO --type st_BINIO_XTRADATA \
  --output build/ps2-binio-layouts.json
```
