#include "protocol_dispatch.h"
uint8_t mkr_build_simple_response(uint8_t status, uint8_t cmd, uint8_t *out, uint8_t cap)
{
    if (cap < 5u) {
        return 0u;
    }
    out[0] = MKR_SOF_RESP;
    out[1] = 0x03u;
    out[2] = status;
    out[3] = cmd;
    out[4] = mkr_xor_chk(out, 4u);
    return 5u;
}
static uint8_t build_data(uint8_t status, uint8_t cmd, const uint8_t *data, uint8_t n, uint8_t *out, uint8_t cap)
{
    const uint8_t len_field = (uint8_t)(3u + n);
    const uint8_t total = (uint8_t)(2u + len_field);
    if (n > 5u || total > cap || len_field > MKR_MAX_LEN) {
        return 0u;
    }
    out[0] = MKR_SOF_RESP;
    out[1] = len_field;
    out[2] = status;
    out[3] = cmd;
    for (uint8_t i = 0u; i < n; i++) {
        out[4u + i] = data[i];
    }
    out[total - 1u] = mkr_xor_chk(out, (size_t)(total - 1u));
    return total;
}
static void respond_simple(mkr_response_t *resp, uint8_t status, uint8_t cmd)
{
    resp->respond = true;
    resp->length = mkr_build_simple_response(status, cmd, resp->frame, MKR_RESPONSE_MAX);
}
void mkr_dispatch_frame(mkr_fsm_t *fsm, const uint8_t *frame, uint8_t len, mkr_response_t *resp)
{
    resp->respond = false;
    resp->length = 0u;
    if (len < 4u || frame[0] != MKR_SOF_REQ) {
        return;
    }
    const uint8_t L = frame[1];
    if (L < 2u || L > MKR_MAX_LEN || len < (uint8_t)(2u + L)) {
        return;
    }
    const uint8_t cmd = frame[2];
    uint8_t st = MKR_ST_NACK_BAD_PARAM;
    bool with_data = false;
    uint8_t data[5];
    uint8_t data_n = 0u;
    switch (cmd) {
    case MKR_CMD_SEND_MARKER:
        if (L != 3u) {
            break;
        }
        st = mkr_fsm_send_marker(fsm, frame[3]);
        break;
    case MKR_CMD_CLEAR:
        if (L != 2u) {
            break;
        }
        st = mkr_fsm_clear(fsm);
        break;
    case MKR_CMD_SET_MODE:
        if (L != 3u) {
            break;
        }
        st = mkr_fsm_set_mode(fsm, frame[3]);
        break;
    case MKR_CMD_SET_PULSE:
        if (L != 4u) {
            break;
        }
        st = mkr_fsm_set_pulse(fsm, (uint16_t)(frame[3] | ((uint16_t)frame[4] << 8)));
        break;
    case MKR_CMD_SET_GAP:
        if (L != 4u) {
            break;
        }
        st = mkr_fsm_set_gap(fsm, (uint16_t)(frame[3] | ((uint16_t)frame[4] << 8)));
        break;
    case MKR_CMD_GET_STATUS:
        if (L != 2u) {
            break;
        }
        st = MKR_ST_ACK_RX_ONLY;
        with_data = true;
        data[0] = (uint8_t)(((uint8_t)fsm->mode & 1u) | (uint8_t)((fsm->state != MKR_STATE_IDLE) ? 0x02u : 0u) |
                            (uint8_t)(fsm->ready ? 0x04u : 0u));
        data[1] = (uint8_t)(fsm->pulse_ms & 0xFFu);
        data[2] = (uint8_t)((fsm->pulse_ms >> 8) & 0xFFu);
        data[3] = (uint8_t)(fsm->gap_ms & 0xFFu);
        data[4] = (uint8_t)((fsm->gap_ms >> 8) & 0xFFu);
        data_n = 5u;
        break;
    case MKR_CMD_GET_VERSION:
        if (L != 2u) {
            break;
        }
        st = MKR_ST_ACK_RX_ONLY;
        with_data = true;
        data[0] = MKR_FW_VERSION_MAJOR;
        data[1] = MKR_FW_VERSION_MINOR;
        data[2] = MKR_FW_VERSION_PATCH;
        data_n = 3u;
        break;
    default:
        st = MKR_ST_NACK_BAD_PARAM;
        break;
    }
    resp->respond = true;
    if (with_data && st == MKR_ST_ACK_RX_ONLY) {
        resp->length = build_data(st, cmd, data, data_n, resp->frame, MKR_RESPONSE_MAX);
        if (resp->length == 0u) {
            respond_simple(resp, MKR_ST_NACK_BAD_PARAM, cmd);
        }
        return;
    }
    respond_simple(resp, st, cmd);
}
void mkr_dispatch_raw(mkr_fsm_t *fsm, uint8_t code)
{
    if (code == 0u) {
        (void)mkr_fsm_clear(fsm);
        return;
    }
    (void)mkr_fsm_send_marker(fsm, code);
}
