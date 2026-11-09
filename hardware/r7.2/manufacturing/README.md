# r7.2 manufacturing package

This directory contains the manufacturing source snapshot associated with the r7.2 EVT prototype.

## Contents

- gerber/: copper, solder-mask, paste, silkscreen and board-outline layers.
- drill/: plated and non-plated Excellon drill files and drill report.
- bom/: reference BOMs by designator and grouped value.
- assembly/: schematic and assembly/PCB drawings.
- usb_db25_marker_r7.2_JLC_RS274X_upload.zip: RS-274X Gerber/drill upload package for the fabricator.

## Important

The BOM is a design reference, not a complete procurement contract or a locked vendor-SKU list. Verify manufacturer part numbers, exact suffixes, voltage/current ratings, package, orientation, substitutions and stock before ordering. Do not treat historical PREMATCH BOM/CPL files as an approved production BOM; they are intentionally excluded from this public snapshot.

The RS-274X ZIP is the fabrication upload convenience package. Always inspect the fabricator's CAM/production proof, drill mapping, board outline, isolation clearances and fine-pitch footprints before authorizing a new run.

The files reproduce the r7.2 engineering prototype revision.
