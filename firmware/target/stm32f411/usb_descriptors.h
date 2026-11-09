#ifndef MKR_USB_DESCRIPTORS_H
#define MKR_USB_DESCRIPTORS_H
#include <stdint.h>
/*
 * USB FS CDC descriptors for review enumeration.
 * bmAttributes = 0x80 (bus-powered, no remote wakeup).
 * bMaxPower = 250 (USB units of 2 mA → 500 mA ceiling declared to the host).
 * That ceiling is the r7.1/r7.2 budget cap, not a measured current.
 * USB IDs live only in usb_ids.h. The review pair is ST's public CDC
 * example (0x0483/0x5740). PLACEHOLDER — not a product ID for this board.
 * Do not invent a replacement.
 */
#include "usb_ids.h"
#define MKR_USB_BM_ATTRIBUTES 0x80u
#define MKR_USB_B_MAX_POWER 250u
const uint8_t *mkr_usb_device_descriptor(uint16_t *len);
const uint8_t *mkr_usb_config_descriptor(uint16_t *len);
/* index 0 is LANGID. Unknown index returns 0. */
const uint8_t *mkr_usb_string_descriptor(uint8_t index, uint16_t *len);
#endif
