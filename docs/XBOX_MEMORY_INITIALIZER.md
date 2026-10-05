# Xbox memory initializer and external original data

`iMemInit()` at `0x16c210` is a 103-byte original Xbox function in both payload-identical regions. Its independently observed caller initializes three 220-byte shared heaps with flags `0x8826`, `0x8925`, and `0x892a`, assigns their opposite-heap indices, then clears the active heap index. The initializer allocates `0x384000` bytes and writes the allocation base/end plus every field of the four 12-byte `xMemArea_tag` records in `gMemInfo`.

The ordinary C reconstruction uses the existing shared `xMemInfo_tag` declaration and standard `malloc`. With the established pinned MSVC `/O2 /Ob1 /GL /Gd` and `/LTCG` recipe it reproduces all 103 original bytes after real address/call relocation application. `mem_base_alloc` and `mem_top_alloc` are genuine compiled definitions. The host `malloc` import receives no game-source credit. The original call target is independently identified from the pinned CRT object, so the existing named `REL32` comparison and exact-source reconstruction checks remain in force.

The shared `gMemInfo` object remains original storage at `0x311e38`, size 48. This comparison links an explicit data-only COFF absolute assignment, like a fixed external data address in a linker script. It allocates no section or storage, contributes no data/code matches, and does not make the resulting diagnostic PE a runnable replacement for the XBE. No original game bytes are embedded in source.

## Separate data-binding provenance

Absolute symbols deliberately have no PE `HIGHLOW` relocation. They therefore use an opt-in path separate from the existing compiled-data path:

- Target preparation authenticates the original and its reviewed data witnesses, then binds allowed data objects to the actual target comparison-object hash.
- The profile supplies the exact source linkage, original owner and size; it cannot supply an address, function binding or relocation offset.
- An actual unbound LTCG link must fail on exactly those undefined data symbols from the compiled source object. An unrelated error or an already-defined symbol fails validation.
- A zero-section COFF object provides only the reviewed absolute data symbols. The final MAP must retain those exact `<absolute>` values, outside every source PE section.
- Source operand discovery accepts declared decoded memory displacements inside the reviewed object, with checked access widths. Calls, jumps, immediate constants, source offsets and overlapping `HIGHLOW` fields are rejected. Every decoded reference into the bound object must be accounted for.
- Existing compiled globals still require their real `HIGHLOW` records. Normalization and inverse reconstruction preserve all other instruction bits. Standard objdiff scoring and report denominators remain unchanged.

The old linker rejects absolute symbols as direct `REL32` call targets (`LNK2016`); this feature intentionally does not work around that restriction. The [Microsoft PE/COFF format](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#section-number-values) distinguishes absolute non-relocatable values from section-defined addresses.

## Independent allocator identity

The pinned `libcmt.lib` SHA-256 is `780aa4cbe614efeeb72e3bb9538230f5cd37caeb0adc5d70c346b0dda19ba3a5`. Its actual `build\intel\mt_obj\malloc.obj` member has SHA-256 `682ee05b92be1eab3545eb7b87bf0fdc7a8d0e669b86f7b84e3e8a113e984abb`. The vendor COFF symbol and auxiliary records establish `_malloc` as 18 bytes and `__nh_malloc` as 44 bytes. Both complete original bodies match those named vendor bodies outside only their actual COFF relocations:

- `_malloc` at `0x1bd2a6`: `DIR32` at +2 to the new-mode variable and `REL32` at +11 to its delegate.
- `__nh_malloc` at `0x1bd27a`: `REL32` at +12 and +32 to its heap and new-handler paths.

The original wrapper calls the independently matching delegate. The original RenderWare default allocator initialization also installs the wrapper into its malloc slot. `xbox_malloc.py` replays the pinned archive, function sizes, real relocation records, original bodies and registration witness. The delegate and its callees remain corroborating context, not additional recovered game functions.

The 18-byte wrapper enters a target-only `Runtime/MSVC/malloc.obj` unit, receiving no source credit. Both it and the initializer replace existing anonymous extents, so the known-function and whole-section denominators do not grow. The initializer adds 103 matched bytes and one matched function per Xbox release; the absolute external data contributes zero matched bytes. All prior source profiles and source-unit results remain unchanged.

## Reproduction and scope checks

Run the ordinary platform driver for each Xbox version with the pinned compiler directory, Wine on non-Windows hosts, and objdiff:

```sh
python3 tools/platform_progress.py report --version XBOX-US \
  --orig-dir <private-originals> --build-dir build/xbox-memory \
  --xbox-compilers <pinned-compiler-root> --wine /usr/lib/wine/wine \
  --objdiff <objdiff-cli>
```

Repeat with `XBOX-EU`. Both produce 11,548 matched bytes / 60 matched functions on this revision, versus 11,445 / 59 before the initializer. Every previous source unit and profile is unchanged. The known inventory remains 2,556 functions / 635,415 bytes, and the full `.text` denominator remains 1,798,760 bytes. The original-data proof is `original-data-bindings.json`; actual link commands, expected unbound-link failure, final PE/MAP and function evidence are retained under `source/iMemMgr/`.

The exact source inverse uses two real compiled-global `HIGHLOW` operands, twelve separately verified original-data operands and one independently named allocator call. It reconstructs all 103 original bytes in each release. No synthetic behavior tests or source-score adjustments are involved.

## Remaining closure

There is no recovered Xbox shutdown implementation. A complete raw `.text` reference scan found `gMemInfo` and the allocation base/end only in initialization and heap setup. The source does not invent an empty exit or copy the GameCube OS-heap shutdown body.

The spline audit also proves a static RenderWare memory-function table at `0x3ad2b4` through `0x3ad2c0`, not the GameCube engine-pointer indirection. Its initializer copies four supplied callbacks or installs the default memory functions. The engine buffer starts at `0x3ad180`; the original plugin-registry descriptor records initial size `0x158` and maximum size `0x4000`, and plugin initialization receives this buffer. These findings guide a future authentic allocator/RW closure; no RenderWare global definition or method stub is introduced by this change.

The source profile remains partial and executable-link completion remains false.
