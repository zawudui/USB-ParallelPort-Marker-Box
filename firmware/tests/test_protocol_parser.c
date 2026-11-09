#include "protocol_parser.h"
#include <assert.h>
static void feed_all(mkr_parser_t *p, const uint8_t *b, unsigned n, mkr_parser_status_t *last)
{
    for (unsigned i = 0u; i < n; i++) {
        *last = mkr_parser_feed(p, b[i]);
    }
}
int main(void)
{
    mkr_parser_t p;
    mkr_parser_status_t st = MKR_PARSER_NEED_MORE;
    const uint8_t frame[5] = {0xAAu, 0x03u, 0x10u, 0x2Au, 0x93u};
    mkr_parser_init(&p);
    assert(mkr_parser_feed(&p, 0x05u) == MKR_PARSER_RAW && p.raw == 0x05u);
    assert(mkr_parser_feed(&p, 0xAAu) == MKR_PARSER_NEED_MORE && p.in_frame);
    feed_all(&p, &frame[1], 4u, &st);
    assert(st == MKR_PARSER_FRAME && p.filled == 5u && p.buf[3] == 0x2Au);
    assert(mkr_parser_feed(&p, 0xAAu) == MKR_PARSER_NEED_MORE);
    assert(mkr_parser_feed(&p, 0x03u) == MKR_PARSER_NEED_MORE);
    assert(mkr_parser_feed(&p, 0x10u) == MKR_PARSER_NEED_MORE);
    assert(mkr_parser_feed(&p, 0x2Au) == MKR_PARSER_NEED_MORE);
    assert(mkr_parser_feed(&p, 0x00u) == MKR_PARSER_BAD_CHK && p.buf[2] == 0x10u && !p.in_frame);
    assert(mkr_parser_feed(&p, 0xAAu) == MKR_PARSER_NEED_MORE);
    assert(mkr_parser_feed(&p, 0x00u) == MKR_PARSER_BAD_LEN && !p.in_frame);
    assert(mkr_parser_feed(&p, 0xAAu) == MKR_PARSER_NEED_MORE);
    assert(mkr_parser_feed(&p, 0xAAu) == MKR_PARSER_NEED_MORE && p.in_frame && p.filled == 1u);
    for (unsigned i = 0u; i < MKR_PARSER_FRAME_TIMEOUT_MS - 1u; i++) {
        assert(!mkr_parser_tick_1ms(&p));
    }
    assert(p.in_frame);
    assert(mkr_parser_tick_1ms(&p) && !p.in_frame);
    assert(mkr_parser_feed(&p, 0x11u) == MKR_PARSER_RAW && p.raw == 0x11u);
    return 0;
}
