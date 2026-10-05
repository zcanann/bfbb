# Xbox particle-group source coverage and genuine CRT support

Complete `xParGroup.cpp` and its real `xPar.cpp` dependency now compile and link
under the unchanged pinned MSVC profile. Five independently identified original
functions enter normal partial source comparison in both Xbox releases:

| Function | Original address | Original bytes | Code match |
| --- | --- | ---: | ---: |
| Register | 0x14e0e0 | 85 | 52.692307% |
| Unregister | 0x14e140 | 51 | 100% |
| Animate | 0x14e180 | 361 | 100% |
| AddPar | 0x14e2f0 | 209 | 51.566265% |
| KillPar | 0x14e3d0 | 137 | 40.166668% |

This raises matched code **1111 -> 1523 bytes (+412)** in each region. All prior
string and particle unit reports, source bodies, and named relocations remain
identical. No game source, headers, flags, or scoring policy change. Both full
section denominators remain unchanged. Three previously unclassified entries
become known functions; other entries replace existing anonymous identities.
The known inventory is now 2522 functions / 627917 bytes. No complete TU or
executable relink is claimed; source methods inlined into unrelated callers are
not invented as standalone originals.

The original allocator/initializer calls anchor AddPar. Its lower-priority
reclamation calls identify KillPar; Animate calls the same killer and integrates
four independently recognizable particle color channels, size, and lifetime.
Register zeroes 255 DWORD slots, initializes its state, and stores a byte index;
Unregister searches that same table. Checked caller addresses, terminal returns,
branch targets, hashes, alignment, and identical regional payloads are recorded
in reviewed metadata. The 0.0 and 255.0 literals are checked from actual original
bytes, not inferred from a fuzzy score.

The two exact functions independently reconstruct both authenticated original
bytes and actual linked source PE bytes. Only 20 genuine absolute operands and
five direct call operands are normalized. Source absolute fields must exactly
cover PE HIGHLOW records. Names and object ownership come from the actual MAP.
The file-static initialization scalar is absent from MAP data names, so a unique
A1 load in the named Register source function, also backed by HIGHLOW, supplies
its source-only address witness. Repeated identical literal loads use explicit
counts; no operand is silently dropped.

Partial Register/AddPar/KillPar remain compared. Retail inlines helpers that this
source context emits separately. Actual named source-only helper calls remain
visible; they do not assert invented original function extents. KillPar's two
E9 tail transfers are independently decoded to real source functions. The
existing source CFG extractor now accepts explicitly named E9 exits alongside
E8 calls, with inverse displacement checks. Unknown and indirect transfers
still fail closed.

## Runtime provenance and provisioning

