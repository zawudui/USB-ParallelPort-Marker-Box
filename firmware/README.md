# Firmware — STM32F411CEU6

This directory contains the portable marker protocol/state machine and the STM32F411 target implementation.

## Current firmware behavior

- USB Full-Speed CDC device, host serial settings 115200 8N1.
- Eight active-high outputs D0–D7 on PB0–PB7.
- Default mode after MCU startup: automatic pulse, 2 ms nominal width, 10 ms minimum gap.
- Configurable pulse width: 1–1000 ms; minimum gap: 0–1000 ms.
- DB25 output mapping: pins 2–9 = D0–D7; pins 18–25 = GND_ISO.
- Safety-oriented startup and shutdown sequencing, watchdog, and USB suspend/resume handling are implemented in the target code.

## Build and test

### Host tests

Requirements: CMake 3.16+ and GCC or Clang.

    cmake -S firmware -B build
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

### STM32F411 target image

Requirements: CMake 3.16+, GNU Arm Embedded Toolchain (arm-none-eabi-gcc, arm-none-eabi-binutils).

    cmake -S firmware/target/stm32f411 -B build-target \
      -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake \
      -DCMAKE_BUILD_TYPE=Release
    cmake --build build-target --parallel

Outputs: marker_box.elf, marker_box.hex, and marker_box.bin in build-target/.

GitHub Actions runs host tests and compiles the target image. A successful build does not mean the board was programmed or electrically tested.

## USB VID/PID — important

The current descriptor values 0x0483:0x5740 are ST's CDC example placeholder pair. They are **not assigned to this project**. Do not advertise them as a product identifier or ship a commercial product using them as though they were assigned. A production release must replace them with an appropriately licensed/assigned VID/PID and update the associated tests and documentation.

See the [USB identifier decision guide](../docs/release/USB_ID_PLAN.md) for legitimate assignment routes and the required source/test changes. The existing CI-generated image is a review image, not a production or shipment image; do not flash it for commercial distribution without the identifier decision.

## Timing and validation scope

The 2 ms default is a firmware timing target. A single bench capture on 2026-11-09 measured one D1 pulse at approximately 2.002 ms. A separate 100-command test measured 0.1796–0.3522 ms host write-to-complete-ACK round trips; this is not one-way host-to-output latency. These measurements are a limited bench observation, not a latency, jitter, or certification spec.

Verify the receiving instrument's DB25 pinout and electrical requirements before connection.

## Source layout

- core/: portable timing, power-state, watchdog and bring-up plans.
- src/: marker state machine and framed protocol.
- target/stm32f411/: register-level USB device and STM32F411 board support.
- tests/: host-side unit tests and fake register maps.

The legacy src/main.c stub from early development is intentionally not part of this curated release. The active target entry point is target/stm32f411/main.c.
