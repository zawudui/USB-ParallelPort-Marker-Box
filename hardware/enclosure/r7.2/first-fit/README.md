# r7.2 enclosure CAD — first-fit reference

This folder contains an earlier screw-fastened first-fit enclosure design and its generated CAD outputs.

> **Historical reference only — superseded by the selected C / PA12 screwless snap-fit design in [../snap-fit-pa12/](../snap-fit-pa12/).** This design uses four external M3 screws and is not the selected enclosure for the current product. Retained for engineering history; do not use it as the current print package.

## Model

- Two-piece enclosure: base + lid.
- Nominal outer dimensions in the CAD parameter set: 171 × 130 × 33 mm.
- USB-C opening on one side and DB25 opening on the opposite side.
- Four external case screws; PCB rests on internal support pads.
- Intended print materials: PETG (FDM) or PA12 (SLS/MJF).

## Files

- build_marker_enclosure.py: CadQuery source.
- pcb_step_clearance.py: optional solid-clearance analysis against a PCB STEP export.
- base_r72.step / lid_r72.step: editable solid models.
- base_r72.stl / lid_r72.stl: printable meshes.
- assembled_enclosure_r72.step: assembly preview, not a single printable part.
- params.json: design parameters and assumptions.
- mesh_verification.json: recorded geometry/mesh checks.
- SHA256_REFERENCE.txt: hashes of the generated source and CAD outputs.

## Reproduction note

The clearance script expects a PCB 3D STEP export from the project/fabricator. That external reference STEP is not bundled in this public snapshot because it contains third-party component 3D models whose redistribution terms have not been confirmed. See reference/PCB_STEP_SOURCE.md. The r7.2 Gerber/Excellon upload archive is available in hardware/r7.2/manufacturing/.

The CAD files can be opened directly in FreeCAD. Before printing or modifying, check the DB25 connector's actual mating hood and thumbscrew envelope; the board STEP preview did not contain a complete DB25 connector solid.
