#ifndef MKR_POWER_SEQUENCE_H
#define MKR_POWER_SEQUENCE_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Floor after a successful USB configuration (Status IN already done by
 * the device controller) before PA15 may rise. The sequence stores
 * deadline_us = now + 10_000 µs. A 1 ms tick can observe that deadline
 * late. It cannot enable PA15 early, including when the event falls 1 µs
 * after a tick. Worst-case enable on a 1 ms grid is just under 11 ms.
 * This is not a scope capture.
 */
#define MKR_POWER_CONFIG_DELAY_MS 10u
/*
 * USB suspend reaches SUSPENDED only after markers/READY, then PA15, are off.
 * The target enters MCU Stop from the main loop after that transition.
 * Resume still follows the rules in this file, including "ISO power only if
 * the device is still configured". Nothing here is a measured suspend current.
 *
 * Provisional software guard only: 100 ms has NOT been measured on r7.2 and
 * is NOT an electrical sign-off. It does not encode a verified margin for
 * TPS61085 soft-start, the Q1 gate ramp, or TMR startup. The same deadline
 * rule as the 10 ms floor applies, so a 1 ms tick cannot end this wait
 * early. Set the final value only after oscilloscope verification under
 * docs/r7.2/验证与投板门禁.md.
 */
#define MKR_POWER_ISO_SETTLE_MS 100u
typedef enum {
    MKR_POWER_STATE_UNCONFIGURED = 0,
    MKR_POWER_STATE_WAIT_CONFIG_DELAY,
    MKR_POWER_STATE_WAIT_ISO_STABLE,
    MKR_POWER_STATE_READY,
    MKR_POWER_STATE_SUSPENDED,
    MKR_POWER_STATE_FAULT
} mkr_power_state_t;
typedef enum {
    MKR_POWER_EVENT_USB_CONFIGURED = 0,
    MKR_POWER_EVENT_USB_UNCONFIGURED,
    MKR_POWER_EVENT_USB_BUS_RESET,
    MKR_POWER_EVENT_USB_SUSPEND,
    MKR_POWER_EVENT_USB_RESUME,
    MKR_POWER_EVENT_CLOCK_FAULT
} mkr_power_event_t;
typedef struct {
    /* Target implementation must atomically update PB0-PB8 via one BSRR write. */
    void (*write_marker)(uint8_t code, bool ready, void *context);
    void (*set_iso_power)(bool enabled, void *context);
    void *context;
} mkr_power_io_t;
typedef struct {
    mkr_power_state_t state;
    /* Nominal floor in milliseconds. Not a countdown. deadline_us is the gate. */
    uint16_t timer_ms;
    uint32_t deadline_us;
    bool usb_configured;
    mkr_power_io_t io;
} mkr_power_sequence_t;
void mkr_power_sequence_init(mkr_power_sequence_t *sequence, const mkr_power_io_t *io);
void mkr_power_sequence_handle_event(mkr_power_sequence_t *sequence, mkr_power_event_t event);
void mkr_power_sequence_tick_1ms(mkr_power_sequence_t *sequence);
#ifdef __cplusplus
}
#endif
#endif
