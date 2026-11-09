#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MKR_STATE_IDLE = 0,
    MKR_STATE_ACTIVE,
    MKR_STATE_GAP
} mkr_state_t;

typedef enum {
    MKR_MODE_LEVEL = 0, /* Mode A */
    MKR_MODE_PULSE = 1  /* Mode B */
} mkr_mode_t;

typedef struct {
    mkr_state_t state;
    mkr_mode_t  mode;
    uint16_t    pulse_ms;     /* default 10 in the portable FSM; device runtime sets 2 */
    uint16_t    gap_ms;       /* default 10 */
    uint8_t     current_code; /* 0 = idle level */
    bool        ready;        /* OE / READY line */
    /* timer_ms_left is not the pulse timebase. Pulse and gap use deadline_us. */
    uint32_t    timer_ms_left;
    uint32_t    deadline_us;
    bool        deadline_armed;
} mkr_fsm_t;

void mkr_fsm_init(mkr_fsm_t *f);
/* Host stepper: advances the software microsecond clock by 1 ms, then services.
 * The device does not use this for pulse width. One call cannot finish a 2 ms pulse. */
void mkr_fsm_tick_1ms(mkr_fsm_t *f);
/* Service pulse/gap against a monotonic microsecond clock. A sample before
 * deadline_us leaves the output unchanged, so a 1 ms tick phase cannot
 * shorten the pulse. */
void mkr_fsm_on_us(mkr_fsm_t *f, uint32_t now_us);
void mkr_fsm_force_idle(mkr_fsm_t *f);

/** @return status byte for response */
uint8_t mkr_fsm_send_marker(mkr_fsm_t *f, uint8_t code);
uint8_t mkr_fsm_clear(mkr_fsm_t *f);
uint8_t mkr_fsm_set_mode(mkr_fsm_t *f, uint8_t mode);
uint8_t mkr_fsm_set_pulse(mkr_fsm_t *f, uint16_t ms);
uint8_t mkr_fsm_set_gap(mkr_fsm_t *f, uint16_t ms);

void mkr_hw_output(uint8_t code);     /* platform: GPIO -> optos */
void mkr_hw_set_ready(bool enable);   /* platform: READY line */
void mkr_hw_enter_safe(void);         /* clear code + ready low */

#ifdef __cplusplus
}
#endif
