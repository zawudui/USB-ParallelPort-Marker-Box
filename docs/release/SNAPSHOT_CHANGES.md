# Personal source snapshot changes — 2026-11-09

Baseline: curated r7.2 source revision `43cc30cfc41bbf8086dc98116405143cde52e5f2` (retained privately). No private Git history is included.

- Original authorship/publisher changed to `Li Jingwei` after the owner explicitly confirmed individual rights and requested personal attribution. Third-party credits remain separate.
- Restored source Git blob bytes after Windows checkout conversion. Added `.gitattributes` to prevent future automatic line-ending conversion.
- Corrected the source audit to validate exactly two STL meshes plus the existing print instruction text in the original print ZIP. The ZIP is unchanged. Added the missing standalone lid STL, extracted byte-for-byte from that ZIP.
- Removed supplier transaction identifiers and private CAM filenames from public enclosure parameters/design notes; dimensions, locating-hole coordinates and CAD source/solids remain unchanged.
- Refreshed the public enclosure checksum manifest for the edited public metadata. `delivery-verification.json` is explicitly historical, retaining its original delivery hashes; it is not silently rewritten as new test evidence.
- Added the full upstream KiCad library licence/exception notice, CC BY-SA 4.0 and CC BY 4.0 licence texts and explicit library-file scope. Full historical upstream revision reconstruction is recorded separately from licence compliance.
- Replaced organization-specific staging/PR instructions with personal snapshot upload instructions. Included both source ZIP assets in `.gitignore` exceptions.
- Repaired the rail-fit adapter so it cannot overwrite its own path variables. Independently regenerated C geometry; solid volumes, STL bounds and geometric checks match the archive. Added a dated regeneration record and pinned CAD dependencies.
- The root `SOURCE_SHA256SUMS` and deterministic packaging tool define the files for the first personal upload; no supplier PDFs, order sheets, tool installers or prebuilt firmware are added.
