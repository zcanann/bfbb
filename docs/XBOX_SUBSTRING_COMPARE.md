# Xbox substring comparison and reviewed direct calls

The already verified anonymous function at `0x15b550` is
`icompare(const substr&, const substr&)`, **53 bytes**, SHA-256
`c947b0d7e14316b7f117475c8788b74c11c4c3bb3bb68b023452f33723e2abb0`.
Its body loads both substring lengths, selects their minimum, passes the two
string pointers and that count to independently reviewed `imemcmp` at `0x15b510`,
then preserves nonzero comparison results or orders the lengths. The existing
anonymous registry supplies closed control flow and 13 direct call sites from
five callers. At `0x10da67`, an actual caller constructs a pointer/length pair
for five bytes at `0x26b580`, passes another real substring, calls at `0x10da80`,
and tests equality before selecting a game value. Other callers similarly use
constant substring pairs. These identify the routine independently of compilation.

The original receives s1 in EDX and s2 in ECX, saves EBP/ESI/EBX/EDI, and returns
its signed result in EAX. Its internal call pushes the comparison length and
removes that argument afterward. These are observed whole-program allocations,
not a claimed fixed public register ABI. Both return paths and trailing INT3
alignment independently bound the 53-byte body.

The first actual complete `xString.cpp` compilation emitted 59 bytes. Only the
unequal-length result differed: separate assignments and a branch versus retail's
`SBB / AND -2 / INC`. The ordinary equivalent
`result = s1.size < s2.size ? -1 : 1` reproduces the original sequence under the
unchanged pinned compiler profile. This spelling is Xbox-only; the non-Xbox source
is preserved. No additional source variants, compiler flags or assembly were used.

This is the first compared Xbox function containing a direct call. The pipeline
now supports explicit reviewed E8 calls, using ordinary i386 COFF REL32 records.
The original operand at function offset24 decodes to the independently reviewed
`imemcmp` entry. The source operand is discovered by its decoded CFG, not copied
from the original offset, and must resolve to the actual named same-TU function
in the compiler's linked MAP. Unknown or indirect calls remain rejected. The
existing no-call profiles retain their default behavior.

Only each proven four-byte displacement becomes a zero-addend REL32 relocation.
The target and source reconstruction checks independently recover their exact
original linked bytes. The comparison COFF references the defined callee symbol
when that function is in the same object. Existing DIR32 global-address records
are unchanged. The section-report adapter restores the target call and compares
it to authenticated original `.text`; its exact-match check also requires equal
source/target bytes and named relocation records. Standard objdiff scoring and
the original executable denominator are unchanged.

Both Xbox production reports gain **53 bytes**, increasing matched code
**955 -> 1008**, with all eight previous string bodies and particle bytes/records
unchanged. The source and original comparison reconstruct exactly in both regions;
the final source also happens to be raw-identical because the real callee lies
64 bytes before the caller in both layouts. Named-call verification preserves
that identity independently of this coincident relative placement. The already-known
anonymous extent is promoted, so the **2,519 functions**, **627,572 known function
bytes** and **1,798,760-byte .text denominator** do not change. This remains partial
source coverage, with no whole-TU or executable completion claim.

The actual PS2 string object retains every allocated section's bytes, size and
alignment and all symbol-resolved relocations. Final GameCube source build,
report and retail checksum verification belong to the integration checkout.
The existing `verify_xbox_reviewed.py` command now checks this named original
call destination as part of its actual-original verification; both normal
`platform_progress.py report` Xbox paths compile the complete real source TU.
