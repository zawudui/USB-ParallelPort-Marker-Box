#include "protocol_dispatch.h"
#include "usb_descriptors.h"
#include "usb_ids.h"
#include <assert.h>
int main(void)
{
    uint16_t len = 0u;
    const uint8_t *dev = mkr_usb_device_descriptor(&len);
    const uint8_t *cfg = mkr_usb_config_descriptor(&len);
    const uint8_t *lang = mkr_usb_string_descriptor(0u, &len);
    /* PLACEHOLDER. ST's public CDC example, not an ID assigned to this
     * project. Do not invent a replacement here. A green host test is not
     * USB enumeration on a board. */
    assert(MKR_USB_IDS_ARE_PLACEHOLDER == 1);
    assert(MKR_USB_VID == 0x0483u && MKR_USB_PID == 0x5740u);
    assert(dev[7] == 64u);
    assert(dev[8] == MKR_USB_VID_LO && dev[8] == 0x83u);
    assert(dev[9] == MKR_USB_VID_HI && dev[9] == 0x04u);
    assert(dev[10] == MKR_USB_PID_LO && dev[10] == 0x40u);
    assert(dev[11] == MKR_USB_PID_HI && dev[11] == 0x57u);
    assert(dev[12] == MKR_USB_BCD_LO && dev[13] == MKR_USB_BCD_HI);
    assert(MKR_USB_BCD_DEVICE == 0x0210u);
    assert(((MKR_FW_VERSION_MAJOR << 8) | (MKR_FW_VERSION_MINOR << 4) | MKR_FW_VERSION_PATCH) ==
           MKR_USB_BCD_DEVICE);
    assert(cfg[0] == 9u && cfg[1] == 2u);
    assert(cfg[2] == 67u && cfg[3] == 0u);
    assert(cfg[4] == 2u && cfg[5] == 1u);
    assert(cfg[7] == MKR_USB_BM_ATTRIBUTES && cfg[7] == 0x80u);
    assert(cfg[8] == MKR_USB_B_MAX_POWER && cfg[8] == 250u);
    assert((cfg[7] & 0xA0u) == 0x80u);
    assert(cfg[14] == 0x02u && cfg[15] == 0x02u && cfg[16] == 0x01u);
    assert(cfg[39] == 0x82u && cfg[55] == 0x01u && cfg[62] == 0x81u);
    assert(lang[0] == 4u && lang[2] == 0x09u && lang[3] == 0x04u);
    assert(mkr_usb_string_descriptor(9u, &len) == 0);
    return 0;
}
