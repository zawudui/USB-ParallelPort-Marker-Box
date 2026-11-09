#include "protocol_parser.h"
static void hunt(mkr_parser_t *p)
{
    p->in_frame = false;
    p->filled = 0u;
    p->expected = 0u;
    p->idle_ms = 0u;
}
void mkr_parser_init(mkr_parser_t *p)
{
    hunt(p);
    p->raw = 0u;
}
mkr_parser_status_t mkr_parser_feed(mkr_parser_t *p, uint8_t byte)
{
    p->idle_ms = 0u;
    if (!p->in_frame) {
        if (byte != MKR_SOF_REQ) {
            p->raw = byte;
            return MKR_PARSER_RAW;
        }
        p->in_frame = true;
        p->buf[0] = byte;
        p->filled = 1u;
        p->expected = 0u;
        return MKR_PARSER_NEED_MORE;
    }
    if (p->filled >= MKR_FRAME_MAX) {
        hunt(p);
        return MKR_PARSER_BAD_LEN;
    }
    p->buf[p->filled++] = byte;
    if (p->filled == 2u) {
        if (byte < 2u || byte > MKR_MAX_LEN) {
            const bool restart = (byte == MKR_SOF_REQ);
            hunt(p);
            if (restart) {
                p->in_frame = true;
                p->buf[0] = MKR_SOF_REQ;
                p->filled = 1u;
                return MKR_PARSER_NEED_MORE;
            }
            return MKR_PARSER_BAD_LEN;
        }
        p->expected = (uint8_t)(byte + 2u);
        return MKR_PARSER_NEED_MORE;
    }
    if (p->expected != 0u && p->filled == p->expected) {
        const uint8_t chk_i = (uint8_t)(p->expected - 1u);
        const bool ok = (mkr_xor_chk(p->buf, chk_i) == p->buf[chk_i]);
        p->in_frame = false;
        if (!ok) {
            return MKR_PARSER_BAD_CHK;
        }
        return MKR_PARSER_FRAME;
    }
    return MKR_PARSER_NEED_MORE;
}
bool mkr_parser_tick_1ms(mkr_parser_t *p)
{
    if (!p->in_frame) {
        return false;
    }
    if (p->idle_ms < 255u) {
        p->idle_ms++;
    }
    if (p->idle_ms >= MKR_PARSER_FRAME_TIMEOUT_MS) {
        hunt(p);
        return true;
    }
    return false;
}
