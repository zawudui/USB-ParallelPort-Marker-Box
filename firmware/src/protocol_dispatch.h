#ifndef MKR_PROTOCOL_DISPATCH_H
#define MKR_PROTOCOL_DISPATCH_H
/*
 * Stage 1 command set (design-pack v0.2). Response DATA is allowed by that
 * frame format. GetStatus uses a 5-byte subset that keeps LEN <= 8:
 *   flags, pulse_ms LE, gap_ms LE
 *   flags bit0 = mode (0 level, 1 pulse), bit1 = busy, bit2 = fsm.ready
 * last_code and a separate flags byte do not fit. GetVersion carries the
 * firmware stage as three bytes (major, minor, patch), not a hardware sign-off.
 */
#include <stdbool.h>
#include <stdint.h>
#include "marker_fsm.h"
#define MKR_FW_VERSION_MAJOR 2u
#define MKR_FW_VERSION_MINOR 1u
#define MKR_FW_VERSION_PATCH 0u
#define MKR_RESPONSE_MAX 12u
typedef struct {
    bool respond;
    uint8_t frame[MKR_RESPONSE_MAX];
    uint8_t length;
} mkr_response_t;
uint8_t mkr_build_simple_response(uint8_t status, uint8_t cmd, uint8_t *out, uint8_t cap);
void mkr_dispatch_frame(mkr_fsm_t *fsm, const uint8_t *frame, uint8_t len, mkr_response_t *resp);
/* One-byte path: no response. 0 clears; any other code is SendMarker. */
void mkr_dispatch_raw(mkr_fsm_t *fsm, uint8_t code);
#endif
