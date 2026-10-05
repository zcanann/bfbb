# French QuickCull initializer recovery

The stripped SLES-53623 original now has both missing `xQuickCullInit`
entry points: the 28-byte box overload at `0x1fb0a0` and the 252-byte
six-float initializer at `0x1fb0c0`. Nine previously corroborated neighboring
functions are preserved verbatim. No game source, compiler flags, original
binary bytes, or generic control-flow acceptance rules change.

`tools/platforms/france_quickcull_sequence.py` checks the complete eleven-member
QuickCull sequence against each authenticated USA, Europe, and German original.
All three named DWARF sequences have 2,236 function bytes over 2,300 bytes with
zero-only alignment. The full masked sequence has one location in the French
CPU text region, and all nine prior member placements agree. Only actual direct
J/JAL destinations vary; arithmetic, registers, load displacements, branch
immediates, and other instruction bits remain intact. The external box helper
must retain its independently confirmed original name, source, extent, and hash.

The short overload is a scoped exception to the generic return-body proof,
not a relaxation of that checker. Its exact seven instruction words are six
LWC1 argument loads from a1 and a terminal J to the immediately adjacent full
initializer, with the sixth load in the delay slot. These instructions do not
write SP or RA. The destination independently passes strict frame/return checks
in all four originals. The registry records both the generic check's rejection
reasons and this explicit terminal-transfer evidence.

The French source profile now selects all eleven verified entries by name and
address, distinguishing the two overloads. All fourteen direct transfers use
original-only identities and the existing inverse-checked relocation path.
The complete unchanged `xQuickCull.cpp` is compiled with the established PS2
compiler/profile. Ten functions, totaling 2,196 bytes, match; both newly named
initializers add 280 exact bytes. Independently applying the actual source
object relocations reproduces all ten original bodies byte for byte.
`xQuickCullIsects` remains unmatched (40 original bytes versus the scalar source
implementation). This is not a complete TU or executable link claim.

Private verification artifacts are under `build/quickcull246`: original proof,
actual command and whole object, target object, normal objdiff report, and raw
relocation proof. The standard French production report is regenerated there;
all pre-existing registry records and section denominators are preserved. The
full run compiled 60 source units and reports 34,384 matched bytes / 209 matched
functions, up by 280 / 2. Every prior function record is identical. Six unrelated
unit aggregate fuzzy percentages differ only by floating-point summation
roundoff below 1e-12; no underlying function score or integer measure changed.

Reproduce original-only registry validation with:

```text
python tools/platforms/france_tu_sequences.py --orig-dir orig --check
```
