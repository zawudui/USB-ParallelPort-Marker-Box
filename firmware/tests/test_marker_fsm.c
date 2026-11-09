#include "marker_fsm.h"
#include <assert.h>
static uint8_t code_seen; static bool ready_seen;
void mkr_hw_output(uint8_t code) { code_seen=code; }
void mkr_hw_set_ready(bool ready) { ready_seen=ready; }
void mkr_hw_enter_safe(void) { code_seen=0u; ready_seen=false; }
int main(void) {
 mkr_fsm_t f; mkr_fsm_init(&f); assert(!f.ready && code_seen==0u && !ready_seen);
 assert(mkr_fsm_send_marker(&f,42u)==MKR_ST_NACK_NOT_READY);
 f.ready=true; mkr_hw_set_ready(true);
 assert(mkr_fsm_send_marker(&f,42u)==MKR_ST_ACK_ACCEPTED && code_seen==42u);
 assert(mkr_fsm_send_marker(&f,7u)==MKR_ST_NACK_BUSY);
 for(unsigned i=0;i<10u;i++) mkr_fsm_tick_1ms(&f);
 assert(f.state==MKR_STATE_GAP && code_seen==0u);
 for(unsigned i=0;i<10u;i++) mkr_fsm_tick_1ms(&f);
 assert(f.state==MKR_STATE_IDLE);
 assert(mkr_fsm_set_gap(&f,0u)==MKR_ST_ACK_RX_ONLY);
 assert(mkr_fsm_set_pulse(&f,1u)==MKR_ST_ACK_RX_ONLY);
 assert(mkr_fsm_send_marker(&f,1u)==MKR_ST_ACK_ACCEPTED); mkr_fsm_tick_1ms(&f);
 assert(f.state==MKR_STATE_IDLE && f.timer_ms_left==0u && code_seen==0u);
 assert(mkr_fsm_send_marker(&f,9u)==MKR_ST_ACK_ACCEPTED);
 assert(mkr_fsm_clear(&f)==MKR_ST_ACK_RX_ONLY && f.state==MKR_STATE_IDLE);
 assert(mkr_fsm_set_pulse(&f,0u)==MKR_ST_NACK_BAD_PARAM);
 assert(mkr_fsm_set_pulse(&f,1001u)==MKR_ST_NACK_BAD_PARAM);
 assert(mkr_fsm_set_gap(&f,1001u)==MKR_ST_NACK_BAD_PARAM);
 assert(mkr_fsm_set_mode(&f,2u)==MKR_ST_NACK_BAD_PARAM);
 /* One 1 ms step must not finish the device's 2 ms pulse. */
 assert(mkr_fsm_set_pulse(&f,2u)==MKR_ST_ACK_RX_ONLY);
 assert(mkr_fsm_set_gap(&f,0u)==MKR_ST_ACK_RX_ONLY);
 assert(mkr_fsm_send_marker(&f,3u)==MKR_ST_ACK_ACCEPTED);
 mkr_fsm_tick_1ms(&f);
 assert(f.state==MKR_STATE_ACTIVE && code_seen==3u);
 mkr_fsm_tick_1ms(&f);
 assert(f.state==MKR_STATE_IDLE && code_seen==0u);
 return 0;
}
