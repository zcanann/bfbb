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


## Ordinary inline particle helpers

The original Register and KillPar bodies contain the complete existing private
initialization and particle-list operations, where the previous source emitted
calls. Xbox now exposes the unchanged AddParP, AddParToDeadList and xParFree
bodies as ordinary header inlines, and marks the existing file-static
RegisterInit inline. The real gParDead definition stays in xPar.cpp; its header
reference is a declaration. xParAlloc stays out of line and retains its existing
exact source comparison. No artificial caller, compiler flag, storage, or helper
implementation was introduced.

Both complete production reports retain every compared function and show:

| Function | Original bytes | Before | After |
| --- | ---: | ---: | ---: |
| xParGroupRegister | 85 | 52.692307% | 100% |
| xParGroupKillPar | 137 | 40.166668% | 100% |
| xParGroupAddPar | 209 | 51.566265% | 72.83132% |

The actual emitted exact bodies belong to group.obj. All 222 bytes reconstruct
both authenticated originals after only six and two actual PE HIGHLOW fields,
respectively. Registration's static flag and count are independently selected
from its unique source A1 and A3 uses; the count is not represented as an alias
or one-past pointer to the flag. No direct calls remain in either exact body.

This exposed one missing annotation in the existing original registration
metadata: the ten-byte C7 /0 instruction at function offset 24 stores 1 to the
already reviewed sParGroupRegTableInit at 0x373f64. Its displacement at offset 26
is now restored using that anchor. The stored value and all other instruction
bits are preserved. Both authenticated originals have the same instruction;
the source has an actual HIGHLOW at the corresponding operand. The strict
exporter rejected the incomplete old annotation before accepting this corrected
comparison. No backend or score-rule change was needed. All other original
records and function bounds remain unchanged; anonymous metadata changes only
the digest of the reviewed registry.

Matched totals rise from 13,070 / 67 to **13,292 bytes / 69 functions** per
release. Every other report unit/function, all coverage denominators, and all
completion fields remain unchanged. The complete group unit remains partial;
no executable relink is claimed. Sixteen ordered allocated sections across
GameCube xPar, xParGroup and xParCmd are byte-identical to their prior builds.
PS2 and GameCube retain their previous non-Xbox definitions.

The normal report commands above reproduce the result. Ignored local evidence
is under build/xbox238: final-verification.json, original-store-proof.json,
gc/proof.json, and both production reports with actual PE/MAP/source extents.
No original or compiler binary is committed.


## Allocator visibility and reconstructed reporting ownership

A separate unchanged-body inline control recovers the remaining AddPar body.
The original first allocation is embedded directly in AddPar, while its
reclamation path calls the independently reviewed 37-byte allocator at
0x1499b0. An ordinary Xbox header-inline xParAlloc reproduces that same choice:
the actual complete xParGroup.cpp build naturally emits the 37-byte helper in
group.obj and calls it once from AddPar. No host-entry call, forced emission,
additional dependency, compiler flag, or implementation stub was added.

The standalone xPar.cpp link no longer emits that inline helper. Its original
reviewed source=xPar.cpp assignment came from the shared reference definition,
not Xbox debug metadata. The original linkage name and original definition/header
TU remain unknown. Following the existing xatan2 reporting precedent, only the
allocator's reconstructed reporting group changes to xParGroup.cpp. Explicit
metadata records the reference definition in xPar.cpp, the reconstructed inline
body in xPar.h, and actual emitted group.obj owner. This is not a claim about
original TU ownership.

The profile moves the same canonical xParAlloc() comparison and its existing
two data operands exactly once from the xPar.cpp report to the
xParGroup.cpp source build. The strict main-source-object check is unchanged:
all six compared bodies must actually belong to group.obj. Both original
registries preserve every one of their 114 reviewed identities, addresses,
sizes, original-byte hashes, call witnesses and operand proofs. The only
registry change is that documented reporting source/provenance and its derived
symbol entry/digest. No backend change was needed.

Both complete reports preserve the old exact allocator and raise AddPar from
72.83132% to **100% / 209 bytes**. Independently reconstructing both authenticated
originals from the actual linked source proves all 37 allocator bytes after two
real PE HIGHLOW operands, and all 209 AddPar bytes after four HIGHLOW operands
and its three named direct calls. The original and source both keep the single
reclamation-path allocator call. The first pop/clear sequence is inline in both.

