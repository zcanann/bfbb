# PS2 lasso renderer source comparison

The 7520-byte `zLasso_Render` improves from 85.162766% to 88.31542% by
making `vec2vecMat`'s PS2 vector inputs read-only and spelling the guide
flag's false case as a logical negation. The helper only reads its two
input vectors. Const inputs let the compiler keep their components in
registers across the normalization calls, recovering more of the original
instruction ordering. Expanding the helper directly at its call sites
independently reproduced the main improvement; that duplicated code is
not retained.

The helper remains non-const on GameCube to preserve its original emitted
symbol. The complete GameCube report is unchanged from the refreshed
staging baseline, including `zLasso_Render` at 99.587944%. The PS2 update
routine and all other functions in the unit retain their scores. This is
a partial source comparison, not a new exact-function or compiler-patch
claim. The const-qualified helper signature is a source reconstruction;
the PS2 debug data does not provide a separate `vec2vecMat` function DIE.

Original renderer DWARF records `loop`, `crossSection`, `sections`, `dif`,
`b1`, `b2`, `b3`, two `rotMatrix` locals, `temp`, and `norm`. Experiments
matching array declaration order and merging sequential vector or UV
temporaries did not produce a useful combined improvement; the vector
merges regressed the stronger const-input candidate. Passing helper
vectors by value also regressed the comparison. Those variants are not
retained, and the remaining differences are not blamed on a compiler
version.
