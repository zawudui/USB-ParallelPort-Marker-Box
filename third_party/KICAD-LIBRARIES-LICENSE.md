# KiCad library attribution and licence scope

The project-local library collection is supplied under **CC BY-SA 4.0 with the KiCad libraries exception**. Original local additions/modifications are Copyright (c) 2026 Li Jingwei; incorporated KiCad material is credited to the **KiCad library contributors**. These library files are excluded from the original hardware's CERN-OHL-S licence scope.

The full [upstream licence/exception notice](licenses/KiCad-LIBRARIES-LICENSE.txt) and [CC BY-SA legal text](licenses/CC-BY-SA-4.0.txt) accompany the files. Official upstreams: [footprints](https://gitlab.com/kicad/libraries/kicad-footprints), [symbols](https://gitlab.com/kicad/libraries/kicad-symbols), and [licensing policy](https://www.kicad.org/libraries/license/).

## Source evidence and modifications

All paths below are relative to `hardware/r7.2/kicad/`.

| File / symbol | Source evidence and changes |
|---|---|
| `MarkerBox.pretty/DSUB-25_Socket_NorComp_182-025-213R531.kicad_mod` | Metadata records derivation from KiCad `DSUB-25_Socket_Horizontal_P2.77x2.84mm_EdgePinOffset7.70mm`; modified for NorComp contact/board-lock drills and project metadata. |
| `MarkerBox.pretty/TMR2_0511_SIP8.kicad_mod` | Engineering provenance records KiCad-derived TRACO SIP-8 geometry, with project-specific references and locations. |
| `MarkerBox.kicad_sym`: `TPS61085PWR` | Historical engineering script `r72_apply_schematic.py` extracts KiCad `Regulator_Switching:TPS61085DGK`, renames it and changes package/value/footprint metadata. |
| `MarkerBox.kicad_sym`: `TPD4E05U06DQAR` | The same script extracts KiCad `Power_Protection:TPD4EUSB30`, renames units/value and changes project metadata. |
| Other local symbols: `TLV75533PDBVR`, `74LVC06A`, `TMR0511`, `Q_2N7002_SOT23`, `Crystal_GND24` | Mixed original/adapted project symbols. Exact historic per-symbol revisions are unrecorded. The complete collection, including original local additions, is supplied under the same CC BY-SA 4.0 + KiCad-exception terms rather than claiming exclusive project authorship. |

This records available provenance, not a byte-identical match to an unrecorded upstream version. Existing attribution in the supplied files is retained. Manufacturer names/datasheet links identify dimensional and electrical references, not a manufacturer CAD redistribution licence.

## Design versus library collection

The KiCad exception covers electronic designs and generated files using library data; it does not waive share-alike/notice requirements when redistributing this library collection. Standard libraries referenced by the project tables are installed separately through KiCad.

Licence-text sources retrieved 2026-11-09: the official KiCad footprint-library GitHub mirror `LICENSE.md`, and SPDX's CC BY-SA 4.0 transcription of the Creative Commons legal code. Full texts and upstream notices are retained in the linked local files.