The public [Microsoft Visual C++ Toolkit 2003 announcement](https://devblogs.microsoft.com/buckh/download-the-vc-toolkit/)
identifies the historical toolkit. Its archived installer is pinned by SHA256
`03aad135c22e953e0928b118705338afdbd08abf8e4039038ef77945504e65fa`.
Windows Authenticode verified the Microsoft Corporation signature, certificate
thumbprint `2A1049B2557DE78CF6592BF68504E23C91ADBF8C`.

The MSI File table identifies cabinet member
`_52534B1E423F4D6BAF1F10C5B396DE4D` as `libcmt.lib` (2937240 bytes).
Its SHA256 is `780aa4cbe614efeeb72e3bb9538230f5cd37caeb0adc5d70c346b0dda19ba3a5`.
The archive linker index defines `__ftol2` through the genuine `ftol2.obj` member.
Its complete 117-byte linked body equals the original helper at `0x1bd670` in
both releases, independently corroborating Animate's four conversion calls.
This runtime body is a named target/callee, **not a reconstructed source gain**.
No helper alias, handwritten implementation, original-address import, or stub
is used.

Download the profile's pinned archive to private build storage and run:

```sh
python tools/platforms/extract_xbox_runtime.py VCToolkitSetup.exe \
  --compilers /opt/compilers --sevenzip 7z
```

The extractor never executes an installer. It verifies the archive, decodes the
single InstallShield MSI member, extracts its CAB with full 7-Zip, verifies the
library hash, and places it in `msvc7.1-13.10.3077/Lib/libcmt.lib`. The container
transform follows the public [ISx format implementation](https://github.com/Coldblackice/InstallShield-installer-extractor-ISx/blob/098e866fa5341db4424d3831d40943c01b88aefe/ISx.c).
Only metadata and extraction code are public; proprietary binaries stay in the
private build image. Actual provisioning and both complete production report
runs validate this path.

## Particle-command callbacks and shared math enablement

The next batch compiles complete `xParCmd.cpp`, `xMath.cpp`, `xMath3.cpp`,
`xVec3.cpp`, and the existing complete `gc/iMath.cpp`/`gc/iMath3.cpp`
implementations. Xbox-only guards restore ordinary scalar header inlines from
existing definitions, select standard compiler math headers, and exclude PPC
intrinsics. Original `xMat3x3LookVec` calls normalization at `0x15cf00`; the
normalizer performs an actual x87 `FSQRT` at `0x15cf83`, corroborating the Xbox
`sqrtf` helper. The new Xbox `iMath3.h` contains genuine shared API declarations
and an opaque matrix union; no Xbox SDK layout is invented.

The original initializer at `0x149a40` contains 721 bytes of straight-line
constant stores. It writes command IDs, asset sizes, and callback addresses into
a 35-slot table at `0x373dc0` (12 bytes per slot). These stores independently
identify all 21 nonempty registered callbacks. `xbox_particle_commands.py`
replays the constant register/store dataflow and checks each original entry,
then follows the callback's closed direct CFG to its recorded terminal return.
It does not consume a compiled object. The shared empty callback aliases at
`0x0bcd00` are not counted repeatedly or assigned speculative exclusive ownership.

All 21 callbacks (4771 original bytes) enter normal comparison, including the
13 partial matches. Eight independently reconstruct the original and actual
linked source bytes exactly:

| Callback | Original address | Bytes |
| --- | --- | ---: |
| Accelerate | 0x14a070 | 119 |
| Move | 0x14a0f0 | 119 |
| VelocityApply | 0x14a630 | 50 |
| CollideFall | 0x14a9f0 | 83 |
| CollideFallSticky | 0x14aa50 | 118 |
| SizeInOut | 0x14ab30 | 332 |
| AlphaInOut | 0x14ac80 | 386 |
| Shaper | 0x14ae10 | 624 |

Both regions increase **1523 -> 3354 matched bytes (+1831)**. The original
inventory gains 21 named functions / 4771 bytes, reaching 2543 functions /
632688 known bytes, while the full **1798760-byte code denominator** and data
denominator stay unchanged. Existing xString/xPar/xParGroup report units and
all their normalized source bytes/named relocations are identical. Every new
source comparison inversely reproduces its actual linked PE bytes. The exact
eight also reproduce both authenticated originals; only 79 independently
verified literal operands and three known `__ftol2` call operands are restored.
Other original math callees retain raw operands rather than inferred identities.
No compiler flags, scoring policy, TU completion, or executable relink claim changes.

The per-unit host context uses real exports from the already pinned
`msvcr71.dll` for math calls; `link /dump /exports` verified the `_CI*` and C math
names in `math_crt.def`. This prevents pulling Windows process startup merely
to satisfy genuine CRT math dependencies. The actual `xMath.cpp` supplies
`xatof`, so this unit omits the existing diagnostic duplicate wrapper. Pinned
`libcmt.lib` still supplies the unchanged exact `__ftol2`; runtime and host
support bytes are excluded from source gains.

Core out-of-line copies in xBound/xCollide/xClimate/xAnim/xCamera and camera/HUD
inlines are guarded against duplicate Xbox definitions. GC and PS2 preprocessing
paths retain their existing definitions. Additional, currently unsupported game
TUs with private copies of these helpers need the same ownership reconciliation
when enabled; this batch does not edit parked game targets.

Reproduce the original evidence and normal source reports with:

```sh
python tools/platforms/verify_xbox_reviewed.py --orig-dir /private/orig
python tools/platform_progress.py report --version XBOX-US \
  --orig-dir /private/orig --build-dir build/platforms \
  --xbox-compilers /xbox-compilers --objdiff /tools/objdiff-cli --wine /usr/bin/wine
```

Repeat the report for `XBOX-EU`. Local actual-production artifacts and independent
source/original inverse checks are under `build/xbox172/production` and
`build/xbox172/validation.json`; no binary artifacts are committed.
