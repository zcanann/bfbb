# France Dutchman parameter cluster

`platforms/france_dutchman_tweaks.py` proves four complete functions, totalling
16,252 bytes. The principal anchor is the 15,588-byte Dutchman `register_tweaks`
at `0x3a5cc0`; the other three are its `zParamGetVector`, `zParamGetFloat` and
`zParamGetInt` callees. This establishes a complete caller/callee cluster,
not the complete Dutchman translation unit or its surrounding candidate span.

For each of the three authenticated debug originals, the verifier requires:

- The complete original DWARF source ownership, exact size and canonical
  linkage identity of all four bodies. Helper overloads are selected by the
  original caller's actual JAL destinations, never by their shared short names.
- Unchanged instruction bits, except explicit direct-call destinations and
  strictly reaching-LUI-proven address pairs. There is no register, opcode,
  arithmetic-immediate, branch-displacement or inserted-instruction tolerance.
- Strict closed control flow, stack, return and alignment checks on every
  original and French body.
- A unique occurrence of the complete masked 15,588-byte caller in the French
  loaded image. All 120 changed data operands point to complete, byte-identical
  parameter strings, with an injective address mapping.
- All 120 caller transfers preserve the same three complete helper bodies:
  111 float, six vector and three integer calls. Helper transfers are checked
  against independently proven xStrHash, xStrParseFloatList and xatof bodies.
  The remaining unchanged integer-helper runtime transfer remains unmasked
  and has a byte-identical 64-byte context. That context receives no identity,
  function extent or relocation claim.

The new `reviewed-complete-caller-callee-cluster` kind describes this narrower
scope explicitly. It is stored in the existing combined TU registry for
compatibility with the canonical regeneration pipeline. Report ingestion
accepts the new kind only on that pipeline and still requires the committed
registry to equal freshly regenerated original evidence, along with unchanged
executable identity, function hashes, positive aligned bounds and overlap
checks. The search map recognizes the independently proven extents after
registry integration; fuzzy hypotheses still cannot occupy ranges.

Standalone validation passed across all three references in 15.54 seconds on
2026-10-08. Authenticated-original tests also reject changed instructions,
changed complete strings and changed opaque runtime context. Synthetic
ingestion tests reject unsupported labels, stale proof output, wrong identity,
missing boundaries, changed body hashes and conflicting extents. No whole
translation-unit or linked executable claim follows from this cluster proof.

Two France-only source profiles select these four proven members and restore
125 independently verified direct calls. Authenticated source compilation and
comparison produced two exact helpers / 336 exact bytes out of 16,252 selected
bytes, with 99.655426% fuzzy matching. Dutchman register_tweaks is 99.69207% and
the vector helper is 97.560974%. These newly covered functions were previously
absent from the France comparison. This expands the selected-function comparison;
the full production CPU-code denominator remains unchanged.
Existing source and other version profiles are unchanged. Private validation:
`build/dutchman-france-pilot/report.json` in the regional worker checkout.
