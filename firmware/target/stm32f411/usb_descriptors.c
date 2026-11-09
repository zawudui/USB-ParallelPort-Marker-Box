#include "usb_descriptors.h"
/*
 * Device class 02 / subclass 02. The CDC control interface is class 02,
 * subclass 02, protocol 01, which is what Windows 10+ usbser.sys matches.
 * Single function, no IAD.
 */
/* idVendor/idProduct bytes are the placeholder pair in usb_ids.h
 * (ST CDC example 0x0483/0x5740). Not this product's USB ID. */
static const uint8_t k_device[] = {
    18u, 0x01u, 0x00u, 0x02u, 0x02u, 0x02u, 0x00u, 64u,
    MKR_USB_VID_LO, MKR_USB_VID_HI, MKR_USB_PID_LO, MKR_USB_PID_HI,
    MKR_USB_BCD_LO, MKR_USB_BCD_HI, 0x01u, 0x02u, 0x03u, 0x01u};
/*
 * 9 config + 9 comm IF + 5 header + 5 call + 4 ACM + 5 union + 7 notify
 * + 9 data IF + 7 bulk OUT + 7 bulk IN = 67.
 * EP 0x82 interrupt IN (notification, 8 bytes), EP 0x01 bulk OUT, EP 0x81 bulk IN.
 */
static const uint8_t k_config[] = {
    0x09u, 0x02u, 67u, 0x00u, 0x02u, 0x01u, 0x00u, MKR_USB_BM_ATTRIBUTES, MKR_USB_B_MAX_POWER,
    0x09u, 0x04u, 0x00u, 0x00u, 0x01u, 0x02u, 0x02u, 0x01u, 0x00u,
    0x05u, 0x24u, 0x00u, 0x10u, 0x01u,
    0x05u, 0x24u, 0x01u, 0x00u, 0x01u,
    0x04u, 0x24u, 0x02u, 0x02u,
    0x05u, 0x24u, 0x06u, 0x00u, 0x01u,
    0x07u, 0x05u, 0x82u, 0x03u, 0x08u, 0x00u, 0x10u,
    0x09u, 0x04u, 0x01u, 0x00u, 0x02u, 0x0Au, 0x00u, 0x00u, 0x00u,
    0x07u, 0x05u, 0x01u, 0x02u, 0x40u, 0x00u, 0x00u,
    0x07u, 0x05u, 0x81u, 0x02u, 0x40u, 0x00u, 0x00u};
_Static_assert(sizeof k_config == 67u, "CDC config wTotalLength");
_Static_assert(MKR_USB_BM_ATTRIBUTES == 0x80u, "bmAttributes bus-powered, no remote wakeup");
_Static_assert(MKR_USB_B_MAX_POWER == 250u, "bMaxPower 250 x 2 mA");
_Static_assert((MKR_USB_BM_ATTRIBUTES & 0x20u) == 0u, "remote wakeup must stay off");
_Static_assert((MKR_USB_BM_ATTRIBUTES & 0x40u) == 0u, "not self-powered");
/* "review" */
static const uint8_t k_lang[] = {4u, 3u, 0x09u, 0x04u};
static const uint8_t k_mfr[] = {14u, 3u, 'r', 0, 'e', 0, 'v', 0, 'i', 0, 'e', 0, 'w', 0};
/* "MarkerBox" */
static const uint8_t k_product[] = {
    20u, 3u, 'M', 0, 'a', 0, 'r', 0, 'k', 0, 'e', 0, 'r', 0, 'B', 0, 'o', 0, 'x', 0};
/* "STAGE2" */
static const uint8_t k_serial[] = {14u, 3u, 'S', 0, 'T', 0, 'A', 0, 'G', 0, 'E', 0, '2', 0};
const uint8_t *mkr_usb_device_descriptor(uint16_t *len)
{
    *len = (uint16_t)sizeof k_device;
    return k_device;
}
const uint8_t *mkr_usb_config_descriptor(uint16_t *len)
{
    *len = (uint16_t)sizeof k_config;
    return k_config;
}
const uint8_t *mkr_usb_string_descriptor(uint8_t index, uint16_t *len)
{
    const uint8_t *p = 0;
    switch (index) {
    case 0:
        p = k_lang;
        break;
    case 1:
        p = k_mfr;
        break;
    case 2:
        p = k_product;
        break;
    case 3:
        p = k_serial;
        break;
    default:
        return 0;
    }
    *len = p[0];
    return p;
}
