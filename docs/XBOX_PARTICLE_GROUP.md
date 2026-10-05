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
