# France math recovery

France now compares all 35 `xMath3.cpp` functions / 8,600 bytes, extending the
previous 12-function / 3,192-byte subset. The unchanged shared source matches
13 functions / 1,772 bytes; all remaining functions retain partial scores.
That adds eight matched functions / 728 bytes, bringing France to **16,652
matched bytes / 108 matched functions**. This is normal code matching, not a
complete-TU or retail-link claim.

## Original sequence and boundaries

Three original debug builds independently provide the same ordered DWARF names,
canonical linkages and extents. Each full reference sequence has exactly one
French location, starting at `0x1ee5c0`. All non-address instruction bits remain
identical. Direct transfers and proven address operands are the only exceptions.

An interleaved 112-byte `xVec3.h::__mi` operator lies between
`xMat3x3Normalize` and `xBoxFromCone`, preceded by eight zero alignment bytes.
All originals identify its separate header ownership and canonical linkage;
its full bytes match exactly and its local CFG closes. It is retained as a
placement anchor only, not reassigned to math or added as a new matched function.

Thirty-two math bodies pass the ordinary strict bounds checker. Three remaining
bodies are completely covered frame-free leaf tails, with valid delay slots,
no calls or return-address writes, and no other outgoing local edges:

- `xQuatMul` (136 bytes) tails to the same-TU `xQuatNormalize` entry.
- The vector `xMat3x3Euler` overload (16 bytes) tails to the scalar overload.
- `xMath3Exit` (8 bytes) tails to the original DWARF-named `iMath3Exit`.
  That separate eight-byte original body is byte-identical and independently
  passes the strict bounds checker. It is not promoted as a new named anchor.

Twenty-three math entries have rooted direct-call witnesses. The remaining
entries rely on the unique complete sequence and checked call relationships.
External neighbors provide at most 32 bytes of entry-prefix corroboration;
they do not become recovered extents or independently named relocation anchors.

## Named data and floating-point accesses

The verifier accounts for all 29 distinct changed data addresses using original
DWARF declarations, aggregate types and actual instruction consumers:

- `g_O3`, `g_X3`, `g_Y3`, `g_Z3` have the recorded 12-byte vector layout.
- `g_IQ` has the recorded 16-byte quaternion layout.
- `g_I3` has the recorded 64-byte matrix layout, with the 48-byte matrix base
  inherited at zero and position at offset 48. Both whole-matrix pointer uses
  and all twelve component stores retain their actual original offsets.
- The local `nxt` array has an original DWARF descriptor for indices 0 through 2
  and four-byte long elements. All twelve bytes preserve the cyclic 1, 2, 0
  index table across the originals.

Initialized aggregate payloads match completely. Matrix storage is contained
in the independently authenticated startup-zeroed BSS range. These checks do
not add speculative global extents to the production data registry.

A TU-local resolver recognizes LWC1/SWC1 address consumers while retaining
reaching-LUI, register-clobber, delay-slot and incoming-edge checks. The only
extra GPR-effect cases are the actual single-format MUL, NEG, ADDA, SUBA, MULA,
MADD and MSUB operations encountered here. Their encoding and floating-point
operand roles agree with the [PCSX2 assembler table](https://github.com/PCSX2/pcsx2/blob/master/pcsx2/DebugTools/MipsAssemblerTables.cpp).
Unrecognized COP1 instructions still fail; shared strict defaults are unchanged.

## Source comparison and regression checks

The complete France profile restores thirteen calls with independently proven
callee identities. Existing particle-command and motion calls to the scalar
Euler overload now use its verified `name@address` key. This resolves the new
overload ambiguity without dropping either previously validated relocation.
The backend constructs those aliases only from already verified original
function metadata; unchanged inverse J/JAL checks still require the actual
original instruction to reach the selected entry.

The prior 12 math extents are deduplicated. France's known inventory rises from
425 functions / 132,224 bytes to 448 / 137,632 bytes. The five-TU original proof
registry contains 138 records / 54,000 bytes, with all prior 103 records unchanged.

Actual production compilation and stock objdiff reporting pass. Every other
unit report remains unchanged; existing math functions retain their prior
scores. Total code stays 2,979,968 bytes, data 2,384,384 bytes, and completion
zero. No game source, compiler flags, debug-version profiles, scoring options
or report math change. The unit's whole-scope fuzzy percentage includes its
newly compared holdouts; it is not directly comparable to the former subset.

```sh
python tools/platforms/france_tu_sequences.py --orig-dir orig --check
```
