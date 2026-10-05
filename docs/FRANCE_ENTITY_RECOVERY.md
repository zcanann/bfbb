# France entity recovery

France's original-only verifier now identifies the complete `xEnt.cpp` unit:
48 functions / 19,036 bytes. Actual production compilation matches 23 functions /
3,580 bytes, bringing France to **22,468 matched bytes / 139 functions**. The whole
unit has 74.3261188% fuzzy matching and retains every holdout. These are normal
code scores, not a completed-TU or retail-link claim.

## Complete original placement

All three original debug builds independently give the same ordered names,
canonical linkages and extents. Their complete sequence maps uniquely to France
starting at `0x1d1190`. The 19,420-byte sequence includes alignment and the
separately retained context below; its unchanged search anchor spans 1,400 bytes.
Every non-address instruction bit and alignment byte is
preserved. The verifier only permits actual direct transfers and address operands
with checked producers and consumers. No compiled candidate supplies identities.

Forty-seven bodies pass the ordinary strict bounds checker. The 12-byte
`xEntSetupPipeline` overload is a frame-free tail to the complete same-TU
500-byte overload. It preserves the incoming return address and has no other
outgoing edge, call or uncovered word. Thirty-four named entries also have
rooted direct-call witnesses.

One interleaved 136-byte leaf has no original DWARF identity. Its complete bytes
are identical in all four originals and its local CFG independently closes.
Twelve preceding and eight following alignment bytes are zero. This immutable
context helps prove placement but is neither named, assigned to `xEnt.cpp`, nor
added to source coverage. Internal transfers into this interval are accepted
only at its independently checked entry. Exactly one such context is required
per reference, preventing reuse of another reference's evidence.

`xEntGetAllEntsBox` is exactly `LUI v0; JR ra; ADDIU v0,v0,lo`. A TU-local
resolver recognizes that complete 12-byte leaf and its architectural return
delay slot. The result must be the original named `all_ents_box` address.
No general indirect-transfer or address-lifetime rule is relaxed.

## Original data ownership

The verifier accounts for all 53 distinct changed data addresses:

- Six callback pointers target complete verified same-TU bodies.
- The recorded `offs[4][3][2]` array has signed four-byte integer elements.
  Its nested original DWARF descriptors establish all 96 bytes, and its entire
  initialized payload is identical across the originals.
- `receive_models[15]` has the recorded unsigned four-byte integer elements.
  The 60-byte storage, all element stores, start pointer and one-past pointer
  at offset 60 have the same original roles and relative addresses. The end
  pointer does not extend the object's size.
- Fifteen null-terminated model-name strings match in full.
- `all_ents_box`, `g_O3`, `g_I3` and the three collision grids use original
  named types and concrete member offsets. Matrix inheritance is checked at
  offset zero. The zero-vector payload matches in full; mutable storage lies
  inside independently authenticated startup-zeroed BSS.

The original source profile's call names are used only when an independently
verified French callee identity and inverse J/JAL check agree. Other external
neighbors contribute no more than entry-prefix corroboration and are not
promoted to new named relocation anchors.

## Validation scope

The ten previously known entity extents deduplicate. Known French coverage is
520 functions / 185,308 bytes. The seven-unit sequence registry contains
222 records / 109,684 bytes; all prior 174 records remain unchanged.

The actual production report changes only the entity unit and unresolved CPU
range against the verified collision CI baseline. Every other unit report, all
existing profiles and all three debug-version effective profiles remain unchanged.
The new profile restores 94 independently verified calls, using canonical
original linkage to distinguish equal-size overloads before their checked
name/address aliases.

No game source, compiler flags, report math or denominator changes are part of
this recovery. Total code remains 2,979,968 bytes and data 2,384,384 bytes; completion
remains zero. A complete-TU/source-link claim remains false.

```sh
python tools/platforms/france_tu_sequences.py --orig-dir orig --check
```
