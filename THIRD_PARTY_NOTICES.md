# Third-party notices

This repository includes or refers to third-party components and materials. They remain under their respective terms; no project licence overrides them.

## KiCad libraries

The project-local symbol/footprint collection credits the KiCad library contributors and is distributed under CC BY-SA 4.0 with the KiCad libraries exception, separately from the original CERN-OHL-S hardware design. See the [file-level attribution record](third_party/KICAD-LIBRARIES-LICENSE.md), [upstream licence/exception notice](third_party/licenses/KiCad-LIBRARIES-LICENSE.txt), and [full CC BY-SA legal code](third_party/licenses/CC-BY-SA-4.0.txt). Original local additions/modifications are credited to Li Jingwei under the same library terms. Historical upstream revisions are not recorded for every entry; the record states the available evidence without inventing a version. Standard system libraries are installed separately through KiCad.

## Manufacturer documentation and drawings

Manufacturer datasheets, package drawings, product names, and trademarks belong to their respective owners. This curated release links to official manufacturer sources rather than redistributing manufacturer PDF files unless redistribution permission is confirmed.

The NorComp DB25 footprint records the connector model and cites the NorComp 182-YYY-213RYY1 mechanical drawing revision used to check dimensions. The footprint is a project-specific modification of KiCad's D-Sub socket footprint, not a bundled copy of a manufacturer CAD model. Confirm the manufacturer's current documentation and terms when adapting it.

The TRACO TMR 0511 footprint uses KiCad-derived geometry. Its descriptive URL in the original engineering source pointed to a third-party datasheet mirror; the component itself should be checked against the official TRACO Power TMR 2-series datasheet before procurement or redesign.

## MCU and semiconductor materials

The STM32 device register map in this project is a minimal project-authored register map based on public device documentation; it is not a copy of the ST CMSIS device header. Confirm applicable vendor terms when modifying or redistributing vendor-provided materials.

## Fonts and generated CAD

The enclosure generator uses DejaVu Sans available in the build environment. The font is not bundled as a separate font file. See https://dejavu-fonts.github.io/License.html.

## Exclusions

This release snapshot excludes third-party datasheet PDFs, the external PCB STEP preview containing third-party component models, unverified reference photographs, supplier order sheets, invoices, purchase records, and factory account documents.
