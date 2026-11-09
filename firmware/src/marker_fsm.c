#include "marker_fsm.h"
#include "time_us.h"

/* Weak stubs — replace in board support. */
__attribute__((weak)) void mkr_hw_output(uint8_t code) { (void)code; }
__attribute__((weak)) void mkr_hw_set_ready(bool enable) { (void)enable; }
__attribute__((weak)) void mkr_hw_enter_safe(void)
{
    mkr_hw_output(0);
    mkr_hw_set_ready(false);
}

static uint32_t ms_to_us(uint16_t ms)
{
    return (uint32_t)ms * 1000u;
}

static bool deadline_reached(uint32_t now, uint32_t deadline)
{
    return (uint32_t)(now - deadline) < 0x80000000u;
}

static void disarm(mkr_fsm_t *f)
{
    f->deadline_armed = false;
    f->deadline_us = 0u;
    f->timer_ms_left = 0u;
    mkr_hw_disarm_deadline();
}

static void arm_from(mkr_fsm_t *f, uint32_t now, uint32_t width_us)
{
    f->deadline_us = now + width_us;
    f->deadline_armed = true;
    f->timer_ms_left = 0u;
    mkr_hw_arm_deadline(f->deadline_us);
}

void mkr_fsm_force_idle(mkr_fsm_t *f)
{
    f->state = MKR_STATE_IDLE;
    f->current_code = 0u;
    f->ready = false;
    disarm(f);
}

void mkr_fsm_init(mkr_fsm_t *f)
{
    f->state = MKR_STATE_IDLE;
    f->mode = MKR_MODE_PULSE;
    /* Portable default stays 10 ms so host tests and the v0.2 examples do not move.
     * The Stage 2 device runtime applies a 2 ms auto-pulse after this init. */
    f->pulse_ms = 10;
    f->gap_ms = 10;
    f->current_code = 0;
    f->ready = false;
    disarm(f);
    mkr_hw_enter_safe();
}

void mkr_fsm_on_us(mkr_fsm_t *f, uint32_t now_us)
{
    if (!f->deadline_armed || !deadline_reached(now_us, f->deadline_us)) {
        return;
    }
    if (f->state == MKR_STATE_ACTIVE && f->mode == MKR_MODE_PULSE) {
        f->current_code = 0;
        mkr_hw_output(0);
        if (f->gap_ms == 0u) {
            f->state = MKR_STATE_IDLE;
            disarm(f);
        } else {
            f->state = MKR_STATE_GAP;
            arm_from(f, now_us, ms_to_us(f->gap_ms));
        }
        return;
    }
    if (f->state == MKR_STATE_GAP) {
        f->state = MKR_STATE_IDLE;
        disarm(f);
    }
}

void mkr_fsm_tick_1ms(mkr_fsm_t *f)
{
    /* Each call is one millisecond of the software clock. A 2 ms pulse
     * therefore survives the first call. Device pulse edges are TIM2. */
    mkr_time_soft_advance_us(1000u);
    mkr_fsm_on_us(f, mkr_hw_time_us());
}

static bool mkr_busy(const mkr_fsm_t *f)
{
    return f->state == MKR_STATE_ACTIVE || f->state == MKR_STATE_GAP;
}

uint8_t mkr_fsm_send_marker(mkr_fsm_t *f, uint8_t code)
{
    if (!f->ready) {
        return MKR_ST_NACK_NOT_READY;
    }
    if (code == 0) {
        return mkr_fsm_clear(f);
    }
    if (mkr_busy(f)) {
        return MKR_ST_NACK_BUSY; /* no change */
    }
    f->current_code = code;
    mkr_hw_output(code);
    f->state = MKR_STATE_ACTIVE;
    if (f->mode == MKR_MODE_PULSE) {
        /* Sample after the pin rises so the high time is not shorter than
         * the programmed width. IRQ latency can make it longer. */
        arm_from(f, mkr_hw_time_us(), ms_to_us(f->pulse_ms));
    } else {
        disarm(f); /* level hold until Clear */
    }
    return MKR_ST_ACK_ACCEPTED;
}

uint8_t mkr_fsm_clear(mkr_fsm_t *f)
{
    /* Immediate low; from Active still enter Gap (do not skip). */
    f->current_code = 0;
    mkr_hw_output(0);

    if (f->state == MKR_STATE_ACTIVE) {
        if (f->gap_ms == 0u) {
            f->state = MKR_STATE_IDLE;
            disarm(f);
        } else {
            f->state = MKR_STATE_GAP;
            arm_from(f, mkr_hw_time_us(), ms_to_us(f->gap_ms));
        }
        return MKR_ST_ACK_RX_ONLY;
    }
    /* Gap: keep the gap deadline running. Idle: stay Idle. */
    if (f->state == MKR_STATE_IDLE) {
        disarm(f);
    }
    return MKR_ST_ACK_RX_ONLY;
}

uint8_t mkr_fsm_set_mode(mkr_fsm_t *f, uint8_t mode)
{
    if (mode > 1) {
        return MKR_ST_NACK_BAD_PARAM;
    }
    if (mkr_busy(f)) {
        return MKR_ST_NACK_BUSY;
    }
    f->mode = (mkr_mode_t)mode;
    return MKR_ST_ACK_RX_ONLY;
}

uint8_t mkr_fsm_set_pulse(mkr_fsm_t *f, uint16_t ms)
{
    if (ms < 1 || ms > 1000) {
        return MKR_ST_NACK_BAD_PARAM;
    }
    f->pulse_ms = ms;
    return MKR_ST_ACK_RX_ONLY;
}

uint8_t mkr_fsm_set_gap(mkr_fsm_t *f, uint16_t ms)
{
    if (ms > 1000) {
        return MKR_ST_NACK_BAD_PARAM;
    }
    f->gap_ms = ms;
    return MKR_ST_ACK_RX_ONLY;
}
