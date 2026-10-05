# Xbox hash loop ownership

The remaining two reviewed string-hash functions now match the authenticated
Xbox US and EU originals exactly:

| Function | Original entry | Bytes | Before | After |
| --- | --- | ---: | ---: | ---: |
| xStrHash(const char*, size_t) | 0x15b110 | 55 | 85.71429% | 100% |
| xStrHashCat(U32, const char*) | 0x15b150 | 48 | 87% | 100% |

The bounded hash already had every original operation. Its generated counter
and cursor increments preceded the final hash ADD, whereas the original ADD
comes first. The concatenating hash incremented its cursor before the ADD and
loaded the next byte through `[esi]`; the original uses `[esi+1]` before the
increment. The current source advanced these variables before hashing the
captured character in both functions.

One source-supported correction moves these existing updates into the `for`
iteration clause on Xbox. This retains the loop condition, captured signed byte,
folding arithmetic, prefix value and termination behavior while putting updates
after each hash calculation. It recovers both complete original bodies under
the unchanged compiler profile. Non-Xbox spelling is preserved. No extra
variables, special qualifiers, compiler flags, assembly or padding are used.

Both complete-TU production builds and standard reports pass, increasing Xbox
matched code **1008 -> 1111 bytes** (+103). The final source PE bytes are compared
directly against each original, with no relocations or PE HIGHLOW operands in
these two extents. All seven other reviewed string bodies and their named call
relocations remain identical, as do the particle bodies and address relocations.
All nine reviewed functions in the string source unit are now exact; unreviewed
functions remain outside that partial source comparison, so this is not a full
translation-unit or executable completion claim.

The existing 2,519-function inventory, 627,572 known function bytes and full
1,798,760-byte `.text` denominator are unchanged. Original boundary/name metadata,
compiler configuration and report tooling are untouched. Actual PS2 compilation
confirms identical allocated section bytes, sizes, alignments and symbol-resolved
relocations; final GameCube checks run in the integration checkout.
