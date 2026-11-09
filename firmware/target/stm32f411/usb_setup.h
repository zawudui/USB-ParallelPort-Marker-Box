#ifndef MKR_USB_SETUP_H
#define MKR_USB_SETUP_H
#include <stdbool.h>
#include <stdint.h>
typedef enum {
    MKR_USB_SETUP_STALL = 0,
    MKR_USB_SETUP_STATUS_IN,
    MKR_USB_SETUP_IN_DATA,
    MKR_USB_SETUP_SET_LINE_CODING,
    MKR_USB_SETUP_SET_ADDRESS,
    MKR_USB_SETUP_CONFIGURED,
    MKR_USB_SETUP_UNCONFIGURED,
    MKR_USB_SETUP_HALT_EP,
    MKR_USB_SETUP_CLEAR_HALT
} mkr_usb_setup_kind_t;
typedef struct {
    mkr_usb_setup_kind_t kind;
    uint16_t value;
    const uint8_t *data;
    uint16_t len;
    uint16_t w_length;
} mkr_usb_setup_result_t;
/* line is the current 7-byte CDC line coding, used by GET_LINE_CODING. */
void mkr_usb_setup_decode(const uint8_t setup[8], bool configured, const uint8_t line[7], mkr_usb_setup_result_t *out);
#endif
