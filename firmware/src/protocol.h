#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MKR_SOF_REQ          0xAAu
#define MKR_SOF_RESP         0x55u
#define MKR_MAX_PAYLOAD      6u
#define MKR_MAX_LEN          8u  /* CMD+PAYLOAD+CHK */

#define MKR_CMD_SET_MODE     0x01u
#define MKR_CMD_SET_PULSE    0x02u
#define MKR_CMD_SET_GAP      0x03u
#define MKR_CMD_SEND_MARKER  0x10u
#define MKR_CMD_CLEAR        0x11u
#define MKR_CMD_GET_STATUS   0x20u
#define MKR_CMD_GET_VERSION  0x21u

#define MKR_ST_ACK_ACCEPTED  0x00u
#define MKR_ST_ACK_RX_ONLY   0x01u
#define MKR_ST_NACK_BUSY     0x10u
#define MKR_ST_NACK_BAD_PARAM 0x11u
#define MKR_ST_NACK_BAD_CHK  0x12u
#define MKR_ST_NACK_TIMEOUT  0x13u
#define MKR_ST_NACK_NOT_READY 0x14u

/** CHK over SOF..last payload byte (excludes CHK itself). */
static inline uint8_t mkr_xor_chk(const uint8_t *bytes, size_t n)
{
    uint8_t x = 0;
    for (size_t i = 0; i < n; ++i) {
        x ^= bytes[i];
    }
    return x;
}

/**
 * Build SendMarker(code) request into out[5].
 * Example: code=42 -> AA 03 10 2A 93
 */
static inline void mkr_build_send_marker(uint8_t code, uint8_t out[5])
{
    out[0] = MKR_SOF_REQ;
    out[1] = 0x03; /* CMD + 1 payload + CHK */
    out[2] = MKR_CMD_SEND_MARKER;
    out[3] = code;
    out[4] = (uint8_t)(out[0] ^ out[1] ^ out[2] ^ out[3]);
}

#ifdef __cplusplus
}
#endif
