# Xbox particle initializer comparison

The authenticated US and EU XBE payloads contain the same `xParInit(xPar*)`
body at `0x1499e0..0x149a40` (96 bytes, SHA-256
`db6a26ad744420fd9c7cf979db15eb7cd5ccc947c1096c9979bcb6c42e135b15`).
This promotes an existing anonymous boundary to a reviewed source identity;
it adds no recovered bytes or functions to the denominator.

The body is a contiguous 33-instruction leaf ending in `RET`. Its field stores
agree with the shared 96-byte particle layout: four float color channels receive
255.0, four byte channels receive 255, and the same position, velocity, size,
lifetime, color velocity, pointer, flag, texture-index and rotation fields are
cleared. Mode, lifespan, asset and padding remain untouched. The preceding
allocator returns at `0x1499d4`, followed by eleven `INT3` alignment bytes.
The independently decoded two-range caller at `0x14e2f0` calls the allocator at
`0x14e377`, checks its EAX result, calls this initializer at `0x14e380`, and links
that same particle into the group while updating its count/live flag. Reviewed
metadata records original hashes, the direct call, and the complete caller's
bounding bytes including its internal alignment gap. It does not infer identity
from a source match.

The actual complete `xPar.cpp` is compiled with the existing pinned MSVC/LTCG
profile. Its per-unit host entry keeps all four particle functions reachable;
only the independently reviewed initializer is extracted for comparison. The
other functions, host entry and CRT support do not become compared code. The
existing xString host entry remains the default. Separate build directories keep
each unit's actual PE, MAP and extent records. The only shared header change uses
the directly required `xVec3.h` on Xbox; the GameCube and PS2 include paths and
source bodies are unchanged.

Both production Xbox reports give the initializer **88.030304%**. Its actual
compiled 96-byte body has SHA-256
`bbbb7a5b9e99ca98346adecbdb9a5e252be67a5cf4e9cb12bcfe7ffd38d9dc85`.
Compared with retail, the color-byte stores occur later and the first two rotation
bytes are cleared separately instead of with one word store. Three private
ordinary initialization controls (color-first; color-first with array memset;
one position store then colors with chained rotation clears) did not recover the
retail body and were not retained. No particle implementation was rewritten.

Validation used both authenticated originals and the normal production command:

```powershell
python tools/platform_progress.py report --version XBOX-US --orig-dir <originals> --build-dir <private-build> --xbox-compilers <pinned-compilers> --objdiff <objdiff-cli>
python tools/platform_progress.py report --version XBOX-EU --orig-dir <originals> --build-dir <private-build> --xbox-compilers <pinned-compilers> --objdiff <objdiff-cli>
python tools/platforms/verify_xbox_reviewed.py --orig-dir <originals>
```

Against the preceding staging CI artifacts, the complete xString report unit is
identical, including its 48 code-matched bytes. The data and unresolved-text units
are identical; the anonymous unit only loses the now-named initializer. Both
regions retain 2,518 recovered functions, 627,523 recovered code bytes and the
full 1,798,760-byte `.text` denominator. Anonymous-only coverage becomes 2,514
functions / 627,276 bytes. There is no additional 100% function, whole-unit
completion or executable relink claim.
