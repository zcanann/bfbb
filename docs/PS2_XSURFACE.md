# PS2 xSurface source comparison

The existing whole `xSurface.cpp` compiles without source/header changes using
the established MW PS2 3.0 build 38 compiler. The seven retail DWARF functions
occupy 436 bytes in each debug release. Six functions (80 bytes) reproduce the
complete original bytes after applying independently verified relocations.
`xSurfaceInit` remains 356 bytes and 95.44944% fuzzy matched; this is not a whole
unit or executable-link completion claim.

Retail Save and Load each consist of `j xBaseSave/Load; nop`. The existing
compiler profile left C++ exceptions enabled (the compiler's documented default),
which generated 28-byte call wrappers. The single `-Cpp_exceptions off` control
recovers both exact eight-byte tail wrappers. Tail-call optimization itself is
already enabled by optimization level 3/4. Explicit void returns did not recover
them with exceptions enabled.

Before changing the shared profile, every one of the 29 existing profile
functions was compiled under both exception settings: instruction bytes and
relocation references were identical, including the existing xordarray miss.
The independently validated xBehaveGoalSimple profile also retains all 680 exact
bytes with exceptions disabled. This is a normal compiler option, not a binary
patch or source workaround.

The direct-transfer profile now accepts an explicit J opcode (2), defaulting to
the existing JAL opcode (3). Validation still requires the actual instruction to
have the selected opcode, its decoded destination including PC high bits to equal
an independently named original function, and inverse encoding to reproduce the
original word. The two tail jumps and the allocation JAL are proved separately.
All GP accesses to `surfs`, `nsurfs` and `gActiveHeap` resolve through the original
.reginfo GP and independently named DWARF data-address anchors. No data-layout
completion or invented extent is claimed.

After applying every actual source relocation, Init differs in exactly eight
instructions at offsets 0x04 through 0x20. Retail tests the original input before
masking it, stores the count in the branch delay slot, then prepares allocation
arguments. Source masks the parameter before the test and orders those operations
differently. Every instruction from 0x24 through the final 0x160 is byte-identical,
including the complete unrolled index loop. Stored-count conditions and restoring
DWARF's file-local global linkage were neutral; all source probes were reverted.

The xSurface profile is restricted to the three authenticated debug executable
SHA-1s. France remains unchanged; no missing French bounds are inferred. Private
artifacts under `build/surface149/` retain actual compiler commands, independently
extracted references, original-applied source comparisons and full report checks.

The combined production prepare/compile/objdiff/section-export pipeline passes
for all four PS2 originals: 3,372 exact bytes in each debug region and France
unchanged at 1,460. Existing symbol registries compare identically. xSurface is
reported as six of seven exact functions (80 of 436 bytes), never complete.
