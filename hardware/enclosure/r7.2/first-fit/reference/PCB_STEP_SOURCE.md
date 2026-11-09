# PCB STEP reference — intentionally not bundled

The original engineering workspace used a JLCPCB 3D STEP preview named colorsilk_preview_1792580399245.step, packaged as colorsilk_preview_1792580399245.zip, for collision/clearance analysis.

That export is not included in this public repository snapshot because it contains third-party component 3D models and redistribution permissions have not been confirmed.

To reproduce the optional PCB-solid clearance analysis:

1. Open the r7.2 KiCad project and export a PCB STEP with component bodies, or obtain an equivalent STEP export for the exact PCB revision.
2. Review the export's component-body coverage, especially the DB25 connector, USB-C connector and any components represented only by placeholder solids.
3. Place the export at the expected local reference path and update its SHA-256 in the analysis script before running it.
4. Treat missing/placeholder solids as unverified, not as proof of clearance.

The enclosure STEP/STL files in this directory remain available for inspection and printing, but the full original clearance analysis cannot be reproduced from this public snapshot without the external PCB STEP reference.
