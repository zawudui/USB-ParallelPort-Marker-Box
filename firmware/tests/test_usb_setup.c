#include "usb_setup.h"
#include <assert.h>
#include <string.h>
static void setup(uint8_t t, uint8_t r, uint16_t value, uint16_t index, uint16_t length, uint8_t out[8])
{
    memset(out, 0, 8u);
    out[0] = t;
    out[1] = r;
    out[2] = (uint8_t)value;
    out[3] = (uint8_t)(value >> 8);
    out[4] = (uint8_t)index;
    out[5] = (uint8_t)(index >> 8);
    out[6] = (uint8_t)length;
    out[7] = (uint8_t)(length >> 8);
}
int main(void)
{
    uint8_t s[8];
    uint8_t line[7] = {0x00u, 0xC2u, 0x01u, 0x00u, 0x00u, 0x00u, 0x08u};
    mkr_usb_setup_result_t o;
    setup(0x00u, 0x09u, 1u, 0u, 0u, s);
    mkr_usb_setup_decode(s, false, line, &o);
    assert(o.kind == MKR_USB_SETUP_CONFIGURED);
    setup(0x00u, 0x09u, 0u, 0u, 0u, s);
    mkr_usb_setup_decode(s, true, line, &o);
    assert(o.kind == MKR_USB_SETUP_UNCONFIGURED);
    setup(0x00u, 0x09u, 2u, 0u, 0u, s);
    mkr_usb_setup_decode(s, false, line, &o);
    assert(o.kind == MKR_USB_SETUP_STALL);
    setup(0x00u, 0x03u, 1u, 0u, 0u, s);
    mkr_usb_setup_decode(s, true, line, &o);
    assert(o.kind == MKR_USB_SETUP_STALL);
    setup(0x00u, 0x05u, 5u, 0u, 0u, s);
    mkr_usb_setup_decode(s, false, line, &o);
    assert(o.kind == MKR_USB_SETUP_SET_ADDRESS && o.value == 5u);
    setup(0x80u, 0x06u, 0x0200u, 0u, 255u, s);
    mkr_usb_setup_decode(s, false, line, &o);
    assert(o.kind == MKR_USB_SETUP_IN_DATA && o.data[7] == 0x80u && o.data[8] == 250u);
    assert(o.len == 67u && o.w_length == 255u);
    setup(0x80u, 0x00u, 0u, 0u, 2u, s);
    mkr_usb_setup_decode(s, true, line, &o);
    assert(o.kind == MKR_USB_SETUP_IN_DATA && o.data[0] == 0u && o.data[1] == 0u);
    setup(0x21u, 0x20u, 0u, 0u, 7u, s);
    mkr_usb_setup_decode(s, true, line, &o);
    assert(o.kind == MKR_USB_SETUP_SET_LINE_CODING);
    setup(0xA1u, 0x21u, 0u, 0u, 7u, s);
    mkr_usb_setup_decode(s, true, line, &o);
    assert(o.kind == MKR_USB_SETUP_IN_DATA && o.len == 7u && o.data == line);
    setup(0x21u, 0x22u, 1u, 0u, 0u, s);
    mkr_usb_setup_decode(s, true, line, &o);
    assert(o.kind == MKR_USB_SETUP_STATUS_IN);
    setup(0x80u, 0x06u, 0x0600u, 0u, 10u, s);
    mkr_usb_setup_decode(s, false, line, &o);
    assert(o.kind == MKR_USB_SETUP_STALL);
    return 0;
}
