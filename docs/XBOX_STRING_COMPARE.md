# Xbox string-comparison leaves and caller cleanup

Two more existing anonymous extents now have independently reviewed identities:

| Function | Original address | Bytes | Raw original SHA-256 |
| --- | --- | ---: | --- |
| xStricmp(const char*, const char*) | 0x15b380 | 158 | c1f765998d6091d3d76228e29a5755324033d39babc247e0671ceb95c819f16b |
| imemcmp(const void*, const void*, size_t) | 0x15b510 | 61 | a80be96c6ee14609a2de3d77a2b2e2bca925966570d3d27d58be1c2e6df537e1 |

The original `xStricmp` is a contiguous leaf that folds ASCII a-z to uppercase,
advances two NUL-terminated strings while folded bytes agree, and returns zero or
signed ordering. Real callers pass string pointers and test its zero result when
selecting game values. Its instructions already match the complete shared
`xString.cpp` without any source-body correction.

The original `imemcmp` sign-extends bytes, applies the game's existing
`c | ((c >> 1) & 32)` folding helper, and returns the first difference or zero
after the specified count. It is not substituted with a locale-aware CRT routine.
Its original caller at `0x15b550` chooses the shorter of two substring lengths,
passes their pointers in EBX/EDI and pushes the length before calling at
`0x15b567`. Other independently decoded callers use the same bounded comparison.
The reviewed registry records original hashes, local branches, register saves,
direct calls and result consumption. Both identities were investigated before
compiler comparison; neither adds a new denominator extent.

The first real complete-TU compile emitted calls to the existing anonymous
`tolower` overloads. Xbox-only inline declarations recover the actual inlined
bit-folding sequence under the existing `/Ob1` setting. No helper implementation,
lookup table, assembly or compiler binary was invented or replaced.

The remaining difference exposed a concrete profile error: both `imemcmp` exits
were `RET 4`, while retail has `RET` and its callers explicitly execute
`ADD ESP, 4`. The bounded string hash has the same caller-cleanup evidence.
The old profile's guessed `/Gr` selected callee cleanup. A single standard
`/Gd` control produced the correct cleanup, recovering all **61 bytes** of
`imemcmp`, and improved the bounded hash **83.57143% -> 85.71429%**, shrinking it
from 57 to the original 55 bytes. Actual MAP decorated names change from `YI`
to `YA`; the profile uses those emitted names. Whole-program argument registers
remain compiler-selected, so this does not assert a conventional register ABI
or uniquely establish the original compiler version.

Before adopting `/Gd`, all previously profiled code was checked. The unbounded
hash and hexadecimal parser remain raw-exact; all three particle functions have
identical normalized bytes and named relocations; the concatenating hash score
is unchanged. The new `xStricmp` is also byte-exact. Both complete source TUs and
their genuine host contexts are compiled with the same candidate profile.

Both authenticated Xbox production reports passed and increase matched code
**245 -> 464 bytes** (+219), with no previous match lost. The **2,519** recovered
functions, **627,572** known function bytes and full **1,798,760-byte** `.text`
denominator remain unchanged. These are two additional function matches, not
whole-TU or executable-link completion. An actual PS2 string compile preserves
all allocated sections and relocation records; final GameCube checks run in the
integration checkout. The normal report policy and relocation checks are unchanged.
