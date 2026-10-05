# Complete PS2 Dutchman source comparison

Two PS2-only includes provide the actual streaming and immediate-mode rendering
APIs used by `zNPCTypeDutchman.cpp`: `xstransvc.h` and `rwim3d.h`. The complete
shared translation unit compiles with pinned MW PS2 3.0b38 and the existing flags.
No function body, platform structure, or compiler setting changes.

The original-derived profiles retain all 98 owned functions in each region:

| Original | Original code bytes | Standard code matches | Fuzzy similarity |
| --- | --- | --- | --- |
| SLUS-20680 | 59,088 | 20 / 2,624 bytes | 62.169983% |
| SLES-51968 | 59,076 | 20 / 2,624 bytes | 62.07712% |
| SLES-51970 | 59,076 | 20 / 2,624 bytes | 62.07712% |

The USA call offsets cannot be reused in the European originals: 15 profiled call
sites shift, including sites in Reset and add_blast_effects. Strict inverse checks
caught this before integration. Each region's complete call/GP profile was rebuilt
from its own authenticated original instructions and independently named original
symbols. The European symbolic profiles agree exactly and share one SHA-scoped
profile; USA uses another. Passing both together through production preparation
reproduces the individually verified targets byte-for-byte. No validation rule
was loosened and no source object supplies an original address or identity.

Independent application of actual source ELF relocations reconstructs 18 functions
/ 792 bytes exactly in each original. AnimPick and the animation-table builder are
standard code matches with unresolved local/literal addresses; their remaining
1,832 bytes are not claimed raw-reconstructed or linked. Each original profile
also retains 39 unresolved scanned references. Standard objdiff policy remains
unchanged, and every partial original function stays represented.

The normal and `-g` objects have identical allocated sections. A full set-valued
layout audit verifies all 215 shared named concrete aggregate variants against
all three originals, including repeated nested names such as tri_data. Direct
bitfields additionally agree in storage offsets, widths and fundamental types;
xNPCBasic's original-backed wrappers are compared in their flattened form. Opaque
or original-only declarations outside the shared set are not claimed verified.

An actual isolated GameCube compile using the existing production command yields
the exact same complete object as the parent baseline, SHA-256
`de01beecdad65fe00f054fd97d7dcb46b0d528740ba7026b26e61d97385157c2`.
All seven allocated sections match in order. Removing only the inactive PS2 include
block also reproduces the baseline source exactly. The Xbox preprocessing branch
is likewise unchanged; no Xbox comparison/profile is introduced here.

Private evidence lives in `build/dutchman218-review`: `validation.json`,
`source-scope.json`, `gc/verification.json`, `fixed/profiles.json`, regional
`profile.json` / `report.json` / `unknown-relocations.json`,
`fixed/*-layout-inventory.json`, and `fixed/raw-proof.json`. The two additive
profiles are handed to parent integration separately. No whole-unit completion,
France recovery, or full PS2 link is claimed.
