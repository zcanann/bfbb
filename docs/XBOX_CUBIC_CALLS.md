# Xbox cubic solver call provenance

The cubic solver's source-lifetime correction initially reached ordinary
objdiff 100%, but the strict exporter rejected it because the original target
still carried eight unnamed raw calls. The exporter was retained unchanged.

The original calls at displacement offsets 42 and 493 reach the independently
reviewed quadratic solver and xatan2 entries. The remaining six reach the
original x87 power wrapper at 0x1bd6ec in both authenticated releases.

The pinned Microsoft static runtime's pow.obj independently supplies the named
__CIpow intrinsic and its __CIpow_default implementation. The public entry's
disabled-SSE2 branch reaches that default. All 527 default implementation bytes
match the original after its two-byte entry jump, except exactly 22 genuine
vendor COFF relocation fields. Repeated operands agree, calls stay in original
text, data operands have unique owners, and the complete original name is
`pow` followed by NUL. Context callees are not promoted to named identities.

Only the independently decoded, RET-terminated 27-byte callable wrapper enters
known coverage. The shared implementation remains uncredited context. The
wrapper gets no runtime source credit. All 2,556 older symbol records and
2,440 anonymous proof records survive unchanged; the anonymous registry's
manual-input hash alone is refreshed, preserving its compact layout.

All eight cubic call fields are checked against independently identified
original destinations and replayed in reverse. Both complete 13-unit source
reports now pass the unchanged exact-byte/relocation exporter guard, adding
654 exact bytes and one function per Xbox release. Eight original-backed tests
reject changed entry jumps, bodies, names, repeated runtime operands, bounds,
vendor identities and unproved cubic destinations, and replay the real archive.

Evidence: `build/oct09-xbox-pow-{integration,layout,tests}.log`,
`build/oct09-math-particle-laser-pow-final-xbox` and its comparison logs.
No compiler patch, runtime binary, whole-executable link or source-data claim
is introduced.
