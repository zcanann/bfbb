# Reusing authenticated debug decoding

French proof generation repeatedly decodes the same three 12.8 MB original
debug sections. The decoder now retains at most three completed immutable
buffers. Each iteration receives fresh attribute dictionaries; cached values
are immutable scalars, strings and byte blocks. Mutable buffers bypass the
cache. Partial iterations and failed decodes are never cached, preserving lazy
validation and preventing callers from sharing modified proof attributes.

A benchmark against the previous decoder checks every row and attribute in
all three authenticated originals: 312,615 USA rows, 312,624 PAL and 312,643
German. Warm iterations take 0.49 / 0.53 / 0.69 seconds, versus 1.08 / 1.47 /
1.84 before caching. Cold decoding takes longer, 1.73 / 2.12 / 3.01 seconds,
and the cache retains decoded metadata as well as its three input buffers.
The repeated proof passes benefit; the decoder's checks and outputs stay intact.

Five tests cover mutation isolation during initial and repeated iterations,
distinct owners, lazy malformed-tail rejection, incomplete iteration and mutable
input changes. Evidence: root `build/oct09-dwarf-cache-benchmark.{json,log}`.
Full production proof regeneration remains an integration gate.
