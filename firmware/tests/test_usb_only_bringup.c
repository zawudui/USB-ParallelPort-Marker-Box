#include "usb_only_bringup.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
static uint32_t now;
static unsigned unsafe_writes;
uint32_t mkr_hw_time_us(void) { return now; }
static void marker(uint8_t code, bool ready, void *ctx)
{
    (void)ctx;
    if (code != 0u || ready) { ++unsafe_writes; }
}
static void iso(bool enabled, void *ctx)
{
    (void)ctx;
    if (enabled) { ++unsafe_writes; }
}
int main(void)
{
    mkr_power_sequence_t power;
    const mkr_power_io_t io = {marker, iso, 0};
    mkr_power_sequence_init(&power, &io);
    const mkr_power_event_t events[] = {
        MKR_POWER_EVENT_USB_CONFIGURED, MKR_POWER_EVENT_USB_SUSPEND,
        MKR_POWER_EVENT_USB_RESUME, MKR_POWER_EVENT_USB_CONFIGURED,
        MKR_POWER_EVENT_USB_BUS_RESET, MKR_POWER_EVENT_USB_RESUME};
    for (unsigned i = 0u; i < sizeof events / sizeof events[0]; ++i) {
        mkr_power_sequence_handle_event(&power, mkr_usb_only_filter_event(events[i]));
        for (unsigned tick = 0u; tick < 250u; ++tick) {
            now += 1000u;
            mkr_power_sequence_tick_1ms(&power);
        }
        assert(!power.usb_configured);
        assert(unsafe_writes == 0u);
    }
    mkr_power_sequence_handle_event(&power, mkr_usb_only_filter_event(MKR_POWER_EVENT_CLOCK_FAULT));
    mkr_power_sequence_handle_event(&power, mkr_usb_only_filter_event(MKR_POWER_EVENT_USB_CONFIGURED));
    assert(power.state == MKR_POWER_STATE_FAULT);
    assert(unsafe_writes == 0u);
    return 0;
}
