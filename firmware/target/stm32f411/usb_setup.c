#include "usb_setup.h"
#include "usb_descriptors.h"
static uint16_t le16(const uint8_t *p)
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}
static void stall(mkr_usb_setup_result_t *out)
{
    out->kind = MKR_USB_SETUP_STALL;
    out->value = 0u;
    out->data = 0;
    out->len = 0u;
    out->w_length = 0u;
}
static void status_in(mkr_usb_setup_result_t *out)
{
    out->kind = MKR_USB_SETUP_STATUS_IN;
    out->value = 0u;
    out->data = 0;
    out->len = 0u;
    out->w_length = 0u;
}
static void in_data(mkr_usb_setup_result_t *out, const uint8_t *data, uint16_t len, uint16_t w_length)
{
    out->kind = MKR_USB_SETUP_IN_DATA;
    out->value = 0u;
    out->data = data;
    out->len = len;
    out->w_length = w_length;
}
void mkr_usb_setup_decode(const uint8_t s[8], bool configured, const uint8_t line[7], mkr_usb_setup_result_t *out)
{
    const uint8_t type = s[0];
    const uint8_t req = s[1];
    const uint16_t w_value = le16(&s[2]);
    const uint16_t w_index = le16(&s[4]);
    const uint16_t w_length = le16(&s[6]);
    stall(out);
    out->w_length = w_length;
    /* CDC class, interface recipient. */
    if (type == 0x21u && req == 0x20u) {
        if (w_index == 0u && w_length == 7u) {
            out->kind = MKR_USB_SETUP_SET_LINE_CODING;
        }
        return;
    }
    if (type == 0x21u && req == 0x22u && w_index == 0u && w_length == 0u) {
        status_in(out);
        return;
    }
    if (type == 0xA1u && req == 0x21u && w_index == 0u && line != 0) {
        in_data(out, line, 7u, w_length);
        return;
    }
    /* Standard device. */
    if (type == 0x00u && req == 0x05u && w_length == 0u) {
        if (w_value <= 127u) {
            out->kind = MKR_USB_SETUP_SET_ADDRESS;
            out->value = w_value;
        }
        return;
    }
    if (type == 0x00u && req == 0x09u && w_length == 0u && w_index == 0u) {
        if (w_value == 1u) {
            out->kind = MKR_USB_SETUP_CONFIGURED;
            out->value = 1u;
        } else if (w_value == 0u) {
            out->kind = MKR_USB_SETUP_UNCONFIGURED;
        }
        return;
    }
    if (type == 0x00u && req == 0x03u && w_length == 0u) {
        /* Device remote wakeup is not advertised. Stall it. */
        return;
    }
    if (type == 0x02u && req == 0x03u && w_value == 0u && w_length == 0u) {
        out->kind = MKR_USB_SETUP_HALT_EP;
        out->value = w_index;
        return;
    }
    if (type == 0x02u && req == 0x01u && w_value == 0u && w_length == 0u) {
        out->kind = MKR_USB_SETUP_CLEAR_HALT;
        out->value = w_index;
        return;
    }
    if (type == 0x01u && req == 0x0Bu && w_length == 0u && w_value == 0u && w_index <= 1u) {
        status_in(out);
        return;
    }
    if (type == 0x80u && req == 0x06u) {
        const uint8_t desc_type = (uint8_t)(w_value >> 8);
        const uint8_t desc_index = (uint8_t)(w_value & 0xFFu);
        uint16_t len = 0u;
        const uint8_t *data = 0;
        if (desc_type == 1u && desc_index == 0u) {
            data = mkr_usb_device_descriptor(&len);
        } else if (desc_type == 2u && desc_index == 0u) {
            data = mkr_usb_config_descriptor(&len);
        } else if (desc_type == 3u) {
            data = mkr_usb_string_descriptor(desc_index, &len);
        }
        if (data != 0) {
            in_data(out, data, len, w_length);
        }
        return;
    }
    if (type == 0x80u && req == 0x08u && w_length >= 1u) {
        static uint8_t cfg_byte;
        cfg_byte = configured ? 1u : 0u;
        in_data(out, &cfg_byte, 1u, w_length);
        return;
    }
    if (type == 0x80u && req == 0x00u && w_length >= 2u) {
        /* Bus-powered, remote wakeup disabled. */
        static const uint8_t status_bits[2] = {0x00u, 0x00u};
        in_data(out, status_bits, 2u, w_length);
        return;
    }
    if (type == 0x81u && req == 0x00u && w_length >= 2u && w_index <= 1u) {
        static const uint8_t status_bits[2] = {0x00u, 0x00u};
        in_data(out, status_bits, 2u, w_length);
        return;
    }
    if (type == 0x81u && req == 0x0Au && w_length >= 1u && w_index <= 1u) {
        static const uint8_t alt0 = 0u;
        in_data(out, &alt0, 1u, w_length);
        return;
    }
    if (type == 0x82u && req == 0x00u && w_length >= 2u) {
        static const uint8_t status_bits[2] = {0x00u, 0x00u};
        in_data(out, status_bits, 2u, w_length);
        return;
    }
}
