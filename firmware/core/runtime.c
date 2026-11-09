#include "runtime.h"
#include "protocol_dispatch.h"
#include "time_us.h"
static mkr_runtime_t *g_rt;
static void publish(mkr_runtime_t *rt, uint8_t code, bool ready)
{
    rt->code = ready ? code : 0u;
    if (rt->hw_write != 0) {
        rt->hw_write(rt->code, ready, rt->ctx);
    }
}
static void quiesce(mkr_runtime_t *rt)
{
    rt->outputs_live = false;
    mkr_fsm_force_idle(&rt->fsm);
    rt->code = 0u;
}
static void on_power_marker(uint8_t code, bool ready, void *ctx)
{
    mkr_runtime_t *rt = ctx;
    (void)code; /* power_sequence publishes 0; the FSM owns the data code */
    if (!ready) {
        quiesce(rt);
        publish(rt, 0u, false);
        return;
    }
    rt->outputs_live = true;
    rt->fsm.ready = true;
    publish(rt, rt->fsm.current_code, true);
}
static void on_power_iso(bool enabled, void *ctx)
{
    mkr_runtime_t *rt = ctx;
    if (rt->set_iso != 0) {
        rt->set_iso(enabled, rt->ctx);
    }
}
static void lock_rt(mkr_runtime_t *rt)
{
    if (rt->lock != 0) {
        rt->lock(rt->ctx);
    }
}
static void unlock_rt(mkr_runtime_t *rt)
{
    if (rt->unlock != 0) {
        rt->unlock(rt->ctx);
    }
}
static void tx_bytes(mkr_runtime_t *rt, const uint8_t *data, uint16_t len)
{
    if (rt->tx != 0 && len > 0u) {
        rt->tx(data, len, rt->ctx);
    }
}
void mkr_hw_output(uint8_t code)
{
    if (g_rt == 0) {
        return;
    }
    g_rt->code = code;
    if (g_rt->outputs_live && g_rt->fsm.ready) {
        publish(g_rt, code, true);
    } else {
        publish(g_rt, 0u, false);
    }
}
void mkr_hw_set_ready(bool enable)
{
    if (g_rt == 0) {
        return;
    }
    if (enable && g_rt->outputs_live && g_rt->fsm.ready) {
        publish(g_rt, g_rt->code, true);
    } else {
        publish(g_rt, 0u, false);
    }
}
void mkr_runtime_init(mkr_runtime_t *rt, const mkr_runtime_io_t *io)
{
    rt->outputs_live = false;
    rt->code = 0u;
    rt->hw_write = 0;
    rt->set_iso = 0;
    rt->tx = 0;
    rt->lock = 0;
    rt->unlock = 0;
    rt->ctx = 0;
    mkr_parser_init(&rt->parser);
    if (io != 0) {
        rt->hw_write = io->write_marker;
        rt->set_iso = io->set_iso_power;
        rt->tx = io->tx;
        rt->lock = io->lock;
        rt->unlock = io->unlock;
        rt->ctx = io->ctx;
    }
    g_rt = rt;
    lock_rt(rt);
    {
        const mkr_power_io_t pio = {on_power_marker, on_power_iso, rt};
        mkr_power_sequence_init(&rt->power, &pio);
    }
    mkr_fsm_init(&rt->fsm);
    (void)mkr_fsm_set_pulse(&rt->fsm, MKR_DEVICE_DEFAULT_PULSE_MS);
    unlock_rt(rt);
}
void mkr_runtime_on_usb_event(mkr_runtime_t *rt, mkr_power_event_t event)
{
    lock_rt(rt);
    mkr_power_sequence_handle_event(&rt->power, event);
    unlock_rt(rt);
}
static void service_marker(mkr_runtime_t *rt)
{
    if (rt->outputs_live && rt->fsm.ready) {
        mkr_fsm_on_us(&rt->fsm, mkr_hw_time_us());
        return;
    }
    if (rt->fsm.deadline_armed) {
        mkr_fsm_force_idle(&rt->fsm);
    }
}
void mkr_runtime_on_tick_1ms(mkr_runtime_t *rt)
{
    lock_rt(rt);
    mkr_power_sequence_tick_1ms(&rt->power);
    (void)mkr_parser_tick_1ms(&rt->parser);
    /* Backup for a missed compare: a tick before the deadline does not
     * end the pulse. The TIM2 ISR is the on-time edge. */
    service_marker(rt);
    unlock_rt(rt);
}
void mkr_runtime_on_marker_deadline(mkr_runtime_t *rt)
{
    if (rt == 0) {
        return;
    }
    lock_rt(rt);
    service_marker(rt);
    unlock_rt(rt);
}
static void feed_byte(mkr_runtime_t *rt, uint8_t byte)
{
    const mkr_parser_status_t st = mkr_parser_feed(&rt->parser, byte);
    if (st == MKR_PARSER_RAW) {
        mkr_dispatch_raw(&rt->fsm, rt->parser.raw);
        return;
    }
    if (st == MKR_PARSER_FRAME) {
        mkr_response_t resp;
        mkr_dispatch_frame(&rt->fsm, rt->parser.buf, rt->parser.filled, &resp);
        if (resp.respond) {
            tx_bytes(rt, resp.frame, resp.length);
        }
        return;
    }
    if (st == MKR_PARSER_BAD_CHK && rt->parser.filled >= 3u) {
        uint8_t frame[5];
        const uint8_t n = mkr_build_simple_response(MKR_ST_NACK_BAD_CHK, rt->parser.buf[2], frame, sizeof frame);
        tx_bytes(rt, frame, n);
    }
}
void mkr_runtime_on_rx(mkr_runtime_t *rt, const uint8_t *data, size_t len)
{
    lock_rt(rt);
    for (size_t i = 0; i < len; i++) {
        feed_byte(rt, data[i]);
    }
    unlock_rt(rt);
}
void mkr_runtime_latch_clock_fault(mkr_runtime_t *rt)
{
    if (rt == 0) {
        return;
    }
    quiesce(rt);
    rt->power.state = MKR_POWER_STATE_FAULT;
    rt->power.timer_ms = 0u;
    rt->power.deadline_us = 0u;
    rt->power.usb_configured = false;
    if (rt->hw_write != 0) {
        rt->hw_write(0u, false, rt->ctx);
    }
    if (rt->set_iso != 0) {
        rt->set_iso(false, rt->ctx);
    }
}
