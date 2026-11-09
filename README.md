# USB-DB25 Marker Box

**An open-hardware USB-to-DB25 event marker interface for EEG/ERP and experimental research.**

开发者与个人发布者：**zawudui**。中文说明见 [README_CN.md](README_CN.md)。

Developed and published by **zawudui**. This personal source snapshot is based on the previously curated project files; third-party credits and licences are preserved. This repository contains the r7.2 hardware design, STM32F411 firmware, MATLAB host client, enclosure CAD, manufacturing source files, and validation notes.

> [!IMPORTANT]
> **Release candidate / prototype status.** This repository is being prepared for its first public release. The hardware is an experimental research accessory, not a medical device. Compatibility with a particular EEG/DAQ system must be verified against that system's DB25 pinout, input voltage, polarity, pulse width, and trigger sampling behavior. Do not connect it to valuable acquisition equipment until the electrical interface has been independently checked.

Personal repository: [zawudui/USB-ParallelPort-Marker-Box](https://github.com/zawudui/USB-ParallelPort-Marker-Box).

## What it is

- USB-C connection to a host computer.
- Eight active-high digital marker outputs on DB25 pins 2–9 (D0–D7).
- Isolated-side signal reference on DB25 pins 18–25 (GND_ISO).
- STM32F411-based firmware with a framed command protocol.
- MATLAB client and examples.
- KiCad schematic/PCB sources and a two-piece screwless PA12 snap-fit enclosure.

## Validation summary (2026-11-09)

One prototype board was recovered and tested on a bench:

- 100/100 command/ACK transactions passed; measured host write-to-complete-ACK round trip: 0.1796–0.3522 ms under the recorded setup.
- One DB25 D1 pulse measured approximately 2.002 ms.
- One unloaded-DB25 MCU-output-to-DB25-output comparison showed approximately 31 ns difference, **uncalibrated for probe/channel skew**. Ordinary oscilloscope probe grounds temporarily commoned host ground and GND_ISO during this capture, so it is not evidence of isolation performance.

These measurements are not an end-to-end host-to-DB25 latency specification, jitter guarantee, long-duration reliability test, ESD qualification, isolation certification, or proof of compatibility with every EEG/DAQ system.

## Hardware interface

| DB25 pin | Signal | Description |
|---|---|---|
| 2–9 | D0–D7 | Eight active-high marker bits |
| 18–25 | GND_ISO | Isolated-side signal reference |
| 1, 10–17 | NC | Not connected |

Always confirm connector orientation from the mating-face view and verify the receiving instrument's pin assignment before connection.

## Build and use

- Hardware design: [hardware/r7.2](hardware/r7.2/)
- Firmware build and host tests: [firmware](firmware/)
- MATLAB client: [tools/matlab](tools/matlab/)
- Enclosure CAD: [r7.2 screwless PA12 snap-fit enclosure](hardware/enclosure/r7.2/snap-fit-pa12/)
- Manufacturing notes: [hardware/r7.2/manufacturing](hardware/r7.2/manufacturing/)
- Electrical interface and limitations: [electrical specifications](docs/hardware/electrical-specifications.md)
- Validation scope: [validation notes](docs/validation/README.md)

The firmware's current USB VID/PID values (0x0483:0x5740) are **ST example placeholder IDs**, not a VID/PID assigned to this project. Do not treat them as a product identifier or ship a commercial product with them as if assigned.

## Licensing

- Hardware design files: CERN-OHL-S-2.0, subject to exclusions in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
- Firmware and host-side software: MIT.
- Original documentation: CC BY 4.0.
- Third-party materials retain their own licences.

## Safety and limitations

This is an experimental research accessory. No claim is made that the complete assembly has passed IEC ESD testing, dielectric withstand testing, EMC testing, USB power-budget qualification, or exhaustive receiving-device compatibility testing. Device-level component ratings do not establish system-level qualification. Review electrical notes and validate the actual receiving setup before use.

## Release readiness

This is a source-only prototype candidate, not a production firmware release. See [release checks](docs/release/RELEASE_GATE.md) and [personal repository upload](docs/release/PERSONAL_REPO_MIGRATION.md). Run `python tools/release_audit.py` before upload. The source package contains no private Git history, supplier orders or prebuilt firmware.

## Attribution

Copyright (c) 2026 zawudui and contributors, for their original contributions. KiCad-derived library material has separate attribution and licensing. See individual licence files and notices.
