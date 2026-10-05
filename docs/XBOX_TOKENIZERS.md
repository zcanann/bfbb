# Xbox tokenizer leaves

Two existing anonymously verified extents now have independently reviewed names:

| Function | Original address | Bytes | Original SHA-256 |
| --- | --- | ---: | --- |
| xStrTok(char*, const char*, char**) | 0x15b180 | 242 | 6a57223b8b1cc13b460a228ef4c94965f6bb6c3bcd95d0bb3bab146714e83f6f |
| xStrTokBuffer(const char*, const char*, void*) | 0x15b280 | 249 | 20e0ffffd9a13f1597920ef147224235da1d4bbcb7c826ab831f05b860dc31e7 |

Both originals construct a 256-bit delimiter bitmap, including the terminating
NUL, and skip leading delimiters. The first replaces the next delimiter with NUL,
stores the continuation pointer through its third argument, and returns the
in-place token. The second preserves the input, copies token bytes to `buffer+4`,
appends NUL, and stores its continuation pointer at `buffer+0`. NULL input resumes
from the saved pointer; absence of a token returns NULL.

The original caller at `0x114dbf` passes a string, the space delimiter at
`0x26b9a0`, and a stack continuation slot to `xStrTok`. It consumes token bytes,
then calls again with NULL and the same slot. Another caller at `0x142503` uses
newline/CR delimiters at `0x26c624`. The buffer caller at `0x11a460` loads a real
buffer pointer from `0x285c94`, passes delimiters space/comma/tab/newline/CR at
`0x26bfc0`, tests the returned pointer, and hashes its token bytes. Later calls
pass NULL with the same buffer. Both functions receive control in EAX and two
stack arguments, return in EAX, and leave argument cleanup to their callers.
These observed whole-program registers are not a public ABI declaration.

The extents were already independently accepted by the closed-CFG anonymous
registry. Original direct callers, local branches, saved-register pairs, final
RET and following INT3 alignment corroborate the reviewed boundaries. The US
and EU executable hashes differ but their authenticated section payloads agree.
No new denominator bytes or anonymous heuristic promotions are introduced.

Actual complete `xString.cpp` compilation initially scored 69.68421% and
87.546394%. Capturing the existing control pointer before bitmap clearing,
matching the original prologue, makes the buffer routine raw-exact. In the
ordinary tokenizer, reading `*str` directly in the scan preserves the original
byte-load boundary; returning the existing NULL/token choice as a conditional
expression recovers the original branchless pointer mask. The final tokenizer
is also raw-exact. These three ordinary source differences are Xbox-only;
non-Xbox spelling is preserved. No compiler flags, assembly, casts beyond the
existing byte-pointer access, added storage, or instruction padding were needed.

Normal production compilation and standard reports recover all **491 bytes** in
both Xbox regions. Matched code increases **464 -> 955 bytes**. All six previous
string bodies and all particle normalized bytes/relocations remain unchanged.
The final linked compiler PE supplies the untouched new bodies, with no HIGHLOW
fields in either extent; each is independently compared byte-for-byte to its
original. The full `.text` denominator remains **1,798,760 bytes**, with **2,519**
known functions and **627,572** known function bytes. This is partial shared-TU
coverage, not a whole-TU or Xbox executable relink claim.

The actual PS2 string object has identical allocated section contents, sizes,
alignments and symbol-resolved relocations. Final GameCube source build, report
and retail checksum verification run in the integration checkout. The original
metadata check remains `python tools/platforms/verify_xbox_reviewed.py`; both
ordinary `platform_progress.py report --version XBOX-US` / `XBOX-EU` paths build
and compare the complete source TU using the existing pinned compiler profile.
