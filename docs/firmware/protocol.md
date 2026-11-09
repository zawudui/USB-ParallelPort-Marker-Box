# Host protocol

USB CDC serial configuration: 115200 baud, 8 data bits, no parity, 1 stop bit, no flow control. The host client asserts DTR.

Request frame:

`AA LEN CMD PAYLOAD... XOR`

Response frame:

`55 LEN STATUS CMD_ECHO PAYLOAD... XOR`

The XOR checksum is the byte-wise XOR of all preceding bytes in the frame. Multi-byte pulse and gap values are unsigned 16-bit little-endian milliseconds.

| Command | CMD | Payload | Meaning |
|---|---:|---|---|
| SetMode | 0x01 | 1 byte: 1 auto-pulse, 0 hold | Select output mode |
| SetPulse | 0x02 | uint16 LE, 1–1000 | Pulse width in ms |
| SetGap | 0x03 | uint16 LE, 0–1000 | Minimum gap in ms |
| SendMarker | 0x10 | 1 byte, 1–255 | Output marker code |
| Clear | 0x11 | none | Clear outputs |
| GetStatus | 0x20 | none | Read ready/busy/mode and timing |
| GetVersion | 0x21 | none | Read firmware version |

Code 0 sent through SendMarker is treated as Clear. Code 0xAA must be sent inside a SendMarker frame because 0xAA is the request-frame start byte. The firmware defaults to auto-pulse mode, 2 ms pulse width, and 10 ms minimum gap after MCU startup.

A missing ACK does not prove that no marker was emitted. Do not blindly retry an unacknowledged marker, as this may create a duplicate event.
