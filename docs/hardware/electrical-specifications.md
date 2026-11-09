# Electrical interface and specification status

This document separates the intended design values from measurements actually recorded on the prototype.

## Host connection

| Item | Value | Status |
|---|---|---|
| Connector | USB-C device port | Implemented |
| USB class | Full-Speed CDC virtual serial port | Implemented in firmware |
| Serial settings | 115200 baud, 8 data bits, no parity, 1 stop bit | Host client configuration |
| DTR | Host client asserts DTR | Required by the firmware session behavior |
| USB VID/PID | 0x0483:0x5740 | ST example placeholder; not assigned to this project |

## DB25 output

| Pin(s) | Signal | Design description |
|---|---|---|
| 2–9 | D0–D7 | Eight active-high marker bits, LSB first |
| 18–25 | GND_ISO | Isolated-side signal reference |
| 1, 10–17 | NC | No connection |

The output word is an 8-bit code. Each output bit is asserted when the corresponding bit in the code is 1. The DB25 connector is not a universal EEG trigger standard: check the receiving instrument's pinout, input thresholds, maximum ratings, polarity, and pulse-capture behavior.

## Power and isolation architecture

- The host side is powered from USB VBUS.
- A controlled power path feeds the boost stage and the primary side of the isolated DC/DC converter.
- The isolated secondary nominal rail is 5 V and supplies the output-side logic/optocoupler circuitry.
- The DB25 signal reference is GND_ISO, not host USB ground.
- The design uses optocouplers and an isolated DC/DC converter.

The 4.40–5.50 V USB input range and approximately 6.3 V boost target are design objectives from the r7.2 engineering design.

## Marker timing

- Firmware default: automatic pulse mode, nominal 2 ms high time and 10 ms minimum gap.
- Configurable pulse width: 1–1000 ms.
- Configurable minimum gap: 0–1000 ms.
- A single D1 output pulse was measured at approximately 2.002 ms on 2026-11-09.
- One test of 100 command/ACK transactions measured host write-to-complete-ACK round trips of 0.1796–0.3522 ms.
- A single MCU-output-to-DB25-output comparison showed approximately 31 ns difference, without calibration for probe/channel skew. During this unloaded-DB25 bench capture, ordinary oscilloscope probe ground clips temporarily commoned host ground and GND_ISO; this is not evidence of isolation performance and must not be repeated while connected to an EEG/DAQ receiver.

These measurements are a limited bench observation, not a latency, jitter, or certification spec. The host ACK time is not the one-way trigger latency. When measuring across the isolation barrier, use appropriate differential/isolated measurement methods; ordinary earth-referenced probe grounds can defeat the barrier.

Use a dummy load for initial tests. Do not connect to valuable acquisition equipment until both sides' electrical requirements and the exact cable pinout have been checked.
