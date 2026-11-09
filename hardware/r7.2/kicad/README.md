# KiCad project

Open usb_db25_marker.kicad_pro in KiCad 9. The project includes the r7.2 schematic, PCB, design rules and project-local symbol/footprint libraries.

The standard KiCad library references in sym-lib-table are resolved through the installed KiCad library environment. Install the standard KiCad symbol and footprint libraries before editing.

The project-local MarkerBox libraries contain the custom TLV75533/TPS61085/TPD4E05U06 symbols and the NorComp DB25 / TRACO TMR 0511 footprints used by this design. Review THIRD_PARTY_NOTICES.md before redistributing or adapting any library-derived geometry.

The two project-local footprint files with identified KiCad ancestry and the project-local symbol library require specific upstream provenance checks before public distribution. Refer to [the KiCad library attribution record](../../../third_party/KICAD-LIBRARIES-LICENSE.md) and [release gates](../../../docs/release/RELEASE_GATE.md). Do not treat the broad hardware CERN-OHL-S-2.0 licence as replacing upstream footprint-library terms.
