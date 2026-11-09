#ifndef MKR_PROTOCOL_PARSER_H
#define MKR_PROTOCOL_PARSER_H
/*
 * Streaming parser for the Stage 1 short frame, plus a one-byte marker path.
 * 0xAA is always SOF, never a raw marker. Any other byte outside a frame is
 * one marker code (MATLAB can write a single uint8). A frame that does not
 * finish within MKR_PARSER_FRAME_TIMEOUT_MS is dropped.
 */
#include <stdbool.h>
#include <stdint.h>
#include "protocol.h"
#define MKR_FRAME_MAX (2u + MKR_MAX_LEN)
#define MKR_PARSER_FRAME_TIMEOUT_MS 50u
typedef enum {
    MKR_PARSER_NEED_MORE = 0,
    MKR_PARSER_RAW,
    MKR_PARSER_FRAME,
    MKR_PARSER_BAD_LEN,
    MKR_PARSER_BAD_CHK
} mkr_parser_status_t;
typedef struct {
    uint8_t buf[MKR_FRAME_MAX];
    uint8_t filled;
    uint8_t expected;
    uint8_t idle_ms;
    uint8_t raw;
    bool in_frame;
} mkr_parser_t;
void mkr_parser_init(mkr_parser_t *parser);
mkr_parser_status_t mkr_parser_feed(mkr_parser_t *parser, uint8_t byte);
/* Returns true when a partial frame was aborted. */
bool mkr_parser_tick_1ms(mkr_parser_t *parser);
#endif
