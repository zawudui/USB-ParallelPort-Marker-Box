# DB25 interface

Connector: DB25 female socket. Pin numbers below refer to the mating face of the device connector; verify orientation before wiring.

| Pin | Signal | Meaning |
|---|---|---|
| 2 | D0 | Marker bit 0 (LSB), active high |
| 3 | D1 | Marker bit 1 |
| 4 | D2 | Marker bit 2 |
| 5 | D3 | Marker bit 3 |
| 6 | D4 | Marker bit 4 |
| 7 | D5 | Marker bit 5 |
| 8 | D6 | Marker bit 6 |
| 9 | D7 | Marker bit 7 (MSB) |
| 18–25 | GND_ISO | Isolated-side signal reference |
| 1, 10–17 | NC | Not connected |

The output code is represented as an 8-bit value across D0–D7. Outputs are active-high. The receiving device's DB25 pinout, input threshold, maximum voltage, polarity, and pulse-width requirements must be checked independently. A DB25 connector does not imply a universal trigger standard.
