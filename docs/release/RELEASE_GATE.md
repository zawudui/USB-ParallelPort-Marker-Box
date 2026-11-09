# Personal source release checks — r7.2

Scope: the clean first source snapshot for `zawudui/USB-ParallelPort-Marker-Box`, prepared 2026-11-09. This does not change the original private engineering or organization repositories.

## Source publication

| Check | State | Evidence |
|---|---|---|
| Personal authorship/publisher | Owner confirmed | Owner confirmed individual rights and requested original material credited to Li Jingwei; third-party credits retained |
| Source scope and history | Prepared | Latest curated snapshot only; no inherited `.git` history, supplier orders, installers, external PCB model collection or prebuilt firmware |
| Firmware source and tests | Verified locally | 24/24 host tests; target image build recorded in [verification](VERIFICATION.json); new CI reruns on upload |
| Manufacturing files | Included | Gerber/drill, reference BOM, drawings and unchanged fabrication ZIP; package provenance and SHA-256 records included |
| C / PA12 source delivery | Included and regenerated | Base/lid/assembly STEP, separate STL files, original print ZIP and generator; [regeneration record](CAD_REGENERATION.json) |
| CAD regeneration | Geometry-only verified | Repaired compatibility adapter; both regenerated STL hashes match archived meshes; solid volumes, mesh bounds and geometric checks match. STEP export bytes differ between environments. Full PCB-model collision replay is not included |
| Library notices and licence texts | Included, scope explicit | [KiCad record](../../third_party/KICAD-LIBRARIES-LICENSE.md); full licence/exception and CC legal texts. Unrecorded historical per-entry revisions are disclosed, not invented |
| Sensitive material cleanup | Snapshot reviewed | Transaction identifiers removed from editable public metadata; supplier/account documents and raw machine logs excluded. Historical source checksums retained as provenance |
| Fresh package integrity | Required before upload | `python tools/release_audit.py` checks all manifest entries, local links, ZIP contents and standalone meshes; CI runs Windows and Linux |
| Repository visibility | Private until final release decision | Upload and inspect this concrete source snapshot before changing visibility; organization PR merge is not required |

The local audit only checks source integrity. Existing historical delivery records are identified as historical, and current public checksums describe current bytes.

## Physical product and production firmware

These remain explicitly unverified and are not represented as passing source-publication checks:

- PA12 physical fit, four latch engagements, release/retention force and opening-cycle life.
- Receiver-side interface compatibility, event-loss/jitter characterization and end-to-end latency.
- Assigned/authorized production USB VID/PID. Source retains clearly marked ST example placeholders; no production-ready firmware binary is shipped. See [USB ID plan](USB_ID_PLAN.md).

Publish the repository as an experimental source candidate with these limitations.
