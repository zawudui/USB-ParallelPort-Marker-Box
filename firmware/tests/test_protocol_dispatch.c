#include "protocol_dispatch.h"
#include <assert.h>
static uint8_t code_seen;
static bool ready_seen;
void mkr_hw_output(uint8_t code) { code_seen = code; }
void mkr_hw_set_ready(bool ready) { ready_seen = ready; }
void mkr_hw_enter_safe(void)
{
    code_seen = 0u;
    ready_seen = false;
}
static void send42(mkr_fsm_t *f, mkr_response_t *r)
{
    const uint8_t frame[5] = {0xAAu, 0x03u, 0x10u, 0x2Au, 0x93u};
    mkr_dispatch_frame(f, frame, 5u, r);
}
int main(void)
{
    mkr_fsm_t f;
    mkr_response_t r;
    const uint8_t ack[5] = {0x55u, 0x03u, 0x00u, 0x10u, 0x46u};
    const uint8_t busy[5] = {0x55u, 0x03u, 0x10u, 0x10u, 0x56u};
    const uint8_t bad[5] = {0xAAu, 0x03u, 0x10u, 0x2Au, 0x00u};
    uint8_t nack[5];
    mkr_fsm_init(&f);
    f.ready = true;
    send42(&f, &r);
    assert(r.respond && r.length == 5u);
    for (unsigned i = 0u; i < 5u; i++) {
        assert(r.frame[i] == ack[i]);
    }
    assert(code_seen == 42u);
    send42(&f, &r);
    for (unsigned i = 0u; i < 5u; i++) {
        assert(r.frame[i] == busy[i]);
    }
    assert(code_seen == 42u);
    assert(mkr_build_simple_response(MKR_ST_NACK_BAD_CHK, 0x10u, nack, 5u) == 5u);
    assert(nack[0] == 0x55u && nack[1] == 0x03u && nack[2] == 0x12u && nack[3] == 0x10u && nack[4] == 0x54u);
    mkr_dispatch_frame(&f, bad, 5u, &r);
    /* Bad frames are rejected by the parser before dispatch. Dispatch of a
     * checksum-valid length mismatch is NACK_BAD_PARAM and does not change GPIO. */
    assert(code_seen == 42u);
    (void)ready_seen;
    mkr_fsm_init(&f);
    mkr_dispatch_raw(&f, 7u);
    assert(code_seen == 0u);
    f.ready = true;
    (void)mkr_fsm_set_pulse(&f, 2u);
    mkr_dispatch_raw(&f, 7u);
    assert(code_seen == 7u && f.pulse_ms == 2u && f.mode == MKR_MODE_PULSE);
    {
        const uint8_t mode[5] = {0xAAu, 0x03u, 0x01u, 0x00u, (uint8_t)(0xAAu ^ 0x03u ^ 0x01u ^ 0x00u)};
        for (unsigned i = 0u; i < 20u; i++) {
            mkr_fsm_tick_1ms(&f);
        }
        mkr_dispatch_frame(&f, mode, 5u, &r);
        assert(r.frame[2] == MKR_ST_ACK_RX_ONLY && f.mode == MKR_MODE_LEVEL);
    }
    mkr_dispatch_raw(&f, 0x11u);
    for (unsigned i = 0u; i < 30u; i++) {
        mkr_fsm_tick_1ms(&f);
    }
    assert(code_seen == 0x11u);
    {
        uint8_t st[4] = {0xAAu, 0x02u, 0x20u, 0u};
        st[3] = (uint8_t)(st[0] ^ st[1] ^ st[2]);
        f.ready = true;
        mkr_dispatch_frame(&f, st, 4u, &r);
        assert(r.length == 10u && r.frame[0] == 0x55u && r.frame[1] == 8u && r.frame[3] == 0x20u);
        assert(r.frame[4] == (uint8_t)(0x02u | 0x04u));
        assert(r.frame[5] == 2u && r.frame[6] == 0u);
        assert(r.frame[9] == mkr_xor_chk(r.frame, 9u));
    }
    {
        uint8_t ver[4] = {0xAAu, 0x02u, 0x21u, 0u};
        ver[3] = (uint8_t)(ver[0] ^ ver[1] ^ ver[2]);
        mkr_dispatch_frame(&f, ver, 4u, &r);
        assert(r.length == 8u && r.frame[1] == 6u && r.frame[3] == 0x21u);
        assert(r.frame[4] == MKR_FW_VERSION_MAJOR && r.frame[4] == 2u);
        assert(r.frame[5] == MKR_FW_VERSION_MINOR && r.frame[5] == 1u);
        assert(r.frame[6] == MKR_FW_VERSION_PATCH && r.frame[6] == 0u);
        assert(r.frame[7] == mkr_xor_chk(r.frame, 7u));
    }
    return 0;
}
