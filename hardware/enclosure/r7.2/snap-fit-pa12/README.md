# r7.2 screwless snap-fit enclosure — C / PA12

**Selected mechanical concept: two-piece, tool-free snap-fit enclosure. No screws or separate fastening hardware.**

This is the C variant archived from the 2026-11-05 first-trial package. The base and lid each integrate their own retention features. Four releasable cantilever snaps on the lid engage openings in the base; the release pads are accessible from the side windows.

## Print exactly two parts

| File | Qty | Purpose |
|---|---:|---|
| base_r72_snap.stl | 1 | Base, PCB supports, locating pins, side windows and latch engagement features |
| lid_r72_snap.stl | 1 | Lid, four integral snap beams, PCB stops and alignment rim |

Both files are in millimetres and positioned for printing at Z=0. Print at 100% scale. The ZIP file print-package-two-pa12-parts.zip contains these two printable parts plus 打印要求.txt (a text instruction file, not a third printed part). Do not print assembled_r72_snap.step as a single part.

## Required material and process

- Unfilled PA12 nylon.
- MJF or SLS powder-bed printing.
- No screws, nuts, inserts, foam, glue, washers, separate clips or coating are required for assembly.
- Normal depowdering and removal of printing residue are required.
- This geometry is not qualified for FDM PLA/PETG or standard SLA resin.

## Main geometry

- Closed enclosure: 171 × 144 × 33 mm.
- End walls: 2.8 mm; long side walls: 2.6 mm.
- Four snap beams: 1.2 mm thick × 8 mm wide, 20.1 mm nominal cantilever length, 0.8 mm root radius.
- Side release windows allow the clips to be pressed inward to open the lid.
- PCB is located by tooling-edge holes, support pedestals and two diagonal locating features; the lid captures the PCB with nominal 0.45 mm vertical clearance rather than hard-clamping it.

## CAD files

- base_r72_snap.step and lid_r72_snap.step: separate native neutral CAD solids for the base and lid.
- assembled_r72_snap.step: closed assembly reference for CAD inspection (not a printable single part).
- base_r72_snap.stl and lid_r72_snap.stl: separately downloadable printable meshes, byte-identical to the print ZIP members.
- print-package-two-pa12-parts.zip: authoritative two-part print package containing both base_r72_snap.stl and lid_r72_snap.stl.
- optional_coupon/: optional clip test coupons, not used in the final assembly.
- C_geometry_preview.png and C_latch_section.png: geometry and latch cross-section illustrations.
- params.json, mesh_verification.json, delivery-verification.json and SHA256SUMS.txt: design parameters, historical validation records and the current public-file checksum manifest. The historical delivery-verification.json records the original private delivery filenames and hashes; it is not the manifest for edited public metadata. See [public snapshot changes](../../../../docs/release/SNAPSHOT_CHANGES.md).

## Assembly

1. Print one base and one lid in the specified PA12 process.
2. Before inserting the PCB, test the four clips on the empty enclosure. Confirm that all clips engage and that the lid seam closes evenly.
3. Press the clips through the side release windows and lift the lid gently to confirm that all four can be released without excessive force.
4. Place the PCB over the locating pins and onto the support pedestals. It should seat without bending or forcing the board.
5. Align the lid rim and press evenly at the four clip positions until each clip engages.
6. Check USB-C insertion/removal and the DB25 mating connector, cable hood and jack-screw clearance.
7. To open, press the release pads through the side windows and lift the lid gradually. Do not pry one corner upward aggressively.

## Validation status and limitations

The archived CAD checks report valid single solids, watertight STL meshes, zero base/lid rigid intersection, zero CAM-rail intersection, zero DB25 keep-out-envelope intersection, and no collision among the 120 PCB STEP components that were modelled. The reported minimum modelled component clearance is 2.1 mm at R93.

The snap-beam strain calculation is a preliminary small-deflection screening calculation only. It is not FEA and does not establish actual engagement force, retention force, fatigue life or repeated opening life. The geometry report is marked PASS_WITH_UNVERIFIED_GAPS.

The PCB STEP used for clearance analysis did not fully model J2, U20, R99, TP201, TP202 or TP203. The DB25 keep-out envelope was checked, but the actual DB25 cable hood and thumbscrew dimensions still require physical mating checks. The first printed set must be checked for clip engagement, release force, PCB fit and USB/DB25 plug clearance.

## Parametric source availability

The original archived parametric source is now in build_snap_enclosure.py (SHA-256 matching the archived geometry report). The rail-fit adapter is a *new reconstruction* and is **not** byte-identical to the older rail-fit script recorded in the archived verification JSON; geometry-only regeneration was independently run on 2026-11-09 with the repaired adapter. Both BREP volumes and STL bounds match the archived values exactly, both meshes are watertight, and the geometric clearance checks match. This does not claim byte-identical CAD exports; see [the regeneration record](../../../../docs/release/CAD_REGENERATION.json). An adapter at ../rail-fit/build_marker_enclosure.py resolves its older rail-fit dependency using the checked-in first-fit reference and the original locating-hole dimensions in params.json. For a geometry-only regeneration, install the pinned dependencies with `python -m pip install -r requirements-cad.txt` from the repository root, then run `python build_snap_enclosure.py --geometry-only` from this directory. The normal full PCB component-clearance check requires an archived third-party PCB STEP reference that is intentionally not distributed here pending redistribution permission. The archival STEP and STL models and their precomputed verification JSON are what you print; regenerated geometry is a separate verification result, and physical fit has not been tried.

See [the Chinese design notes](设计说明.md), [print instructions](打印要求.txt), and [the validation record](delivery-verification.json).
