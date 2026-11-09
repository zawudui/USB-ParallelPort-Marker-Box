#include "marker_fsm.h"
#include <assert.h>
/*
 * Pulse edges are microsecond deadlines. A 1 ms service that lands 1 µs
 * after the rising edge must not end a 2 ms pulse. This is not a scope
 * capture. CI passing here is not a board measurement.
 */
static uint8_t g_code;
static uint32_t g_now;
static uint32_t g_armed;
static int g_arm_on;
static int g_disarms;
uint32_t mkr_hw_time_us(void) { return g_now; }
void mkr_hw_arm_deadline(uint32_t deadline_us)
{
    g_armed = deadline_us;
    g_arm_on = 1;
}
void mkr_hw_disarm_deadline(void)
{
    g_arm_on = 0;
    g_disarms++;
}
void mkr_hw_output(uint8_t code) { g_code = code; }
void mkr_hw_set_ready(bool ready) { (void)ready; }
void mkr_hw_enter_safe(void) { g_code = 0u; }
static void service(mkr_fsm_t *f, uint32_t now)
{
    g_now = now;
    mkr_fsm_on_us(f, now);
}
int main(void)
{
    mkr_fsm_t f;
    mkr_fsm_init(&f);
    f.ready = true;
    assert(mkr_fsm_set_pulse(&f, 2u) == MKR_ST_ACK_RX_ONLY);
    assert(mkr_fsm_set_gap(&f, 10u) == MKR_ST_ACK_RX_ONLY);
    /* Event 1 µs before a 1 ms boundary. Old SysTick countdown could drop
     * the line on the second tick (~1.001 ms). */
    g_now = 999u;
    assert(mkr_fsm_send_marker(&f, 42u) == MKR_ST_ACK_ACCEPTED);
    assert(g_code == 42u);
    assert(g_arm_on == 1);
    assert(g_armed == 999u + 2000u);
    service(&f, 1000u);
    assert(f.state == MKR_STATE_ACTIVE && g_code == 42u);
    service(&f, 2000u);
    assert(f.state == MKR_STATE_ACTIVE && g_code == 42u);
    service(&f, 2998u);
    assert(f.state == MKR_STATE_ACTIVE && g_code == 42u);
    service(&f, 2999u);
    assert(f.state == MKR_STATE_GAP && g_code == 0u);
    assert(g_armed == 2999u + 10000u);
    service(&f, 12998u);
    assert(f.state == MKR_STATE_GAP && g_code == 0u);
    service(&f, 12999u);
    assert(f.state == MKR_STATE_IDLE && g_code == 0u);
    assert(g_arm_on == 0);
    /* Exact 1 ms and 10 ms boundaries. */
    assert(mkr_fsm_set_pulse(&f, 1u) == MKR_ST_ACK_RX_ONLY);
    assert(mkr_fsm_set_gap(&f, 0u) == MKR_ST_ACK_RX_ONLY);
    g_now = 50000u;
    assert(mkr_fsm_send_marker(&f, 7u) == MKR_ST_ACK_ACCEPTED);
    assert(g_armed == 51000u);
    service(&f, 50999u);
    assert(g_code == 7u && f.state == MKR_STATE_ACTIVE);
    service(&f, 51000u);
    assert(g_code == 0u && f.state == MKR_STATE_IDLE);
    assert(mkr_fsm_set_pulse(&f, 10u) == MKR_ST_ACK_RX_ONLY);
    assert(mkr_fsm_set_gap(&f, 0u) == MKR_ST_ACK_RX_ONLY);
    g_now = 0u;
    assert(mkr_fsm_send_marker(&f, 8u) == MKR_ST_ACK_ACCEPTED);
    service(&f, 9999u);
    assert(g_code == 8u);
    service(&f, 10000u);
    assert(g_code == 0u && f.state == MKR_STATE_IDLE);
    /* Level-hold ignores the clock. */
    assert(mkr_fsm_set_mode(&f, MKR_MODE_LEVEL) == MKR_ST_ACK_RX_ONLY);
    assert(mkr_fsm_send_marker(&f, 9u) == MKR_ST_ACK_ACCEPTED);
    assert(g_arm_on == 0);
    service(&f, 10000000u);
    assert(g_code == 9u && f.state == MKR_STATE_ACTIVE);
    assert(mkr_fsm_clear(&f) == MKR_ST_ACK_RX_ONLY);
    assert(g_code == 0u && f.state == MKR_STATE_IDLE);
    /* Clear during a pulse starts a full gap from the clear timestamp. */
    assert(mkr_fsm_set_mode(&f, MKR_MODE_PULSE) == MKR_ST_ACK_RX_ONLY);
    assert(mkr_fsm_set_pulse(&f, 2u) == MKR_ST_ACK_RX_ONLY);
    assert(mkr_fsm_set_gap(&f, 10u) == MKR_ST_ACK_RX_ONLY);
    g_now = 1000u;
    assert(mkr_fsm_send_marker(&f, 4u) == MKR_ST_ACK_ACCEPTED);
    g_now = 1500u;
    assert(mkr_fsm_clear(&f) == MKR_ST_ACK_RX_ONLY);
    assert(g_code == 0u && f.state == MKR_STATE_GAP);
    assert(g_armed == 1500u + 10000u);
    service(&f, 11499u);
    assert(f.state == MKR_STATE_GAP);
    service(&f, 11500u);
    assert(f.state == MKR_STATE_IDLE);
    /* Deadline wrap. 2 ms starting 16 µs before the uint32 roll. */
    g_now = 0xFFFFFFF0u;
    assert(mkr_fsm_set_gap(&f, 0u) == MKR_ST_ACK_RX_ONLY);
    assert(mkr_fsm_send_marker(&f, 5u) == MKR_ST_ACK_ACCEPTED);
    assert(g_armed == 0xFFFFFFF0u + 2000u);
    service(&f, 0xFFFFFFF0u + 1999u);
    assert(f.state == MKR_STATE_ACTIVE && g_code == 5u);
    service(&f, 0xFFFFFFF0u + 2000u);
    assert(f.state == MKR_STATE_IDLE && g_code == 0u);
    return 0;
}
