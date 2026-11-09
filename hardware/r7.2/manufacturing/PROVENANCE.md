# r7.2 source provenance

The r7.2 KiCad and manufacturing files in this directory are a curated copy of the reviewed EVT source snapshot from the private engineering repository.

- Source repository: internal engineering repository (retained privately by the project owner)
- Source branch: reviewed r7.2 manufacturing snapshot
- Source tree commit: daf99b495fd826ca45cf22918a517e507552ed9c
- KiCad PCB Git blob: 5dcde4610fef613b7ec49e14f956da0690d1b691
- KiCad schematic Git blob: 4020ccab37e25b6205e642ab5234ccb15375469f
- RS-274X upload ZIP SHA-256: 0a72863f5b290eb9801c037aea976e6b05967d50d45d6ef465c23e8ccdced8ab

The source branch and commit are recorded to preserve traceability. The Gerber/drill archive is a manufacturing output; it must not be regenerated from another KiCad revision without re-running ERC/DRC, reviewing the CAM output and updating this provenance record.

The public snapshot intentionally excludes historical supplier pre-match files, procurement spreadsheets, purchase/order records, third-party datasheet PDFs, and unverified reference photos.
