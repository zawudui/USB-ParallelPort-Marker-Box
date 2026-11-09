#ifndef MKR_USB_ONLY_BRINGUP_H
#define MKR_USB_ONLY_BRINGUP_H
#include "power_sequence.h"
/* USB driver configuration still succeeds, but no event may authorize the
 * isolated power state machine in this explicit bring-up image. */
static inline mkr_power_event_t mkr_usb_only_filter_event(mkr_power_event_t event)
{
    return (event == MKR_POWER_EVENT_USB_CONFIGURED || event == MKR_POWER_EVENT_USB_RESUME)
               ? MKR_POWER_EVENT_USB_UNCONFIGURED : event;
}
#endif
