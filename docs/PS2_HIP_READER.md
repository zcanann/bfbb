# PS2 HIP reader source comparison

The complete existing xhipio.cpp source compiles after removing an unnecessary xFile.h include from the binary-I/O API header. xbinio.h uses only its own callback/record types and common scalar types; its implementation now directly includes xFile.h. No PS2 file structure, allocator API, or implementation stub is introduced. All 224 GameCube game translation units rebuild with byte-identical allocated sections.

Original debug information owns 16 HIP-reader functions covering 2796 bytes. Every target remains in the source comparison, including nonmatching functions. The standard objdiff code report counts 13 functions / 1644 matched bytes. This is instruction-code matching under the existing relocation policy, not a completed executable link or a claim that every data reference has been reconstructed.

Calls use independently named original functions and the existing reviewed memset target. The static name g_loadlock occurs in multiple original translation units. Its seven GP references therefore use an explicit target_source owner in the profile. The optional filter selects original DWARF address anchors whose recorded source references match that owner, then still requires exactly one address and the existing GP/instruction inverse proof. Profiles without an owner retain their previous behavior; an owner that leaves multiple possible addresses is still rejected. No compiler object is used to establish original identity.

The comparison profile is restricted to the three debug executable hashes. France's current source coverage is unchanged. Unit completion remains false.
