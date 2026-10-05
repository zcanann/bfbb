# PS2 event dispatch

The complete shared xEvent.cpp compiles through the existing zSceneLookup.h API. The PS2 compiler's include search now includes the real src/SB/Game directory after the existing platform/core paths; the source inventory uses the same search paths. GameCube retains the full scene header.

All ten original event overloads / 880 bytes match across the three debug versions. Four wrappers call the full seven-argument dispatcher directly on PS2, exactly as the named original call destinations require, while retaining the same null/default arguments. The main dispatcher uses the natural positive linkCount > 0 predicate; its emitted branch and byte-value handling reproduce retail. GameCube emits identical bytes for this equivalent predicate.

An independent check applies every real source J/JAL relocation to the authenticated original DWARF callee address and reproduces all 880 original bytes in each debug version. No instructions, unknown calls, or source extents are substituted. All original functions remain in the standard report, and whole-executable completion remains false.

The normal GameCube whole-source build, retail executable hash and full report pass unchanged. The event object's allocated sections are byte-identical to the prior baseline. Private original-object and inverse-relocation evidence is under build/event164.
