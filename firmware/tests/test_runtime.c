#include "runtime.h"
#include "time_us.h"
#include <assert.h>
#include <string.h>
typedef struct {
    int depth;
    uint8_t code;
    bool ready;
    bool power;
    uint8_t order[8];
    unsigned order_n;
    uint8_t tx[32];
    uint8_t tx_len;
} fake_t;
static void wr(uint8_t code, bool ready, void *ctx)
{
    fake_t *f = ctx;
    assert(f->depth > 0);
    f->code = code;
    f->ready = ready;
    if (f->order_n < 8u) {
        f->order[f->order_n++] = 1u;
    }
}
static void pw(bool enabled, void *ctx)
{
    fake_t *f = ctx;
    assert(f->depth > 0);
    f->power = enabled;
    if (f->order_n < 8u) {
        f->order[f->order_n++] = 2u;
    }
}
static void tx(const uint8_t *data, uint16_t len, void *ctx)
{
    fake_t *f = ctx;
    assert(f->depth > 0);
    f->tx_len = (uint8_t)len;
    memcpy(f->tx, data, len);
}
static void lock(void *ctx) { ((fake_t *)ctx)->depth++; }
static void unlock(void *ctx)
{
    fake_t *f = ctx;
    assert(f->depth > 0);
    f->depth--;
}
static void one_ms(mkr_runtime_t *rt)
{
    mkr_time_soft_advance_us(1000u);
    mkr_runtime_on_tick_1ms(rt);
}
static void ticks(mkr_runtime_t *rt, unsigned n)
{
    for (unsigned i = 0u; i < n; i++) {
        one_ms(rt);
    }
}
static void reach_ready(mkr_runtime_t *rt, fake_t *f)
{
    mkr_runtime_on_usb_event(rt, MKR_POWER_EVENT_USB_CONFIGURED);
    ticks(rt, MKR_POWER_CONFIG_DELAY_MS - 1u);
    assert(!f->power);
    one_ms(rt);
    assert(f->power && !f->ready);
    ticks(rt, MKR_POWER_ISO_SETTLE_MS - 1u);
    assert(!f->ready);
    one_ms(rt);
    assert(rt->power.state == MKR_POWER_STATE_READY && f->ready && f->power && f->code == 0u);
}
int main(void)
{
    fake_t f;
    mkr_runtime_t rt;
    const mkr_runtime_io_t io = {wr, pw, tx, lock, unlock, &f};
    const uint8_t frame[5] = {0xAAu, 0x03u, 0x10u, 0x2Au, 0x93u};
    const uint8_t ack[5] = {0x55u, 0x03u, 0x00u, 0x10u, 0x46u};
    memset(&f, 0, sizeof f);
    mkr_runtime_init(&rt, &io);
    assert(f.depth == 0);
    assert(!f.power && !f.ready && f.code == 0u);
    assert(rt.fsm.mode == MKR_MODE_PULSE && rt.fsm.pulse_ms == MKR_DEVICE_DEFAULT_PULSE_MS);
    assert(rt.fsm.gap_ms == 10u);
    f.tx_len = 0u;
    mkr_runtime_on_rx(&rt, frame, 5u);
    assert(f.tx_len == 5u && f.tx[2] == MKR_ST_NACK_NOT_READY);
    assert(f.code == 0u && !f.power);
    reach_ready(&rt, &f);
    f.tx_len = 0u;
    mkr_runtime_on_rx(&rt, frame, 5u);
    assert(f.code == 42u && f.ready);
    for (unsigned i = 0u; i < 5u; i++) {
        assert(f.tx[i] == ack[i]);
    }
    /* 2 ms pulse stays high across a 1 ms tick and at 1.999 ms. */
    one_ms(&rt);
    assert(f.code == 42u && rt.fsm.state == MKR_STATE_ACTIVE);
    mkr_time_soft_advance_us(999u);
    mkr_runtime_on_tick_1ms(&rt);
    assert(f.code == 42u && rt.fsm.state == MKR_STATE_ACTIVE);
    mkr_time_soft_advance_us(1u);
    mkr_runtime_on_tick_1ms(&rt);
    assert(f.code == 0u && f.ready && rt.fsm.state == MKR_STATE_GAP);
    mkr_time_soft_advance_us(9999u);
    mkr_runtime_on_tick_1ms(&rt);
    assert(rt.fsm.state == MKR_STATE_GAP && f.code == 0u);
    mkr_time_soft_advance_us(1u);
    mkr_runtime_on_tick_1ms(&rt);
    assert(rt.fsm.state == MKR_STATE_IDLE && f.code == 0u && f.ready);
    {
        const uint8_t bad[5] = {0xAAu, 0x03u, 0x10u, 0x2Au, 0x00u};
        const uint8_t nack[5] = {0x55u, 0x03u, 0x12u, 0x10u, 0x54u};
        mkr_runtime_on_rx(&rt, bad, 5u);
        assert(f.code == 0u && f.ready);
        for (unsigned i = 0u; i < 5u; i++) {
            assert(f.tx[i] == nack[i]);
        }
    }
    {
        const uint8_t mode[5] = {0xAAu, 0x03u, 0x01u, 0x00u, (uint8_t)(0xAAu ^ 0x03u ^ 0x01u)};
        mkr_runtime_on_rx(&rt, mode, 5u);
        assert(rt.fsm.mode == MKR_MODE_LEVEL);
    }
    {
        const uint8_t raw = 0x11u;
        mkr_runtime_on_rx(&rt, &raw, 1u);
        assert(f.code == 0x11u);
        ticks(&rt, 30u);
        assert(f.code == 0x11u && f.ready);
    }
    f.order_n = 0u;
    mkr_runtime_on_usb_event(&rt, MKR_POWER_EVENT_USB_SUSPEND);
    assert(rt.power.state == MKR_POWER_STATE_SUSPENDED);
    assert(f.order_n >= 2u && f.order[0] == 1u && f.order[1] == 2u);
    assert(!f.power && !f.ready && f.code == 0u);
    mkr_runtime_on_usb_event(&rt, MKR_POWER_EVENT_USB_RESUME);
    assert(rt.power.state == MKR_POWER_STATE_WAIT_CONFIG_DELAY && !f.power);
    mkr_runtime_on_usb_event(&rt, MKR_POWER_EVENT_CLOCK_FAULT);
    assert(rt.power.state == MKR_POWER_STATE_FAULT && !f.power && f.code == 0u);
    ticks(&rt, 500u);
    mkr_runtime_on_usb_event(&rt, MKR_POWER_EVENT_USB_CONFIGURED);
    assert(rt.power.state == MKR_POWER_STATE_FAULT && !f.power);
    memset(&f, 0, sizeof f);
    mkr_runtime_init(&rt, &io);
    reach_ready(&rt, &f);
    {
        const uint8_t partial = 0xAAu;
        const uint8_t code = 0x05u;
        mkr_runtime_on_rx(&rt, &partial, 1u);
        assert(f.code == 0u);
        ticks(&rt, MKR_PARSER_FRAME_TIMEOUT_MS);
        mkr_runtime_on_rx(&rt, &code, 1u);
        assert(f.code == 0x05u && f.ready);
    }
    f.order_n = 0u;
    mkr_runtime_on_usb_event(&rt, MKR_POWER_EVENT_USB_BUS_RESET);
    assert(rt.power.state == MKR_POWER_STATE_UNCONFIGURED && !rt.power.usb_configured);
    assert(!f.power && !f.ready && f.code == 0u);
    assert(f.order[0] == 1u && f.order[1] == 2u);
    return 0;
}
