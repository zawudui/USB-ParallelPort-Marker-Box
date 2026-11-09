# USB VID/PID decision guide for USB-DB25 Marker Box

Current firmware descriptor: **0x0483:0x5740** (STMicroelectronics CDC example pair). The manufacturer string is `review`; the serial string is a fixed review-stage value. These are engineering placeholders, **not an assigned project USB identity**. Current firmware CI artifacts are review-only.

## Legitimate identifier routes

1. **Own an assigned VID:** Apply through USB-IF and allocate a product-specific PID under that VID. See https://www.usb.org/getting-vendor-id and https://usb.org/developers .
2. **Use an approved/contractual product-ID allocation under another VID:** Obtain explicit written authorization from the VID holder and verify the terms cover the intended commercial products, devices and firmware releases. Do not assume possession of the component, development kit or ST SDK grants use of the ST example VID/PID.
3. **Community registries for open-source projects:** The independently administered https://pid.codes/howto/ describes allocation under VID 0x1209 to eligible open-source hardware projects. This service is **not endorsed by USB-IF**; its FAQ and USB-IF treatment of obsolete/invalid identifiers must be reviewed before choosing it, particularly for commercial products. Do not invent or claim an allocation.

**No value is allocated in this repository.** The choice must be documented and approved by the project owner before production or shipment.

## Required implementation checklist after receiving authorization

- Record the authorizing body, allocation evidence, exact VID/PID, product description and permissions in the internal compliance record. Avoid publishing personal or confidential allocation documents.
- Change `firmware/core/usb_ids.h` with the authorized VID/PID and switch `MKR_USB_IDS_ARE_PLACEHOLDER` to `0`. Search the entire firmware, tests, examples and documentation for `0483`, `5740` and `STAGE2`; remove obsolete product-identification assumptions.
- Review all USB descriptors, manufacturer/product/serial string generation and Windows host association. Use unique serial identifiers when required for stable enumeration of multiple units; do not retain the shared `STAGE2` string as the production serial identity.
- Update descriptor/unit tests, rebuild host tests and the target firmware, capture the exact source commit and HEX/BIN hashes.
- Program and **physically enumerate** actual boards under the assigned IDs on the relevant host OSes; check USB CDC driver binding, reconnect behavior, multiple-device behavior and the host client. Verify the DB25 output separately.
- Update the manual/README and product labelling; do not treat CI success as proof of enumerating real hardware.

## Release boundary

Publishing documented source with non-production example descriptors is a different decision from distributing a finished, ready-to-flash **production** binary or commercially selling hardware. Do not present ST example identifiers as authorized product IDs; require the owner to decide whether a public source release will deliberately retain review-only sample firmware. Never silently substitute an unassigned VID/PID.

Sources: https://www.st.com/resource/en/user_manual/um1717-stm32f0x2xx-usb-full-speed-device-library-stmicroelectronics.pdf ; https://www.usb.org/getting-vendor-id ; https://pid.codes/howto/ .
