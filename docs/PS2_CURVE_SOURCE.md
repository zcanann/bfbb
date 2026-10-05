# PS2 curve source comparison

The complete existing xCurveAsset.cpp compiles after adding the standard int abs(int) declaration to the PS2 C runtime header. No runtime replacement, compiler flag change, or curve source rewrite is introduced.

The source emits 400 bytes against the original 460-byte evaluator and currently compares at 74.04348%. The original calls a stripped runtime absolute-value entry and retains explicit zero-based interval arithmetic; the present compiler inlines abs and simplifies the source's arithmetic. This remains a partial comparison, with no exact function or completed-unit claim. Earlier no-intrinsics experiments did not resolve the entire function and are not repeated or adopted here.

All three debug originals provide the same canonical linkage. France already has an independently reviewed evaluator extent; its profile uses that established identity and does not infer a new function boundary. The whole original evaluator remains in each report.
