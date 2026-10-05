# PS2 text utility source

The missing PS2 `strtosjis` and `BCDtoi` implementations are restored in the complete shared `xutil.cpp`, behind the existing PS2 conditional. Standard `printf`, `sprintf`, `atoi`, and `exit` declarations supply no replacement runtime code.

Original utility DWARF records both function signatures, BCD's 16-byte character buffer, the Shift-JIS integer and byte temporaries, and the two lookup tables. The tables are `unsigned short[3][2]` and `unsigned short[33]`; their recorded data addresses and contents establish the alphanumeric offsets and punctuation mappings. The original conversion clears 64 destination bytes, emits two-byte codes, and prints an error followed by exit for unsupported input. The restored source preserves these behaviors, including the existing caller buffer assumptions.

The complete source emits a 552-byte `strtosjis`, the same size as retail. Its instruction words agree except for eight genuine address relocation operands: the error string, two runtime calls, and the two tables. This is code comparison evidence, not proof of independently resolved runtime identities or a retail executable link. The standard report remains authoritative for matched code.

`BCDtoi` deliberately preserves the original `sprintf(c, "%x", hex)` followed by decimal parsing. Replacing this with nibble arithmetic would change behavior for invalid BCD digits. The current compiler emits 48 bytes against retail's 52 because its short format string uses a GP-relative load; this function remains nonmatching. No compiler flags or instruction substitutions are introduced.
