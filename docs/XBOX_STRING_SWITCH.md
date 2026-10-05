# Xbox string switch and parser comparison

The complete shared `xString.cpp` now retains all twelve public APIs in its
ordinary diagnostic link. Eleven functions have independently reviewed original
locations; `xStrupr` remains unassigned. Existing compiler flags, runtime support
and scoring policy are unchanged.

## Actual original evidence

`find_char` occupies `0x15b600..0x15ba00`: 1024 bytes containing 1004 reachable
instruction bytes and 20 bytes of internal alignment. Three decoded calls from
previously verified original functions reach its entry. The original checks both
substring pointers, dispatches on the character-set length, scans one through
eleven characters with unrolled byte comparisons, and uses a nested scan for
larger sets. The zero-length case returns null. These operations establish its
identity independently of adjacency or a fuzzy search ranking.

The original comparisons operate directly on bytes. The shared source's `U8`
local compared against a signed `char` pointer instead produced widened mixed-
signedness comparisons on Xbox. A normal Xbox `char` local restores the original
1024-byte instruction sequence. Other platforms retain their original declaration.

The twelve-entry table at `0x15ba00` occupies a further 48 bytes of original
`.text`. It is data outside the recovered function extent and receives no source
function credit. Every original and source table entry is decoded independently;
the ordered relative destinations are:

```text
1014, 44, 94, 151, 210, 281, 358, 444, 548, 653, 774, 896
```

The source dispatch pointer and all twelve table pointers have actual PE HIGHLOW
relocations. Normalizing the one instruction operand to the verified table symbol
makes all 1024 function bytes identical. Inverting that operand reproduces the
actual linked source and both authenticated originals exactly.

`xStrParseFloatList` occupies `0x15b420..0x15b50a` (234 bytes). Its delimiter scan,
sign handling, numeric/exponent scan, temporary terminator, floating conversion,
restored input byte and output loop establish its identity; direct-call witnesses
and a complete original CFG establish its boundary. The actual linked source is
238 bytes and remains partial. The original conversion destination `0x1bd1bd`
remains unnamed and unnormalized. The existing real host `xatof` wrapper and host
CRT remain excluded from source credit.

## Restricted switch verification

`xbox_switch.py` accepts only the explicitly selected sequence:

```text
CMP ESI,11; PUSH EBX; PUSH EDI; JA default; JMP DWORD PTR [ESI*4+table]
```

Incoming-edge checks prevent bypassing the unsigned bound check. The intervening
pushes preserve ESI and comparison flags. Every table destination and the default
must be decoded inside the same closed function CFG, and the table must follow
the recovered instruction extent. Unknown indirect transfers retain the previous
rejection behavior. Source table pointers require exact PE HIGHLOW coverage;
original table bytes and the independently recovered CFG are revalidated.

Target preparation generates `switch-tables.json` only after those checks and
binds the ordered case/default mapping to the executable identity and actual
comparison-object SHA256. Source extraction proves its own boundaries first,
then requires the same relative case/default destinations before writing a
comparison object. A changed or permuted table fails closed; a future different
code layout needs a richer correspondence proof. The code score cannot silently
hide a changed table behind the normalized dispatch pointer.

The source evidence writer also fixes an existing variable-shadowing bug: a
function with direct calls now records its own MAP linkage name, rather than the
last callee's. Extraction and scoring already used the correct function.

## Validation

Both normal Xbox reports increase **9629 -> 10653 matched bytes**. The string
unit has **10/11 exact functions**, with **2049/2283 matched bytes** and
**99.56344% fuzzy**. All nine previous string function scores and every prior
source comparison body/relocation are unchanged; the seven other compared
unit reports are identical. All eleven string bodies reconstruct their actual
linked PE, and the new exact function reconstructs both originals. The final
backend was replayed over every compiled source object, including its MAP labels
and switch ownership.

Known inventory grows by two functions /1258 bytes to **2553 functions /634859
bytes**; full section code/data denominators remain unchanged. The 48-byte switch
table stays outside source-match credit.

Both normal Xbox builds and original inverse reconstruction are checked under `build/xbox186/final` and
`build/xbox186/validation.json`. The established original verifier replays the
103 reviewed functions / 20879 bytes and 129 direct-call witnesses in each region.
No proprietary binary or synthetic test fixture is committed. This remains
partial function comparison, without a complete-TU or retail-relink claim.

```sh
python tools/platforms/verify_xbox_reviewed.py --orig-dir /private/orig
python tools/platform_progress.py report --version XBOX-US \
  --orig-dir /private/orig --build-dir build/platforms \
  --xbox-compilers /xbox-compilers --objdiff /tools/objdiff-cli --wine /usr/bin/wine
```

Repeat for `XBOX-EU`.
