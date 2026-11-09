# Hardware revision r7.2

This directory contains the curated KiCad source and manufacturing outputs for the r7.2 engineering prototype.

## Provenance

The r7.2 KiCad source and fabrication outputs are a curated copy of the reviewed engineering manufacturing snapshot associated with the EVT prototype. The schematic, PCB, Gerber, drill and reference BOM are kept together so that the design can be traced to the manufactured board.

Do not substitute a KiCad file from a different branch or revision without regenerating and reviewing all manufacturing outputs.

## Design files

- Screwless enclosure: [r7.2 C / PA12 snap-fit design](../enclosure/r7.2/snap-fit-pa12/).

- kicad/: KiCad 9 project, schematic, PCB, design rules and project-local libraries.
- manufacturing/: Gerber, Excellon drill files, assembly drawings, reference BOM and manufacturing notes.
- reports/: ERC/DRC outputs and layout review images.

## Engineering status

The r7.2 prototype was manufactured and assembled as an EVT sample. A prototype board was subsequently powered, programmed and bench-tested. The 2026-11-09 evidence is summarized in ../../docs/validation/README.md.

This is not a certified medical device or a qualified production design. System-level ESD, EMC, dielectric withstand, USB power/transient, long-duration reliability and broad receiver compatibility have not been established. The design is published for research and engineering evaluation.

## Before manufacturing

1. Verify the source revision and SHA-256 manifest.
2. Review the complete schematic and PCB in KiCad 9.
3. Check the BOM against exact manufacturer part numbers, package suffixes and current availability.
4. Have the fabricator review the Gerber/drill data and return a production proof.
5. Independently validate the assembled board before connecting it to EEG/DAQ equipment.
