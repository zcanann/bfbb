# Xbox hexadecimal parser

`atox(const substr&, size_t&)` is independently identified at
`0x15b590..0x15b5ff` in both authenticated Xbox originals: **111 bytes**, SHA-256
`1e9df4ce7f47069f17e803da8edfd0c1c198864f10bf11140f227dc17a025e15`.
It was already a verified anonymous extent. Naming it changes neither recovered
function counts nor code-byte denominators.

The original leaf accepts the substring structure in EAX and the output-count
address in EDX. It returns zero for a null text pointer, otherwise clamps length
to eight, clears the count, recognizes the three ASCII hexadecimal ranges,
accumulates `(value << 4) + digit`, and stops at the bound or first invalid byte.
Both return paths and every local branch were checked against actual original
instructions. Its predecessor ends at `0x15b584`, followed by eleven alignment
bytes; its own final return is followed by one alignment byte.

The independently decoded call at `0x13b858` corroborates that identity. Its
caller checks a ten-character input against the literal `"0x"` at `0x26c4c4`,
advances the substring pointer by two, sets its size to eight, passes a local
output-count address, and consumes the return as an identifier. The alternate
path calls the already reviewed bounded string hash. Original body/caller hashes,
branches, register preservation and direct call are recorded in reviewed metadata.

The actual complete `xString.cpp` initially produced **88.77551%** for this body.
The original uses signed byte branches and sign extensions; using a signed `char`
local only on Xbox raises that to **99.79592%**. The sole remaining difference was
two adjacent increments. Moving `text++` into the existing `for` update after
`read_size++` recovers **100%** and all 111 untouched original bytes. This ordinary
loop form preserves parser behavior and leaves the non-Xbox local type intact.

The host reachability entry also calls this parser; it remains excluded from
coverage. The same complete source TU still emits byte-identical implementations
of all three previously compared hash functions. No compiler flags, replacement
functions, original-address imports, byte patches or scoring policy were changed.

Both normal Xbox production reports and original verification passed.
The matched-code increase is **134 -> 245 bytes** in each
region. Existing hash/particle scores, the **2,519** recovered functions and full
**1,798,760-byte** `.text` denominator remain unchanged. This is a function match,
not a complete translation unit or executable link. An actual PS2 xString compile
already preserves every allocated section and relocation record against the
prior production object; the integration checkout also verifies GameCube output.
