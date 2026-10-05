# Xbox utility source coverage

The complete shared `xutil.cpp` now compiles with the normal pinned Xbox flags.
Its only source change selects standard C headers on Xbox, as already done on
PS2. The diagnostic host retains all seven portable public APIs and links the
real `xMath.cpp` dependency. No SDK structures, substitute implementations,
compiler flags, or report metrics change.

The pinned Microsoft `msvcr71.dll` exports `isprint` (ordinal 710, RVA 0x12093).
The utility import definition uses that genuine export, avoiding unrelated
static CRT startup dependencies. Host/runtime bodies receive no source credit.

## Reviewed original coverage

| Function | Original address | Extent | Result |
| --- | --- | ---: | --- |
| `xUtil_crc_init` | `0x164400` | 175 | Exact after named relocations |
| `xUtil_crc_update` | `0x1644b0` | 61 | Exact after named relocations |
| `xUtil_yesno` | `0x1644f0` | 104 | Partial, 38.454544% |
| `xUtil_wtadjust` | `0x164560` | 296 | Exact after named relocations |

These four independently reviewed extents total 636 bytes; three reconstructed
matches contribute 532 bytes. Startup, shutdown, and tag formatting have real
compiled bodies but no assigned original identities. The unit remains partial.

CRC identity follows the original 256-entry, eight-step polynomial loop using
`0x04c11db7`, its `0xffffffff` return, and the update function's identical flag
and table accesses. The flag at `0x285bf8` is initialized to one and cleared
after generation. The table at `0x3816f0` owns 256 DWORDs. Checked instruction
witnesses preserve both original bodies and their concrete storage references.
Actual source static-data references are discovered in the named source function
through PE HIGHLOW fields; no target addresses or offsets locate source data.

Weight adjustment has a closed original CFG with four-wide and remainder loops,
absolute-value accumulation, reference/sum division, and scaling. The original
caller at `0x9e8aa` supplies EDI=array, EBX=count, and a stack reference of 1.0.
Its eight-byte internal alignment gap belongs to the independently recovered
296-byte extent; trailing alignment receives no credit.

The original yes/no body contains the already reviewed random LCG inline.
Current source calls real `xurand`, so that instruction difference remains
visible. No shape or compiler-option trial was used to hide it.

## Validation

The original verifier checks both authenticated releases, reviewed hashes,
closed CFGs, direct-call witnesses, literal values, and CRC storage witnesses.
Production comparison uses actual whole-TU linked PE output, MAP ownership,
PE HIGHLOW fields, and named DIR32/REL32 records. Reapplying those relocations
must reconstruct the compiled bodies; the three exact comparisons also
reconstruct each original byte for byte. Both production reports now show 11,185 matched bytes across 57 functions,
up 532 bytes and three functions. All eight older unit reports and comparison
bodies/relocations are identical. The full code denominator remains 1,798,760
bytes. Known boundaries gain one function/296 bytes because weight adjustment
was previously an unpromoted candidate; CRC and yes/no replace existing
anonymous identities. GC and PS2 select precisely
their previous source preprocessing branches.

Private reproducibility artifacts: `build/xbox188/` contains the twenty-TU
compile inventory, full utility compile/link logs, original/source disassembly,
caller evidence, relocation proof, final reports, and regression validation.