Totals become **13,501 matched bytes / 70 functions** per Xbox release. All
2,556 known functions remain present exactly once and every other function score
is unchanged. The original 1,798,760-byte code denominator and completion fields
are unchanged. Regenerated COFF offsets shift only inside the two affected
reporting units; original executable addresses do not move. All six reviewed
functions / 880 bytes in the reconstructed group unit match, but this does not
establish original TU completeness or a full executable relink. Sixteen ordered
GameCube allocated sections remain byte-identical.

Ignored evidence is under build/xbox240: header-owner-proof.json,
final-verification.json, gc/proof.json, and both production PE/MAP/extents/reports.
The earlier build/xbox238 evidence is preserved separately.

## KillSlow root acquisition

The original `xParCmdKillSlow_Update` first reads its command, computes the
speed threshold and tests `kill_less_than`, then acquires the group's root
pointer for the two particle loops. The shared source acquired that pointer
before reading the command. Both loop bodies already matched the original;
the differing entry order also changed the reused stack argument slot.

For Xbox only, the existing root-pointer initialization now follows the speed
threshold calculation. No intervening calls or writes occur, and predicates,
loop bodies, field stores and non-Xbox ordering are unchanged. This single
original-supported control improves 93.53125% to 100%.

Both complete production reports add 164 exact bytes and one function, reaching
14,099 bytes / 77 functions from this branch's prior baseline. The actual
164-byte source extent is independently closed and owned by `xParCmd.obj`.
Every byte equals both authenticated originals directly: no calls, address
expressions, HIGHLOW fields or normalization are involved. All other function
records and all denominator/completion fields are unchanged apart from generated
COFF offsets. All 33 ordered GameCube consumer sections remain byte-identical.
No profile, original metadata, compiler settings or backend changes are needed.
This completes another reviewed callback, not the original whole TU.

Ignored reproducible evidence: `build/xbox268/{inventory-disasm.txt,compile.py,
compare.py,consumers,production,verify_final.py,final-verification.json,
gc/proof.json}`.

## Age root acquisition

The sibling Age callback provides the same independent entry-order evidence:
the original reads its command and asset before the group pointer, then reuses
the command argument's stack slot for the scaled aging rate. The source used
the opposite acquisition order, while its complete particle loop already
matched. For Xbox only, the root-pointer initialization now follows `age_rate`.
There are no intervening calls or writes; field expressions, linked-list walk,
store order and non-Xbox behavior are unchanged.

This one control raises `xParCmdAge_Update` from 98% to 100%, adding 49 exact
bytes and one function. The independently closed 49-byte actual `xParCmd.obj`
body equals both authenticated originals literally, without relocations or any
normalization. Both full reports preserve every other function record and all
denominator/completion fields apart from generated COFF offsets. All 33 ordered
GameCube consumer sections remain identical. No profile or metadata changes
are needed; source-group and original-TU completeness are not claimed.

Ignored evidence: `build/xbox270/{inventory-disasm.txt,compile.py,compare.py,
consumers,production,verify_final.py,final-verification.json,gc/proof.json}`.

## ApplyWind vector-local reconstruction

The original Xbox ApplyWind reserves a 12-byte local frame, stores its computed
magnitude at offsets 0 and 8, and reloads those two fields separately through the
particle loop. The former scalar locals coalesced into one reused argument slot.
An Xbox-only ordinary `xVec3 wind` local restores the observed field spacing and
independent loads. Only x/z are initialized and used; there is no artificial y
write, padding, volatile access or changed arithmetic. The vector type is a
source inference from that layout, not a recovered Xbox debug declaration.
PS2 debug locals retain only the particle pointer and do not prove the type.

Both complete Xbox reports improve ApplyWind from **66.40909% to 99.09091%**,
with a genuine **63-byte `xParCmd.obj` body**, equal to the original extent.
The remaining operand differences are retained honestly. All other function
scores, prior **14148 exact bytes / 78 exact functions**, denominators and
completion fields stay unchanged. Seven actual consumer builds pass; all 33
ordered GameCube allocated sections remain byte-identical. The other platforms
keep the prior scalar source. No headers, profiles, compiler flags or reporting
rules change. Private proof and full reports are under `build/xbox288/`.
