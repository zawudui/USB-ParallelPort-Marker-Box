#ifndef MKR_USB_IDS_H
#define MKR_USB_IDS_H
/*
 * PLACEHOLDER REVIEW IDs — NOT A PRODUCT ID.
 *
 * idVendor 0x0483 and idProduct 0x5740 are STMicroelectronics' public CDC
 * example pair, so a review host can try the inbox Windows usbser driver.
 * This project has no USB-IF vendor ID. Do not invent one. Do not print
 * 0483:5740 on a label, manual, or EEPROM as this board's ID.
 *
 * To replace both IDs, edit the two macros below and set
 * MKR_USB_IDS_ARE_PLACEHOLDER to 0 only when a real assignment exists.
 * The device descriptor is built from these macros. Host tests lock the
 * review pair while the flag stays 1.
 *
 * MKR_USB_BCD_DEVICE is bcdDevice 2.10, kept in step with firmware 2.1.0.
 * It is not a hardware revision and not an electrical sign-off.
 */
#include <stdint.h>
#define MKR_USB_VID 0x0483u
#define MKR_USB_PID 0x5740u
#define MKR_USB_IDS_ARE_PLACEHOLDER 1
#define MKR_USB_BCD_DEVICE 0x0210u
#define MKR_USB_VID_LO ((uint8_t)((MKR_USB_VID) & 0xFFu))
#define MKR_USB_VID_HI ((uint8_t)((MKR_USB_VID) >> 8))
#define MKR_USB_PID_LO ((uint8_t)((MKR_USB_PID) & 0xFFu))
#define MKR_USB_PID_HI ((uint8_t)((MKR_USB_PID) >> 8))
#define MKR_USB_BCD_LO ((uint8_t)((MKR_USB_BCD_DEVICE) & 0xFFu))
#define MKR_USB_BCD_HI ((uint8_t)((MKR_USB_BCD_DEVICE) >> 8))
_Static_assert(MKR_USB_IDS_ARE_PLACEHOLDER == 0 || MKR_USB_IDS_ARE_PLACEHOLDER == 1,
               "placeholder flag is 0 or 1");
#endif
