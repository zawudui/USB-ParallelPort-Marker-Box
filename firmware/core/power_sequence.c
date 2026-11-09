#include "power_sequence.h"
#include "time_us.h"
static bool reached(uint32_t now, uint32_t deadline)
{
    /* True when now is at or after deadline, including uint32 wrap.
     * Distances of 2^31 µs or more are treated as still in the future. */
    return (uint32_t)(now - deadline) < 0x80000000u;
}
static void write_marker(mkr_power_sequence_t *s, uint8_t code, bool ready)
{
    if (s->io.write_marker != 0) {
        s->io.write_marker(code, ready, s->io.context);
    }
}
static void set_iso_power(mkr_power_sequence_t *s, bool enabled)
{
    if (s->io.set_iso_power != 0) {
        s->io.set_iso_power(enabled, s->io.context);
    }
}
/* Clear DB25-facing data and internal READY before removing isolated power. */
static void force_safe_off(mkr_power_sequence_t *s)
{
    write_marker(s, 0u, false);
    set_iso_power(s, false);
}
static void begin_enable_delay(mkr_power_sequence_t *s)
{
    const uint32_t now = mkr_hw_time_us();
    force_safe_off(s);
    s->state = MKR_POWER_STATE_WAIT_CONFIG_DELAY;
    s->timer_ms = MKR_POWER_CONFIG_DELAY_MS;
    s->deadline_us = now + (uint32_t)MKR_POWER_CONFIG_DELAY_MS * 1000u;
}
static void begin_iso_settle(mkr_power_sequence_t *s)
{
    const uint32_t now = mkr_hw_time_us();
    set_iso_power(s, true);
    s->state = MKR_POWER_STATE_WAIT_ISO_STABLE;
    s->timer_ms = MKR_POWER_ISO_SETTLE_MS;
    s->deadline_us = now + (uint32_t)MKR_POWER_ISO_SETTLE_MS * 1000u;
}
static void enter_unconfigured(mkr_power_sequence_t *s)
{
    s->usb_configured = false;
    force_safe_off(s);
    s->state = MKR_POWER_STATE_UNCONFIGURED;
    s->timer_ms = 0u;
    s->deadline_us = 0u;
}
void mkr_power_sequence_init(mkr_power_sequence_t *s, const mkr_power_io_t *io)
{
    s->state = MKR_POWER_STATE_UNCONFIGURED;
    s->timer_ms = 0u;
    s->deadline_us = 0u;
    s->usb_configured = false;
    if (io != 0) {
        s->io = *io;
    } else {
        s->io.write_marker = 0;
        s->io.set_iso_power = 0;
        s->io.context = 0;
    }
    force_safe_off(s);
}
void mkr_power_sequence_handle_event(mkr_power_sequence_t *s, mkr_power_event_t e)
{
    if (e == MKR_POWER_EVENT_CLOCK_FAULT) {
        force_safe_off(s);
        s->state = MKR_POWER_STATE_FAULT;
        s->timer_ms = 0u;
        s->deadline_us = 0u;
        s->usb_configured = false;
        return;
    }
    if (s->state == MKR_POWER_STATE_FAULT) {
        return;
    }
    switch (e) {
    case MKR_POWER_EVENT_USB_CONFIGURED:
        /* Duplicate CONFIGURED events restart the full sequence. */
        s->usb_configured = true;
        begin_enable_delay(s);
        break;
    case MKR_POWER_EVENT_USB_UNCONFIGURED:
    case MKR_POWER_EVENT_USB_BUS_RESET:
        enter_unconfigured(s);
        break;
    case MKR_POWER_EVENT_USB_SUSPEND:
        force_safe_off(s);
        s->state = MKR_POWER_STATE_SUSPENDED;
        s->timer_ms = 0u;
        s->deadline_us = 0u;
        break;
    case MKR_POWER_EVENT_USB_RESUME:
        if (s->state != MKR_POWER_STATE_SUSPENDED) {
            break;
        }
        if (s->usb_configured) {
            begin_enable_delay(s);
        } else {
            s->state = MKR_POWER_STATE_UNCONFIGURED;
            s->timer_ms = 0u;
            s->deadline_us = 0u;
        }
        break;
    case MKR_POWER_EVENT_CLOCK_FAULT:
        break;
    default:
        force_safe_off(s);
        s->state = MKR_POWER_STATE_FAULT;
        s->timer_ms = 0u;
        s->deadline_us = 0u;
        s->usb_configured = false;
        break;
    }
}
void mkr_power_sequence_tick_1ms(mkr_power_sequence_t *s)
{
    const uint32_t now = mkr_hw_time_us();
    switch (s->state) {
    case MKR_POWER_STATE_WAIT_CONFIG_DELAY:
        if (!reached(now, s->deadline_us)) {
            break;
        }
        if (!s->usb_configured) {
            enter_unconfigured(s);
            return;
        }
        begin_iso_settle(s);
        break;
    case MKR_POWER_STATE_WAIT_ISO_STABLE:
        if (!reached(now, s->deadline_us)) {
            break;
        }
        if (!s->usb_configured) {
            enter_unconfigured(s);
            return;
        }
        write_marker(s, 0u, true);
        s->state = MKR_POWER_STATE_READY;
        break;
    case MKR_POWER_STATE_UNCONFIGURED:
    case MKR_POWER_STATE_READY:
    case MKR_POWER_STATE_SUSPENDED:
    case MKR_POWER_STATE_FAULT:
    default:
        break;
    }
}
