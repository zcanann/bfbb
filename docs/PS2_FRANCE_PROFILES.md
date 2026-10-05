# France source profiles with verified partial ownership

Six complete shared translation units are now compiled for the stripped France
PS2 release. Their comparison targets contain every function already recovered
for each unit by the existing reviewed/corroborated registries:

| Unit | Verified French functions / reference unit | Verified French bytes / reference unit |
| --- | ---: | ---: |
| xPar | 3 / 4 | 276 / 352 |
| xFactory | 1 / 11 | 120 / 1396 |
| xParGroup | 3 / 12 | 564 / 1488 |
| xString | 8 / 12 | 1472 / 3948 |
| xMath | 9 / 18 | 1804 / 3700 |
| xMath3 | 12 / 35 | 3192 / 8600 |

These 36 functions and 7428 bytes were already present in France's verified
inventory. This change adds actual whole-source comparison without adding or
removing function boundaries, extending code coverage, or declaring any complete
translation unit. `xJaw` and `xVec3` have no verified French members yet and remain
outside the new profiles.

The profiles are restricted to the authenticated French executable SHA-1. The
three debug-region profiles remain unchanged. Each French profile includes all
currently verified members of its unit, including nonmatching functions, while
the compiler processes the entire ordinary source file. Unknown original members
remain part of the existing full CPU text denominator, not invented functions.

Canonical symbol spellings come from original reference DWARF attributes. For
each French member, the profile records its verified registry and every available
reference version, executable hash, and function address. The authenticated
reference function must agree with the verified source, human name, complete size,
and recorded body hash; its canonical linkage must agree across all references.
No source object is used to infer a function name or boundary.

`xString` contains two verified `xStrHash` overloads, at `0x0020f1f0` (104 bytes)
and `0x0020f260` (88 bytes). The opt-in `name_address` selector distinguishes them
using their existing verified diagnostic identities. The profile maps these to
`xStrHash__FPCcUi` and `xStrHash__FPCc`, respectively, as established by reference
DWARF. The same selector is used by target construction and the standard report
exporter. Existing name and linkage selectors keep their behavior.

Only independently known French call destinations and the previously reviewed
`gActiveHeap` data anchor are restored as relocations. Other addresses remain
unresolved. Standard objdiff code matching uses the project's existing relocation
policy; it does not establish relocated-byte equality or an executable link.
No custom score adjustment, placeholder symbol, or new boundary rule is used.

Private evidence from this pass is under `build/france155`: `coverage.json`,
`candidate-audit.json`, `profile-feasibility.json`, `linkage-proof.json`, and the
four full production reports. An additional 15 unique full-byte candidates
(2540 bytes) passed the current local control-flow checks but lack the required
entry-rooted JAL evidence. They remain separate unconfirmed candidates; this
source-profile change does not promote them.

Validation with the actual compiler and standard report pipeline passes for all
four PS2 releases. Every debug-region report remains exactly unchanged at 9844
code-matched bytes. France rises from 1460 to 3528 code-matched bytes: `xPar` adds
276, `xString` 344, `xMath` 404, and `xMath3` 1044. These are 14 additional
100%-instruction-score functions under the standard policy, not new raw-link
claims. France retains 331 verified functions and its 2,979,968-byte CPU code
region denominator; all previously compared units retain their results.

`xFactory` and `xParGroup` also compile in full and retain their existing verified
subsets for comparison. The single verified factory constructor currently has
no automatic objdiff pairing despite equal canonical symbol spellings, so it
contributes zero; no manual score is assigned. All symbol/split registries remain
unchanged, and the 15 unconfirmed exact-byte candidates remain excluded.

## Single-function section pairing

Generated target code sections now use the compiler's `.text` name. With only
one known function in a unit, an address-suffixed section name survived objdiff's
section merging and prevented pairing with the source `.text` section, despite
identical canonical symbol names. The xFactory constructor exposes this case:
normal section naming pairs its existing 120-byte source and target bodies at
100% code match. No instruction bytes, symbol bounds, relocation policy or
objdiff scoring settings change.
